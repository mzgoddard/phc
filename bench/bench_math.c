#include "ph.h"
#include "bench.h"

phv a = phv(1, 2);
phv b = phv(2, 3);

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

void bench_math() {
  note("\n** math **\n");

  {
    note("1");
    bench(
      "add", b_add, NULL,
      NULL
    );
  }

  {
    note("100");
    bench(
      "add", b_add100, NULL,
      NULL
    );
  }

  {
    note("1e6");
    bench(
      "add", b_add1e6, NULL,
      NULL
    );
  }
}
