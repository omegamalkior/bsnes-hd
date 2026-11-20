#include <sfc/sfc.hpp>

namespace SuperFamicom {

CPU cpu;
#include "dma.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "timing.cpp"
#include "irq.cpp"
#include "serialization.cpp"

auto CPU::synchronizeSMP() -> void {
  if(smp.clock < 0) scheduler.resume(smp.thread);
}

auto CPU::synchronizePPU() -> void {
  if(ppu.clock < 0) scheduler.resume(ppu.thread);
}

auto CPU::synchronizeCoprocessors() -> void {
  for(auto coprocessor : coprocessors) {
    if(coprocessor->clock < 0) scheduler.resume(coprocessor->thread);
  }
}

auto CPU::Enter() -> void {
  while(true) {
    scheduler.synchronize();
    cpu.main();
  }
}

auto CPU::main() -> void {
  if(r.wai) return instructionWait();
  if(r.stp) return instructionStop();
  if(ppufast.smw.hooks) smwHook();
  if(!status.interruptPending) return instruction();

  if(status.nmiPending) {
    status.nmiPending = 0;
    r.vector = r.e ? 0xfffa : 0xffea;
    return interrupt();
  }

  if(status.irqPending) {
    status.irqPending = 0;
    r.vector = r.e ? 0xfffe : 0xffee;
    return interrupt();
  }

  if(status.resetPending) {
    status.resetPending = 0;
    for(uint repeat : range(22)) step<6,0>();  //step(132);
    r.vector = 0xfffc;
    return interrupt();
  }

  status.interruptPending = 0;
}

//built-in Super Metroid widescreen. the game program itself is left untouched; at a fixed list of
//places in it the core either listens (to learn where each sprite really is) or adjusts the value
//the game is about to compare, so that decisions the game makes for a 256 pixel wide picture
//(is this enemy on screen? has this shot left the screen?) are made for the wider picture instead.
namespace {
  enum : uint8_t { SmwNone, SmwBoxOpen, SmwBoxClose, SmwEnemyActive, SmwEnemyDraw, SmwSpritesDone,
                   SmwSpriteRel, SmwSpriteAbs, SmwSpriteNone, SmwBias };
  struct SmwSite { uint32_t pc; uint8_t kind; int8_t l, r, p; };  //SmwBias: A += l*extL + r*extR + p*pan
  const SmwSite smwSites[] = {
    {0x858080, SmwBoxOpen}, {0x8580be, SmwBoxClose},
    {0xa08f00, SmwEnemyActive}, {0xa0ade8, SmwEnemyDraw},
    {0x80896e, SmwSpritesDone},
    //STA $0370,x after "LDA $0000,y : ADC $14": spritemap entry added to the position in $14
    {0x8187bd, SmwSpriteRel}, {0x818871, SmwSpriteRel}, {0x818940, SmwSpriteRel}, {0x8189cf, SmwSpriteRel},
    {0x818a68, SmwSpriteRel}, {0x818acb, SmwSpriteRel}, {0x818b35, SmwSpriteRel}, {0x818ba9, SmwSpriteRel},
    {0x818c1e, SmwSpriteRel}, {0x818c93, SmwSpriteRel},
    //STA $0370,x where A already is the full position relative to the camera
    {0x9086a5, SmwSpriteAbs}, {0x908ae4, SmwSpriteAbs}, {0x90b700, SmwSpriteAbs}, {0x90b766, SmwSpriteAbs},
    {0x90b7ab, SmwSpriteAbs}, {0x90b7e4, SmwSpriteAbs}, {0x90c70d, SmwSpriteAbs}, {0x94b0b0, SmwSpriteAbs},
    {0x94b116, SmwSpriteAbs}, {0x94b159, SmwSpriteAbs}, {0xa6dc43, SmwSpriteAbs}, {0xa99419, SmwSpriteAbs},
    //STA $0370,x with a fixed value (used to park a sprite off screen): no position to record
    {0x818909, SmwSpriteNone}, {0x81904a, SmwSpriteNone},
    //Samus's shots: removed when outside -64..320; now outside the wider picture by the same margin
    {0x90b174, SmwBias, +1, 0, 0}, {0x90b179, SmwBias, -1, -1, 0},
    //Samus's shots (one draw routine only draws within -48..304)
    {0x938388, SmwBias, 0, -1, 0}, {0x93838d, SmwBias, +1, +1, 0},
    //explosions, smoke and other effect sprites: drawn within -16..272
    {0xb4bd51, SmwBias, +1, 0, 0}, {0xb4bd56, SmwBias, -1, -1, 0},
    //enemy shots: drawn within -128..384 of the camera; follow the pan
    {0x8683f6, SmwBias, 0, 0, -1},
    //enemy shots that remove themselves at the edges of the original picture
    {0x86bd2d, SmwBias, +1, 0, 0}, {0x86bd39, SmwBias, 0, +1, 0},
    {0x86dad1, SmwBias, +1, 0, 0}, {0x86dadd, SmwBias, 0, +1, 0},
    {0x86dbc5, SmwBias, +1, 0, 0}, {0x86dbd1, SmwBias, 0, +1, 0},
    {0x86dfa3, SmwBias, +1, 0, 0}, {0x86dfaf, SmwBias, 0, +1, 0},
    {0x86e0b3, SmwBias, +1, 0, 0}, {0x86e0bf, SmwBias, 0, +1, 0},
    {0x86ec1b, SmwBias, +1, 0, 0}, {0x86ec27, SmwBias, 0, +1, 0},
  };
  uint8_t smwSiteMap[8192];   //one bit per low 16 bits of the program counter
  uint8_t smwSiteSlot[256];   //small hash from program counter to site index + 1
  bool smwSitesReady = false;
  inline auto smwHash(uint pc) -> uint { return (pc ^ pc >> 7 ^ pc >> 15) & 255; }
  auto smwBuildSites() -> void {
    memset(smwSiteMap, 0, sizeof smwSiteMap);
    memset(smwSiteSlot, 0, sizeof smwSiteSlot);
    for(uint n = 0; n < sizeof smwSites / sizeof *smwSites; n++) {
      uint pc = smwSites[n].pc;
      smwSiteMap[(pc & 0xffff) >> 3] |= 1 << (pc & 7);
      uint h = smwHash(pc);
      while(smwSiteSlot[h]) h = (h + 1) & 255;
      smwSiteSlot[h] = n + 1;
    }
    smwSitesReady = true;
  }
}

auto CPU::smwHook() -> void {
  uint pc = r.pc.d & 0xffffff;
  if(!smwSitesReady) smwBuildSites();
  if(!(smwSiteMap[(pc & 0xffff) >> 3] >> (pc & 7) & 1)) return;
  const SmwSite* site = nullptr;
  for(uint h = smwHash(pc); smwSiteSlot[h]; h = (h + 1) & 255) {
    if(smwSites[smwSiteSlot[h] - 1].pc == pc) { site = &smwSites[smwSiteSlot[h] - 1]; break; }
  }
  if(!site) return;

  auto& smw = ppufast.smw;
  auto r16 = [&](uint a) -> int { return (int16_t)(wram[a] | wram[a + 1] << 8); };
  int ext = ppufast.wsExt;

  switch(site->kind) {
  case SmwBoxOpen:  smw.box = true;  return;
  case SmwBoxClose: smw.box = false; return;
  case SmwSpritesDone: ppufast.smwSpriteFinalize(); return;

  case SmwSpriteNone: {
    uint index = r.x.w >> 2;
    if(!r.e && !r.p.x && index < 128) smw.sprBuild[index] = 0x7fff;
    return;
  }

  case SmwSpriteRel:
  case SmwSpriteAbs: {
    if(r.p.m || r.e || r.d.w) return;          //not the 16-bit situation these places are known in
    uint index = r.x.w >> 2;
    if(r.x.w & 3 || index >= 128) return;
    int pos;
    if(site->kind == SmwSpriteRel) {
      int base = r16(0x14);
      int entry = (r.a.w - base) & 0x1ff;      //the spritemap entry's own offset: 9 bits, signed
      if(entry & 0x100) entry -= 0x200;
      pos = base + entry;
    } else {
      pos = (int16_t)r.a.w;
    }
    smw.sprBuild[index] = pos > -2048 && pos < 2048 ? pos : 0x7fff;
    return;
  }

  case SmwEnemyActive:
  case SmwEnemyDraw: {
    if(!smw.level || ext <= 0) return;
    uint e = (site->kind == SmwEnemyActive ? r.x.w : r16(0x0e54)) & 0xffff;
    if(e >= 0x800) return;
    int x = r16(0x0f7a + e), y = r16(0x0f7e + e), rad = r16(0x0f82 + e);
    int camX = r16(0x0911), camY = r16(0x0915);
    int viewL = camX - ext + smw.pan, viewR = camX + 0x100 + ext + smw.pan;
    bool visible = x + rad >= viewL && x - rad <= viewR && y + 8 >= camY && camY + 0xf8 >= y;
    if(site->kind == SmwEnemyActive) r.pc.w = visible ? 0x8f34 : 0x8f54;
    else                             r.pc.w = visible ? 0xae1f : 0xae24;
    return;
  }

  case SmwBias: {
    if(!smw.level || ext <= 0 || r.p.m || r.e) return;
    //the wider picture shows camera-relative x from -extL to 256+extR
    int extL = ext - smw.pan, extR = ext + smw.pan;
    r.a.w += site->l * extL + site->r * extR + site->p * smw.pan;
    return;
  }
  }
}

auto CPU::load() -> bool {
  version = configuration.system.cpu.version;
  if(version < 1) version = 1;
  if(version > 2) version = 2;
  return true;
}

auto CPU::power(bool reset) -> void {
  WDC65816::power();
  Thread::create(Enter, system.cpuFrequency());
  coprocessors.reset();
  PPUcounter::reset();
  PPUcounter::scanline = {&CPU::scanline, this};

  function<uint8 (uint, uint8)> reader;
  function<void  (uint, uint8)> writer;

  reader = {&CPU::readRAM, this};
  writer = {&CPU::writeRAM, this};
  bus.map(reader, writer, "00-3f,80-bf:0000-1fff", 0x2000);
  bus.map(reader, writer, "7e-7f:0000-ffff", 0x20000);

  reader = {&CPU::readAPU, this};
  writer = {&CPU::writeAPU, this};
  bus.map(reader, writer, "00-3f,80-bf:2140-217f");

  reader = {&CPU::readCPU, this};
  writer = {&CPU::writeCPU, this};
  bus.map(reader, writer, "00-3f,80-bf:2180-2183,4016-4017,4200-421f");

  reader = {&CPU::readDMA, this};
  writer = {&CPU::writeDMA, this};
  bus.map(reader, writer, "00-3f,80-bf:4300-437f");

  if(!reset) random.array(wram, sizeof(wram));

  if(configuration.hacks.hotfixes) {
    //Dirt Racer (Europe) relies on uninitialized memory containing certain values to boot without freezing.
    //the game itself is broken and will fail to run sometimes on real hardware, but for the sake of expedience,
    //WRAM is initialized to a constant value that will allow this game to always boot in successfully.
    if(cartridge.headerTitle() == "DIRT RACER") {
      for(auto& byte : wram) byte = 0xff;
    }
  }

  for(uint n : range(8)) {
    channels[n] = {};
    if(n != 7) channels[n].next = channels[n + 1];
  }

  counter = {};
  io = {};
  alu = {};

  status = {};
  status.dramRefreshPosition = (version == 1 ? 530 : 538);
  status.hdmaSetupPosition = (version == 1 ? 12 + 8 - dmaCounter() : 12 + dmaCounter());
  status.hdmaPosition = 1104;
  status.resetPending = 1;
  status.interruptPending = 1;
}

}
