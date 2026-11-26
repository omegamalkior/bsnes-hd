# Nintendo 3DS build of the libretro core (a static library that RetroArch's Makefile.ctr links in).
# needs devkitARM (devkitPro). usage, from this folder:
#   make            -> bsnes_hd_10-7_libretro_ctr.a
# then, in a RetroArch source tree:
#   cp bsnes_hd_10-7_libretro_ctr.a <RetroArch>/libretro_ctr.a
#   make -f Makefile.ctr LIBRETRO=bsnes_hd_10-7 USE_CTRULIB_2=1
ifeq ($(strip $(DEVKITARM)),)
$(error DEVKITARM is not set: install devkitPro's devkitARM)
endif

TARGET := bsnes_hd_10-7_libretro_ctr.a
ROOT   := ../../..
CC     := $(DEVKITARM)/bin/arm-none-eabi-gcc
CXX    := $(DEVKITARM)/bin/arm-none-eabi-g++
AR     := $(DEVKITARM)/bin/arm-none-eabi-ar

SOURCES := \
	bsnes/target-libretro/libretro.cpp \
	bsnes/emulator/emulator.cpp \
	libco/libco.c \
	bsnes/filter/filter.cpp \
	bsnes/lzma/lzma.cpp \
	bsnes/sfc/interface/interface.cpp \
	bsnes/sfc/system/system.cpp \
	bsnes/sfc/controller/controller.cpp \
	bsnes/sfc/cartridge/cartridge.cpp \
	bsnes/sfc/memory/memory.cpp \
	bsnes/sfc/cpu/cpu.cpp \
	bsnes/sfc/smp/smp.cpp \
	bsnes/sfc/dsp/dsp.cpp \
	bsnes/sfc/ppu/ppu.cpp \
	bsnes/sfc/ppu-fast/ppu.cpp \
	bsnes/sfc/expansion/expansion.cpp \
	bsnes/sfc/coprocessor/coprocessor.cpp \
	bsnes/sfc/slot/slot.cpp \
	bsnes/gb/Core/apu.c \
	bsnes/gb/Core/camera.c \
	bsnes/gb/Core/display.c \
	bsnes/gb/Core/gb.c \
	bsnes/gb/Core/joypad.c \
	bsnes/gb/Core/mbc.c \
	bsnes/gb/Core/memory.c \
	bsnes/gb/Core/printer.c \
	bsnes/gb/Core/random.c \
	bsnes/gb/Core/rewind.c \
	bsnes/gb/Core/save_state.c \
	bsnes/gb/Core/sgb.c \
	bsnes/gb/Core/sm83_cpu.c \
	bsnes/gb/Core/symbol_hash.c \
	bsnes/gb/Core/timing.c \
	bsnes/processor/arm7tdmi/arm7tdmi.cpp \
	bsnes/processor/spc700/spc700.cpp \
	bsnes/processor/wdc65816/wdc65816.cpp

OBJECTS := $(patsubst %,obj/%.o,$(basename $(SOURCES)))

# ARM11 MPCore, hard float. the memory map is kept per 4 KB page (BSNES_COMPACT_BUS: the full map
# takes 80 MB) and the renderer's line buffers small (2048 pixels per line, about
# 11 MB in all): HD Mode 7 up to 2x, widescreen up to 32:9 at 1x and up to about 21:9 at 2x
ARCH   := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -mtp=soft -marm
DEFS   := -D_3DS -D__3DS__ -DARM11 -D__LIBRETRO__ -DGB_INTERNAL -DDISABLE_DEBUGGER -DBUILD_PERFORMANCE \
          -DBSNES_HD_LINE_PIXELS=2048 -DBSNES_COMPACT_BUS
FLAGS  := $(ARCH) $(DEFS) -O2 -fomit-frame-pointer -ffunction-sections -fdata-sections -mword-relocations \
          -Wno-narrowing -Wno-multichar -I$(ROOT) -I$(ROOT)/bsnes -I$(DEVKITPRO)/libctru/include
CFLAGS   := $(FLAGS) -std=gnu11 -D_GNU_SOURCE
CXXFLAGS := $(FLAGS) -std=gnu++17

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $^

obj/%.o: $(ROOT)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/%.o: $(ROOT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj $(TARGET)

.PHONY: all clean
