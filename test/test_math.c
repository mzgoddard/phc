#include <math.h>

#include "ph.h"
#include "tap.h"

void test_math() {
  {
    note("\n** vector **");
    phv a = phv(1, 2);
    phv b = phv(2, 3);
    phv dest = phAdd(a, b);
    ok(dest.x == 3 && dest.y == 5, "Add two vectors.");

    dest = phSub(a, b);
    ok(dest.x == -1 && dest.y == -1, "Subtract two vectors.");

    dest = phMul(a, b);
    ok(dest.x == 2 && dest.y == 6, "Multiply two vectors.");

    dest = phDiv(a, b);
    ok(dest.x == 0.5 && dest.y == 2.0 / 3.0, "Divide two vectors.");

    dest = phScale(a, 2);
    ok(dest.x == 2 && dest.y == 4, "Scale one vector by 2.");

    phdouble dec = phDot(a, b);
    ok(dec == 8, "Dot two vectors.");

    dec = phCross(a, b);
    ok(dec == -1, "Cross two vectors.");

    dec = phMag2(a);
    ok(dec == 5, "Mag2 one vector.");

    dec = phMag(a);
    ok(dec == sqrt(5), "Mag one vector.");

    dest = phUnit(a);
    ok(
      dest.x == 1.0 / sqrt(5) && dest.y == 2.0 / sqrt(5),
      "Normalize one vector."
    );
  }
}
