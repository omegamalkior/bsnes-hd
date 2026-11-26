//pixels the renderer keeps per line: (256 + 2 * widescreen) * scale * scale must fit.
//small devices (3DS) build with a smaller value, which limits the HD scale and widescreen width
#ifndef BSNES_HD_LINE_PIXELS
#define BSNES_HD_LINE_PIXELS 61440
#endif

namespace HdToolkit {
  //the highest HD scale whose plain 256 wide picture fits
  static constexpr auto maxScale() -> int {
    int s = 1;
    while (256 * (s + 1) * (s + 1) <= BSNES_HD_LINE_PIXELS) s++;
    return s;
  }

  static constexpr auto determineWsExt(int ws, bool overscan, bool aspectCorrection) -> int {
    double val = ws;
    if (ws > 200) {
      int w = ws / 100;
      int h = ws % 100;
      val = overscan ? 224.0 : 216.0;
      val *= w;
      val /= h;
      if (aspectCorrection) {
        val *= 7;
        val /= 8;
      }
      if (val <= 256) {
        return 0;
      }
      val -= 256;
      val /= 2;
    }
    val /= 8;
    if (overscan || aspectCorrection) {
      val += 0.5;
    }
    ws = (int)val;
    if (ws <= 0) {
      return 0;
    }
    ws *= 8;
    return ws;
  }

  //very wide pictures (32:9) at the highest HD scales would not fit the renderer's line buffers:
  //narrow them just enough that they do.
  static constexpr auto clampWsExt(int ws, int scale) -> int {
    if (scale < 1) scale = 1;
    while (ws > 0 && (256 + 2 * ws) * scale * scale > BSNES_HD_LINE_PIXELS) ws -= 8;
    return ws < 0 ? 0 : ws;
  }
}
