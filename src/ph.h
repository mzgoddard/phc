#ifndef PH_H_7BP0TPQV
#define PH_H_7BP0TPQV

#include <stdlib.h>

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
  void *_value[4];
} phlistiterator;

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
  void *_value1[3];
  phint _value2;
} pharrayiterator;

typedef struct phcollision {
  phdouble ingress;
  phdouble lx, ly;
} phcollision;

typedef struct phparticle {
  phv position, lastPosition;
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

typedef struct phddvtiterator {
  phiterator iterator;
} phddvtiterator;

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
phbox phCombine(phbox, phbox);
phbool phContain(phbox, phbox);
phbool phIntersect(phbox, phbox);
phbox phBoxTranslate(phbox, phv);

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

phiterator * phFind(phiterator *, phcmpfn, void *);
phint phIndexOf(phiterator *, phcmpfn, void *);

void * phGetIndex(phiterator *, phint);
phbool phSame(void *ctx, void *item);

pharray * phToArray(phiterator *, pharray *);
phiterator * phArrayIterator(pharray *, pharrayiterator *);

// phParticle

void phParticleDump(phparticle *);
void phParticleCopy(phparticle *, phparticle *);

void phIntegrate(phparticle *, phdouble dt);
void phTest(phparticle *, phparticle *, phcollision *);
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
void phDdvtRemove(phddvt *, phparticle *);
void phDdvtUpdate(phddvt *, phparticle *);
phiterator * phDdvtIterator(phddvt *, phddvtiterator *);
phiterator * phDdvtPairIterator(phddvt *, phddvtiterator *);

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

// Memory

#ifndef phAlloc
#define phAlloc(type) ((type *) malloc(sizeof(type)))
#endif

#ifndef phFree
#define phFree(data) (free(data))
#endif

#endif /* end of include guard: PH_H_7BP0TPQV */
