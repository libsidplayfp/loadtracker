//
// BME (Blasphemous Multimedia Engine) sound main module
//

#include "bme_snd.h"

#include "bme_main.h"
#include "bme_win.h"
#include "bme_io.h"
#include "ltlog.h"

#ifdef USE_JACK
#  include <jack/jack.h>
#  include <jack/transport.h>
#endif

#ifdef USE_MIDI_INPUT
#  include <rtmidi/rtmidi_c.h>
#endif

#include <SDL3/SDL.h>

#include <new>

#include <cstdlib>
#include <cstring>
#include <cstdio>

#define VM_OFF 0
#define VM_ON 1
#define VM_ONESHOT 0
#define VM_LOOP 2
#define VM_16BIT 4

#ifdef USE_JACK
typedef jack_default_audio_sample_t sample_t;
#endif

static bool snd_initchannels(unsigned channels);
static bool snd_initmixer();
static void snd_uninitmixer();
static void snd_mixdata(Uint8 *dest, unsigned bytes);
static void snd_mixchannels(Sint32 *dest, unsigned samples);
static void snd_mixer_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);


// Lowlevel mixing functions
static void snd_clearclipbuffer(Sint32 *clipbuffer, unsigned clipsamples);
static void snd_mixchannel(CHANNEL *chptr, Sint32 *dest, unsigned samples);
#ifdef USE_JACK
static void snd_jack_postprocess(Sint32 *src, sample_t* dest, unsigned samples);
#endif
static void snd_float_postprocess(Sint32 *src, float* dest, unsigned samples);
static void snd_16bit_postprocess(Sint32 *src, Sint16 *dest, unsigned samples);
static void snd_8bit_postprocess(Sint32 *src, Uint8 *dest, unsigned samples);

void (*snd_player)() = nullptr;
CHANNEL *snd_channel = nullptr;
int snd_channels = 0;
bool snd_sndinitted = false;
int snd_bpmcount;
int snd_bpmtempo = 125;
unsigned snd_mixmode;
unsigned snd_mixrate;

static void (*snd_custommixer)(Sint32 *dest, unsigned samples) = nullptr;
static unsigned snd_buffersize;
static unsigned snd_framesize;
static unsigned snd_previouschannels = 0xffffffff;
static int snd_atexit_registered = 0;
static Sint32 *snd_clipbuffer = nullptr;
SDL_AudioStream *stream = nullptr;
static SDL_AudioSpec spec;

#ifdef USE_JACK
static bool use_jack = true;
static bool use_jack_audio = false;

static jack_client_t* client;
static jack_port_t* output_port;
#endif

#ifdef USE_MIDI_INPUT
RtMidiInPtr midi_device = nullptr;
#endif

void playtestnote(int note, int ins, int chnnum);
void insertnote(int newnote);

#define VISIBLEPATTROWS 34

extern int einum;
extern int epchn;
extern int epview[];
extern int eppos;

int current_note_on = -1;

#ifdef USE_JACK
int snd_jack_process(jack_nframes_t nframes, void *)
{
    if (use_jack_audio)
    {
        sample_t* buffer = (sample_t*)jack_port_get_buffer(output_port, nframes);
        snd_mixdata((Uint8*)buffer, sizeof(sample_t) * nframes);
    }
    return 0;
}

bool snd_init_jack()
{
    jack_status_t status;

    client = jack_client_open("goattracker2", JackNoStartServer, &status);
    if (client == 0)
    {
        ltlog::error("Failed to create jack client");
        return false;
    }

    if (use_jack_audio)
    {
        snd_mixrate = jack_get_sample_rate(client);

        snd_bpmcount = 0;
        snd_sndinitted = true;

        // force 16 bit
        snd_mixmode = SIXTEENBIT;
        snd_framesize = 2;

        snd_buffersize = 1880;

        if (!snd_initmixer())
        {
            snd_uninit();
            return false;
        }
    }


    jack_set_process_callback(client, snd_jack_process, 0);

    if (use_jack_audio)
    {
        output_port = jack_port_register(client, "playback",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

        if (!output_port)
        {
            ltlog::error("Failed to register midi port");
            return false;
        }
    }

    if (jack_activate(client))
    {
        ltlog::error("Failed to activate jack client");
        return false;
    }

    return true;
}
#endif

#ifdef USE_MIDI_INPUT
void noteOn(unsigned char note)
{
    current_note_on = note;
    insertnote(note + 72);
    epview[epchn] = eppos-VISIBLEPATTROWS/2;
}

void noteOff(unsigned char note)
{
    if (note == current_note_on)
    {
        playtestnote(190, einum, epchn); // off note
        current_note_on = -1;
    }
}

void snd_midi_process(double, const unsigned char *message, size_t messageSize, void*)
{
    for (size_t i = 0; i < messageSize; i++)
    {
         //printf("size: %u: %02X %u %u\n", messageSize,
         //    *message, *(message+1), *(message+2));

        unsigned char midi_cmd = message[i++];
        if ((midi_cmd & 0xf0) == 0x90)
        {
            // note on
            unsigned char note = message[i++];
            unsigned char velocity = message[i];
            if (velocity != 0)
            {
                noteOn(note);
            }
            else
            {
                noteOff(note);
            }
        }
        else if ((midi_cmd & 0xf0) == 0x80)
        {
            // note off
            unsigned char note = message[i++];
            noteOff(note);
        }
    }
}

bool snd_init_midi()
{
    RtMidiInPtr midi_device = rtmidi_in_create(RTMIDI_API_UNSPECIFIED, "goattracker2", 100);
    if (!midi_device->ok)
    {
        std::fprintf(stderr, "failed to activate midi: %s\n", midi_device->msg);
        return false;
    }

    unsigned int ports = rtmidi_get_port_count(midi_device);
    if (!ports)
    {
        ltlog::error("No available MIDI ports");
        return false;
    }

    rtmidi_open_port(midi_device, 0, "midi_in");
    if (!midi_device->ok)
    {
        std::fprintf(stderr, "failed to open port: %s\n", midi_device->msg);
        return false;
    }

    rtmidi_in_set_callback(midi_device, snd_midi_process, nullptr);
    if (!midi_device->ok)
    {
        std::fprintf(stderr, "failed to set midi callback: %s\n", midi_device->msg);
        return false;
    }

    return true;
}
#endif

bool snd_init(unsigned mixrate, unsigned mixmode)
{
    // If user wants to re-initialize, shutdown first

    snd_uninit();

#ifdef USE_MIDI_INPUT
    snd_init_midi();
#endif

#ifdef USE_JACK
    if (use_jack) {
        snd_init_jack();
        if (use_jack_audio) return true;
    }
#endif
    // Register snd_uninit as an atexit function

    if (!snd_atexit_registered)
    {
        std::atexit(snd_uninit);
        snd_atexit_registered = 1;
    }

    // Check for illegal config

    if (!mixrate)
    {
        snd_uninit();
        return false;
    }

    spec.freq = mixrate;
    spec.format = (mixmode & SIXTEENBIT) ? SDL_AUDIO_S16 : SDL_AUDIO_U8;
    spec.channels = (mixmode & STEREO) ? 2 : 1;

    // Init tempo count

    snd_bpmcount = 0;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, snd_mixer_callback, nullptr);

    if (!stream)
    {
        snd_uninit();
        return false;
    }
    snd_sndinitted = true;

    snd_mixmode = 0;
    snd_framesize = 1;

    if (spec.channels == 2)
    {
        snd_mixmode |= STEREO;
        snd_framesize <<= 1;
    }

    // (Re)allocate channels if necessary
    if (!snd_initchannels(spec.channels)) {
        return false;
    }

    if ((SDL_AUDIO_BITSIZE(spec.format) == 16) &&
       SDL_AUDIO_ISSIGNED(spec.format) &&
       SDL_AUDIO_ISINT(spec.format))
    {
        snd_mixmode |= SIXTEENBIT;
        snd_framesize <<= 1;
    }
    else if ((SDL_AUDIO_BITSIZE(spec.format) == 32) &&
       SDL_AUDIO_ISSIGNED(spec.format) &&
       SDL_AUDIO_ISFLOAT(spec.format))
    {
        snd_mixmode |= FLOAT32BIT;
        snd_framesize <<= 2;
    }
    else
    {
        snd_uninit();
        return false;
    }

    int sample_frames;
    SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &spec, &sample_frames);

    snd_buffersize = sample_frames;
    snd_mixrate = spec.freq;

    // Allocate mixer tables

    if (!snd_initmixer())
    {
        snd_uninit();
        return false;
    }

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));
    return true;
}

bool snd_initchannels(unsigned channels) {

    if (snd_previouschannels != channels)
    {
        if (snd_channel)
        {
            delete [] snd_channel;
            snd_channel = nullptr;
            snd_channels = 0;
        }

        snd_channel = new (std::nothrow) CHANNEL[channels];
        if (!snd_channel)
        {
            snd_uninit();
            return false;
        }
        CHANNEL *chptr = &snd_channel[0];
        snd_channels = channels;
        snd_previouschannels = channels;

        // Init all channels (no sound played, no sample, mastervolume 64)
        for (int c = snd_channels; c > 0; c--)
        {
            chptr->voicemode = VM_OFF;
            chptr->smp = nullptr;
            chptr->mastervol = 64;
            chptr++;
        }
    }

    return true;
}


void snd_uninit()
{
    if (snd_sndinitted
#ifdef USE_JACK
        && !use_jack_audio
#endif
        )
    {
        SDL_DestroyAudioStream(stream);
        snd_sndinitted = false;
    }
    snd_uninitmixer();
#ifdef USE_MIDI_INPUT
    if (midi_device != nullptr)
        rtmidi_in_free(midi_device);
#endif
}

void snd_setcustommixer(void (*custommixer)(Sint32 *dest, unsigned samples))
{
    snd_custommixer = custommixer;
}

static bool snd_initmixer()
{
    snd_uninitmixer();

    size_t bufSize = snd_buffersize * sizeof(Sint32);
    if (snd_mixmode & STEREO)
    {
        bufSize *= 2;
    }

    snd_clipbuffer = new (std::nothrow) Sint32[bufSize];
    if (!snd_clipbuffer) return false;

    return true;
}

static void snd_uninitmixer()
{
    if (snd_clipbuffer)
    {
        delete [] snd_clipbuffer;
        snd_clipbuffer = nullptr;
    }
}

void snd_mixer_callback(void*, SDL_AudioStream *stream, int additional_amount, int)
{
    if (additional_amount > 0)
    {
        Uint8 *data = SDL_stack_alloc(Uint8, additional_amount);
        if (data)
        {
            snd_mixdata(data, additional_amount);
            SDL_PutAudioStreamData(stream, data, additional_amount);
            SDL_stack_free(data);
        }
    }
}

static void snd_mixdata(Uint8 *dest, unsigned bytes)
{
    unsigned mixsamples = bytes / snd_framesize;
    unsigned clipsamples = mixsamples;
    if (snd_mixmode & STEREO) clipsamples <<= 1;
    Sint32 *clipptr = (Sint32 *)snd_clipbuffer;
#ifdef USE_JACK
    if (use_jack_audio) {
        clipsamples = bytes / sizeof(sample_t);
        mixsamples = clipsamples;
    }
#endif
    snd_clearclipbuffer(snd_clipbuffer, clipsamples);

    if (snd_player) // Must the player be called?
    {
        while(mixsamples)
        {
            if ((!snd_bpmcount) && (snd_player)) // Player still active?
            {
                // Call player
                snd_player();
                // Reset tempocounter
                snd_bpmcount = ((snd_mixrate * 5) >> 1) / snd_bpmtempo;
            }

            int musicsamples = mixsamples;
            if (musicsamples > snd_bpmcount) musicsamples = snd_bpmcount;
            snd_bpmcount -= musicsamples;
            if (!snd_custommixer)
            {
                snd_mixchannels(clipptr, musicsamples);
            }
            else
            {
                snd_custommixer(clipptr, musicsamples);
            }
            if (snd_mixmode & STEREO) clipptr += musicsamples * 2;
            else clipptr += musicsamples;
            mixsamples -= musicsamples;
        }
    }
    else
    {
        if (!snd_custommixer)
        {
            snd_mixchannels(clipptr, mixsamples);
        }
        else
        {
            snd_custommixer(clipptr, mixsamples);
        }
    }

    clipptr = (Sint32 *)snd_clipbuffer;
#ifdef USE_JACK
    if (use_jack_audio)
    {
        snd_jack_postprocess(clipptr, (sample_t*)dest, clipsamples);
    }
    else
#endif
    if (snd_mixmode & FLOAT32BIT)
    {
        snd_float_postprocess(clipptr, (float *)dest, clipsamples);
    }
    else if (snd_mixmode & SIXTEENBIT)
    {
        snd_16bit_postprocess(clipptr, (Sint16 *)dest, clipsamples);
    }
    else
    {
        snd_8bit_postprocess(clipptr, dest, clipsamples);
    }
}

static void snd_mixchannels(Sint32 *dest, unsigned samples)
{
    CHANNEL *chptr = &snd_channel[0];

    for (int c = snd_channels; c; c--)
    {
        snd_mixchannel(chptr, dest, samples);
        chptr++;
    }
}

static void snd_clearclipbuffer(Sint32 *clipbuffer, unsigned clipsamples)
{
    memset(clipbuffer, 0, clipsamples*sizeof(Sint32));
}

#ifdef USE_JACK
static void snd_jack_postprocess(Sint32* src, sample_t* dest, unsigned samples) {
    while (samples--)
    {
        int sample = *src++;
        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;
        *dest++ = sample / 32768.0;
    }
}
#endif

static void snd_float_postprocess(Sint32* src, float* dest, unsigned samples) {
    while (samples--)
    {
        int sample = *src++;
        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;
        *dest++ = sample / 32768.0f;
    }
}

static void snd_16bit_postprocess(Sint32 *src, Sint16 *dest, unsigned samples)
{
    while (samples--)
    {
        int sample = *src++;
        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;
        *dest++ = sample;
    }
}

static void snd_8bit_postprocess(Sint32 *src, Uint8 *dest, unsigned samples)
{
    while (samples--)
    {
          int sample = *src++;
          if (sample > 32767) sample = 32767;
          if (sample < -32768) sample = -32768;
          *dest++ = (sample >> 8) + 128;
    }
}

static void snd_mixchannel(CHANNEL *chptr, Sint32 *dest, unsigned samples)
{
    if (chptr->voicemode & VM_ON)
    {
          unsigned freq = chptr->freq;
          if (freq > 535232) freq = 535232;
          unsigned intadd = freq / snd_mixrate;
          unsigned fractadd = (((freq % snd_mixrate) << 16) / snd_mixrate) & 65535;

          if (snd_mixmode & STEREO)
          {
                int leftvol = (((chptr->vol * chptr->mastervol) >> 6) * (255-chptr->panning)) >> 7;
                int rightvol = (((chptr->vol * chptr->mastervol) >> 6) * (chptr->panning)) >> 7;
                if (leftvol < 0) leftvol = 0;
                if (leftvol > 255) leftvol = 255;
                if (rightvol < 0) rightvol = 0;
                if (rightvol > 255) rightvol = 255;

                if (chptr->voicemode & VM_16BIT)
                {
                    Sint16 *pos = (Sint16 *)chptr->pos;
                    Sint16 *end = (Sint16 *)chptr->end;
                    Sint16 *repeat = (Sint16 *)chptr->repeat;

                    if (chptr->voicemode & VM_LOOP)
                    {
                          while (samples--)
                          {
                                *dest = *dest + ((*pos * leftvol) >> 8);
                                dest++;
                                *dest = *dest + ((*pos * rightvol) >> 8);
                                dest++;
                                chptr->fractpos += fractadd;
                                if (chptr->fractpos > 65535)
                                {
                                    chptr->fractpos &= 65535;
                                    pos++;
                                }
                                pos += intadd;
                                while (pos >= end) pos -= (end - repeat);
                          }
                    }
                    else
                    {
                          while (samples--)
                          {
                                *dest = *dest + ((*pos * leftvol) >> 8);
                                dest++;
                                *dest = *dest + ((*pos * rightvol) >> 8);
                                dest++;
                                chptr->fractpos += fractadd;
                                if (chptr->fractpos > 65535)
                                {
                                    chptr->fractpos &= 65535;
                                    pos++;
                                }
                                pos += intadd;
                                if (pos >= end)
                                {
                                    chptr->voicemode &= ~VM_ON;
                                    break;
                                }
                          }
                    }
                    chptr->pos = (Sint8 *)pos;
                }
                else
                {
                    Sint8 *pos = (Sint8 *)chptr->pos;
                    Sint8 *end = chptr->end;
                    Sint8 *repeat = chptr->repeat;

                    if (chptr->voicemode & VM_LOOP)
                    {
                          while (samples--)
                          {
                                *dest = *dest + (*pos * leftvol);
                                dest++;
                                *dest = *dest + (*pos * rightvol);
                                dest++;
                                chptr->fractpos += fractadd;
                                if (chptr->fractpos > 65535)
                                {
                                    chptr->fractpos &= 65535;
                                    pos++;
                                }
                                pos += intadd;
                                while (pos >= end) pos -= (end - repeat);
                          }
                    }
                    else
                    {
                          while (samples--)
                          {
                                *dest = *dest + (*pos * leftvol);
                                dest++;
                                *dest = *dest + (*pos * rightvol);
                                dest++;
                                chptr->fractpos += fractadd;
                                if (chptr->fractpos > 65535)
                                {
                                    chptr->fractpos &= 65535;
                                    pos++;
                                }
                                pos += intadd;
                                if (pos >= end)
                                {
                                    chptr->voicemode &= ~VM_ON;
                                    break;
                                }
                          }
                    }
                    chptr->pos = (Sint8 *)pos;
                }
          }
          else
          {
                int vol = ((chptr->vol * chptr->mastervol) >> 6);
                if (vol < 0) vol = 0;
                if (vol > 255) vol = 255;

                if (chptr->voicemode & VM_16BIT)
                {
                    Sint16 *pos = (Sint16 *)chptr->pos;
                    Sint16 *end = (Sint16 *)chptr->end;
                    Sint16 *repeat = (Sint16 *)chptr->repeat;

                    if (chptr->voicemode & VM_LOOP)
                    {
                          while (samples--)
                          {
                                *dest = *dest + ((*pos * vol) >> 8);
                                dest++;
                                chptr->fractpos += fractadd;
                                if (chptr->fractpos > 65535)
                                {
                                    chptr->fractpos &= 65535;
                                    pos++;
                                }
                                pos += intadd;
                                while (pos >= end) pos -= (end - repeat);
                          }
                    }
                    else
                    {
                          while (samples--)
                          {
                                *dest = *dest + ((*pos * vol) >> 8);
                                dest++;
                                chptr->fractpos += fractadd;
                                if (chptr->fractpos > 65535)
                                {
                                    chptr->fractpos &= 65535;
                                    pos++;
                                }
                                pos += intadd;
                                if (pos >= end)
                                {
                                    chptr->voicemode &= ~VM_ON;
                                    break;
                                }
                          }
                    }
                    chptr->pos = (Sint8 *)pos;
                }
                else
                {
                    Sint8 *pos = (Sint8 *)chptr->pos;
                    Sint8 *end = chptr->end;
                    Sint8 *repeat = chptr->repeat;

                    if (chptr->voicemode & VM_LOOP)
                    {
                          while (samples--)
                          {
                                *dest = *dest + (*pos * vol);
                                dest++;
                                chptr->fractpos += fractadd;
                                if (chptr->fractpos > 65535)
                                {
                                    chptr->fractpos &= 65535;
                                    pos++;
                                }
                                pos += intadd;
                                while (pos >= end) pos -= (end - repeat);
                          }
                    }
                    else
                    {
                          while (samples--)
                          {
                                *dest = *dest + (*pos * vol);
                                dest++;
                                chptr->fractpos += fractadd;
                                if (chptr->fractpos > 65535)
                                {
                                    chptr->fractpos &= 65535;
                                    pos++;
                                }
                                pos += intadd;
                                if (pos >= end)
                                {
                                    chptr->voicemode &= ~VM_ON;
                                    break;
                                }
                          }
                    }
                    chptr->pos = (Sint8 *)pos;
                }
          }
    }
}
