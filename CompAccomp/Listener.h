#pragma once
#include <stdint.h>
#include <thread>
#include <chrono>
#include <vector>
#include "essentia/algorithmfactory.h"
#include "essentia/essentiamath.h"
#include "essentia/pool.h"
#include "essentia/streaming/streamingalgorithm.h"
#include "essentia/streaming/algorithms/vectorinput.h"
#include "essentia/streaming/algorithms/vectoroutput.h"
#include "essentia/streaming/algorithms/ringbufferinput.h"
#include "essentia/streaming/algorithms/ringbufferoutput.h"
#include "essentia/streaming/algorithms/poolstorage.h"
#include "essentia/scheduler/network.h"
#include "soundio.h"

typedef uint16_t note;

namespace config {
    const int ringBufferDuration = 30;
    const int frameSize = 4096;
    const int hopSize = 4096;
    const float threshold = 500; //placeholder
};

class Listener {
private:
    //soundio
    static SoundIoFormat prioritized_formats[19];
    SoundIo* sio;
    SoundIoInStream* instream;
    SoundIoDevice* in_device;
    RecordContext* rc;
    static void readCallback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max);
    static void overflowCallback(struct SoundIoInStream *instream);
    void sioSetup();
    //essentia
    essentia::standard::Algorithm *window, *cqt;
    std::vector<essentia::Real> in, windowed, out;
    essentia::Pool pool;
    void essentiaSetup();
    //core
    void listen();
    note compute();

public:
    Listener();
    ~Listener();
};

struct RecordContext {
    struct SoundIoRingBuffer *ring_buffer;
};