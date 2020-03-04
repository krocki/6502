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
  _6502.reset(PC,SP); _6502.show_debug=0
  return _6502

def run_prg(prog,n,gif=0):

    ffi = FFI()

    #C header stuff
    ffi.cdef("""
       typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
       void reset(u16,u8); u8 show_debug;
       void cpu_step(u32);
       extern u8 mem[0x10000];
    """)

    #initial program counter and stack pointer
    PC=0x600; SP=0xff; FB=0x200

    C = init(ffi, PC, SP);
    for i in range(len(prog)): C.mem[PC+i] = prog[i] # todo: memcpy

    if gif:
      imgs = []
      for jj in range(50):
        C.cpu_step(n // 50)
        _scr = np.zeros((2048,1), dtype='uint8')
        for i in range(512): _scr[i] = C.mem[i] # zero pg,stack
        for i in range(512): _scr[i+512] = C.mem[PC+i] # program
        for i in range(1024): _scr[i+1024] = C.mem[FB+i]  # 'screen'
        frame = _scr.reshape(1,64,32);
        zp = frame[:,0:32,:]; prg = frame[:,32:64,:]
        frame = np.zeros((1,32,64))
        frame[:,:,0:32] = zp; frame[:,:,32:64] = prg
        ff = frame.repeat(3, axis=0)
        imgs.append(ff);
      return imgs

    else:
      C.cpu_step(n) # just run for n cycles

      # todo, this is temporary
      _scr = np.zeros((1024,1), dtype='uint8')
      for i in range(1024): _scr[i] = C.mem[FB+i]
      return _scr.reshape(1,32,32)

def draw_parent(parents):
    for i in range(len(parents)):
        r = random.uniform(0,1)
        if r < 0.01: return parents[i][0],i
    return parents[-1][0],len(parents)-1

def crossover(x,y): # x and y are parents
   i = random.randint(1, len(x) - 2) # crossover point 0 (i)
   j = random.randint(1, len(y) - 2) # crossover point 1 (j)
   if i > j: i, j = j, i
   p,q = x[:i] + y[i:j] + x[j:], y[:i] + x[i:j] + y[j:]
   return p, q # 2 children

def mutate(chromosome, prob):
    r = random.uniform(0,1)
    if r < prob:
      i = random.randint(0, len(chromosome) - 1); j = random.randint(0, 7);
      chromosome[i]^=(1 << j);
    return chromosome


def fitness(prog):
  o = run_prg(prog, cpu_cycles)
  v1 = im_array.astype(np.int32)
  v1[v1>0] = 1
  v2 = o.astype(np.int32)
  v2[v2>0] = 1
  return np.sum(v1==v2), o

def random_chromo(length, chars): return [random.choice(chars) for i in range(length)]
def initial_population(length, chars, N): return [random_chromo(length, chars) for _ in range(N)]

im_array = np.zeros((32,32))
# target
im_array[:, :] = 1
frame = np.array(im_array).reshape(1,32,32)
ff = frame.repeat(3, axis=0)
exp_name = "test"
cpu_cycles = 50000


def write_bin(fname, byte_arr):
    f = open(fname, 'w+b')
    binary_format = bytearray(byte_arr)
    f.write(binary_format)
    f.close()

def printlog(dirname, s, end='\n', mode='a'):
    print(s); f=open('{}_log.txt'.format(exp_name),mode) ; f.write(s) ; f.close()

def f(i): return (i,fitness(i))

if __name__ == "__main__":

    pool = Pool(4)

    imgs=[]
    N=400 # pop size
    randd=3 # how many new random individuals each gener
    prog_len=128
    alphabet = [i for i in range(256)]
    crossover_prob=0.2 # probability of 2 parents breeding
    mutate_prob=0.02 # probability of random mutation
    t0 = time.time()

    population,generation=initial_population(prog_len, alphabet, N),0

    while generation < 100000:
      pop_fitness = pool.map(f, population)
      sorted_pop = sorted(pop_fitness, key=lambda x: -x[1][0])
      # print top individual
      # for i in range(5): print(sorted_pop[i][1][0], sorted_pop[i][0])
      avg = sum([i[1][0] for i in sorted_pop]) / float(N);
      new_population = []

      if generation % 10 == 0: # monitor progress
          frame = np.array(sorted_pop[0][1][1]).reshape(32,32) > 0
          ff2 = ndimage.zoom(frame, 2, order=0)
          ff2 = ff2.reshape(1,64,64)
          ff2 = ff2.repeat(3, axis=0) * 255
          imgs.append(ff2)
          write_gif(imgs, 'framebuffer_{:07d}.gif'.format(generation), fps=10)
          #save memory space + frambuff
          #write_gif(run_prg(sorted_pop[0][0],cpu_cycles,True), exp_name + '_run_{:07d}.gif'.format(generation), fps=10)
          #save binary prog
          #write_bin('prog_{}_{:07d}.bin'.format(exp_name, generation), sorted_pop[0][0]);

      top_k = N//2
      # copy top invididuals
      for i in range(top_k):
        new_population.append(sorted_pop[i][0])

      for i in range(N//4): # 1st half of the population is used, 2nd half dies

        # selection
        x,xi = draw_parent(sorted_pop); del xi; y,yi = draw_parent(sorted_pop); del yi;
        # crossover
        if random.random() < crossover_prob: p,q=crossover(x,y)
        else: p,q=x,y
        # mutate
        #p = mutate(p, alphabet) if random.random() < mutate_prob else p
        #q = mutate(q, alphabet) if random.random() < mutate_prob else q

        new_population.append(mutate(p,mutate_prob)); new_population.append(mutate(q,mutate_prob));

      population = new_population

      printlog(exp_name, "{:7.1f} generation {:3d} fitness {:3.3f} avg {:3.3f}".format(time.time()-t0, generation, sorted_pop[0][1][0], avg))
      generation += 1
