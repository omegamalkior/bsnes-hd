#include <sfc/sfc.hpp>

namespace SuperFamicom {

bool Memory::GlobalWriteEnable = false;
Bus bus;

#if defined(BSNES_COMPACT_BUS)
auto Bus::freePages() -> void {
  if(!pages) return;
  for(uint n : range(4096)) {
    delete[] pages[n].ids;
    delete[] pages[n].targets;
  }
  delete[] pages;
  pages = nullptr;
}

auto Bus::assign(uint addr, uint id, uint offset) -> void {
  auto& p = pages[addr >> 12];
  uint o = addr & 0xfff;
  if(!p.ids) {
    if(p.id == id && (id == 0 || p.base + o == offset)) return;  //already so
    p.ids = new uint8[4096];
    p.targets = new uint32[4096];
    for(uint n : range(4096)) p.ids[n] = p.id, p.targets[n] = p.base + n;
  }
  p.ids[o] = id;
  p.targets[o] = offset;
}

//a mixed page whose addresses all go to one place again becomes a uniform page
auto Bus::compact(uint page) -> void {
  auto& p = pages[page];
  if(!p.ids) return;
  uint id = p.ids[0], base = p.targets[0];
  for(uint n : range(4096)) {
    if(p.ids[n] != id) return;
    if(id && p.targets[n] != base + n) return;
  }
  delete[] p.ids;
  delete[] p.targets;
  p.ids = nullptr;
  p.targets = nullptr;
  p.id = id;
  p.base = base;
}

Bus::~Bus() {
  freePages();
}
#else
auto Bus::assign(uint addr, uint id, uint offset) -> void {
  lookup[addr] = id;
  target[addr] = offset;
}

Bus::~Bus() {
  if(lookup) delete[] lookup;
  if(target) delete[] target;
}
#endif

auto Bus::reset() -> void {
  for(uint id : range(256)) {
    reader[id].reset();
    writer[id].reset();
    counter[id] = 0;
  }

#if defined(BSNES_COMPACT_BUS)
  freePages();
  pages = new Page[4096];
#else
  if(lookup) delete[] lookup;
  if(target) delete[] target;

  lookup = new uint8 [16 * 1024 * 1024]();
  target = new uint32[16 * 1024 * 1024]();
#endif

  reader[0] = [](uint, uint8 data) -> uint8 { return data; };
  writer[0] = [](uint, uint8) -> void {};
}

auto Bus::map(
  const function<uint8 (uint, uint8)>& read,
  const function<void  (uint, uint8)>& write,
  const string& addr, uint size, uint base, uint mask
) -> uint {
  uint id = 1;
  while(counter[id]) {
    if(++id >= 256) return print("SFC error: bus map exhausted\n"), 0;
  }

  reader[id] = read;
  writer[id] = write;

  auto p = addr.split(":", 1L);
  auto banks = p(0).split(",");
  auto addrs = p(1).split(",");
  for(auto& bank : banks) {
    for(auto& addr : addrs) {
      auto bankRange = bank.split("-", 1L);
      auto addrRange = addr.split("-", 1L);
      uint bankLo = bankRange(0).hex();
      uint bankHi = bankRange(1, bankRange(0)).hex();
      uint addrLo = addrRange(0).hex();
      uint addrHi = addrRange(1, addrRange(0)).hex();

      for(uint bank = bankLo; bank <= bankHi; bank++) {
        for(uint addr = addrLo; addr <= addrHi; addr++) {
          uint pid = idAt(bank << 16 | addr);
          if(pid && --counter[pid] == 0) {
            reader[pid].reset();
            writer[pid].reset();
          }

          uint offset = reduce(bank << 16 | addr, mask);
          if(size) base = mirror(base, size);
          if(size) offset = base + mirror(offset, size - base);
          assign(bank << 16 | addr, id, offset);
          counter[id]++;
        }
        #if defined(BSNES_COMPACT_BUS)
        for(uint page = (bank << 16 | addrLo) >> 12; page <= (bank << 16 | addrHi) >> 12; page++) compact(page);
        #endif
      }
    }
  }

  return id;
}

auto Bus::unmap(const string& addr) -> void {
  auto p = addr.split(":", 1L);
  auto banks = p(0).split(",");
  auto addrs = p(1).split(",");
  for(auto& bank : banks) {
    for(auto& addr : addrs) {
      auto bankRange = bank.split("-", 1L);
      auto addrRange = addr.split("-", 1L);
      uint bankLo = bankRange(0).hex();
      uint bankHi = bankRange(1, bankRange(0)).hex();
      uint addrLo = addrRange(0).hex();
      uint addrHi = addrRange(1, addrRange(1)).hex();

      for(uint bank = bankLo; bank <= bankHi; bank++) {
        for(uint addr = addrLo; addr <= addrHi; addr++) {
          uint pid = idAt(bank << 16 | addr);
          if(pid && --counter[pid] == 0) {
            reader[pid].reset();
            writer[pid].reset();
          }

          assign(bank << 16 | addr, 0, 0);
        }
        #if defined(BSNES_COMPACT_BUS)
        for(uint page = (bank << 16 | addrLo) >> 12; page <= (bank << 16 | addrHi) >> 12; page++) compact(page);
        #endif
      }
    }
  }
}

}
