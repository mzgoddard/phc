#include "ph.h"
#include "tap.h"

void test_world() {
  note("\n** world **");

  {
    note("Add and Remove particle");
    phworld world = phworld(phbox(-16, 16, 16, -16));
    phparticle p = phparticle(phv(0, 0));
    phWorldAddParticle(&world, &p);
    ok(world.particles.length == 1, "1 particle in world");
    ok(world._optimization.ddvt.particles.length == 1, "1 particle in ddvt");
    phlistiterator litr;
    phiterator *itr = phIterator(&world.particles, &litr);
    phIterate(itr, (phitrfn) phWorldRemoveParticle, &world);
    ok(world.particles.length == 0, "0 particles in world");
    ok(world._optimization.ddvt.particles.length == 0, "0 particles in ddvt");
    phWorldDump(&world);
  }

  {
    note("phWorldStep");
    phworld world = phworld(phbox(-16, 16, 16, -16));
    phparticle p = phparticle(phv(0, 0));
    phWorldAddParticle(&world, &p);
    ok(world.particles.length == 1, "1 particle in world");
    ok(world._optimization.ddvt.particles.length == 1, "1 particle in ddvt");
    phWorldStep(&world, 0.02);
    ok(phMag(phSub(p.position, phv(0, 0))) < 0.001, "particle didn't move");

    phparticle q = phparticle(phv(0.5, 0));
    phWorldAddParticle(&world, &q);
    ok(world.particles.length == 2, "2 particles in world");
    ok(world._optimization.ddvt.particles.length == 2, "2 particles in ddvt, %d", world._optimization.ddvt.particles.length);
    phWorldStep(&world, 0.02);
    ok(world._optimization.collisions.length == 0, "collisions is empty");
    ok(phMag(phSub(p.position, phv(0, 0))) > 0.001, "particle moved");
    ok(phMag(phSub(q.position, phv(0.5, 0))) > 0.001, "other particle moved");
    phWorldDump(&world);
  }
}
