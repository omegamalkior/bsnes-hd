auto Bus::mirror(uint addr, uint size) -> uint {
  if(size == 0) return 0;
  uint base = 0;
  uint mask = 1 << 23;
  while(addr >= size) {
    while(!(addr & mask)) mask >>= 1;
    addr -= mask;
    if(size > mask) {
      size -= mask;
      base += mask;
    }
    mask >>= 1;
  }
  return base + addr;
}

auto Bus::reduce(uint addr, uint mask) -> uint {
  while(mask) {
    uint bits = (mask & -mask) - 1;
    addr = ((addr >> 1) & ~bits) | (addr & bits);
    mask = (mask & (mask - 1)) >> 1;
  }
  return addr;
}

#if defined(BSNES_COMPACT_BUS)
auto Bus::idAt(uint addr) const -> uint {
  auto& p = pages[addr >> 12];
  return p.ids ? p.ids[addr & 0xfff] : p.id;
}

auto Bus::read(uint addr, uint8 data) -> uint8 {
  auto& p = pages[addr >> 12];
  if(p.ids) { uint o = addr & 0xfff; return reader[p.ids[o]](p.targets[o], data); }
  return reader[p.id](p.base + (addr & 0xfff), data);
}

auto Bus::write(uint addr, uint8 data) -> void {
  auto& p = pages[addr >> 12];
  if(p.ids) { uint o = addr & 0xfff; return writer[p.ids[o]](p.targets[o], data); }
  return writer[p.id](p.base + (addr & 0xfff), data);
}
#else
auto Bus::idAt(uint addr) const -> uint {
  return lookup[addr];
}

auto Bus::read(uint addr, uint8 data) -> uint8 {
  return reader[lookup[addr]](target[addr], data);
}

auto Bus::write(uint addr, uint8 data) -> void {
  return writer[lookup[addr]](target[addr], data);
}
#endif
