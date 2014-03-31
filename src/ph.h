#ifndef PH_H_7BP0TPQV
#define PH_H_7BP0TPQV

#include <stdlib.h>
#include <tgmath.h>

// https://groups.google.com/forum/#!topic/comp.std.c/d-6Mj5Lko_s
#define PP_NARG(...) \
  PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
  PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
  _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
  _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
  _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
  _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
  _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
  _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
  _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
  63,62,61,60, \
  59,58,57,56,55,54,53,52,51,50, \
  49,48,47,46,45,44,43,42,41,40, \
  39,38,37,36,35,34,33,32,31,30, \
  29,28,27,26,25,24,23,22,21,20, \
  19,18,17,16,15,14,13,12,11,10, \
  9,8,7,6,5,4,3,2,1,0

// Memory

#ifndef phAlloc
#define phAlloc(type) ((type *) malloc(sizeof(type)))
#endif

#ifndef phFree
#define phFree(data) (free(data))
#endif

// Arithmetic types

#ifndef phdouble
#define phdouble double
#endif

#ifndef phint
#define phint int
#endif

#ifndef phchar
#define phchar char
#endif

#ifndef phbyte
#define phbyte unsigned char
#endif

#ifndef phbool
#define phbool _Bool
#endif

// Large Arithmetic Types

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

// Iteration Types

typedef phbool (*phiteratornext)(void *);
typedef void * (*phiteratorderef)(void *);

typedef struct phiterator {
  phiteratornext next;
  phiteratorderef deref;
} phiterator;

typedef void (*phitrfn)(void *ctx, void *item);
typedef void (*phitrstopfn)(void *ctx, void *item, phbool *stop);
typedef phbool (*phcmpfn)(void *ctx, void *item);

typedef struct phlistnode {
  struct phlistnode *next, *prev;
  void *item;
} phlistnode;

typedef struct phlist {
  phlistnode *first, *last, *freeList;
  phint length;
} phlist;

#define phlist() ((phlist) {NULL, NULL, NULL, 0})
#define phlistv(...) ({ \
  pharray array = pharrayv(__VA_ARGS__); \
  pharrayiterator _itr; \
  phlist list = phlist(); \
  phIterate(phArrayIterator(&array, &_itr), (phitrfn) phAppend, &list); \
  list; \
})

typedef struct phlistiterator {
  phiterator iterator;
  phlist *list;
  phlistnode *node;
  phlistnode fakeHead;
} phlistiterator;

#define _phlistiterator phlistiterator
// typedef struct _phlistiterator {
//   phiterator iterator;
//   phlist *list;
//   phlistnode *node;
//   phlistnode fakeHead;
// } _phlistiterator;

typedef struct pharray {
  phint capacity;
  void **items;
} pharray;

#define pharray(c, items) ((pharray) {c, items})
#define pharrayv(...) ((pharray) { \
  PP_NARG(__VA_ARGS__), \
  ((void *[]) {__VA_ARGS__}) \
})

typedef struct pharrayiterator {
  void *_value1;
  void *_value2;
  void **_value3;
  void **_value4;
  phbool _value5;
} pharrayiterator;

typedef struct _pharrayiterator {
  phiterator iterator;
  void **items;
  void **end;
  phbool test;
} _pharrayiterator;

#define pharrayiterator(array) ((pharrayiterator) { \
  (phiteratornext) phArrayNext, (phiteratorderef) phArrayDeref, \
  array->items - 1, array->items + array->capacity, 0 \
})

// Physics Types

typedef struct phcollision {
  void *a, *b;
  phdouble ingress;
  phdouble distance;
  phdouble lambx, lamby;
} phcollision;

typedef struct phparticleworlddata {
  phint sleepCounter;
  phlist collideWith;
  phv oldPosition;
  phbox box, oldBox;
} phparticleworlddata;

typedef struct phparticle {
  phv position, lastPosition, acceleration;
  phdouble radius, mass, friction, factor;
  phbool isStatic;
  phbool isTrigger;
  phbool isSleeping;
  phlist ignore;

  phint collideMask;
  phint collideAgainst;

  void *data;
  void (*collide)(
    struct phparticle *, struct phparticle *, struct phcollision *
  );

  phparticleworlddata *_worldData;
} phparticle;

#define phparticle(pos) ((phparticle) { \
  pos, pos, phZero(), \
  1, 1, 1, 0.5, \
  0, 0, 0, phlist(), \
  ~0, ~0, \
  NULL, NULL, \
  NULL \
})

#define phparticleworlddata() ((phparticleworlddata) { \
  0, phlist(), phv(0, 0), phbox(0, 0, 0, 0), phbox(0, 0, 0, 0) \
})

typedef struct phddvt {
  phbox box;
  phlist particles;
  phint length;
  phint minParticles;
  phint maxParticles;
  phbool isSleeping;
  struct phddvt *parent;
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

typedef struct phddvtpair {
  union {
    phparticle *pair[2];
    struct {
      phparticle *a;
      phparticle *b;
    };
  };
} phddvtpair;

#define phddvt(parent, box, min, max) ((phddvt) { \
  box, phlist(), 0, min, max, 0, \
  parent, NULL, NULL, NULL, NULL \
})

#define phddvtpair(a, b) ((phddvtpair) {a, b})

typedef struct phddvtiterator {
  phiterator iterator;
  phddvt *topDdvt;
  phddvt *ddvt;
  phlistiterator leafItr;
} phddvtiterator;

#define phddvtiterator(ddvt) ((phddvtiterator) { \
  (phiteratornext) phDdvtNext, (phiteratorderef) phDdvtDeref, \
  ddvt->parent, ddvt, \
  NULL, NULL, NULL, NULL, NULL, NULL, NULL \
})

typedef struct phddvtpairiterator {
  phiterator iterator;
  phddvt *topDdvt;
  phddvt *ddvt;
  phlistiterator leafItr1;
  phlistiterator leafItr2;
  phddvtpair pair;
} phddvtpairiterator;

#define phddvtpairiterator(ddvt) ((phddvtpairiterator) { \
  (phiteratornext) phDdvtPairNext, (phiteratorderef) phDdvtPairDeref, \
  ddvt->parent, ddvt, \
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, \
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, \
  NULL, NULL \
})

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
    phlist collisions;
    phlist nextCollisions;
  } _optimization;
  phint particlesAlive;
} phworld;

#define phworld(box) ((phworld) { \
  phlist(), phlist(), \
  phlist(), phlist(), \
  1, 20, \
  1 / 60.0, 0, 10, \
  box, phddvt(NULL, box, 64, 128) \
  0 \
})

typedef struct phrayiterator {
  phiterator iterator;
} phrayiterator;

typedef struct phboxiterator {
  phiterator iterator;
} phboxiterator;

typedef phchar phconstraintid[];

typedef struct phconstrainttype {
  phconstraintid *id;
  void * (*copy)(void *, phlist *, phlist *);
  phbool (*beforeStep)(void *);
  void (*update)(void *, phworld *);
} phconstrainttype;

#define phconstrainttype(id, copy, before, update) ((phconstrainttype) { \
  id, copy, before, update \
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

#ifdef EMSCRIPTEN
// Compiling under emscripten complains about implicit sqrt declaration.
double sqrt(double);
#endif

static phv phZero() {return phv(0, 0);}

static phv phAdd(phv a, phv b) {
  return phv(a.x + b.x, a.y + b.y);
}

static phv phSub(phv a, phv b) {
  return phv(a.x - b.x, a.y - b.y);
}

static phv phMul(phv a, phv b) {
  return phv(a.x * b.x, a.y * b.y);
}

static phv phDiv(phv a, phv b) {
  return phv(a.x / b.x, a.y / b.y);
}

static phv phScale(phv a, phdouble s) {
  return phv(a.x * s, a.y * s);
}

static phdouble phDot(phv a, phv b) {
  return a.x * b.x + a.y * b.y;
}

static phdouble phCross(phv a, phv b) {
  return a.x * b.y - a.y * b.x;
}

static phdouble phMag2(phv a) {
  return phDot(a, a);
}

static phdouble phMag(phv a) {
  return sqrt(phMag2(a));
}

static phdouble phHypot2(phdouble a, phdouble b) {
  return a * a + b * b;
}

static phdouble phHypot(phdouble a, phdouble b) {
  return sqrt(a * a + b * b);
}

static phv phUnit(phv a) {
  phdouble mag = phMag(a);
  return phScale(a, 1.0 / mag);
}

// phM, Matrix functions

phm phIdentity();
phv phXf(phm, phv);

// phBox

static phbox phAabb(phv c, phdouble r) {
  return phbox(c.x - r, c.y + r, c.x + r, c.y - r);
}

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

static phbox phCombine(phbox a, phbox b) {
  return phbox(
    MIN(a.left, b.left), MAX(a.top, b.top),
    MAX(a.right, b.right), MIN(a.bottom, b.bottom)
  );
}

static phbool phIntersect(phbox a, phbox b) {
  return a.left <= b.right && a.right >= b.left &&
    a.bottom <= b.top && a.top >= b.bottom;
}

static phbox phBoxTranslate(phbox a, phv t) {
  a.left += t.x;
  a.top += t.y;
  a.right += t.x;
  a.bottom += t.y;
  return a;
}

// define CENTERX for corner box functions phTL, phTR, phBL, phBR
#define CENTERX (box.left + box.right) / 2
#define CENTERY (box.bottom + box.top) / 2

static phbox phTL(phbox box) {
  return phbox(box.left, box.top, CENTERX, CENTERY);
}

static phbox phTR(phbox box) {
  return phbox(CENTERX, box.top, box.right, CENTERY);
}

static phbox phBL(phbox box) {
  return phbox(box.left, CENTERY, CENTERX, box.bottom);
}

static phbox phBR(phbox box) {
  return phbox(CENTERX, CENTERY, box.right, box.bottom);
}

// phList, List functions

void phClean(phlist *, void (*freeFn)(void *));
void phDump(phlist *, void (*freeFn)(void *));
phlist * phAppend(phlist *, void *);
phlist * phInsert(phlist *, int index, void *);
phlist * phRemove(phlist *, void *);

#define phlistiterator(list, node) ((phlistiterator) { \
  (phiteratornext) phListNext, (phiteratorderef) phListDeref, list, node, \
  list->first, NULL, NULL \
})

static phbool phListNext(_phlistiterator *self) {
  phlistnode *node = self->node;
  if (node) {
    self->node = node->next;
  }
  return self->node != NULL;
}

static void * phListDeref(_phlistiterator *self) {
  phlistnode *node = self->node;
  return node ? node->item : NULL;
}

static phiterator * phIterator(phlist *self, phlistiterator *itr) {
  if (itr == NULL) {
    itr = phAlloc(phlistiterator);
  }
  *itr = phlistiterator(self, NULL);
  ((_phlistiterator *) itr)->node = &((_phlistiterator *) itr)->fakeHead;
  return (phiterator *) itr;
}

// phIterator

static phbool phNext(phiterator *self) {
  return self->next(self);
}

static void * phDeref(phiterator *self) {
  return self->deref(self);
}

static void phStaticIterate(
  phbool (*next)(phiterator *),
  void * (*deref)(phiterator *),
  phiterator *self,
  phitrfn itr,
  void *ctx
) {
  while (next(self)) {
    itr(ctx, deref(self));
  }
}

static void phIterate(phiterator *self, phitrfn itr, void *ctx) {
  while (phNext(self)) {
    itr(ctx, phDeref(self));
  }
}

void phIterateN(phiterator *, phint, phitrfn, void *);
void phIterateStop(phiterator *, phitrstopfn, void *);

phiterator * phFind(phiterator *, phcmpfn, void *);
phint phIndexOf(phiterator *, phcmpfn, void *);
phbool phContains(phiterator *, void *);

void * phGetIndex(phiterator *, phint);
phbool phSame(void *ctx, void *item);

pharray * phToArray(phiterator *, pharray *);

static phbool phArrayNext(_pharrayiterator *self) {
  self->items++;
  self->test = self->items < self->end;
  return self->test;
}

static void * phArrayDeref(_pharrayiterator *self) {
  return self->test ? *(self->items) : NULL;
}

static phiterator * phArrayIterator(pharray *self, pharrayiterator *itr) {
  if (itr == NULL) {
    itr = phAlloc(pharrayiterator);
  }
  *itr = pharrayiterator(self);
  return (phiterator *) itr;
}

// phParticle

void phParticleDump(phparticle *);
void phParticleCopy(phparticle *, phparticle *);

void phIntegrate(phparticle *, phdouble dt);
phbool phTest(phparticle *, phparticle *, phcollision *);
void phSolve(phparticle *, phparticle *, phcollision *);
// Ignore another particle or it's data.
void phIgnore(phparticle *, void *);
void phStopIgnore(phparticle *, void *);
phv phVelocity(phparticle *);
phv phScaledVelocity(phparticle *, phdouble dt);
void phSetVelocity(phparticle *, phv);
void phSetScaledVelocity(phparticle *, phv, phdouble dt);

// phConstraint

phconstraint * phConstraintCopy(void *, phlist *dst, phlist *src);
phbool phConstraintBeforeStep(void *);
void phConstraintUpdate(void *, phworld *);

// phDdvt

void phDdvtDump(phddvt *);
void phDdvtAdd(phddvt *, phparticle *);
void phDdvtRemove(phddvt *, phparticle *, phbox);
void phDdvtWake(phddvt *, phparticle *);
void phDdvtUpdate(phddvt *, phparticle *, phbox, phbox);
phiterator * phDdvtIterator(phddvt *, phddvtiterator *);
phiterator * phDdvtPairIterator(phddvt *, phddvtpairiterator *);

phbool phDdvtNext(phddvtiterator *);
void * phDdvtDeref(phddvtiterator *);

// phWorld

void phWorldDump(phworld *);
void phWorldStep(phworld *, phdouble);
phparticle * phWorldRayCast(phworld *, phray);
phiterator * phWorldRayIterator(phworld *, phray, phrayiterator *);
phiterator * phWorldBoxIterator(phworld *, phbox, phboxiterator *);

phworld * phWorldAddParticle(phworld *, phparticle *);
phworld * phWorldRemoveParticle(phworld *, phparticle *);
phworld * phWorldAddConstraint(phworld *, phconstraint *);
phworld * phWorldRemoveConstraint(phworld *, phconstraint *);

// phComposite

phbyte * phCompositeFreeze(phlist *, phlist *);
void phCompositeThaw(phlist *, phlist *, phbyte *);
void phCompositeCopy(phlist *dst1, phlist *dst2, phlist *src1, phlist *src2);

#endif /* end of include guard: PH_H_7BP0TPQV */
