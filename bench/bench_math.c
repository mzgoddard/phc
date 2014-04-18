#include "ph.h"
#include "bench.h"

phv a = phv(1, 2);
phv b = phv(2, 3);

phbox box1 = phbox(0, 0, 1, 1);
phbox box2 = phbox(2, 2, 3, 3);
phbox box3 = phbox(0, 2, 1, 3);
phbox box4 = phbox(-2, 0, -1, 1);
phbox box5 = phbox(0, -1, 1, -2);

void b_add(void *ctx) {
  phv result = phAdd(a, b);
}

void b_add100(void *ctx) {
  phv result = phZero();
  for (phint i = 0; i < 100; ++i) {
    result = phAdd(result, a);
  }
}

void b_add1e6(void *ctx) {
  phv result = phZero();
  for (phint i = 0; i < 1000000; ++i) {
    result = phAdd(result, a);
  }
}

#if __SSE__
typedef phdouble _phv __attribute__((ext_vector_type(2)));
#define _phv(x, y) ((_phv) {x, y});

_phv _phAdd(_phv a, _phv b) {
  return a + b;
}

_phv _a = _phv(1, 2);
_phv _b = _phv(2, 3);

void b_addv(void *ctx) {
  _phv result = _phAdd(_a, _b);
}

void b_addv100(void *ctx) {
  _phv result = _phv(0, 0);
  for (phint i = 0; i < 100; ++i) {
    result = _phAdd(result, _a);
  }
}

void b_addv1e6(void *ctx) {
  _phv result = _phv(0, 0);
  for (phint i = 0; i < 1000000; ++i) {
    result = _phAdd(result, _a);
  }
}
#endif

void b_intersect(void *ctx) {
  phbool result;
  for (phint i = 0; i < 1000000; ++i) {
    result = phIntersect(box1, box2);
    result = phIntersect(box1, box3);
    result = phIntersect(box1, box4);
    result = phIntersect(box1, box5);
  }
}

phbool _phIntersect(phbox a, phbox b) {
  return a.left > b.right || a.right < b.left ||
    a.bottom > b.top || a.top < b.bottom;
}

void b_intersectAlt(void *ctx) {
  phbool result;
  for (phint i = 0; i < 1000000; ++i) {
    result = _phIntersect(box1, box2);
    result = _phIntersect(box1, box3);
    result = _phIntersect(box1, box4);
    result = _phIntersect(box1, box5);
  }
}

void bench_math() {
  note("\n** math **\n");

  {
    note("1");
    bench(
      "add", b_add, NULL,
      #if __SSE__
      "vector add", b_addv, NULL,
      #endif
      NULL
    );
  }

  {
    note("100");
    bench(
      "add", b_add100, NULL,
      #if __SSE__
      "vector add", b_addv100, NULL,
      #endif
      NULL
    );
  }

  {
    note("1e6");
    bench(
      "add", b_add1e6, NULL,
      #if __SSE__
      "vector add", b_addv1e6, NULL,
      #endif
      NULL
    );
  }

  {
    note("phIntersect");
    bench(
      "current", b_intersect, NULL,
      "alt", b_intersectAlt, NULL,
      NULL
    );
  }
}
