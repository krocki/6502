GCC=gcc
#GCC=arm-none-eabi

C_FLAGS=-Ofast -fPIC
OS:=$(shell uname)

ifeq ($(OS),Darwin) #OSX
  GL_FLAGS=-lglfw -framework OpenGL -lpthread
else # Linux or other
  GL_FLAGS=-lglfw -lGL -lpthread
endif

all: 6502.so

6502.o: 6502.c 6502.h Makefile
	${GCC} ${C_FLAGS} -c 6502.c -o $@

6502.so: 6502.o Makefile 6502.h luts.o
	${GCC} luts.o 6502.o ${C_FLAGS} -shared -o $@

%.o: %.c
	${GCC} ${C_FLAGS} -c $< -o $@

clean:
	rm -rf *.so *.o 6502_lcd
