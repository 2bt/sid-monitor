#include "sidengine.hpp"
#include "resid-0.16/sid.h"
#include <array>

namespace {


enum { MIXRATE = 44100 };

class ReSidEngine : public SidEngine {
public:
    ReSidEngine() {
        m_sid.reset();
        m_sid.set_sampling_parameters(985248, SAMPLE_RESAMPLE_INTERPOLATE, MIXRATE);
    }
    char const* name() const override { return "reSID"; }
    void enable_filter(bool e) override {
        m_sid.enable_filter(e);
    }
    void set_chip_model(ChipModel cm) override {
        m_sid.set_chip_model((chip_model)cm);
    }
    void update_registers(uint8_t const* regs) override {
        for (int i = 0; i < REGISTER_COUNT; ++i) m_sid.write(i, regs[i]);
    }
    void mix(int16_t* buffer, int length) override {
        int c = 999999999;
        m_sid.clock(c, buffer, length);
    }
private:
    SID m_sid;
};


class TinySidEngine : public SidEngine {
public:
    char const* name() const override { return "TinySID"; }
    void enable_filter(bool e) override {}
    void set_chip_model(ChipModel cm) override {}
    void update_registers(uint8_t const* regs) override;
    void mix(int16_t* buffer, int length) override;

private:

    enum {
        CHANNEL_COUNT = 3
    };
    enum State { RELEASE, ATTACK, DECAY, SUSTAIN };
    enum {
        F_GATE  = 0x01,
        F_SYNC  = 0x02,
        F_RING  = 0x04,
        F_TRI   = 0x10,
        F_SAW   = 0x20,
        F_PULSE = 0x40,
        F_NOISE = 0x80,
    };
    enum {
        T_LOW  = 1,
        T_BAND = 2,
        T_HIGH = 4,
    };

    struct Channel {
        State    state;
        int      adsr[4];
        int      flags;
        uint32_t next_pulsewidth;
        uint32_t pulsewidth;
        uint32_t freq;


        // internal things
        int      level;
        uint32_t phase;
        uint32_t noise_phase;
        uint32_t shift = 0x7ffff8;
        int      noise;
        bool     filter;
    };

    struct {
        uint8_t type;
        float   resonance;
        float   freq;

        float   high;
        float   band;
        float   low;

    } m_filter;

    std::array<Channel, CHANNEL_COUNT> m_channels;
};

void TinySidEngine::update_registers(uint8_t const* regs) {
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        Channel& chan = m_channels[i];
        uint8_t const* r = regs + (i * 7);

        static const std::array<int, 16> ATTACK_SPEEDS = {
            168867, 47495, 24124, 15998, 10200, 6908, 5692, 4855,
            3877, 1555, 777, 486, 389, 129, 77, 48,
        };

        static const std::array<int, 16> RELEASE_SPEEDS = {
            42660, 15468, 7857, 5210, 3322, 2250, 1853, 1581,
            1262, 506, 253, 158, 126, 42, 25, 15,
        };

        chan.freq            = (r[0] | (r[1] << 8)) * (15872000 / MIXRATE);
        chan.next_pulsewidth = (r[2] | ((r[3] & 15) << 8)) << 16;
        chan.flags           = r[4];
        chan.adsr[0]         = ATTACK_SPEEDS[r[5] >> 4];
        chan.adsr[1]         = RELEASE_SPEEDS[r[5] & 15];
        chan.adsr[2]         = (r[6] >> 4) * 0x111111;
        chan.adsr[3]         = RELEASE_SPEEDS[r[6] & 15];
        chan.filter          = (regs[23] >> i) & 1;
    }

    // filter
    int ff             = (regs[21] & 7) | (regs[22] << 3);
    m_filter.freq      = ff * (21.5332031 / MIXRATE);
    m_filter.resonance = 1.2 - 0.04 * (regs[23] >> 4);
    m_filter.type      = (regs[24] >> 4) & 7;
}

void TinySidEngine::mix(int16_t* buffer, int length) {
    for (int i = 0; i < length; ++i) {

        int out[2] = {};

        for (int c = 0; c < CHANNEL_COUNT; ++c) {
            Channel& chan = m_channels[c];
            Channel& prev_chan = m_channels[c == 0 ? CHANNEL_COUNT - 1 : c - 1];

            // osc
            chan.phase += chan.freq;
            chan.phase &= 0xfffffff;

            // sync
            if (prev_chan.phase < prev_chan.freq) {
                if (chan.flags & F_SYNC) {
                    chan.phase = prev_chan.phase * chan.freq / prev_chan.freq;
                }
            }

            // envelope
            bool gate = chan.flags & F_GATE;
            if (gate && chan.state == RELEASE) chan.state = ATTACK;
            if (!gate) chan.state = RELEASE;

            switch (chan.state) {
            case ATTACK:
                chan.level += chan.adsr[0];
                if (chan.level >= 0xffffff) {
                    chan.level = 0xffffff;
                    chan.state = DECAY;
                }
                break;
            case DECAY:
                chan.level -= chan.adsr[1];
                if (chan.level <= chan.adsr[2]) {
                    chan.level = chan.adsr[2];
                    chan.state = SUSTAIN;
                }
                break;
            case SUSTAIN:
                if (chan.level != chan.adsr[2]) chan.state = ATTACK;
                break;
            case RELEASE:
                chan.level -= chan.adsr[3];
                if (chan.level < 0) chan.level = 0;
                break;
            }


            // smooth pulsewith change
            if (chan.phase < chan.freq) {
                chan.pulsewidth = chan.next_pulsewidth;
            }

            // waveforms
            uint8_t tri   = ((chan.phase < 0x8000000 ? chan.phase : ~chan.phase) >> 19) & 0xff;
            uint8_t saw   = (chan.phase >> 20) & 0xff;
            uint8_t pulse = ((chan.phase > chan.pulsewidth) - 1) & 0xff;
            if (chan.noise_phase != chan.phase >> 23) {
                chan.noise_phase = chan.phase >> 23;
                uint32_t s = chan.shift;
                chan.shift = s = (s << 1) | (((s >> 22) ^ (s >> 17)) & 1);
                chan.noise = ((s & 0x400000) >> 11) |
                             ((s & 0x100000) >> 10) |
                             ((s & 0x010000) >>  7) |
                             ((s & 0x002000) >>  5) |
                             ((s & 0x000800) >>  4) |
                             ((s & 0x000080) >>  1) |
                             ((s & 0x000010) <<  1) |
                             ((s & 0x000004) <<  2);
            }
            uint8_t noise = chan.noise;

            // ringmod
            if (chan.flags & F_RING && prev_chan.phase < 0x8000000) {
                tri = ~tri;
            }

            int v = 0xff;
            if (chan.flags & F_TRI)   v &= tri;
            if (chan.flags & F_SAW)   v &= saw;
            if (chan.flags & F_PULSE) v &= pulse;
            if (chan.flags & F_NOISE) v &= noise;

            v = ((v - 0x80) * chan.level) >> 18;

            out[chan.filter] += v;
        }


        // filter
        m_filter.high = out[1] - m_filter.band * m_filter.resonance - m_filter.low;
        m_filter.band += m_filter.freq * m_filter.high;
        m_filter.low  += m_filter.freq * m_filter.band;
        int f = 0;
        if (m_filter.type & T_LOW)  f += m_filter.low;
        if (m_filter.type & T_BAND) f += m_filter.band;
        if (m_filter.type & T_HIGH) f += m_filter.high;

        int sample = out[0] + f;
        buffer[i] = std::max(-32768, std::min<int>(sample, 32767));
    }
}


} // namespace


SidEngine* SidEngine::create_resid() { return new ReSidEngine(); }
SidEngine* SidEngine::create_tinysid() { return new TinySidEngine(); }
