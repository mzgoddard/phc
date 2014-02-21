#ifndef PH_H_7BP0TPQV
#define PH_H_7BP0TPQV

#include <stdlib.h>

#ifndef phdouble
#define phdouble double
#endif

#ifndef phint
#define phint int
#endif

#ifndef phbool
#define phbool _Bool
#endif

typedef struct phv {
  phdouble x, y;
} phv;

#define phv(x, y) ((phv) {x, y})

typedef struct phm {
  phdouble aa, ab, ba, bb;
} phm;

#define phm(a, b, c, d) ((phm) {a, b, c, d})

typedef struct phbox {
  union {
    phdouble values[4];
    struct {
      phdouble left, top, right, bottom;
    };
  };
} phbox;

#define phbox(a, b, c, d) ((phbox) {a, b, c, d})

typedef struct phray {
  phv position, direction;
} phray;

#define phray(p, d) ((phray) {p, d})

typedef phbool (*phiteratornext)(void *);
typedef void * (*phiteratorderef)(void *);

typedef struct phiterator {
  phiteratornext next;
  phiteratorderef deref;
} phiterator;

typedef void (*phitrfn)(void *ctx, void *item);
typedef void (*phitrstopfn)(void *ctx, void *item, phbool *stop);

typedef struct phlistnode {
  struct phlistnode *next, *prev;
  void *item;
} phlistnode;

typedef struct phlist {
  phlistnode *first, *last, *freeList;
  phint length;
} phlist;

#define phlist() ((phlist) {NULL, NULL, NULL, 0})

typedef struct phlistiterator {
  void *_value[4];
} phlistiterator;

typedef struct phcollision {
} phcollision;

typedef struct phparticle {
  phv position, lastPosition;
  phdouble radius, mass, friction, factor;
  phbool isStatic;
  phbool isTrigger;
  phbool isSleeping;
  phlist ignore;

  void *data;
  void (*collide)(
    struct phparticle *, struct phparticle *, struct phcollision *
  );

  void *_worldData;
} phparticle;

typedef struct phparticleworlddata {
  phint sleepCounter;
  phlist collisionWith;
  phv oldPosition;
  phbox box, oldBox;
} phparticleworlddata;

typedef struct phddvt {
  phbox box;
  phlist particles;
  phint maxParticles;
  union {
    struct phddvt *children[4];
    struct {
      struct phddvt *tl;
      struct phddvt *tr;
      struct phddvt *bl;
      struct phddvt *br;
    };
  };
} phddvt;

typedef struct phworld {
  phlist particles;
  phlist particleData;
  struct {
    phlist beforeStep, afterStep;
  } constraints;
  struct {
    phbool allowSleep;
    phint maxSleepCounter;
  } config;
  struct {
    phdouble dt;
    phdouble dtInterval;
    phint maxSubSteps;
  } timing;
  struct {
    phbox box;
    phddvt ddvt;
  } _optimization;
  phint particlesAlive;
} phworld;

typedef char phconstraintid[];

typedef struct phconstrainttype {
  phconstraintid *id;
  phbool (*beforeStep)(void *);
  void (*update)(void *, phworld *);
} phconstrainttype;

#define phconstrainttype(id, before, update) ((phconstrainttype) { \
  id, before, update \
})

typedef struct phconstraint {
  phconstrainttype *type;
} phconstraint;

typedef struct phgravity {
  phconstrainttype *type;
  phv acceleration;
} phgravity;

typedef struct pgpeg {
  phconstrainttype *type;
  phparticle *a, *b;
} pgpeg;

typedef struct phstick {
  phconstrainttype *type;
  phparticle *a, *b;
  phv length;
  phv factor;
} phstick;

phv phZero();

phv phAdd(phv, phv);
phv phSub(phv, phv);
phv phMul(phv, phv);
phv phDiv(phv, phv);
phv phScale(phv, phdouble);
phdouble phDot(phv, phv);
phdouble phCross(phv, phv);
phdouble phMag2(phv);
phdouble phMag(phv);
phv phUnit(phv);

// phM, Matrix functions

phm phIdentity();
phv phXf(phm, phv);

// phBox

phbox phAabb(phv, phdouble);
phbool phContain(phbox, phbox);
phbool phIntersect(phbox, phbox);

// phList, List functions

void phClean(phlist *, void (*freeFn)(void *));
void phDump(phlist *, void (*freeFn)(void *));
phiterator * phIterator(phlist *, phlistiterator *);
phlist * phAppend(phlist *, void *);
phlist * phInsert(phlist *, int index, void *);
phlist * phRemove(phlist *, void *);

// phIterator

phbool phNext(phiterator *);
void * phDeref(phiterator *);
void phIterate(phiterator *, phitrfn, void *);
void phIterateN(phiterator *, phint, phitrfn, void *);
void phIterateStop(phiterator *, phitrstopfn, void *);

void phParticleDump(phparticle *);

void phIntegrate(phparticle *, phdouble dt);
void phTest(phparticle *, phparticle *, phcollision *);
void phSolve(phparticle *, phparticle *, phcollision *);
void phIgnore(phparticle *, phparticle *);
void phStopIgnore(phparticle *, phparticle *);
phv phVelocity(phparticle *);
phv phScaledVelocity(phparticle *, phdouble dt);
void phSetVelocity(phparticle *, phv);
void phSetScaledVelocity(phparticle *, phv, phdouble dt);

phbool phConstraintBeforeStep(void *);
void phConstraintSetWorld(void *, phworld *);
void phconstraintUpdate(void *, phdouble dt);

void phDdvtDump(phddvt *);
void phDdvtAdd(phddvt *, phparticle *);
void phDdvtRemove(phddvt *, phparticle *);
void phDdvtUpdate(phddvt *, phparticle *);
phiterator * phDdvtIterator(phddvt *, phiterator *);
phiterator * phDdvtPairIterator(phddvt *, phiterator *);

void phWorldDump(phworld *);
phparticle * phWorldRayCast(phworld *, phray);
phiterator * phWorldRayIterator(phworld *, phray, phiterator *);
phiterator * phWorldBoxIterator(phworld *, phbox, phiterator *);

phworld * phWorldAddParticle(phworld *, phparticle *);
phworld * phWorldRemoveParticle(phworld *, phparticle *);
phworld * phWorldAddConstraint(phworld *, phconstraint *);
phworld * phWorldRemoveConstraint(phworld *, phconstraint *);

// void phFreeParticle(phparticle *);
// void phFreeGravity(phgravity *);

#ifndef phAlloc
#define phAlloc(type) ((type *) malloc(sizeof(type)))
#endif

#ifndef phFree
#define phFree(data) (free(data))
#endif

#endif /* end of include guard: PH_H_7BP0TPQV */
