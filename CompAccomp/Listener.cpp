#include "Listener.h"
#ifdef WIN32
    #include "intrin.h"
    #define __builtin_bswap32 _byteswap_ulong
    #define __builtin_bswap64 _byteswap_uint64
#endif

SoundIoFormat Listener::prioritized_formats[19] = {
    SoundIoFormatFloat32NE,
    SoundIoFormatFloat32FE,
    SoundIoFormatS32NE,
    SoundIoFormatS32FE,
    SoundIoFormatS24NE,
    SoundIoFormatS24FE,
    SoundIoFormatS16NE,
    SoundIoFormatS16FE,
    SoundIoFormatFloat64NE,
    SoundIoFormatFloat64FE,
    SoundIoFormatU32NE,
    SoundIoFormatU32FE,
    SoundIoFormatU24NE,
    SoundIoFormatU24FE,
    SoundIoFormatU16NE,
    SoundIoFormatU16FE,
    SoundIoFormatS8,
    SoundIoFormatU8,
    SoundIoFormatInvalid,
};

Listener::Listener() {
    sioSetup();
    essentiaSetup();
}

Listener::~Listener() {
    soundio_instream_destroy(instream);
    soundio_device_unref(in_device);
    soundio_destroy(sio);
}

void Listener::sioSetup() {
    sio = soundio_create();
    int err = soundio_connect(sio);
    soundio_flush_events(sio);
    int out_device_index = soundio_default_output_device_index(sio);
    int in_device_index = soundio_default_input_device_index(sio);
    in_device = soundio_get_input_device(sio, in_device_index);
    soundio_device_sort_channel_layouts(in_device);
    int sampleRate = 44100;
    SoundIoFormat fmt = SoundIoFormatInvalid;
    SoundIoFormat *fmt_ptr;
    for (fmt_ptr = prioritized_formats; *fmt_ptr != SoundIoFormatInvalid; fmt_ptr += 1) {
        if (soundio_device_supports_format(in_device, *fmt_ptr)) {
            fmt = *fmt_ptr;
            break;
        }
    }
    if (fmt == SoundIoFormatInvalid) {
        fmt = in_device->formats[0];
    }
    SoundIoInStream *instream = soundio_instream_create(in_device);
    instream->format = fmt;
    instream->sample_rate = 44100;
    instream->read_callback = readCallback;
    instream->overflow_callback = overflowCallback;
    instream->userdata = &rc;
    rc->ring_buffer = soundio_ring_buffer_create(sio, config::ringBufferDuration * instream->sample_rate * instream->bytes_per_frame);
}

void Listener::essentiaSetup() {
    essentia::standard::AlgorithmFactory& factory = essentia::standard::AlgorithmFactory::instance();
    window = factory.create("Windowing",
        "type", "hamming");
    cqt = factory.create("SpectrumCQ",
        "binsPerOctave", 12,
        "maxFrequency", 2637, //E7
        "minFrequency", 196); //G3
    window->input("signal").set(in);
    window->output("windowedSignal").set(windowed);
    cqt->input("frame").set(windowed);
    cqt->output("spectrumCQ").set(out);
}

void Listener::listen() {
    int capacity = config::ringBufferDuration * instream->sample_rate * instream->bytes_per_frame;
    soundio_flush_events(sio);
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    int fill_bytes = soundio_ring_buffer_fill_count(rc->ring_buffer);
    char *read_buf = soundio_ring_buffer_read_ptr(rc->ring_buffer);

    int end = (int)(fill_bytes + read_buf);
    if (instream->format <= SoundIoFormatU32BE) { //int case
        std::vector<int> temp;
        temp.reserve(config::ringBufferDuration * instream->sample_rate);
        for (int i = (int)read_buf; i < end; i++) {
            temp[i / instream->bytes_per_frame] |= *(char*)i << (i % instream->bytes_per_frame);
        }
        if (instream->format % 2 == 0) {
            std::for_each(temp.begin(), temp.end(), __builtin_bswap32);
        }
        in = std::vector<float>(&temp, &temp + capacity);
    }
    else { //float case
        std::vector<int64_t> temp(config::ringBufferDuration * instream->sample_rate);
        for (int i = (int)read_buf; i < end; i += instream->bytes_per_frame) {
            temp[i / instream->bytes_per_frame] |= *(char*)i << (i % instream->bytes_per_frame);
        }
        if (instream->format % 2 == 0) {
            std::for_each(temp.begin(), temp.end(), __builtin_bswap64);
        }
        double* arr = reinterpret_cast<double *>(&(temp[0]));
        in = std::vector<float>(arr, arr + capacity);
    }
    soundio_ring_buffer_advance_read_ptr(rc->ring_buffer, fill_bytes);
}

note Listener::compute() {
    window->compute();
    cqt->compute();
    std::vector<float> energy(12);
    for (int i = 0; i < out.size(); i++) {
        energy[i % 12] += out[i];
    }
    note res = 0;
    float previous = energy[0];
    for (int b = 1; b < 12; b++) {
        if (energy[b] > previous) {
            energy[b - 1] = 0;
            previous = energy[b];
        }
        else if (energy[b] < previous) {
            previous = energy[b];
            energy[b] = 0;
        }
    }
    for (int b = 0; b < 12; b++) {
        if (energy[b] > config::threshold) {
            res |= 1 << b;
        }
    }
    return res;
}

//copied code
void Listener::readCallback(SoundIoInStream * instream, int frame_count_min, int frame_count_max)
{
    struct RecordContext *rc = (struct RecordContext*)(instream->userdata); //fishy
    struct SoundIoChannelArea *areas;
    int err;
    char *write_ptr = soundio_ring_buffer_write_ptr(rc->ring_buffer);
    int free_bytes = soundio_ring_buffer_free_count(rc->ring_buffer);
    int free_count = free_bytes / instream->bytes_per_frame;
    if (free_count < frame_count_min) {
        fprintf(stderr, "ring buffer overflow\n");
        exit(1);
    }
    int write_frames = std::min(free_count, frame_count_max);
    int frames_left = write_frames;
    for (;;) {
        int frame_count = frames_left;
        if ((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
            fprintf(stderr, "begin read error: %s", soundio_strerror(err));
            exit(1);
        }
        if (!frame_count)
            break;
        if (!areas) {
            // Due to an overflow there is a hole. Fill the ring buffer with
            // silence for the size of the hole.
            memset(write_ptr, 0, frame_count * instream->bytes_per_frame);
        }
        else {
            for (int frame = 0; frame < frame_count; frame += 1) {
                for (int ch = 0; ch < instream->layout.channel_count; ch += 1) {
                    memcpy(write_ptr, areas[ch].ptr, instream->bytes_per_sample);
                    areas[ch].ptr += areas[ch].step;
                    write_ptr += instream->bytes_per_sample;
                }
            }
        }
        if ((err = soundio_instream_end_read(instream))) {
            fprintf(stderr, "end read error: %s", soundio_strerror(err));
            exit(1);
        }
        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }
    int advance_bytes = write_frames * instream->bytes_per_frame;
    soundio_ring_buffer_advance_write_ptr(rc->ring_buffer, advance_bytes);
}

void Listener::overflowCallback(SoundIoInStream * instream) {
    static int count = 0;
    fprintf(stderr, "overflow %d\n", ++count);
}
