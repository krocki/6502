#include "typedefs.h"
#include <sys/time.h>

#define HAS_MMU 1

//registers
typedef struct {
  // regs
  u8 A; u8 X; u8 Y; u8 SP;
  u16 PC;
  union {
    struct { u8 C:1; u8 Z:1; u8 I:1; u8 D:1; u8 B:1; u8 u:1; u8 V:1; u8 S:1;};
    //struct { u8 S:1; u8 V:1; u8 u:1; u8 B:1; u8 D:1; u8 I:1; u8 Z:1; u8 C:1;};
    u8 P; // flags
  };
} cpu;

cpu n;

#define SP (n.SP)
#define PC (n.PC)
#define  A (n.A)
#define  X (n.X)
#define  Y (n.Y)
#define  P (n.P)
#define  C (n.C)
#define  Z (n.Z)
#define  I (n.I)
#define  D (n.D)
#define  B (n.B)
#define  V (n.V)
#define  S (n.S)

#if HAS_MMU
#else
extern u8 mem[0x10000];  //64kB
#endif

extern u8 has_bcd;
extern u64 cyc;
extern u8 op;
extern void reset(u16 ip, u8 sp, u8 flags);
extern void cpu_step(u32);
extern u8 r8(u16);
extern void w8(u16, u8);
extern u16 r16(u16);
extern u16 r16_ok(u16);
extern u8 limit_speed;
extern u8 show_debug;
