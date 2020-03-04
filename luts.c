#include "typedefs.h"

// some lookup tables
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

const u8 ticktable[256] = { // cycles per op, you actually don't need this
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
const u8 pg[256] = { // page crossed cycl penalty, also not needed
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
