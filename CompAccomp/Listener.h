#pragma once
#include <stdint.h>

typedef uint16_t note;

class Listener {
private:
    note cqt(int* audio, double start, double octaves, double binsPerOctave);
public:
    Listener();
    ~Listener();
};