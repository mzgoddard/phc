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
// #include "SDL/SDL_opengl.h"
#endif

#ifdef AUDIO_OPENAL
#include "openal.h"
#endif

#if EMSCRIPTEN
#include <emscripten.h>
// #include "platform/window.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "opengl.h"

#include "ph.h"

#include "../simple/env.h"

phv screen_size = phv(640, 640);

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

static const int gldata_size = 65536;

struct gldata {
  int index;
  struct glparticle particles[gldata_size];
};

static void set_mesh_square(
  struct glparticle *glp, phbox box, struct glcolor color
) {
  *glp = (struct glparticle) {{
    {box.left, box.top, color},
    {box.right, box.top, color},
    {box.right, box.bottom, color},
    {box.left, box.top, color},
    {box.left, box.bottom, color},
    {box.right, box.bottom, color}
  }};
}

static const phdouble mesh_cell_size = 5;
static const phdouble mesh_cells_wide = 640 / mesh_cell_size;

static void reset_water_mesh(struct gldata *data) {
  for (int i = 0, l = gldata_size; i < l; ++i) {
    set_mesh_square(&data->particles[i], phbox(0, 0, 0, 0), (struct glcolor) {0, 0, 0, 0});
  }
  data->index = mesh_cells_wide * (640 / mesh_cell_size);
  if (gldata_size > data->index) {
    data->index = gldata_size;
  }
}

static void init_water_mesh(struct gldata *data) {
  reset_water_mesh(data);
}

static void toggle_water_mesh_cells(struct gldata *data, phparticle *particle) {
  phbox box = phAabb(particle->position, particle->radius);
  // Align box with mesh_cell_size
  box.left = floor(box.left / mesh_cell_size) * mesh_cell_size;
  box.top = ceil(box.top / mesh_cell_size) * mesh_cell_size;
  box.right = ceil(box.right / mesh_cell_size) * mesh_cell_size;
  box.bottom = floor(box.bottom / mesh_cell_size) * mesh_cell_size;
  for (
    phdouble l = box.left, r = box.right;
    l < r && l < 640 && l >= 0;
    l += mesh_cell_size
  ) {
    for (
      phdouble b = box.bottom, t = box.top;
      b < t &&
        b < floor((gldata_size) / floor(mesh_cells_wide) * mesh_cell_size) &&
        b >= 0;
      b += mesh_cell_size
    ) {
      int index = 
        b / mesh_cell_size * floor(mesh_cells_wide) + l / mesh_cell_size;
      if (data->particles[index].vertices[0].color.a != 255) {
        phbox square = phbox(l, b + mesh_cell_size, l + mesh_cell_size, b);
        set_mesh_square(
          &data->particles[index],
          square,
          (struct glcolor) {0, 0, 255, particle->isStatic ? 128: 255}
        );
      }
    }
  }
}

static void set_particle_vertices(struct gldata *data, phparticle *particle) {
  struct glcolor color = { 255, 255, 255, 128 };

  if ( particle->isStatic ) {
    color = (struct glcolor) { 0, 0, 255, 128 };
  }

  phbox particlebox = phAabb(particle->position, particle->radius);
  struct glparticle *glp = &data->particles[data->index];
  set_mesh_square(glp, particlebox, color);
  data->index++;
}

typedef struct phmouse {
  phint button;
  phbool pressed;
  phv position;
} phmouse;

#define phmouse() ((phmouse) { \
  0, 0, 0, 0 \
})

phcompositeline flowline = phcompositeline(
  phcomposite(phlist(), phlist()), 3
);

void addFlowParticle(void *_self, void *_position) {
  phcomposite *self = _self;
  phv position = *(phv*) _position;
  phparticle *last = phLast(&self->particles);
  phv lastPosition = last ? last->position : position;

  phv direction = phSub(position, lastPosition);
  if (phMag2(direction) == 0) {
    direction = phv(1, 1);
  }
  phv force = phScale(phUnit(direction), 1000);

  phflow *lastFlow = phLast(&self->constraints);
  if (lastFlow) {
    lastFlow->force = force;
  }

  phparticle *particle = phCreate(phparticle, position);
  particle->radius = 10;
  phflow *flow = phCreate(phflow, particle, force);
  phAppend(&self->particles, particle);
  phAppend(&self->constraints, flow);
}

void input(phmouse *state, phmouse *lastState) {
  if (state->pressed && !lastState->pressed) {
    // Remove current particles.
    phWorldRemoveComposite(world, (phcomposite *) &flowline);
    // Free current particles.
    phCompositeLineDump(&flowline);
    // Add first particle.
    phCompositeLineAdd(&flowline, state->position, addFlowParticle);
    printf("press\n");
  } else if (state->pressed && lastState->pressed) {
    // If far enough, add new particle.
    phCompositeLineAdd(&flowline, state->position, addFlowParticle);
  } else if (lastState->pressed && !state->pressed) {
    // Add particles to world.
    phWorldAddComposite(world, (phcomposite *) &flowline);
    printf("release\n");
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

  phv size = phv(640, 640);
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
  reset_water_mesh(&data);
  phworldparticleiterator _wpitr;
  phWorldParticleIterator(world, &_wpitr);
  while (phWorldParticleNext(&_wpitr)) {
    // set_particle_vertices(&data, phWorldParticleDeref(&_wpitr));
    toggle_water_mesh_cells(&data, phWorldParticleDeref(&_wpitr));
  }

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(
    GL_ARRAY_BUFFER,
  sizeof(struct glparticle) * data.index, data.particles,
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
  screen = SDL_SetVideoMode( phvunpack(screen_size), 16, SDL_OPENGL |
    SDL_RESIZABLE );
  #else
  screen = SDL_SetVideoMode( phvunpack(screen_size), 16, SDL_OPENGL );
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
  printf( "VIEWPORT dimensions %d %d %d %d.\n",
    0, 0, phvunpackfmt(screen_size, int)
  );
  glViewport( 0, 0, phvunpack(screen_size) );
#endif
#if EMSCRIPTEN
  emscripten_set_main_loop(main_loop, 0, 0);
#endif

  setGetTicksFunction( SDL_GetTicks );

  init();

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

phmouse mousestate = phmouse();

static void process_events(void) {
  /* Our SDL event placeholder. */
  SDL_Event event;

  int hadEvent = 0;

  phmouse newmousestate = mousestate;

  /* Grab all the events off the queue. */
  while(SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_ESCAPE) {
        SDL_Quit();
        exit(0);
      }
      break;

      case SDL_MOUSEBUTTONDOWN:
      newmousestate.button = event.button.button;
      newmousestate.pressed = 1;
      newmousestate.position =
        phv(event.button.x, screen_size.y - event.button.y);
      input(&newmousestate, &mousestate);
      mousestate = newmousestate;
      break;

      case SDL_MOUSEBUTTONUP:
      newmousestate.button = event.button.button;
      newmousestate.pressed = 0;
      newmousestate.position =
        phv(event.button.x, screen_size.y - event.button.y);
      input(&newmousestate, &mousestate);
      mousestate = newmousestate;
      break;

      case SDL_MOUSEMOTION:
      newmousestate.position =
        phv(event.motion.x, screen_size.y - event.motion.y);
      input(&newmousestate, &mousestate);
      mousestate = newmousestate;
      break;

      case SDL_QUIT:
      /* Handle quit requests (like Ctrl-c). */
      SDL_Quit();
      exit(0);
      break;

      default:
      break;
    }
  }

  input(&newmousestate, &mousestate);
  mousestate = newmousestate;
}
