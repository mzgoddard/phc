#include <math.h>

#include "ph.h"

phv phAdd(phv a, phv b) {
  return phv(a.x + b.x, a.y + b.y);
}

phv phSub(phv a, phv b) {
  return phv(a.x - b.x, a.y - b.y);
}

phv phMul(phv a, phv b) {
  return phv(a.x * b.x, a.y * b.y);
}

phv phDiv(phv a, phv b) {
  return phv(a.x / b.x, a.y / b.y);
}

phv phScale(phv a, phdouble s) {
  return phv(a.x * s, a.y * s);
}

phdouble phDot(phv a, phv b) {
  return a.x * b.x + a.y * b.y;
}

phdouble phCross(phv a, phv b) {
  return a.x * b.y - a.y * b.x;
}

phdouble phMag2(phv a) {
  return phDot(a, a);
}

phdouble phMag(phv a) {
  return sqrt(phMag2(a));
}

phv phUnit(phv a) {
  phdouble mag = phMag(a);
  return phScale(a, 1.0 / mag);
}
