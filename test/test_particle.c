#include "ph.h"
#include "tap.h"

void test_particle() {
  note("\n** particle **");

  {
    note("phTest, phSolve");
    phparticle a = phparticle(phZero());
    phparticle b = phparticle(phv(0.5, 0));
    phcollision col;
    ok(phTest(&a, &b, &col), "particle a and b collide.");
    phSolve(&a, &b, &col);
    phdouble adiff = fabs(phMag(phSub(a.position, phv(-0.1875, 0))));
    phdouble bdiff = fabs(phMag(phSub(b.position, phv(0.6875, 0))));
    ok(adiff < 0.001, "diff of expected a position, %f < 0.001", adiff);
    ok(bdiff < 0.001, "diff of expected b position, %f < 0.001", bdiff);
  }

  {
    note("phIgnore");
    phparticle a = phparticle(phZero());
    phparticle b = phparticle(phv(0.5, 0));
    phparticle c = phparticle(phv(-0.5, 0));
    phint arbitrary = 128;
    c.data = &arbitrary;
    phcollision col;
    phIgnore(&a, &b);
    ok(!phTest(&a, &b, &col), "a ignored collision with b.");
    phIgnore(&a, &arbitrary);
    ok(!phTest(&a, &c, &col), "a ignored collision with given data.");
    phStopIgnore(&a, &arbitrary);
    ok(phTest(&a, &c, &col), "phStopIgnore removed data. Allowed collision.");
    phIgnore(&a, &arbitrary);
    phStopIgnore(&a, &b);
    ok(phTest(&a, &b, &col), "phStopIgnore removed b. Allowed collision.");
  }
}
