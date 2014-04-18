#include <stdio.h>

#include "ph.h"

#include "./env.h"

phworld *world;
static phv gravity;

unsigned int (*getTicks)() = NULL;

void init() {
  int startTime = 0;
  int endTime = 0;

  if ( getTicks ) {
    startTime = getTicks();
  }

  phv viewportSize = phv(640, 640);
  world = phCreate(phworld, phbox(0, viewportSize.y, viewportSize.x, 0));
  world->timing.maxSubSteps = 1;
  gravity = phv(0, -0.01 / kFrameStep / kFrameStep);

  for (int i = 0; i < particle_count; ++i) {
    phparticle *particle = phCreate(phparticle, phZero());
    particle->friction = 0.01;
    particle->factor = 0.5 + (float) rand() / RAND_MAX / 2.5;
    particle->position = phv(
      rand() % (int) world->_optimization.box.right,
      rand() % (int) world->_optimization.box.top / 3 * 2
    );
    particle->lastPosition = particle->position;
    particle->radius =
      kParticleBaseSize + (float) rand() / RAND_MAX * kParticleSizeRange;
    particle->mass = M_PI * particle->radius * particle->radius;
    if (rand() > RAND_MAX * 0.99) {
      // particle->radius += 10;
      particle->mass *= 1000;
    }
    phWorldAddParticle(world, particle);
  }

  if ( getTicks ) {
    endTime = getTicks();
  }

  #ifdef FRAMESTATS
  printf("init %dms\n", endTime - startTime);
  #endif
}

void setWaterTestGravity(float _gravity[3]) {
  gravity.x = _gravity[1] * 32;
  gravity.y = -_gravity[0] * 32;
  // printf("%f %f %f\n", gravity[0], gravity[1], gravity[2]);
}

static void gravityIterator(void *ctx, phparticle *particle) {
  particle->acceleration = phAdd(particle->acceleration, gravity);
}

static void containIterator(phworld *world, phparticle *particle) {
  phbox box = world->_optimization.box;
  phv p = particle->position;
  phv lp = particle->lastPosition;
  phdouble r = particle->radius;
  if (p.x + r > box.right) {
    lp.x = (p.x - lp.x) * 0.5 + lp.x;
    p.x = box.right - r;
  } else if (p.x - r < box.left) {
    lp.x = (p.x - lp.x) * 0.5 + lp.x;
    p.x = box.left + r;
  }
  if (p.y + r > box.top) {
    lp.y = (p.y - lp.y) * 0.5 + lp.y;
    p.y = box.top - r;
  } else if (p.y - r < box.bottom) {
    lp.y = (p.y - lp.y) * 0.5 + lp.y;
    p.y = box.bottom + r;
  }
  particle->position = p;
}

static void gravityContainHandle(phworld *world, phparticle *particle) {
  gravityIterator(NULL, particle);
  containIterator(world, particle);
}

void setGetTicksFunction(unsigned int (*_getTicks)()) {
  getTicks = _getTicks;
}

#define INT_MAX 0x7fffffff

unsigned int minTime( unsigned int frameTimes[], unsigned int length ) {
  unsigned int min = INT_MAX;
  int i = 0;
  for ( ; i < length; ++i ) {
    int frame = frameTimes[ i ];
    if ( frame < min ) {
      min = frame;
    }
  }
  return min;
}

unsigned int maxTime( unsigned int frameTimes[], unsigned int length ) {
  unsigned int max = 0;
  int i = 0;
  for ( ; i < length; ++i ) {
    int frame = frameTimes[ i ];
    if ( frame > max ) {
      max = frame;
    }
  }
  return max;
}

unsigned int avgTime( unsigned int frameTimes[], unsigned int length ) {
  unsigned int sum = 0;
  int i = 0;
  for ( ; i < length; ++i ) {
    sum += frameTimes[ i ];
  }
  return sum / length;
}

float stddevTime( unsigned int frameTimes[], unsigned int length ) {
  unsigned int avg = avgTime( frameTimes, length );
  unsigned int diffSum = 0;
  int i = 0;
  for ( ; i < length; ++i ) {
    int diff = frameTimes[ i ] - avg;
    diffSum += diff * diff;
  }
  return sqrt( diffSum / (float) length );
}

static struct applicationEvents {
  void (*stepStart)();
  void (*stepEnd)();
} applicationEvents = {
  NULL,
  NULL
};

typedef void (*EventHandle)();
void setEventListener( int index, void (*handle)() ) {
  ((EventHandle *) &applicationEvents )[ index ] = handle;
}

static float hertztime = 0;
static float fpstime = 0;
static int frames = 0;
static const unsigned int kMaxFrameTimes = 100;
static unsigned int frameTimes[ 100 ];
static int frameTimeIndex = 0;

void step(float dt) {
  hertztime += dt;

  int startTime = 0;
  int endTime = 0;

  if (hertztime > kFrameFraction) {
    if ( getTicks ) {
      startTime = getTicks();
    }
    if ( applicationEvents.stepStart ) {
      applicationEvents.stepStart();
    }

    frames++;
    phlistiterator _litr;
    phiterator *itr = phIterator(&world->particles, &_litr);
    phStaticIterate(
      (phiteratornext) phListNext, (phiteratorderef) phListDeref,
      itr, (phitrfn) gravityContainHandle, world
    );
    // itr = phIterator(&world->particles, &_litr);
    // phIterate(itr, (phitrfn) containIterator, world);
    phWorldInternalStep(world);
    while (hertztime > kFrameFraction) {
      hertztime -= kFrameFraction;
    }

    if ( applicationEvents.stepEnd ) {
      applicationEvents.stepEnd();
    }
    if ( getTicks ) {
      endTime = getTicks();
    }
  }

  fpstime += dt;
  if (fpstime > 1) {
    // printf("%d frames in %fs\n", frames, fpstime);
    fpstime = 0;
    frames = 0;
  }

  if ( getTicks && startTime != 0 ) {
    frameTimes[ frameTimeIndex++ ] = endTime - startTime;
    if ( frameTimeIndex >= kMaxFrameTimes ) {
      #ifdef FRAMESTATS
      printf(
        "min %d, max %d, avg %d, stddev %f\n",
      minTime( frameTimes, kMaxFrameTimes ),
      maxTime( frameTimes, kMaxFrameTimes ),
      avgTime( frameTimes, kMaxFrameTimes ),
      stddevTime( frameTimes, kMaxFrameTimes )
        );
      #endif
      frameTimeIndex = 0;
    }
  }
}
