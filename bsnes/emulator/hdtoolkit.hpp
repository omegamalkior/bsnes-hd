namespace HdToolkit {
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

  //the renderer keeps 61440 pixels per line ((256 + 2 * ws) * scale * scale). very wide pictures
  //(32:9) at the highest HD scales would not fit: narrow them just enough that they do.
  static constexpr auto clampWsExt(int ws, int scale) -> int {
    if (scale < 1) scale = 1;
    while (ws > 0 && (256 + 2 * ws) * scale * scale > 61440) ws -= 8;
    return ws < 0 ? 0 : ws;
  }
}
