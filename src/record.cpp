#include <fstream>
#include <cstring>
#include <cmath>
#include "record.hpp"
#include "cpu.hpp"

class MyCPU : public CPU {
public:
    void jsr(uint16_t npc, uint8_t na, std::function<void(uint16_t, uint8_t)> f = nullptr) {
        callback = f;
        CPU::jsr(npc, na);
    }

    void setmem(uint16_t addr, uint8_t value) override {
        if (callback) callback(addr, value);
        ram[addr] = value;
    }
    uint8_t getmem(uint16_t addr) override {
        return ram[addr];
    }

    std::array<uint8_t, 0x10000>           ram = {};
    std::function<void(uint16_t, uint8_t)> callback;
};


struct Header {
    uint8_t  magic[4];
    uint16_t version;
    uint16_t offset;
    uint16_t load_addr;
    uint16_t init_addr;
    uint16_t play_addr;
    uint16_t song_count;
    uint16_t start_song;
    uint32_t speed;
    char     song_name[32];
    char     song_author[32];
    char     song_released[32];
    uint16_t flags;
    uint8_t  start_page;
    uint8_t  page_length;
    uint8_t  sid_addr_2;
    uint8_t  sid_addr_3;
} __attribute__((packed));


uint32_t swap(uint16_t v) { return  __builtin_bswap16(v); }
uint32_t swap(uint32_t v) { return  __builtin_bswap32(v); }


bool Record::load(const char* filename, int nr) {
    states.clear();

    // log
    const char* dot = strrchr(filename, '.');
    if (dot && strcmp(dot, ".txt") == 0) {
        FILE* f = fopen(filename, "r");
        if (!f) return false;
        State s;
        int dt, addr, val;
        while (fscanf(f, "%d [%d] = %x ", &dt, &addr, &val) == 3) {
            int idle = 0;
            while (idle++ < 10 && dt > 17000) {
                dt -= 17000;
                states.emplace_back(s);
                s.is_set = {};
            }
            s.reg[addr] = val;
            s.is_set[addr] = true;
        }
        fclose(f);
        song_name  = filename;
        speed      = 1;
        song_nr    = 1;
        song_count = 1;
        return true;
    }


    std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) {
        printf("error: could not open file\n");
        return false;
    }
    auto pos = ifs.tellg();
    std::vector<uint8_t> data(pos);
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*) data.data(), pos);



    Header* h = (Header*) data.data();
    h->version    = swap(h->version);
    h->offset     = swap(h->offset);
    h->load_addr  = swap(h->load_addr);
    h->init_addr  = swap(h->init_addr);
    h->play_addr  = swap(h->play_addr);
    h->song_count = swap(h->song_count);
    h->start_song = swap(h->start_song);
    h->speed      = swap(h->speed);

    // ???
    h->load_addr = data[h->offset] | (data[h->offset + 1] << 8);

    printf("magic:       %.4s\n", h->magic);
    printf("version:     %d\n", h->version);
    printf("offset:      %04x\n", h->offset);
    printf("load addr:   %04x\n", h->load_addr);
    printf("init addr:   %04x\n", h->init_addr);
    printf("play addr:   %04x\n", h->play_addr);
    printf("song count:  %d\n", h->song_count);
    printf("start song:  %d\n", h->start_song);
    printf("speed:       %08x\n", h->speed);
    printf("song name:   %.32s\n", h->song_name);
    printf("song author: %.32s\n", h->song_author);
    printf("copyright:   %.32s\n", h->song_released);
    if (h->version > 1) {
        h->flags = swap(h->flags);
        printf("flags:       %04x\n", h->flags);
        printf("start page:  %d\n", h->start_page);
        printf("page length: %d\n", h->page_length);
        printf("sid 2 addr:  %d\n", h->sid_addr_2);
        printf("sid 3 addr:  %d\n", h->sid_addr_3);
    }

    song_name     = h->song_name;
    song_author   = h->song_author;
    song_released = h->song_released;

    song_nr    = nr > 0 ? nr : h->start_song;
    song_count = h->song_count;

    MyCPU cpu;

    uint16_t j = h->load_addr;
    for (int i = 2 + h->offset; i < (int) data.size(); ++i) {
        cpu.ram[j++] = data[i];
    }

    // init song
    cpu.jsr(h->init_addr, song_nr - 1);

    // check timer
    if (song_nr < 32 && ((h->speed >> (song_nr - 1)) & 1)) {
        int timer = (cpu.ram[0xdc05] << 8) | cpu.ram[0xdc04];
//        speed = (19656 + timer / 2) / timer;
//        speed = 19656.0f / timer;
        speed = round(19656 * 2.0f / timer) / 2;
    }
    else {
        speed = 1;
    }

    // play song
    for (int m = 0; m < 60 * 50 * 10 * speed; ++m) {
        State s;
        cpu.jsr(h->play_addr, 0, [&s](uint16_t addr, uint8_t value) {
            if (addr >= 0xd400 && addr < 0xd400 + s.is_set.size()) {
                s.is_set[addr - 0xd400] = true;
            }
        });
        for (size_t i = 0; i < s.reg.size(); ++i) s.reg[i] = cpu.ram[0xd400 + i];
        states.emplace_back(s);
    }

    return true;
}

