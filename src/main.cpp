#include <cstdio>
#include <chrono>
#include <SDL2/SDL.h>
#include "record.hpp"
#include "fx.hpp"
#include "sidengine.hpp"


enum {
    MIXRATE           = 44100,
    BUFFER_SIZE       = MIXRATE / 50,
    SPEED             = 1,
    FRAMERATE         = 50 * SPEED,
    SAMPLES_PER_FRAME = MIXRATE / FRAMERATE,
    CHANNEL_COUT      = 3,
};

std::array<SidEngine*, 2> engines = {
    SidEngine::create_resid(),
    SidEngine::create_tinysid(),
};

Record     record;
int        frame;
bool       playing       = false;
bool       chan_active[] = { 1, 1, 1 };
int        engine_nr     = 0;
size_t     render_time   = 0;

void tick() {
    const Uint8* ks = SDL_GetKeyboardState(nullptr);
    if (!ks[SDL_SCANCODE_LCTRL] && !ks[SDL_SCANCODE_RCTRL]) {
        bool shift = ks[SDL_SCANCODE_LSHIFT] | ks[SDL_SCANCODE_RSHIFT];
        int  speed = shift ? 5 : 1;
        if (ks[SDL_SCANCODE_LEFT])       frame -= speed;
        else if (ks[SDL_SCANCODE_RIGHT]) frame += speed;
        else if (playing)                ++frame;
        frame = std::min<int>(std::max<int>(frame, 0), record.states.size());
    }

    std::array<uint8_t, 25> reg = record.states[frame].reg;
    for (int c = 0; c < CHANNEL_COUT; c++) {
        for (int r = 0; r < 7; r++) {
            uint8_t& a = reg[c * 7 + r];
            // mute
            if (!chan_active[c] && r == 4) a &= 0xf0;
            if (!chan_active[c] && r == 5) a = 0xff;
            if (!chan_active[c] && r == 6) a = 0x00;
        }
    }
    engines[engine_nr]->update_registers(reg.data());
}

void audio_callback(void* u, Uint8* stream, int bytes) {
    auto then = std::chrono::steady_clock::now();

    int16_t* buffer = (int16_t*) stream;
    int      length = bytes / sizeof(int16_t);
    static int sample = 0;
    while (length > 0) {
        if (sample == 0) tick();
        int l = std::min(SAMPLES_PER_FRAME - sample, length);
        sample += l;
        if (sample == SAMPLES_PER_FRAME) sample = 0;
        length -= l;
        if (playing) engines[engine_nr]->mix(buffer, l);
        else for (int i = 0; i < l; ++i) buffer[i] = 0;
        buffer += l;
    }

    // measure time
    auto now = std::chrono::steady_clock::now();
    size_t nano = std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();
    static size_t nano_acc = 0;
    static size_t count = 0;
    nano_acc += nano;
    count += bytes / 2;
    if (count >= MIXRATE) {
        render_time = nano_acc / (count / float(MIXRATE));
        nano_acc = 0;
        count = 0;
//        printf("%10lu\n", render_time);
    }
}


struct App : fx::App {

    int  scale_x       = 4;
    int  scale_y       = 7;
    int  offset        = 3;
    int  bar           = 48;
    bool show_bar      = true;
    bool filter_active = true;
    int  vert_pos      = 16;
    SidEngine::ChipModel chip_model = SidEngine::MOS8580;

    void init() override {

        engines[engine_nr]->set_chip_model(chip_model);

        SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 1, 0, BUFFER_SIZE, 0, 0, &audio_callback };
        SDL_OpenAudio(&spec, nullptr);
        SDL_PauseAudio(0);
    }

    const char* title() const override { return "sid-monitor"; }

    void key(int code) override {
        const Uint8* ks = SDL_GetKeyboardState(nullptr);
        bool ctrl = ks[SDL_SCANCODE_LCTRL] | ks[SDL_SCANCODE_RCTRL];

        switch (code) {
        case SDL_SCANCODE_SPACE: playing ^= 1; break;
        case SDL_SCANCODE_LEFT: if (ctrl) frame = std::max(0, frame - 1); break;
        case SDL_SCANCODE_RIGHT: if (ctrl) ++frame; break;
        case SDL_SCANCODE_BACKSPACE: frame = 0; break;

        case SDL_SCANCODE_PAGEDOWN: scale_y = std::max(2, scale_y - 1); break;
        case SDL_SCANCODE_PAGEUP: ++scale_y; break;

        case SDL_SCANCODE_B: show_bar ^= 1; break;
        case SDL_SCANCODE_W: ++bar; break;
        case SDL_SCANCODE_S: --bar; break;
        case SDL_SCANCODE_D: ++offset; break;
        case SDL_SCANCODE_A: --offset; break;

        case SDL_SCANCODE_1: chan_active[0] ^= 1; break;
        case SDL_SCANCODE_2: chan_active[1] ^= 1; break;
        case SDL_SCANCODE_3: chan_active[2] ^= 1; break;

        case SDL_SCANCODE_F: engines[engine_nr]->enable_filter(filter_active ^= 1); break;
        case SDL_SCANCODE_M:
            chip_model = chip_model == SidEngine::MOS6581 ? SidEngine::MOS8580 : SidEngine::MOS6581;
            engines[engine_nr]->set_chip_model(chip_model);
            break;

        case SDL_SCANCODE_TAB:
            engine_nr = (engine_nr + 1) % engines.size();
            engines[engine_nr]->set_chip_model(chip_model);
            engines[engine_nr]->enable_filter(filter_active);
            break;

        default: break;
        }
    }

    void textinput(char const* text) override {
        if (text[0] == '+') ++scale_x;
        if (text[0] == '-') scale_x = std::max(1, scale_x - 1);
    }

    void update() override {
        fx::set_color(0, 0, 0);
        fx::clear();
        fx::set_font_color(250, 250, 250);

        int f = frame;
        int frames_per_screen = fx::screen_width() / scale_x;
        int start_frame = std::max(0, f - frames_per_screen / 2);

        // bar
        if (show_bar) {
            fx::set_color(50, 50, 50);
            for (int t = -((start_frame - offset) % bar); t < frames_per_screen; t += bar) {
                 fx::draw_rectangle(true, t * scale_x, 0, 1, fx::screen_height());
            }
        }


        for (int p = -69; p < 40; ++p) {

            int c = "101201011010"[(p + 120) % 12] - '0';
            c = 10 + c * 20;
            fx::set_color(c, c, c);

            int y = -(p + vert_pos) * scale_y + fx::screen_height() / 2;
            fx::draw_rectangle(true, 0, y, fx::screen_width(), 1 + scale_y - 2);

        }

        for (int n = start_frame; n < start_frame + frames_per_screen; ++n) {
            if (n >= (int) record.states.size()) break;
            auto const& state      = record.states[n];
            auto const& prev_state = record.states[n > 0 ? n - 1 : 0];

            int x = (n - start_frame) * scale_x;

            for (int c = 0; c < CHANNEL_COUT; c++) {
                if (!chan_active[c]) continue;

                int   freq      = state.reg[c * 7 + 0] | (state.reg[c * 7 + 1] << 8);
                float real_freq = freq * 17734472.0f * (1.0f / (18 << 24));
                float pitch     = std::log2(real_freq / 440) * 12;

                bool gate       = state.reg[c * 7 + 4] & 1;
                int  wave       = state.reg[c * 7 + 4] >> 4;
                int  prev_wave  = prev_state.reg[c * 7 + 4] >> 4;
                bool noise      = wave & 8;

                float vol = (wave > 0 ? 1 : 0.2) * (1 + gate);

                int v = 255 * std::pow(vol / 2.0f, 0.8f);
                float q = noise ? 0.66 : 0.33;
                if (c == 0) fx::set_color(v, v*q, v*q);
                if (c == 1) fx::set_color(v*q, v, v*q);
                if (c == 2) fx::set_color(v*q, v*q, v);

                int y = -(pitch + vert_pos) * scale_y + fx::screen_height() / 2;
                fx::draw_rectangle(true, x, y, scale_x, scale_y - 1);

                // new note
                if (wave != 0 && prev_wave == 0) {
                    fx::draw_rectangle(false, x - scale_x, y - scale_y, scale_x, scale_y * 3 - 1);
                }
            }

            // filter
            if (filter_active) {
                int freq = state.reg[22];
                fx::set_color(40, 40, 0);
                fx::draw_rectangle(true, x, fx::screen_height() - freq, scale_x, freq);
            }

//            int c = 2;
//            int pw = state.reg[c * 7 + 2] | ((state.reg[c * 7 + 3] & 0xf) << 8);
//            pw = pw * 100 / 0xfff;
//            fx::set_color(50, 0, 0);
//            fx::draw_rectangle(true, x, fx::screen_height() - pw, scale_x, pw);
         }


        // cursor
        fx::set_color(255, 255, 255);
        fx::draw_rectangle(true, (f - start_frame) * scale_x, 0, 1, fx::screen_height());

        // state
        auto const& state = record.states[f];
        for (int i = 0; i < (int) state.is_set.size(); ++i) {
            int c = i / 7;
            int r = i % 7;
            if (c < CHANNEL_COUT && !chan_active[c]) continue;
            if (state.is_set[i]) fx::set_font_color(250, 250, 250);
            else                 fx::set_font_color(150, 150, 150);
            fx::printf(r * 48 + 8, fx::screen_height() - (4 - c) * 24, "%02X", state.reg[i]);
        }
        fx::set_font_color(250, 250, 250);
        for (int c = 0; c < CHANNEL_COUT; ++c) {
            if (!chan_active[c]) continue;
            int y = fx::screen_height() - (4 - c) * 24;

            bool filter = (state.reg[23] & (1 << c)) > 0;
            fx::printf(7 * 48 + 8, y, "%c", ".*"[filter]);

            int   freq      = state.reg[c * 7 + 0] | (state.reg[c * 7 + 1] << 8);
            float real_freq = freq * 17734472.0f * (1.0f / (18 << 24));
            int note        = int(57.5f + std::log2(real_freq / 440) * 12);

            fx::printf(23 * 16 + 8, y, note > 0 ? "%c%c%d" : "...",
                "CCDDEFFGGAAB"[note % 12],
                "-#-#--#-#-#-"[note % 12],
                note / 12,
                note);

            int control = state.reg[c * 7 + 4];
            int pw = state.reg[c * 7 + 2] | ((state.reg[c * 7 + 3] & 0xf) << 8);
            fx::printf(27 * 16 + 8, y, "%c%c%c%c %c%c%c%c%3d%%",
                    ".G"[!!(control & 0x01)],
                    ".S"[!!(control & 0x02)],
                    ".R"[!!(control & 0x04)],
                    ".T"[!!(control & 0x08)],
                    ".T"[!!(control & 0x10)],
                    ".S"[!!(control & 0x20)],
                    ".P"[!!(control & 0x40)],
                    ".N"[!!(control & 0x80)],
                    pw * 100 / 0xfff);
        }

        fx::printf(8, 8, "%s - %d/%d", record.song_name.c_str(), record.song_nr, record.song_count);
        fx::printf(8, 8 + 24, "%s", record.song_author.c_str());
        fx::printf(8, 8 + 48, "%s", record.song_released.c_str());

        fx::printf(fx::screen_width() - 8 - 16 * 14, 8 + 24 * 0, "  time:  %02d:%02d", frame / FRAMERATE / 60, frame / FRAMERATE % 60);
        fx::printf(fx::screen_width() - 8 - 16 * 14, 8 + 24 * 1, "   pos: %6d", f);
        fx::printf(fx::screen_width() - 8 - 16 * 14, 8 + 24 * 2, "   bar: %6d", bar);
        fx::printf(fx::screen_width() - 8 - 16 * 14, 8 + 24 * 3, "filter:    %s", filter_active ? " on" : "off");
        fx::printf(fx::screen_width() - 8 - 16 * 14, 8 + 24 * 4, " model:   %s",
                   chip_model == SidEngine::MOS6581 ? "6581" : "8580");
        fx::printf(fx::screen_width() - 8 - 16 * 14, 8 + 24 * 5, "engine:%7s", engines[engine_nr]->name());

        const Uint8* ks = SDL_GetKeyboardState(nullptr);
        vert_pos += ks[SDL_SCANCODE_DOWN] - ks[SDL_SCANCODE_UP];

    }
};


int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        printf("usage: %s sid-file [song-number]\n", argv[0]);
        return 0;
    }

    if (!record.load(argv[1], argc == 3 ? atoi(argv[2]) : -1)) return 1;

    App app;
    return fx::run(app);
}
