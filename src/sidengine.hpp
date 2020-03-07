#pragma once

#include <cstdint>


class SidEngine {
public:
    enum ChipModel { MOS6581, MOS8580 };
    enum { REGISTER_COUNT = 25 };
    virtual char const* name() const = 0;
    virtual void enable_filter(bool e) = 0;
    virtual void set_chip_model(ChipModel cm) = 0;
    virtual void update_registers(uint8_t const* regs) = 0;
    virtual void mix(int16_t* buffer, int length) = 0;

    static SidEngine* create_resid();
    static SidEngine* create_tinysid();
};
