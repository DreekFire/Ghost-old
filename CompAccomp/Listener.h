#pragma once
#include "algorithmfactory.h"
#include "soundio.h"

typedef uint16_t note;

class Listener {
private:
    static enum SoundIoFormat prioritized_formats[19];
    struct SoundIo* sio;
    static void readCallback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max);
    static void overflowCallback(struct SoundIoInStream *instream);
public:
    Listener();
    ~Listener();
};

struct RecordContext {
    struct SoundIoRingBuffer *ring_buffer;
};

enum SoundIoFormat Listener::prioritized_formats[19] = {
    SoundIoFormatFloat32BE,
    SoundIoFormatFloat32LE,
    SoundIoFormatS32BE,
    SoundIoFormatS32LE,
    SoundIoFormatS24BE,
    SoundIoFormatS24LE,
    SoundIoFormatS16BE,
    SoundIoFormatS16LE,
    SoundIoFormatFloat64BE,
    SoundIoFormatFloat64LE,
    SoundIoFormatU32BE,
    SoundIoFormatU32LE,
    SoundIoFormatU24BE,
    SoundIoFormatU24LE,
    SoundIoFormatU16BE,
    SoundIoFormatU16LE,
    SoundIoFormatS8,
    SoundIoFormatU8,
    SoundIoFormatInvalid,
};