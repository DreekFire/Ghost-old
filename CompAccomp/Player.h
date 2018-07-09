#pragma once
#include <stdint.h>
#include <cmath>
#include <vector>
#define TSF_IMPLEMENTATION
#include "tsf.h"

typedef uint16_t note;

class Player {
    int position;
    double rate;
    std::vector<note> score;
    tsf* soundfont;
    void note_on(note key, float vel);
    void note_off(note key);
    void play(int duration);
};