#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if !EMSCRIPTEN
#define USE_GLEW 0
#endif

#if USE_GLEW
#include "GL/glew.h"
#endif

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#if !USE_GLEW
#include "SDL/SDL_opengl.h"
#endif

#ifdef AUDIO_OPENAL
#include "openal.h"
#endif

#if EMSCRIPTEN
#include <emscripten.h>
#include "platform/window.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "opengl.h"

#include "ph.h"

void main_loop();
static void process_events();

#if !EMSCRIPTEN
static const char * shader_fragment_text = "varying vec4 v_color;\nvoid main() {gl_FragColor = v_color;}";
#else
static const char * shader_fragment_text = "varying lowp vec4 v_color;\nvoid main() {gl_FragColor = v_color;}";
#endif

static const char * shader_vertex_text = "uniform mat4 modelview_projection;\nattribute vec2 a_position;\nattribute vec4 a_color;\nvarying vec4 v_color;\nvoid main() {\n  v_color = a_color;\n  gl_Position = modelview_projection * vec4(a_position, 0, 1);\n}\n";

static GLuint shader_program;
static GLuint buffer;
static GLint positionAttribute;
static GLint colorAttribute;
static GLint matrixtAttribute;
static GLuint compileShader(GLuint shader, const char *source) {
  GLint length = (GLint) strlen(source);
  const GLchar *text[] = {source};
  glShaderSource(shader, 1, text, &length);
  glCompileShader(shader);

  GLint compileStatus;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

  if (compileStatus == GL_FALSE) {
    GLint infologlength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologlength);
    char info_log[infologlength+1];
    glGetShaderInfoLog(shader, infologlength, NULL, info_log);
    printf("shader: failed compilation\n");
    printf("shader(info): %s\n", info_log);
  }
  return shader;
}

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
static phworld *world;
static phv gravity;

void init() {
  phv viewportSize = phv(640, 480);
  world = phCreate(phworld, phbox(0, 480, 640, 0));
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

  glClearColor(0, 0, 0, 0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

  shader_program = glCreateProgram();
  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
  compileShader(fragment, shader_fragment_text);
  glAttachShader(shader_program, fragment);

  GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
  compileShader(vertex, shader_vertex_text);
  glAttachShader(shader_program, vertex);

  glLinkProgram(shader_program);
  GLint programStatus;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &programStatus);
  if (programStatus == GL_FALSE) {
    printf("program failed compilation\n");
  }

  glUseProgram(shader_program);
  positionAttribute = glGetAttribLocation(shader_program, "a_position");
  glEnableVertexAttribArray(positionAttribute);
  colorAttribute = glGetAttribLocation(shader_program, "a_color");
  glEnableVertexAttribArray(colorAttribute);

  GLint modelview_projection = glGetUniformLocation(shader_program, "modelview_projection");
  matrixtAttribute = modelview_projection;
  GLfloat identity[] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
  glUniformMatrix4fv(modelview_projection, 1, GL_TRUE, identity);

  glGenBuffers(1, &buffer);
}

struct glcolor {
  GLubyte r,g,b,a;
};

struct glvertex {
  GLfloat vertex[2];
  struct glcolor color;
};

struct glparticle {
  struct glvertex vertices[6];
};

struct gldata {
  int index;
  struct glparticle particles[kParticleCount+1024];
};

static void set_particle_vertices(void *ctx, phparticle *particle) {
  struct gldata *data = ctx;
  struct glcolor color = { 255, 255, 255, 128 };

  if ( particle->isStatic ) {
    color = (struct glcolor) { 0, 0, 255, 128 };
  }

  phbox particlebox = phAabb(particle->position, particle->radius);
  data->particles[data->index].vertices[0].vertex[0] = particlebox.left;
  data->particles[data->index].vertices[0].vertex[1] = particlebox.top;
  data->particles[data->index].vertices[0].color = color;

  data->particles[data->index].vertices[1].vertex[0] = particlebox.right;
  data->particles[data->index].vertices[1].vertex[1] = particlebox.top;
  data->particles[data->index].vertices[1].color = color;

  data->particles[data->index].vertices[2].vertex[0] = particlebox.right;
  data->particles[data->index].vertices[2].vertex[1] = particlebox.bottom;
  data->particles[data->index].vertices[2].color = color;

  data->particles[data->index].vertices[3].vertex[0] = particlebox.left;
  data->particles[data->index].vertices[3].vertex[1] = particlebox.top;
  data->particles[data->index].vertices[3].color = color;

  data->particles[data->index].vertices[4].vertex[0] = particlebox.left;
  data->particles[data->index].vertices[4].vertex[1] = particlebox.bottom;
  data->particles[data->index].vertices[4].color = color;

  data->particles[data->index].vertices[5].vertex[0] = particlebox.right;
  data->particles[data->index].vertices[5].vertex[1] = particlebox.bottom;
  data->particles[data->index].vertices[5].color = color;
  data->index++;
}

void setWaterTestGravity(float _gravity[3]) {
  gravity.x = _gravity[1] * 32;
  gravity.y = -_gravity[0] * 32;
  // printf("%f %f %f\n", gravity[0], gravity[1], gravity[2]);
}

void gravityIterator(void *ctx, phparticle *particle) {
  particle->acceleration = phAdd(particle->acceleration, gravity);
}

void containIterator(phworld *world, phparticle *particle) {
  phbox box = world->_optimization.box;
  phv p = particle->position;
  phv lp = particle->lastPosition;
  phdouble r = particle->radius;
  if (p.x + r > box.right) {
    lp.x = (p.x - lp.x) * 0.5 + lp.x;
    p.x = box.right - r;
  }
  if (p.x - r < box.left) {
    lp.x = (p.x - lp.x) * 0.5 + lp.x;
    p.x = box.left + r;
  }
  if (p.y + r > box.top) {
    lp.y = (p.y - lp.y) * 0.5 + lp.y;
    p.y = box.top - r;
  }
  if (p.y - r < box.bottom) {
    lp.y = (p.y - lp.y) * 0.5 + lp.y;
    p.y = box.bottom + r;
  }
  particle->position = p;
}

unsigned int (*getTicks)() = NULL;
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

void stepInput() {}

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
    phIterate(itr, (phitrfn) gravityIterator, NULL);
    itr = phIterator(&world->particles, &_litr);
    phIterate(itr, (phitrfn) containIterator, world);
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
      printf(
        "min %d, max %d, avg %d, stddev %f\n",
      minTime( frameTimes, kMaxFrameTimes ),
      maxTime( frameTimes, kMaxFrameTimes ),
      avgTime( frameTimes, kMaxFrameTimes ),
      stddevTime( frameTimes, kMaxFrameTimes )
        );
      frameTimeIndex = 0;
    }
  }
}

void draw() {
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(shader_program);

  // GLfloat vertices[2 * 4] = {
  //   160, 120,
  //   -160, 120,
  //   160, -120,
  //   -160, -120
  // };

  phv size = phv(640, 480);
  // float right = world->aabb.right,
  float right = size.x,
  left = 0,
  // top = world->aabb.top,
  top = size.y,
  bottom = 0,
  zFar = 1000,
  zNear = 0;
  float tx=-(right+left)/(right-left);
  float ty=-(top+bottom)/(top-bottom);
  float tz=-(zFar+zNear)/(zFar-zNear);

  GLfloat matrix[] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
  matrix[0]=2/(right-left);
  matrix[5]=2/(top-bottom);
  matrix[10]=-2/(zFar-zNear);	
  matrix[12]=tx;
  matrix[13]=ty;
  matrix[14]=tz;

  glUniformMatrix4fv(matrixtAttribute, 1, GL_FALSE, matrix);

  struct gldata data;
  data.index = 0;
  phlistiterator _litr;
  phiterator *itr = phIterator(&world->particles, &_litr);
  phIterate(itr, (phitrfn) set_particle_vertices, &data);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(
    GL_ARRAY_BUFFER,
  sizeof(struct glparticle) * (particle_count + 1024), data.particles,
  GL_DYNAMIC_DRAW);

  glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(struct glvertex), (GLvoid *) 0);
  glVertexAttribPointer(colorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct glvertex), (GLvoid *) 8);

  glDrawArrays(GL_TRIANGLES, 0, 6 * data.index );
}

SDL_Surface *screen;

int main(int argc, char *argv[])
{
  // Slightly different SDL initialization
  if ( SDL_Init(SDL_INIT_VIDEO) != 0 ) {
    printf("Unable to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }

  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); // *new*

  #if EMSCRIPTEN
  screen = SDL_SetVideoMode( 640, 480, 16, SDL_OPENGL | SDL_RESIZABLE ); // *changed*
  #else
  screen = SDL_SetVideoMode( 640, 480, 16, SDL_OPENGL ); // *changed*  
  #endif
  if ( !screen ) {
    printf("Unable to set video mode: %s\n", SDL_GetError());
    return 1;
  }

  // Set the OpenGL state after creating the context with SDL_SetVideoMode

  glClearColor( 0, 0, 0, 0 );

#if !EMSCRIPTEN
  glEnable( GL_TEXTURE_2D ); // Need this to display a texture XXX unnecessary in OpenGL ES 2.0/WebGL
#endif

#if EMSCRIPTEN
#ifdef SPACELEAP_VIEWPORT
  enable_resizable();
#endif
#endif

#ifdef VIEWPORT
  printf( "VIEWPORT dimensions %d %d %d %d.\n", VIEWPORT_DIMENSIONS );
  VIEWPORT();
#else
  printf( "VIEWPORT dimensions %d %d %d %d.\n", 0, 0, 640, 480 );
  glViewport( 0, 0, 640, 480 );
#endif
#if EMSCRIPTEN
  emscripten_set_main_loop(main_loop, 0, 0);
#endif

  init();

  setGetTicksFunction( SDL_GetTicks );
#if !EMSCRIPTEN
  while ( 1 ) {
    main_loop();
    SDL_Delay( 1 );
  }
#endif

  return 0;
}

int lastTicks = 0;
void main_loop() {
  process_events();
  int newTicks = SDL_GetTicks();
  step(( newTicks - lastTicks ) / 1000.0 );
  draw();
#if !EMSCRIPTEN
  SDL_GL_SwapBuffers();
#endif
  lastTicks = newTicks;
}

static void process_events( void )
{
  /* Our SDL event placeholder. */
  SDL_Event event;

  int hadEvent = 0;

  /* Grab all the events off the queue. */
  while( SDL_PollEvent( &event ) ) {
    switch( event.type ) {
      case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_ESCAPE) {
        SDL_Quit();
        exit(0);
      }
      break;

      case SDL_QUIT:
      /* Handle quit requests (like Ctrl-c). */
      SDL_Quit();
      exit( 0 );
      break;

      default:
      break;
    }

    stepInput();
  }
}
