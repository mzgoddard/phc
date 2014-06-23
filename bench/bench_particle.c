#include "ph.h"
#include "bench.h"
#include "tap.h"

void b_testtouch1000(void *ctx) {
  phparticle a = phparticle(phZero());
  phparticle b = phparticle(phv(0.5, 0));
  phcollision col;

  for (int i = 0; i < 1000; ++i) {
    phTest(&a, &b, &col);
  }
}

void b_testnotouch1000(void *ctx) {
  phparticle a = phparticle(phZero());
  phparticle b = phparticle(phv(2, 0));
  phcollision col;

  for (int i = 0; i < 1000; ++i) {
    phTest(&a, &b, &col);
  }
}

void b_testsolve1000(void *ctx) {
  phparticle a = phparticle(phZero());
  phparticle b = phparticle(phv(0.5, 0));
  phcollision col;

  for (int i = 0; i < 1000; ++i) {
    phTest(&a, &b, &col);
    col.a = &a;
    col.b = &b;
    phSolve(&col);
    a.position = phZero();
    b.position = phv(0.5, 0);
  }
}

void bench_particle() {
  {
    note("\n** particle **");
    bench(
      "phTest 1,000 touching", b_testtouch1000, NULL,
      "phTest 1,000 not touching", b_testnotouch1000, NULL,
      NULL
    );

    bench(
      "phTest, phSolve 1,000 touching", b_testsolve1000, NULL,
      NULL
    );
  }
}
