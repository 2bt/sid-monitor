// this code is based on kb's 6502 code in tinysid, i think.
#include "cpu.hpp"

#define FLAG_N 128
#define FLAG_V 64
#define FLAG_B 16
#define FLAG_D 8
#define FLAG_I 4
#define FLAG_Z 2
#define FLAG_C 1


enum {
    ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC,
    CLD, CLI, CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR, INC, INX, INY, JMP,
    JSR, LDA, LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI,
    RTS, SBC, SEC, SED, SEI, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
    SLO, AXS, LAX,
    XXX,
};

enum { IMP, IMM, ABS, ABSX, ABSY, ZP, ZPX, ZPY, IND, INDX, INDY, ACC, REL};

const int OPCODE_TABLE[256] = {
    BRK,  ORA,  XXX,  XXX,  XXX,  ORA,  ASL,  XXX,  PHP,  ORA,  ASL,  XXX,  XXX,  ORA,  ASL,  XXX,
    BPL,  ORA,  XXX,  XXX,  XXX,  ORA,  ASL,  XXX,  CLC,  ORA,  XXX,  XXX,  XXX,  ORA,  ASL,  XXX,
    JSR,  AND,  SLO,  XXX,  BIT,  AND,  ROL,  XXX,  PLP,  AND,  ROL,  XXX,  BIT,  AND,  ROL,  XXX,
    BMI,  AND,  XXX,  XXX,  XXX,  AND,  ROL,  XXX,  SEC,  AND,  XXX,  XXX,  NOP,  AND,  ROL,  XXX,
    RTI,  EOR,  XXX,  XXX,  XXX,  EOR,  LSR,  XXX,  PHA,  EOR,  LSR,  XXX,  JMP,  EOR,  LSR,  XXX,
    BVC,  EOR,  XXX,  XXX,  XXX,  EOR,  LSR,  XXX,  CLI,  EOR,  XXX,  XXX,  XXX,  EOR,  LSR,  XXX,
    RTS,  ADC,  XXX,  XXX,  XXX,  ADC,  ROR,  XXX,  PLA,  ADC,  ROR,  XXX,  JMP,  ADC,  ROR,  XXX,
    BVS,  ADC,  XXX,  XXX,  XXX,  ADC,  ROR,  XXX,  SEI,  ADC,  XXX,  XXX,  XXX,  ADC,  ROR,  XXX,
    XXX,  STA,  XXX,  XXX,  STY,  STA,  STX,  XXX,  DEY,  XXX,  TXA,  XXX,  STY,  STA,  STX,  XXX,
    BCC,  STA,  XXX,  XXX,  STY,  STA,  STX,  XXX,  TYA,  STA,  TXS,  XXX,  XXX,  STA,  XXX,  XXX,
    LDY,  LDA,  LDX,  XXX,  LDY,  LDA,  LDX,  LAX,  TAY,  LDA,  TAX,  XXX,  LDY,  LDA,  LDX,  LAX,
    BCS,  LDA,  XXX,  LAX,  LDY,  LDA,  LDX,  XXX,  CLV,  LDA,  TSX,  XXX,  LDY,  LDA,  LDX,  XXX,
    CPY,  CMP,  XXX,  XXX,  CPY,  CMP,  DEC,  XXX,  INY,  CMP,  DEX,  AXS,  CPY,  CMP,  DEC,  XXX,
    BNE,  CMP,  XXX,  XXX,  XXX,  CMP,  DEC,  XXX,  CLD,  CMP,  XXX,  XXX,  XXX,  CMP,  DEC,  XXX,
    CPX,  SBC,  XXX,  XXX,  CPX,  SBC,  INC,  XXX,  INX,  SBC,  NOP,  XXX,  CPX,  SBC,  INC,  XXX,
    BEQ,  SBC,  XXX,  XXX,  XXX,  SBC,  INC,  XXX,  SED,  SBC,  XXX,  XXX,  XXX,  SBC,  INC,  XXX
};

const int MODE_TABLE[256] = {
    IMP,  INDX,  XXX,   XXX,   ZP,   ZP,   ZP,   XXX,  IMP,  IMM,   ACC,  XXX,   ABS,   ABS,   ABS,   XXX,
    REL,  INDY,  INDY,  XXX,   XXX,  ZPX,  ZPX,  XXX,  IMP,  ABSY,  XXX,  XXX,   XXX,   ABSX,  ABSX,  XXX,
    ABS,  INDX,  XXX,   XXX,   ZP,   ZP,   ZP,   XXX,  IMP,  IMM,   ACC,  XXX,   ABS,   ABS,   ABS,   XXX,
    REL,  INDY,  XXX,   XXX,   XXX,  ZPX,  ZPX,  XXX,  IMP,  ABSY,  XXX,  ABSY,  ABSX,  ABSX,  ABSX,  XXX,
    IMP,  INDX,  XXX,   XXX,   ZP,   ZP,   ZP,   XXX,  IMP,  IMM,   ACC,  XXX,   ABS,   ABS,   ABS,   XXX,
    REL,  INDY,  XXX,   XXX,   XXX,  ZPX,  ZPX,  XXX,  IMP,  ABSY,  XXX,  XXX,   XXX,   ABSX,  ABSX,  XXX,
    IMP,  INDX,  XXX,   XXX,   ZP,   ZP,   ZP,   XXX,  IMP,  IMM,   ACC,  XXX,   IND,   ABS,   ABS,   XXX,
    REL,  INDY,  XXX,   XXX,   XXX,  ZPX,  ZPX,  XXX,  IMP,  ABSY,  XXX,  XXX,   XXX,   ABSX,  ABSX,  XXX,
    IMM,  INDX,  XXX,   XXX,   ZP,   ZP,   ZP,   XXX,  IMP,  IMM,   ACC,  XXX,   ABS,   ABS,   ABS,   XXX,
    REL,  INDY,  XXX,   XXX,   ZPX,  ZPX,  ZPY,  XXX,  IMP,  ABSY,  ACC,  XXX,   XXX,   ABSX,  ABSX,  XXX,
    IMM,  INDX,  IMM,   XXX,   ZP,   ZP,   ZP,   ZP,   IMP,  IMM,   ACC,  XXX,   ABS,   ABS,   ABS,   ABS,
    REL,  INDY,  XXX,   INDY,  ZPX,  ZPX,  ZPY,  XXX,  IMP,  ABSY,  ACC,  XXX,   ABSX,  ABSX,  ABSY,  XXX,
    IMM,  INDX,  XXX,   XXX,   ZP,   ZP,   ZP,   XXX,  IMP,  IMM,   ACC,  IMM,   ABS,   ABS,   ABS,   XXX,
    REL,  INDY,  XXX,   XXX,   ZPX,  ZPX,  ZPX,  XXX,  IMP,  ABSY,  ACC,  XXX,   XXX,   ABSX,  ABSX,  XXX,
    IMM,  INDX,  XXX,   XXX,   ZP,   ZP,   ZP,   XXX,  IMP,  IMM,   ACC,  XXX,   ABS,   ABS,   ABS,   XXX,
    REL,  INDY,  XXX,   XXX,   ZPX,  ZPX,  ZPX,  XXX,  IMP,  ABSY,  ACC,  XXX,   XXX,   ABSX,  ABSX,  XXX
};


uint8_t CPU::getaddr(int mode) {
    uint16_t ad, ad2;
    switch (mode) {
    case IMP: cycles += 2; return 0;
    case IMM: cycles += 2; return getmem(pc++);
    case ABS:
        cycles += 4;
        ad = getmem(pc++);
        ad |= getmem(pc++) << 8;
        return getmem(ad);
    case ABSX:
        cycles += 4;
        ad = getmem(pc++);
        ad |= 256 * getmem(pc++);
        ad2 = ad + x;
        if ((ad2 & 0xff00) != (ad & 0xff00)) cycles++;
        return getmem(ad2);
    case ABSY:
        cycles += 4;
        ad = getmem(pc++);
        ad |= 256 * getmem(pc++);
        ad2 = ad + y;
        if ((ad2 & 0xff00) != (ad & 0xff00)) cycles++;
        return getmem(ad2);
    case ZP:
        cycles += 3;
        ad = getmem(pc++);
        return getmem(ad);
    case ZPX:
        cycles += 4;
        ad = getmem(pc++);
        ad += x;
        return getmem(ad & 0xff);
    case ZPY:
        cycles += 4;
        ad = getmem(pc++);
        ad += y;
        return getmem(ad & 0xff);
    case INDX:
        cycles += 6;
        ad = getmem(pc++);
        ad += x;
        ad2 = getmem(ad & 0xff);
        ad++;
        ad2 |= getmem(ad & 0xff) << 8;
        return getmem(ad2);
    case INDY:
        cycles += 5;
        ad  = getmem(pc++);
        ad2 = getmem(ad);
        ad2 |= getmem((ad + 1) & 0xff) << 8;
        ad = ad2 + y;
        if ((ad2 & 0xff00) != (ad & 0xff00)) cycles++;
        return getmem(ad);
    case ACC: cycles += 2; return a;
    }
    return 0;
}

void CPU::setaddr(int mode, uint8_t val) {
    uint16_t ad, ad2;
    switch (mode) {
    case ABS:
        cycles += 2;
        ad = getmem(pc - 2);
        ad |= 256 * getmem(pc - 1);
        setmem(ad, val);
        return;
    case ABSX:
        cycles += 3;
        ad = getmem(pc - 2);
        ad |= 256 * getmem(pc - 1);
        ad2 = ad + x;
        if ((ad2 & 0xff00) != (ad & 0xff00)) cycles--;
        setmem(ad2, val);
        return;
    case ZP:
        cycles += 2;
        ad = getmem(pc - 1);
        setmem(ad, val);
        return;
    case ZPX:
        cycles += 2;
        ad = getmem(pc - 1);
        ad += x;
        setmem(ad & 0xff, val);
        return;
    case ACC: a = val; return;
    }
}

void CPU::putaddr(int mode, uint8_t val) {
    uint16_t ad, ad2;
    switch (mode) {
    case ABS:
        cycles += 4;
        ad = getmem(pc++);
        ad |= getmem(pc++) << 8;
        setmem(ad, val);
        return;
    case ABSX:
        cycles += 4;
        ad = getmem(pc++);
        ad |= getmem(pc++) << 8;
        ad2 = ad + x;
        setmem(ad2, val);
        return;
    case ABSY:
        cycles += 4;
        ad = getmem(pc++);
        ad |= getmem(pc++) << 8;
        ad2 = ad + y;
        if ((ad2 & 0xff00) != (ad & 0xff00)) cycles++;
        setmem(ad2, val);
        return;
    case ZP:
        cycles += 3;
        ad = getmem(pc++);
        setmem(ad, val);
        return;
    case ZPX:
        cycles += 4;
        ad = getmem(pc++);
        ad += x;
        setmem(ad & 0xff, val);
        return;
    case ZPY:
        cycles += 4;
        ad = getmem(pc++);
        ad += y;
        setmem(ad & 0xff, val);
        return;
    case INDX:
        cycles += 6;
        ad = getmem(pc++);
        ad += x;
        ad2 = getmem(ad & 0xff);
        ad++;
        ad2 |= getmem(ad & 0xff) << 8;
        setmem(ad2, val);
        return;
    case INDY:
        cycles += 5;
        ad  = getmem(pc++);
        ad2 = getmem(ad);
        ad2 |= getmem((ad + 1) & 0xff) << 8;
        ad = ad2 + y;
        setmem(ad, val);
        return;
    case ACC:
        cycles += 2;
        a = val;
        return;
    }
}

void CPU::setflags(int flag, int cond) {
    if (cond) p |= flag;
    else p &= ~flag;
}

void CPU::push(uint8_t val) {
    setmem(0x100 + s, val);
    if (s) s--;
}

uint8_t CPU::pop() {
    if (s < 0xff) s++;
    return getmem(0x100 + s);
}

void CPU::branch(int flag) {
    signed char dist;
    dist = (signed char)getaddr(IMM);
    uint16_t wval = pc + dist;
    if (flag) {
        cycles += ((pc & 0x100) != (wval & 0x100)) ? 2 : 1;
        pc = wval;
    }
}

void CPU::parse(uint8_t opc) {
    uint8_t  bval;
    uint16_t wval;
    int addr = MODE_TABLE[opc];
    int c;
    switch (OPCODE_TABLE[opc]) {
    case ADC:
        wval = (uint16_t)a + getaddr(addr) + ((p & FLAG_C) ? 1 : 0);
        setflags(FLAG_C, wval & 0x100);
        a = (uint8_t)wval;
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        setflags(FLAG_V, (!!(p & FLAG_C)) ^ (!!(p & FLAG_N)));
        break;
    case AND:
        bval = getaddr(addr);
        a &= bval;
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;
    case ASL:
        wval = getaddr(addr);
        wval <<= 1;
        setaddr(addr, (uint8_t)wval);
        setflags(FLAG_Z, !wval);
        setflags(FLAG_N, wval & 0x80);
        setflags(FLAG_C, wval & 0x100);
        break;
    case BCC: branch(!(p & FLAG_C)); break;
    case BCS: branch(p & FLAG_C); break;
    case BNE: branch(!(p & FLAG_Z)); break;
    case BEQ: branch(p & FLAG_Z); break;
    case BPL: branch(!(p & FLAG_N)); break;
    case BMI: branch(p & FLAG_N); break;
    case BVC: branch(!(p & FLAG_V)); break;
    case BVS: branch(p & FLAG_V); break;
    case BIT:
        bval = getaddr(addr);
        setflags(FLAG_Z, !(a & bval));
        setflags(FLAG_N, bval & 0x80);
        setflags(FLAG_V, bval & 0x40);
        break;
    case BRK:
        pc = 0; // Just quit the emulation
        break;
    case CLC: setflags(FLAG_C, 0); break;
    case CLD: setflags(FLAG_D, 0); break;
    case CLI: setflags(FLAG_I, 0); break;
    case CLV: setflags(FLAG_V, 0); break;
    case CMP:
        bval = getaddr(addr);
        wval = (uint16_t)a - bval;
        setflags(FLAG_Z, !wval);
        setflags(FLAG_N, wval & 0x80);
        setflags(FLAG_C, a >= bval);
        break;
    case CPX:
        bval = getaddr(addr);
        wval = (uint16_t)x - bval;
        setflags(FLAG_Z, !wval);
        setflags(FLAG_N, wval & 0x80);
        setflags(FLAG_C, x >= bval);
        break;
    case CPY:
        bval = getaddr(addr);
        wval = (uint16_t)y - bval;
        setflags(FLAG_Z, !wval);
        setflags(FLAG_N, wval & 0x80);
        setflags(FLAG_C, y >= bval);
        break;
    case DEC:
        bval = getaddr(addr);
        bval--;
        setaddr(addr, bval);
        setflags(FLAG_Z, !bval);
        setflags(FLAG_N, bval & 0x80);
        break;
    case DEX:
        x--;
        setflags(FLAG_Z, !x);
        setflags(FLAG_N, x & 0x80);
        break;
    case DEY:
        y--;
        setflags(FLAG_Z, !y);
        setflags(FLAG_N, y & 0x80);
        break;
    case EOR:
        bval = getaddr(addr);
        a ^= bval;
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;
    case INC:
        bval = getaddr(addr);
        bval++;
        setaddr(addr, bval);
        setflags(FLAG_Z, !bval);
        setflags(FLAG_N, bval & 0x80);
        break;
    case INX:
        x++;
        setflags(FLAG_Z, !x);
        setflags(FLAG_N, x & 0x80);
        break;
    case INY:
        y++;
        setflags(FLAG_Z, !y);
        setflags(FLAG_N, y & 0x80);
        break;
    case JMP:
        wval = getmem(pc++);
        wval |= 256 * getmem(pc++);
        switch (addr) {
        case ABS: pc = wval; break;
        case IND:
            pc = getmem(wval);
            pc |= 256 * getmem(wval + 1);
            break;
        }
        break;
    case JSR:
        push((pc + 1) >> 8);
        push((pc + 1));
        wval = getmem(pc++);
        wval |= 256 * getmem(pc++);
        pc = wval;
        break;
    case LDA:
        a = getaddr(addr);
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;
    case LDX:
        x = getaddr(addr);
        setflags(FLAG_Z, !x);
        setflags(FLAG_N, x & 0x80);
        break;
    case LDY:
        y = getaddr(addr);
        setflags(FLAG_Z, !y);
        setflags(FLAG_N, y & 0x80);
        break;
    case LSR:
        bval = getaddr(addr);
        wval = (uint8_t)bval;
        wval >>= 1;
        setaddr(addr, (uint8_t)wval);
        setflags(FLAG_Z, !wval);
        setflags(FLAG_N, wval & 0x80);
        setflags(FLAG_C, bval & 1);
        break;
    case NOP: break;
    case ORA:
        bval = getaddr(addr);
        a |= bval;
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;
    case PHA: push(a); break;
    case PHP: push(p); break;
    case PLA:
        a = pop();
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;
    case PLP: p = pop(); break;
    case ROL:
        bval = getaddr(addr);
        c    = !!(p & FLAG_C);
        setflags(FLAG_C, bval & 0x80);
        bval <<= 1;
        bval |= c;
        setaddr(addr, bval);
        setflags(FLAG_N, bval & 0x80);
        setflags(FLAG_Z, !bval);
        break;
    case ROR:
        bval = getaddr(addr);
        c    = !!(p & FLAG_C);
        setflags(FLAG_C, bval & 1);
        bval >>= 1;
        bval |= 128 * c;
        setaddr(addr, bval);
        setflags(FLAG_N, bval & 0x80);
        setflags(FLAG_Z, !bval);
        break;
    case RTI: // Treat RTI like RTS
    case RTS:
        wval = pop();
        wval |= pop() << 8;
        pc = wval + 1;
        break;
    case SBC:
        bval = getaddr(addr) ^ 0xff;
        wval = (uint16_t)a + bval + ((p & FLAG_C) ? 1 : 0);
        setflags(FLAG_C, wval & 0x100);
        a = (uint8_t)wval;
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a > 127);
        setflags(FLAG_V, (!!(p & FLAG_C)) ^ (!!(p & FLAG_N)));
        break;
    case SEC: setflags(FLAG_C, 1); break;
    case SED: setflags(FLAG_D, 1); break;
    case SEI: setflags(FLAG_I, 1); break;
    case STA: putaddr(addr, a); break;
    case STX: putaddr(addr, x); break;
    case STY: putaddr(addr, y); break;
    case TAX:
        x = a;
        setflags(FLAG_Z, !x);
        setflags(FLAG_N, x & 0x80);
        break;
    case TAY:
        y = a;
        setflags(FLAG_Z, !y);
        setflags(FLAG_N, y & 0x80);
        break;
    case TSX:
        x = s;
        setflags(FLAG_Z, !x);
        setflags(FLAG_N, x & 0x80);
        break;
    case TXA:
        a = x;
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;
    case TXS: s = x; break;
    case TYA:
        a = y;
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;

    // secret opcodes
    case SLO:
        bval = getaddr(addr);
        setflags(FLAG_C, bval >> 7);
        bval <<= 1;
        setaddr(addr, bval);
        a |= bval;
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;
    case AXS:
        wval = x = (x & a) - getaddr(addr);
        setflags(FLAG_Z, !x);
        setflags(FLAG_N, x & 0x80);
        setflags(FLAG_C, wval & 0x100);
        break;
    case LAX:
        a = x = getaddr(addr);
        setflags(FLAG_Z, !a);
        setflags(FLAG_N, a & 0x80);
        break;

    default:
        printf("cpu: unknown opcode: %02x\n", opc);
        break;
    }
}


void CPU::jsr(uint16_t npc, uint8_t na) {
    a  = na;
    x  = 0;
    y  = 0;
    p  = 0;
    s  = 255;
    pc = npc;
    push(0);
    push(0);
    while (pc) parse(getmem(pc++));
}


