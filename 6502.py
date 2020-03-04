import time
import argparse
from cffi import FFI
import time, argparse
from array2gif import write_gif
import random
from scipy import ndimage
import numpy as np
from multiprocessing import Pool

# init 6502
def init(ffi, PC, SP):
  _6502 = ffi.dlopen("./6502.so");
  _6502.reset(PC,SP); _6502.show_debug=1
  return _6502

def run_prg(prog,opt):

    print(opt)
    ffi = FFI()

    #C header stuff
    ffi.cdef("""
       typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
       void reset(u16,u8); u8 show_debug;
       void cpu_step(u32);
       extern u8 mem[0x10000];
    """)

    #initial program counter and stack pointer
    PC=opt.pc; SP=0xff; FB=opt.framebuffer

    C = init(ffi, PC, SP);
    n_frames = opt.cycles//10

    for i in range(len(prog)): C.mem[PC+i] = prog[i]

    if opt.output:
      imgs = []
      for jj in range(n_frames):
        C.cpu_step(opt.cycles // n_frames)
        _scr = np.zeros((2048,1), dtype='uint8')
        for i in range(512): _scr[i] = C.mem[i] # zero pg,stack
        for i in range(512): _scr[i+512] = C.mem[PC+i] # program
        for i in range(1024): _scr[i+1024] = 255 * C.mem[FB+i]  # 'screen'screen
        frame = _scr.reshape(1,64,32);
        zp = frame[:,0:32,:]; prg = frame[:,32:64,:]
        frame = np.zeros((1,32,64))
        frame[:,:,0:32] = zp; frame[:,:,32:64] = prg
        ff = frame.repeat(3, axis=0)
        imgs.append(ff);
      return imgs

    else:
      C.cpu_step(opt.cycles) # just run for n cycles

      # todo, this is temporary
      _scr = np.zeros((1024,1), dtype='uint8')
      for i in range(1024): _scr[i] = C.mem[FB+i]
      return _scr.reshape(1,32,32)

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-c', '--cycles', type=int, default=1000, help='number of cycles')
    parser.add_argument('-o', '--output', type=str, default=None, help='output GIF')
    parser.add_argument('-d', '--debug', action="store_true", help='show CPU debug info')
    parser.add_argument('-i', '--input', type=str, help='load binary')
    parser.add_argument('-p', '--pc', type=int, default=0x600, help='program counter start')
    parser.add_argument('-f', '--framebuffer', type=int, default=0x200, help='framebuffer offset')

    opt = parser.parse_args()
    print(opt)
    cpu_cycles = opt.cycles

    # sierpinski
    if opt.input:
      with open(opt.input, "rb") as f:
        prog = f.read()
    else:
      #simple sierpinski
      prog = [
       0xa2,0x00,0xa9,0x00,0x85,0x00,0xa9,0x02,
       0x85,0x01,0x20,0x1f,0x06,0x81,0x00,0xe6,
       0x00,0xf0,0x03,0x4c,0x0a,0x06,0xe6,0x01,
       0xa4,0x01,0xc0,0x06,0xd0,0xec,0x60,0xa5,
       0x00,0x29,0x1f,0x85,0x02,0xa5,0x00,0x4a,
       0x4a,0x4a,0x4a,0x4a,0x85,0x03,0xa5,0x01,
       0x38,0xe9,0x02,0x0a,0x0a,0x0a,0x05,0x03,
       0x25,0x02,0xf0,0x03,0xa9,0x02,0x60,0xa9,
       0x0d,0x60]

    frames = run_prg(prog,opt)
    if opt.output: write_gif(frames, opt.output, fps=25)

