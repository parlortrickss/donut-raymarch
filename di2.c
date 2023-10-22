#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define debug(...)
//#define debug printf

#define R(s,x,y) x-=(y>>s);y+=(x>>s)
const int dz = 5, r1 = 1, r2 = 2;

// CORDIC algorithm to find magnitude of |x,y|
// also bring vector (x2,y2) along for the ride, and write back to x2
int length_cordic(int16_t x, int16_t y, int *x2_, int y2) {
  int x2 = *x2_;
  if (x < 0) { // start in right half-plane
    x = -x;
    y = -y;
    x2 = -x2;
    y2 = -y2;
  }
  for (int i = 0; i < 8; i++) {
    int t = x;
    int t2 = x2;
    if (y < 0) {
      x -= y >> i;
      y += t >> i;
      x2 -= y2 >> i;
      y2 += t2 >> i;
    } else {
      x += y >> i;
      y -= t >> i;
      x2 += y2 >> i;
      y2 -= t2 >> i;
    }
  }
  // divide by 0.625 as a cheap approximation to the 0.607 scaling factor factor
  // introduced by this algorithm (see https://en.wikipedia.org/wiki/CORDIC)
  *x2_ = (x2 >> 1) + (x2 >> 3);
  return (x >> 1) + (x >> 3);
}

void main() {
  // high-precision rotation directions
  int16_t sB = 0, cB = 16384;
  int16_t sA = 11583, cA = 11583;
  int16_t sAsB = 0, cAsB = 0;
  int16_t sAcB = 11583, cAcB = 11583;
  for (;;) {
    int x1_16 = cAcB << 2;

    int p0x = dz*sB >> 6;
    int p0y = dz*sAcB >> 6;
    int p0z = -dz*cAcB >> 6;

    int lxi = sB >> 6;
    int lyi = sAcB - cA >> 6;
    int lzi = -cAcB - sA >> 6;

    const int r1i = r1*256;
    const int r2i = r2*256;

    int niters = 0;
    int nnormals = 0;
    int yinc1 = 12*cA >> 6;
    int yinc2 = 12*sA >> 6;
    int xinc1 = 6*sAsB >> 6;
    int xinc2 = 6*cAsB >> 6;
    int xinc3 = 6*cB >> 6;
    int ycA = -12*yinc1;
    int ysA = -12*yinc2;
    for (int j = 0; j < 23; j++, ycA += yinc1, ysA += yinc2) {
      int xsAsB = -40*xinc1;
      int xcAsB = -40*xinc2;
      int vxi16 = -40*xinc3 - (sB << 2);
      int vyi16 = ycA - (sAcB<<2) - xsAsB;
      int vzi16 = ysA + (cAcB<<2) + xcAsB;
      for (int i = 0; i < 79; i++, vyi16 -= xinc1, vzi16 += xinc2, vxi16 += xinc3) {
        //int t = (int) (256 * dz) - r2i - r1i;
        int t = 512;

        int px = p0x + (vxi16 >> 7); // assuming t = 512, t*vxi>>8 == vxi<<1
        int py = p0y + (vyi16 >> 7);
        int pz = p0z + (vzi16 >> 7);
        debug("pxyz (%+4d,%+4d,%+4d)\n", px, py, pz);
        for (;;) {
          int t0, t1, t2, d;
          int lx = sB;
          int ly = sAcB - cA;
          int lz = -cAcB - sA;
          debug("[%2d,%2d] (px, py) = (%d, %d), (lx, ly) = (%d, %d) -> ", j, i, px, py, lx, ly);
          t0 = length_cordic(px, py, &lx, ly);
          debug("t0=%d (lx', ly') = (%d, %d)\n", t0, lx, ly);
          t1 = t0 - r2i;
          t2 = length_cordic(pz, t1, &lz, lx);
          d = t2 - r1i;
          t += d;
          px += d*vxi16 >> 16;
          py += d*vyi16 >> 16;
          pz += d*vzi16 >> 16;
          if (t > 8*256) {
            putchar(' ');
            break;
          } else if (d < 2) {
            int N = lz >> 11;
            putchar(".,-~:;!*=#$@"[N > 0 ? N < 12 ? N : 11 : 0]);
            nnormals++;
            break;
          }
          niters++;
        }
      }
      puts("");
    }
    printf("%d iterations %d lit pixels", niters, nnormals);
    fflush(stdout);
    R(5, cA, sA);
    R(5, cAsB, sAsB);
    R(5, cAcB, sAcB);
    R(6, cB, sB);
    R(6, cAcB, cAsB);
    R(6, sAcB, sAsB);
    usleep(15000);
    printf("\r\x1b[23A");
  }
}
