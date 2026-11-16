//built-in widescreen support for unmodified Super Metroid.
//nothing here changes the game program: the PPU is given the parts of the room
//that lie beyond the original 256 pixel wide picture, taken from the room data
//the game already holds in memory.

auto PPU::smwPower() -> void {
  smw = {};
  for(uint y : range(240)) smw.maskL[y] = -4096, smw.maskR[y] = 4096;
  auto hashes = cartridge.hashes();
  smw.cart = hashes.size() && hashes[0] == "12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72";
}

static bool smwWasLevel = false;

auto PPU::smwFrame() -> void {
  if(!smw.cart) return;
  int ext = wsExt;
  smw.level = false;
  smw.hud = false;
  if(ext <= 0) {
    smw.pan = smw.panTarget = 0;
    for(uint y : range(240)) smw.maskL[y] = -4096, smw.maskR[y] = 4096;
    return;
  }

  auto r16 = [&](uint a) -> uint { return cpu.wram[a] | cpu.wram[a + 1] << 8; };
  uint state = r16(0x0998);  //game state
  uint stage = r16(0x099c);  //door transition stage
  bool door = state == 0x09 || state == 0x0a || state == 0x0b;

  auto& bg = io.bg1;
  int W = r16(0x07a5), H = r16(0x07a7);
  auto panFor = [&](int camX, int RW) -> int {
    int target = 0;
    if(RW <= 256 + 2 * ext) target = RW / 2 - (camX + 128);
    else if(camX < ext) target = ext - camX;
    else if(camX > RW - 256 - ext) target = RW - 256 - ext - camX;
    return max(-ext, min(ext, target));
  };
  //the room is on screen during play, while it fades for pausing, and at both ends of a door transition
  bool level = state == 0x07 || state == 0x08 || state == 0x09 || state == 0x0a || state == 0x0c || state == 0x12
            || (state == 0x0b && (stage == 0xe17d || stage == 0xe19f || stage == 0xe29e || stage == 0xe2db || stage == 0xe737));
  if(state != 0x08) smw.box = false;
  if(io.bgMode != 1 || bg.screenSize != 1 || bg.tileSize) level = false;
  if(W < 16 || H < 14 || W * H * 2 > 0x6400) level = false;
  if(r16(0x179c)) level = false;  //boss rooms keep the original picture

  if(level) {
    //camera position that matches the scroll registers of the frame about to be drawn
    int l1x = (int16_t)r16(0x0911), l1y = (int16_t)r16(0x0915);
    int dx = (bg.hoffset - r16(0x091d) - l1x) & 511; if(dx >= 256) dx -= 512;
    int dy = (bg.voffset - r16(0x091f) - l1y) & 255; if(dy >= 128) dy -= 256;
    int camX = l1x + dx, camY = l1y + dy;

    //near the edges of a room, shift the picture so that the room fills as much of it as possible
    smw.panTarget = panFor(camX, W * 16);
    if(smwWasLevel) smw.pan += max(-16, min(16, smw.panTarget - smw.pan));
    else smw.pan = smw.panTarget;
    smw.panFrom = smw.panTo = smw.pan;
    smw.doorFrames = 0;
    int RW = W * 16;
    int pan = smw.pan;

    //draw the whole stretch of the room around the camera into the background layers
    int lo = max(min(-128 + pan, -16), -240);
    auto fill = [&](PPU::IO::Background& layer, const uint8_t* data, int cx, int cy) {
      const uint8_t* table = cpu.wram + 0xa000;
      for(int tx = 0; tx < 64; tx++) {
        int d = (tx * 8 - layer.hoffset - lo) & 511; d += lo;
        int wx = cx + d;
        for(int ty = 0; ty < 32; ty++) {
          int e = (ty * 8 - layer.voffset) & 255; if(e >= 240) e -= 256;
          int wy = cy + e;
          uint16_t word = 0x0338;
          if(wx >= 0 && wy >= 0 && (wx >> 4) < W && (wy >> 4) < H) {
            uint i = ((wy >> 4) * W + (wx >> 4)) * 2;
            uint block = data[i] | data[i + 1] << 8;
            uint qx = wx >> 3 & 1, qy = wy >> 3 & 1;
            if(block & 0x400) qx ^= 1;
            if(block & 0x800) qy ^= 1;
            const uint8_t* t = table + (block & 0x3ff) * 8 + (qy * 2 + qx) * 2;
            word = t[0] | t[1] << 8;
            if(block & 0x400) word ^= 0x4000;
            if(block & 0x800) word ^= 0x8000;
          }
          vram[(layer.screenAddress + (tx & 32 ? 0x400 : 0) + ty * 32 + (tx & 31)) & 0x7fff] = word;
        }
      }
    };
    fill(io.bg1, cpu.wram + 0x10002, camX, camY);

    //rooms with their own second layer (rather than a repeating backdrop) keep it in the room data too
    if(!(cpu.wram[0x091b] & 1) && io.bg2.screenSize == 1 && !io.bg2.tileSize) {
      int l2x = (int16_t)r16(0x0917), l2y = (int16_t)r16(0x0919);
      int ex = (io.bg2.hoffset - r16(0x0921) - l2x) & 511; if(ex >= 256) ex -= 512;
      int ey = (io.bg2.voffset - r16(0x0923) - l2y) & 255; if(ey >= 128) ey -= 256;
      fill(io.bg2, cpu.wram + 0x19602, l2x + ex, l2y + ey);
    }

    //show the room as far as the camera would be allowed to go: stop at the room's edges and at
    //the parts the game keeps the camera out of
    int screensW = r16(0x07a9), screensH = r16(0x07ab);
    int sxA = max(0, min(screensW - 1, camX >> 8)), sxB = max(0, min(screensW - 1, (camX + 255) >> 8));
    auto shown = [&](int sx, int sy) -> bool {
      if(sx < 0 || sx >= screensW) return false;
      if(sy < 0 || sy >= screensH || sy * screensW + sx >= 0x32) return true;
      return cpu.wram[0xcd20 + sy * screensW + sx] != 0;
    };
    int lastRow = -9999, L = 0, R = 0;
    for(int y = 0; y < 240; y++) {
      int row = (camY + y) >> 8;
      if(row != lastRow) {
        lastRow = row;
        int a = sxA, b = sxB;
        while(shown(a - 1, row)) a--;
        while(shown(b + 1, row)) b++;
        L = min(0, a * 256 - camX) - pan;
        R = max(256, (b + 1) * 256 - camX) - pan;
      }
      smw.maskL[y] = L, smw.maskR[y] = R;
    }

    smw.level = true;
    smw.hud = true;
  } else if(door) {
    //the black part of a door transition: only the game's own picture is valid. slide it from where
    //the old room was shown to where the new room will be, in step with the game's own scroll
    if(state == 0x0b && (stage == 0xe3c0 || stage == 0xe4a9) && W >= 16 && W <= 240) {
      int l1x = (int16_t)r16(0x0911);
      uint dir = cpu.wram[0x0791] & 3;  //0 right, 1 left, 2 down, 3 up
      float t;
      if(dir < 2) {
        int final = dir == 1 ? (l1x >> 8) << 8 : ((l1x + 255) >> 8) << 8;
        smw.panTo = panFor(final, W * 16);
        t = 1.0f - abs(final - l1x) / 256.0f;
      } else {
        smw.panTo = panFor(l1x, W * 16);
        t = ++smw.doorFrames / 60.0f;
      }
      t = max(0.0f, min(1.0f, t));
      smw.pan = smw.panFrom + (int)((smw.panTo - smw.panFrom) * t + (smw.panTo >= smw.panFrom ? 0.5f : -0.5f));
    } else if(state == 0x0b && (stage == 0xe659 || stage == 0xe664 || stage == 0xe6a2)) {
      smw.pan = smw.panTo;
    }
    smw.hud = true;
    for(uint y : range(240)) smw.maskL[y] = -smw.pan, smw.maskR[y] = 256 - smw.pan;
  } else {
    //anything else (menus, the map, cutscenes) keeps the original picture in the middle
    smw.pan = smw.panTarget = smw.panFrom = smw.panTo = 0;
    for(uint y : range(240)) smw.maskL[y] = 0, smw.maskR[y] = 256;
  }
  smwWasLevel = level;
}
