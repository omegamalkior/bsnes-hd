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
  if(ppufast.smw.level) smwHook();
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

//built-in Super Metroid widescreen: the game only activates and draws enemies
//that touch the original 256 pixel wide picture. when the game reaches those two
//decisions, answer them here for the wider picture instead. the game program
//itself is left untouched.
auto CPU::smwHook() -> void {
  uint pc = r.pc.d & 0xffffff;
  if(pc == 0x858080) { ppufast.smw.box = true;  return; }  //message box opens
  if(pc == 0x8580be) { ppufast.smw.box = false; return; }  //message box closes
  if(pc != 0xa08f00 && pc != 0xa0ade8) return;
  auto r16 = [&](uint a) -> int { return (int16_t)(wram[a] | wram[a + 1] << 8); };
  uint e = (pc == 0xa08f00 ? r.x.w : r16(0x0e54)) & 0xffff;
  if(e >= 0x800) return;
  int ext = ppufast.wsExt;
  if(ext <= 0) return;
  int x = r16(0x0f7a + e), y = r16(0x0f7e + e), rad = r16(0x0f82 + e);
  int camX = r16(0x0911), camY = r16(0x0915);
  int viewL = camX - ext + ppufast.smw.pan, viewR = camX + 0x100 + ext + ppufast.smw.pan;
  bool visible = x + rad >= viewL && x - rad <= viewR && x + rad <= viewR + (128 - ext) + 8
              && y + 8 >= camY && camY + 0xf8 >= y;
  if(pc == 0xa08f00) r.pc.w = visible ? 0x8f34 : 0x8f54;
  else               r.pc.w = visible ? 0xae1f : 0xae24;
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
