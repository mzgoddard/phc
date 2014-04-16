#define kFrameFraction ( 1.0 / 60 )
#define kFrameStep ( 1.0 / 60 )
#ifndef kParticleCount
#define kParticleCount 8192
#endif
#ifndef kParticleBaseSize
#define kParticleBaseSize 3
#endif
#ifndef kParticleSizeRange
#define kParticleSizeRange 1
#endif 

static const int particle_count = kParticleCount;
extern phworld *world;

void init();
void step(float dt);
void setGetTicksFunction(unsigned int (*_getTicks)());
