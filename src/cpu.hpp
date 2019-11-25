// this code is based on kb's 6502 code in tinysid, i think.
#pragma once
#include <cinttypes>
#include <array>

class CPU {
public:
    void jsr(uint16_t npc, uint8_t na);
    virtual ~CPU() {}

    virtual uint8_t getmem(uint16_t addr) = 0;
    virtual void setmem(uint16_t addr, uint8_t value) = 0;

private:
    uint8_t getaddr(int mode);
    void    setaddr(int mode, uint8_t val);
    void    putaddr(int mode, uint8_t val);
    void    setflags(int flag, int cond);
    void    push(uint8_t val);
    uint8_t pop();
    void    branch(int flag);
    void    parse(uint8_t opc);

    int      cycles;
    uint16_t pc;
    uint8_t  a, x, y, s, p;
};
