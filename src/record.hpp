#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <functional>


struct Record {

    struct State {
        std::array<uint8_t, 25> is_set = {};
        std::array<uint8_t, 25> reg    = {};
    };

    std::vector<State> states;
    std::string        song_name;
    std::string        song_author;
    std::string        song_released;
    int                song_count;
    int                song_nr;
    int                speed;

    bool load(const char* filename, int song_nr = 0);
};
