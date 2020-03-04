#include <stdio.h>    // printf
#include <string.h>   // memcpy
#include <unistd.h>   // usleep
#include <stdlib.h>   // atoi
#include <pthread.h>
#include <GLFW/glfw3.h>
#include "typedefs.h"
#include "6502.h"

double t0; // global start time
u8 gl_ok=0;
u8 flip_y=1;
u8 new_frame=1;
u8 limit_speed=1;
u8 pix[3*1024];
const u32 pal[2] = {0x000000ff, 0xffffffff};

double get_time() {
  struct timeval tv; gettimeofday(&tv, NULL);
  return (tv.tv_sec + tv.tv_usec * 1e-6);
}

// GL stuff
#define AUTO_REFRESH 60
#define OFFSET 64
#define WIDTH 256
#define HEIGHT 256
static GLFWwindow* window;
static void error_callback(s32 error, const char* description) { }
#define bind_key(x,y) \
{ if (action == GLFW_PRESS && key == (x)) (y) = 1; if (action == GLFW_RELEASE && key == (x)) (y) = 0; if (y) {printf(#y "\n");} }
static void key_callback(GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_0) show_debug ^= 1;
    if (action == GLFW_PRESS && key == GLFW_KEY_9) limit_speed ^= 1;
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
}
static GLFWwindow* open_window(const char* title, GLFWwindow* share, s32 posX, s32 posY)
{
    GLFWwindow* window;

    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, title, NULL, share);
    if (!window) return NULL;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowPos(window, posX, posY);
    glfwShowWindow(window);

    glfwSetKeyCallback(window, key_callback);

    return window;
}
static void draw_quad()
{
    s32 width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float incr_x = 1.0f/(float)32; float incr_y = 1.0f/(float)32;

    glOrtho(0.f, 1.f, 0.f, 1.f, 0.f, 1.f);
    glBegin(GL_QUADS);
    float i,j;
    u8 px,py;
    for (u8 x=0; x<32; x++) for (u8 y=0; y<32; y++ ) {
         i = x * incr_x; j = y * incr_y; px = x; py = flip_y ? 32 - y - 1 : y; // FLIP vert

          u32 col=pal[mem[0x200+(py*32+px)] % 2];

          pix[3*(px+py*32)+0] = (col >> 24);
          pix[3*(px+py*32)+1] = (col >> 16) & 0xff;
          pix[3*(px+py*32)+2] = (col >> 8) & 0xff;

          glColor4f(pix[3*(px+py*32)+0]/255.0f,
                    pix[3*(px+py*32)+1]/255.0f,
                    pix[3*(px+py*32)+2]/255.0f,
                    255.0f/255.0f);

          glVertex2f(i,      j     );     glVertex2f(i+incr_x, j     );
          glVertex2f(i+incr_x, j+incr_y); glVertex2f(i,      j+incr_y);
    }
    glEnd();
};

s32 lcd_init() {
    s32 x, y, width;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) return -1;

    window = open_window("6502", NULL, OFFSET, OFFSET);
    if (!window) { glfwTerminate(); return -1; }

    glfwGetWindowPos(window, &x, &y);
    glfwGetWindowSize(window, &width, NULL);
    glfwMakeContextCurrent(window);

    gl_ok=1;
    printf("%9.6f, GL_init OK\n", get_time()-t0);

    double frame_start=get_time();
    while (!glfwWindowShouldClose(window))
    {
      double fps = get_time()-frame_start;
      frame_start = get_time();
      glfwMakeContextCurrent(window);
      glClear(GL_COLOR_BUFFER_BIT);
      draw_quad();
      glfwSwapBuffers(window);
      if (AUTO_REFRESH > 0) glfwWaitEventsTimeout(1.0/(double)AUTO_REFRESH);
      else glfwWaitEvents();
   }

    glfwTerminate();
    printf("%9.6f, GL terminating\n", get_time()-t0);
    gl_ok=0;
    return 0;
}

int read_bin(const char* fname, u8* ptr) {
  FILE *fp = fopen(fname, "rb");
  if (fp) {
    fseek(fp, 0, SEEK_END);
    u32 len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(ptr, len, 1, fp);
    fclose(fp);
    printf("read file %s, %d bytes\n", fname, len);
    return 0;
  } else {
    fprintf(stderr, "error opening %s\n", fname);
    return -1;
  }
}

void *work(void *args) {

  while (!gl_ok) usleep(10);
  //printf("%9.3f, ACK\n", get_time()-t0);
  // PC = 0xfffc, SP = 0xfd, P=0x24
  //reset(0xfffc, 0xfd, 0x24);
  read_bin((const char*)args, mem);
  reset(0x400);
  double cpu_ts=get_time();
  while (gl_ok) {
    cpu_step(1);
    if (limit_speed) usleep(1000);
  }
  //printf("%9.6f, terminating\n", get_time()-t0);
  printf("ticks %llu, time %.6f s, MHz %.3f\n", cyc, get_time()-cpu_ts, ((double)cyc/(1000000.0*(get_time()-cpu_ts))));

  return NULL;
}

int main(int argc, char **argv) {

  if (argc >= 3) { show_debug=atoi(argv[2]);  }
  if (argc >= 4) { limit_speed=atoi(argv[3]); }

  printf("DEBUG=%d\n", show_debug); printf("SPEED_LIMIT=%d\n", limit_speed);
  t0=get_time();

  pthread_t cpu_thread;
  if(pthread_create(&cpu_thread, NULL, work, argv[1])) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  lcd_init();

  if(pthread_join(cpu_thread, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }

  return 0;
}
