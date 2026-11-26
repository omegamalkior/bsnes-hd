struct Memory {
  static bool GlobalWriteEnable;

  virtual ~Memory() { reset(); }
  inline explicit operator bool() const { return size() > 0; }

  virtual auto reset() -> void {}
  virtual auto allocate(uint, uint8 = 0xff) -> void {}

  virtual auto data() -> uint8* = 0;
  virtual auto size() const -> uint = 0;

  virtual auto read(uint address, uint8 data = 0) -> uint8 = 0;
  virtual auto write(uint address, uint8 data) -> void = 0;

  uint id = 0;
};

#include "readable.hpp"
#include "writable.hpp"
#include "protectable.hpp"

struct Bus {
  alwaysinline static auto mirror(uint address, uint size) -> uint;
  alwaysinline static auto reduce(uint address, uint mask) -> uint;

  ~Bus();

  alwaysinline auto read(uint address, uint8 data = 0) -> uint8;
  alwaysinline auto write(uint address, uint8 data) -> void;

  auto reset() -> void;
  auto map(
    const function<uint8 (uint, uint8)>& read,
    const function<void (uint, uint8)>& write,
    const string& address, uint size = 0, uint base = 0, uint mask = 0
  ) -> uint;
  auto unmap(const string& address) -> void;

private:
  alwaysinline auto idAt(uint address) const -> uint;
  auto assign(uint address, uint id, uint offset) -> void;

#if defined(BSNES_COMPACT_BUS)
  //small devices (3DS): the full map below takes 80 MB. instead keep one entry per 4 KB page, and
  //per-address entries only for the few pages whose addresses go to different places
  struct Page {
    uint8 id = 0;             //uniform page: every address goes to id,
    uint32 base = 0;          //at base + (address & 0xfff)
    uint8* ids = nullptr;     //mixed page: per address
    uint32* targets = nullptr;
  };
  Page* pages = nullptr;  //4096 pages
  auto freePages() -> void;
  auto compact(uint page) -> void;
#else
  uint8* lookup = nullptr;
  uint32* target = nullptr;
#endif

  function<uint8 (uint, uint8)> reader[256];
  function<void  (uint, uint8)> writer[256];
  uint counter[256];
};

extern Bus bus;
