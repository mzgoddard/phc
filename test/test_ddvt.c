#include "ph.h"
#include "tap.h"

#define ddvtdata() \
  phparticle particles[8]; \
  phddvt ddvt = phddvt(NULL, phbox(-4, 4, 4, -4), 2, 4); \
  for (phint i = 7; i >= 0; --i) { \
    particles[i] = phparticle(phv((i % 2) * 4 - 2, (i / 2 % 2) * -4 + 2)); \
    phparticle *p = &particles[i]; \
    particles[i]._worldData.oldBox = phAabb(p->position, p->radius); \
    phDdvtAdd(&ddvt, p); \
  }

void test_ddvt() {
  note("\n** ddvt **");

  {
    note("phDdvtAdd");
    phparticle a = phparticle(phZero());
    // phparticleworlddata a_data = phparticleworlddata();
    // a._worldData = &a_data;
    a._worldData.oldBox = phAabb(a.position, a.radius);
    phddvt ddvt = phddvt(NULL, phbox(-4, 4, 4, -4), 2, 4);
    phDdvtAdd(&ddvt, &a);
    ok(ddvt.length != 0, "length counts particle");
    phDdvtDump(&ddvt);
  }

  {
    note("phDdvtRemove");
    phparticle a = phparticle(phZero());
    // phparticleworlddata a_data = phparticleworlddata();
    // a._worldData = &a_data;
    a._worldData.oldBox = phAabb(a.position, a.radius);
    phddvt ddvt = phddvt(NULL, phbox(-4, 4, 4, -4), 2, 4);
    phDdvtAdd(&ddvt, &a);
    ok(ddvt.length != 0, "length counts particle");
    phDdvtRemove(&ddvt, &a, a._worldData.oldBox);
    ok(ddvt.length == 0, "ddvt is empty now");
    phDdvtDump(&ddvt);
  }

  {
    note("phDdvtToChildren");
    ddvtdata();
    ok(ddvt.length == 8, "all of the particles are in the ddvt");
    ok(ddvt.tl != NULL, "ddvt is a parent");
    phDdvtDump(&ddvt);
  }

  {
    note("phDdvtFromChildren");
    ddvtdata();
    // Remove 6
    for (phint i = 0; i < 6; ++i) {
      phDdvtRemove(&ddvt, &particles[i], particles[i]._worldData.oldBox);
    }
    ok(
      ddvt.length == 2,
      "right number of particles removed, %d == 2",
      ddvt.length
    );
    ok(ddvt.tl == NULL, "ddvt switched back to a leaf");
    phDdvtDump(&ddvt);
  }

  {
    note("phDdvtIterator shallow");
    ddvtdata();
    // Remove 6
    for (phint i = 2; i < 8; ++i) {
      phDdvtRemove(&ddvt, &particles[i], particles[i]._worldData.oldBox);
    }
    ok(ddvt.length == 2, "right number of particles removed");
    ok(ddvt.tl == NULL, "ddvt switched back to a leaf");

    phint count = 0;
    phddvtiterator ddvtitr;
    phiterator *itr = phDdvtIterator(&ddvt, &ddvtitr);
    while (phNext(itr)) {
      ok(phDeref(itr) != NULL, "iterator points to an object");
      // 0, 1
      ok(
        phDeref(itr) == &particles[count],
        "points at expected particle (index %d)",
        count
      );
      count++;
    }
    ok(count == 2, "iterate 2 (%d == 2) elements in the ddvt", count);
    phDdvtDump(&ddvt);
  }

  {
    note("phDdvtIterator deep");
    ddvtdata();
    phddvtiterator ddvtitr;
    phiterator *itr = phDdvtIterator(&ddvt, &ddvtitr);
    phint count = 0;
    while (phNext(itr)) {
      ok(phDeref(itr) != NULL, "iterator points to an object");
      // 0, 4, 1, 5, 2, 6, 3, 7
      ok(
        phDeref(itr) == &particles[((count + 1) % 2) * 4 + (count / 2)],
        "points at expected particle (index %d)",
        ((count + 1) % 2) * 4 + (count / 2)
      );
      count++;
    }
    ok(count == 8, "iterate each element in the ddvt");
    phDdvtDump(&ddvt);
  }

  {
    note("phDdvtPairIterator");
    ddvtdata();
    phddvtpairiterator ddvtitr;
    phiterator *itr = phDdvtPairIterator(&ddvt, &ddvtitr);
    phint count = 0;
    while (phNext(itr)) {
      // ok(ddvtitr.ddvt->_particleArray.capacity == 2, "array is length 2");
      // ok(ddvtitr.particles.capacity == 2, "array is length 2");
      // ok(ddvtitr.arrayItr2.items < ddvtitr.arrayItr2.end);
      // ok(ddvtitr.arrayItr2.test);
      // ok(ddvtitr.arrayItr2.items == ddvtitr.particles.items + 1, "itr2 points");
      phddvtpair *pair = phDeref(itr);
      ok(pair == &ddvtitr.pair, "pair is pointer to static pair iterator");
      if (pair == NULL) {
        ok(pair != NULL, "pair is not NULL");
        continue;
      }
      ok(pair->a != NULL, "pair->a is not NULL");
      ok(pair->b != NULL, "pair->b is not NULL");
      ok(pair->a != pair->b, "pair does not point to same objects");
      count++;
    }
    ok(count == 4, "iterates over 4 pairs (iterated %d pairs)", count);

    for (phint i = 4; i < 8; ++i) {
      phDdvtRemove(&ddvt, &particles[i], particles[i]._worldData.oldBox);
    }
    ok(ddvt.tl != NULL, "still in parent mode");
    itr = phDdvtPairIterator(&ddvt, &ddvtitr);
    count = 0;
    while (phNext(itr)) {
      count++;
    }
    ok(count == 0, "iterates over 0 pairs (iterated %d pairs)", count);
    phDdvtDump(&ddvt);
  }
}
