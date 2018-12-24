#include <stdio.h>
#include "6502.h"

u8 mem[0x10000];
u64 cyc = 0;
u64 prev_cyc = 0;
u8 op = 0;
//dessing modes
u8 b=0; // operand length
u8 m=0; // mode
u16 d;

u8 show_debug=0;

#define STACK_PG 0x0100
#define ZN(x) { Z=((x)==0); S=((x)>>7) & 0x1; }
#define LDM { d=(m>2) ? r8(d) : d; }
#define LD_A_OR_M() u8 w=(m==1)?A:r8(d)
#define ST_A_OR_M() if (m!=1) w8(d,w); else A=w;

const char* asmtable[256] = { // for disasm
  "BRK","ORA","NOP","SLO","NOP","ORA","ASL","SLO","PHP","ORA","ASL","NOP","NOP","ORA","ASL","SLO",
  "BPL","ORA","NOP","SLO","NOP","ORA","ASL","SLO","CLC","ORA","NOP","SLO","NOP","ORA","ASL","SLO",
  "JSR","AND","NOP","RLA","BIT","AND","ROL","RLA","PLP","AND","ROL","NOP","BIT","AND","ROL","RLA",
  "BMI","AND","NOP","RLA","NOP","AND","ROL","RLA","SEC","AND","NOP","RLA","NOP","AND","ROL","RLA",
  "RTI","EOR","NOP","SRE","NOP","EOR","LSR","SRE","PHA","EOR","LSR","NOP","JMP","EOR","LSR","SRE",
  "BVC","EOR","NOP","SRE","NOP","EOR","LSR","SRE","CLI","EOR","NOP","SRE","NOP","EOR","LSR","SRE",
  "RTS","ADC","NOP","RRA","NOP","ADC","ROR","RRA","PLA","ADC","ROR","NOP","JMP","ADC","ROR","RRA",
  "BVS","ADC","NOP","RRA","NOP","ADC","ROR","RRA","SEI","ADC","NOP","RRA","NOP","ADC","ROR","RRA",
  "NOP","STA","NOP","SAX","STY","STA","STX","SAX","DEY","NOP","TXA","NOP","STY","STA","STX","SAX",
  "BCC","STA","NOP","NOP","STY","STA","STX","SAX","TYA","STA","TXS","NOP","NOP","STA","NOP","NOP",
  "LDY","LDA","LDX","LAX","LDY","LDA","LDX","LAX","TAY","LDA","TAX","NOP","LDY","LDA","LDX","LAX",
  "BCS","LDA","NOP","LAX","LDY","LDA","LDX","LAX","CLV","LDA","TSX","LAX","LDY","LDA","LDX","LAX",
  "CPY","CMP","NOP","DCP","CPY","CMP","DEC","DCP","INY","CMP","DEX","NOP","CPY","CMP","DEC","DCP",
  "BNE","CMP","NOP","DCP","NOP","CMP","DEC","DCP","CLD","CMP","NOP","DCP","NOP","CMP","DEC","DCP",
  "CPX","SBC","NOP","ISB","CPX","SBC","INC","ISB","INX","SBC","NOP","SBC","CPX","SBC","INC","ISB",
  "BEQ","SBC","NOP","ISB","NOP","SBC","INC","ISB","SED","SBC","NOP","ISB","NOP","SBC","INC","ISB"
};
const u32 ticktable[256] = { // cycles per op
   7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
   2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
   6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
   2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
   6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
   2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
   6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
   2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
   2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
   2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
   2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
   2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
   2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
   2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
   2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
   2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};
const u8 pg[256] = { // page crossed cycl penalty
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0
};
void w8(u16 a, u8 v) { mem[a] = v; }
u8 r8(u16 a) { return mem[a]; }
u16 r16_ok(u16 a) { return (r8(a) | (r8(a+1) << 8)); }
//version with the bug
u16 r16(u16 a) { u16 base=a & 0xff00; return (r8(a) | (r8(base|((u8)(a+1))) << 8)); }
u8 f8() { return r8(PC++); }
u16 f16() { return (f8() | ((f8())<<8)); }
u8 pop8() { SP++; return r8(STACK_PG | SP);   }
u16 pop16() { return (pop8() | ((pop8())<<8)); }
void push8(u8 v) { w8(STACK_PG | SP, v); SP--; }
void push16(u16 v) { push8(v>>8); push8(v);  }
void jr(u8 cond) { if (cond) {u8 pg0=(PC >> 8); PC=(u16)d; cyc+=(PC >> 8)==pg0?1:2;} }

void imp()  {m=0;  b=0; } // implied, 1
void acc()  {m=1;  b=0; } // accumulator, 1
void imm()  {m=2;  b=1; d=(u16)f8(); } // immediate, 2
void zp()   {m=3;  b=1; d=(u16)f8(); } // zero page, 2
void zpx()  {m=4;  b=1; u8 r=f8(); d=(r+X) & 0xff;} // zero page, x, 3
void zpy()  {m=5;  b=1; u8 r=f8(); d=(r+Y) & 0xff; } // zero page, y, 3
void rel()  {m=6;  b=1; u8 r=f8(); if (r<0x80) d=PC+r; else d=PC+r-0x100;} // relative, 2
void abso() {m=7;  b=2; d=f16(); } // absolute, 3
void absx() {m=8;  b=2; d=f16(); cyc+=((d>>8)!=((d+X)>>8)) ? pg[op] : 0; d+=X;   } // absolute, x, 3
void absy() {m=9;  b=2; d=f16(); cyc+=(d>>8)!=((d+Y)>>8) ? pg[op] : 0; d+=Y;  } // absolute, y, 3
void ind()  {m=10; b=2; d=r16(f16()); } // indirect, 3
void indx() {m=11; b=1; u8 r=f8(); d=r16((u8)(r + X)); } // indirect x
void indy() {m=12; b=1; u8 r=f8(); d=r16((u8)(r)); cyc+=(d>>8)!=((d+Y)>>8) ? pg[op] : 0; d+=Y;} // indirect y

//instructions
void adc() {
  u8 a = A; LDM; A=d+A+C; ZN(A);
  u16 t = (u16)d + (u16)a + (u16)C; C=(t > 0xff);
  V = (!((a^d) & 0x80)) && (((a^A) & 0x80)>0 );
} //   Add Memory to Accumulator with Carry

void sbc() {
  u8 a = A; LDM; A=A-d-(1-C); ZN(A);
  s16 t = (s16)a - (s16)d - (1-(s16)C); C=(t >= 0x0);
  V = (((a^d) & 0x80)>0) && (((a^A) & 0x80)>0);
} //   Subtract Memory from Accumulator with Borrow

void cp(u8 a, u8 b) { u8 r=a-b; C=(a>=b); ZN(r); }

void ora() { LDM; A|=d; ZN(A); } //   "OR" Memory with Accumulator
void and() { LDM; A&=d; ZN(A); } //   "AND" Memory with Accumulator
void eor() { LDM; A^=d; ZN(A); } //   "XOR" Memory with Accumulator
void cmp() { LDM; cp(A,d); } //   Compare Memory and Accumulator
void cpx() { LDM; cp(X,d); } //   Compare Memory and Index X
void cpy() { LDM; cp(Y,d); } //   Compare Memory and Index Y

void bcc() { jr(!C); } //   Branch on Carry Clear
void bcs() { jr(C);  } //   Branch on Carry Set
void beq() { jr(Z);  } //   Branch on Result Zero
void bit() { LDM; S=(d>>7) & 1; V=(d>>6) & 1; Z=(d & A)==0; } //   Test Bits in Memory with Accumulator
void bmi() { jr(S);  } //   Branch on Result Minus
void bne() { jr(!Z); } //   Branch on Result not Zero
void bpl() { jr(!S); } //   Branch on Result Plus
void brk() { B=1;    } //   Force Break
void bvc() { jr(!V); } //   Branch on Overflow Clear
void bvs() { jr(V);  } //   Branch on Overflow Set

void clc() { C=0; } //   Clear Carry Flag
void cld() { D=0; } //   Clear Decimal Mode
void cli() { I=0; } //   Clear interrupt Disable Bit
void clv() { V=0; } //   Clear Overflow Flag

void dec() { u16 d0 = d; LDM; d--; d &= 0xff; ZN(d); w8(d0,d); } //   Decrement Memory by One
void dex() { X--; ZN(X); } //   Decrement Index X by One
void dey() { Y--; ZN(Y); } //   Decrement Index Y by One


void inc() { u16 d0=d; LDM; d++; d &= 0xff; ZN(d); w8(d0,d); d=d0; } //   Increment Memory by One
void inx() { X++; ZN(X); } //   Increment Index X by One
void iny() { Y++; ZN(Y); } //   Increment Index Y by One

void jmp() {PC=d;} //   Jump to New Location
void jsr() {push16(PC-1); PC=d;} //   Jump to New Location Saving Return Address

void lda() { LDM; A=d; ZN(A); } //   Load Accumulator with Memory
void ldx() { LDM; X=d; ZN(X); } //   Load Index X with Memory
void ldy() { LDM; Y=d; ZN(Y); } //   Load Index Y with Memory
void lsr() { LD_A_OR_M(); C=w & 1; w>>=1; ZN(w); ST_A_OR_M(); } //   Shift Right One Bit (Memory or Accumulator)
void asl() { LD_A_OR_M(); C=(w>>7) & 1; w<<=1; ZN(w); ST_A_OR_M();} //   Shift Left One Bit (Memory or Accumulator)
void rol() { LD_A_OR_M(); u8 c = C; C=(w>>7) & 1; w=(w<<1) | c; ZN(w); ST_A_OR_M(); } //   Rotate One Bit Left (Memory or Accumulator)
void ror() { LD_A_OR_M(); u8 c = C; C=(w & 1); w=(w>>1) | (c<<7); ZN(w); ST_A_OR_M(); } //   Rotate One Bit Right (Memory or Accumulator)


void nop() {} //   No Operation

void pha() { push8(A); } //   Push Accumulator on Stack
void php() { push8(P | 0x10); } //   Push Processor Status on Stack
void pla() { A=pop8(); Z=(A==0); S=(A>>7)&0x1;} //   Pull Accumulator from Stack
void plp() { P=pop8() & 0xef | 0x20;  } //   Pull Processor Status from Stack

void rti() {P=(pop8() & 0xef) | 0x20; PC=pop16();} //   Return from Interrupt
void rts() {PC=pop16()+1;} //   Return from Subroutine

void sec() {C=1;} //   Set Carry Flag
void sed() {D=1;} //   Set Decimal Mode
void sei() {I=1;} //   Set Interrupt Disable Status
void sta() {w8(d,A);} //   Store Accumulator in Memory
void stx() {w8(d,X);} //   Store Index X in Memory
void sty() {w8(d,Y);} //   Store Index Y in Memory

void tax() { X=A; ZN(X); } //   Transfer Accumulator to Index X
void tay() { Y=A; ZN(Y); } //   Transfer Accumulator to Index Y
void tsx() { X=SP;ZN(X);} //   Transfer Stack Pointer to Index X
void txa() { A=X; ZN(A); } //   Transfer Index X to Accumulator
void txs() { SP=X; } //   Transfer Index X to Stack Pointer
void tya() { A=Y; ZN(A); } //   Transfer Index Y to Accumulator

// undocumented
void lax() { lda(); X=A; ZN(A); } // lda, ldx
void sax() { w8(d,A&X); }
void dcp() { dec(); cp(A,d); }
void isb() { inc(); sbc(); }
void slo() { asl(); ora(); }
void rla() { rol(); and(); }
void sre() { lsr(); eor(); }
void rra() { ror(); adc(); }

void (*addrtable[256])() = {
  imp, indx, imp, indx, zp, zp, zp, zp, imp, imm, acc, imm, abso, abso, abso, abso,
  rel, indy, imp, indy, zpx, zpx, zpx, zpx, imp, absy, imp, absy, absx, absx, absx, absx,
  abso, indx, imp, indx, zp, zp, zp, zp, imp, imm, acc, imm, abso, abso, abso, abso,
  rel, indy, imp, indy, zpx, zpx, zpx, zpx, imp, absy, imp, absy, absx, absx, absx, absx,
  imp, indx, imp, indx, zp, zp, zp, zp, imp, imm, acc, imm, abso, abso, abso, abso,
  rel, indy, imp, indy, zpx, zpx, zpx, zpx, imp, absy, imp, absy, absx, absx, absx, absx,
  imp, indx, imp, indx, zp, zp, zp, zp, imp, imm, acc, imm, ind, abso, abso, abso,
  rel, indy, imp, indy, zpx, zpx, zpx, zpx, imp, absy, imp, absy, absx, absx, absx, absx,
  imm, indx, imm, indx, zp, zp, zp, zp, imp, imm, imp, imm, abso, abso, abso, abso,
  rel, indy, imp, indy, zpx, zpx, zpy, zpy, imp, absy, imp, absy, absx, absx, absy, absy,
  imm, indx, imm, indx, zp, zp, zp, zp, imp, imm, imp, imm, abso, abso, abso, abso,
  rel, indy, imp, indy, zpx, zpx, zpy, zpy, imp, absy, imp, absy, absx, absx, absy, absy,
  imm, indx, imm, indx, zp, zp, zp, zp, imp, imm, imp, imm, abso, abso, abso, abso,
  rel, indy, imp, indy, zpx, zpx, zpx, zpx, imp, absy, imp, absy, absx, absx, absx, absx,
  imm, indx, imm, indx, zp, zp, zp, zp, imp, imm, imp, imm, abso, abso, abso, abso,
  rel, indy, imp, indy, zpx, zpx, zpx, zpx, imp, absy, imp, absy, absx, absx, absx, absx};

void (*optable[256])() = { // opcode -> functions map
  brk,ora,nop,slo,nop,ora,asl,slo,php,ora,asl,nop,nop,ora,asl,slo,
  bpl,ora,nop,slo,nop,ora,asl,slo,clc,ora,nop,slo,nop,ora,asl,slo,
  jsr,and,nop,rla,bit,and,rol,rla,plp,and,rol,nop,bit,and,rol,rla,
  bmi,and,nop,rla,nop,and,rol,rla,sec,and,nop,rla,nop,and,rol,rla,
  rti,eor,nop,sre,nop,eor,lsr,sre,pha,eor,lsr,nop,jmp,eor,lsr,sre,
  bvc,eor,nop,sre,nop,eor,lsr,sre,cli,eor,nop,sre,nop,eor,lsr,sre,
  rts,adc,nop,rra,nop,adc,ror,rra,pla,adc,ror,nop,jmp,adc,ror,rra,
  bvs,adc,nop,rra,nop,adc,ror,rra,sei,adc,nop,rra,nop,adc,ror,rra,
  nop,sta,nop,sax,sty,sta,stx,sax,dey,nop,txa,nop,sty,sta,stx,sax,
  bcc,sta,nop,nop,sty,sta,stx,sax,tya,sta,txs,nop,nop,sta,nop,nop,
  ldy,lda,ldx,lax,ldy,lda,ldx,lax,tay,lda,tax,nop,ldy,lda,ldx,lax,
  bcs,lda,nop,lax,ldy,lda,ldx,lax,clv,lda,tsx,lax,ldy,lda,ldx,lax,
  cpy,cmp,nop,dcp,cpy,cmp,dec,dcp,iny,cmp,dex,nop,cpy,cmp,dec,dcp,
  bne,cmp,nop,dcp,nop,cmp,dec,dcp,cld,cmp,nop,dcp,nop,cmp,dec,dcp,
  cpx,sbc,nop,isb,cpx,sbc,inc,isb,inx,sbc,nop,sbc,cpx,sbc,inc,isb,
  beq,sbc,nop,isb,nop,sbc,inc,isb,sed,sbc,nop,isb,nop,sbc,inc,isb
};

void print_mem(uint16_t off, uint16_t n) {

  uint8_t col_count = 16;
  for (uint8_t j = 0; j < n; j++) {
    printf("0x%04X:", off);
    for (uint8_t i = 0; i < col_count; i++) {
      printf(" 0x%02x", r8(off));
      if (i==(col_count-1)) printf("\n");
      off++;
    }
  }
}
void reset() { PC=0xc000; A=0x00; X=0x00; P=0x24; SP=0xfd; cyc=0;}
void print_regs() {
  printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3ld", A, X, Y, P, SP, 3*prev_cyc % 341);
}

void cpu_step(u32 count) {
  for (u32 i=0; i<count; i++) {
    if (show_debug) printf("%04X  ", PC);

    op = f8();

    u16 nn;
    if (show_debug) {
      printf("%02X ", op);
      nn=r16_ok(PC);
      prev_cyc=cyc;
    }

    addrtable[op](); //bits bbb
    if (show_debug) {
      if (b >= 1) { u8 arg=(u8)(nn     ); printf("%02X ", arg); } else { printf("   "); }
      if (b == 2) { u8 arg=(u8)(nn >> 8); printf("%02X ", arg); } else { printf("   "); }
      printf("%4s ", asmtable[op]);
      switch (m) {
        case 0: printf("         \t");        break; // impl
        case 1: printf("A        \t");        break; // acc
        case 2: printf("#$%02X      \t", (u8)nn); break; // imm
        case 3: printf("$%02X = %02X \t", (u8)nn, r8(d)); break; // zp
        case 6: printf("$%04X     \t", PC+(s8)(0xff&nn));   break; // rela
        case 7: printf("$%04X     \t",   nn); break; // abso
      }

      print_regs();
      printf("\n");
    }
    optable[op]();
    cyc += ticktable[op];
  }
}
