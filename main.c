#include <stdio.h>    // printf
#include <string.h>   // memcpy
#include <unistd.h>   // usleep
#include <stdlib.h>   // atoi
#include "typedefs.h"
#include "6502.h"

double t0; // global start time
u64 limit_cycles = 0;

double get_time() {
  struct timeval tv; gettimeofday(&tv, NULL);
  return (tv.tv_sec + tv.tv_usec * 1e-6);
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

  read_bin((const char*)args, mem);
  reset(0x400);
  double cpu_ts=get_time();
  u64 cycles = 0;
  while (limit_cycles == 0 || cycles < limit_cycles) {
    show_debug = limit_cycles > 0 && cycles > (limit_cycles - 100);
    cpu_step(1);
    cycles++;
  }
  printf("ticks %llu, time %.6f s, MHz %.3f\n", cycles, get_time()-cpu_ts, ((double)cycles/(1000000.0*(get_time()-cpu_ts))));

  return NULL;
}

int main(int argc, char **argv) {

  show_debug=0;
  if (argc >= 3) limit_cycles = strtoul(argv[2], NULL, 10);
  t0=get_time();

  work(argv[1]);

  return 0;
}
