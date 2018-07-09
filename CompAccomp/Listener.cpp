#include "Listener.h"

Listener::Listener() {
    struct RecordContext *rc;
    sio = soundio_create();
    int err = soundio_connect(sio);
    soundio_flush_events(sio);
    int out_device_index = soundio_default_output_device_index(sio);
    int in_device_index = soundio_default_input_device_index(sio);
    struct SoundIoDevice *in_device = soundio_get_input_device(sio, in_device_index);
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
    const int ring_buffer_duration_seconds = 30;
    int capacity = ring_buffer_duration_seconds * instream->sample_rate * instream->bytes_per_frame;
    rc->ring_buffer = soundio_ring_buffer_create(sio, capacity);
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
