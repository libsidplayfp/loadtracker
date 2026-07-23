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

#include <SDL3/SDL.h>

#include <new>

#include <cstring>
#include <cstdio>

enum
{
  STEREO        = 1,
  SIXTEENBIT    = 2,
  FLOAT32BIT    = 4
};

enum
{
  VM_OFF    = 0,
  VM_ON     = 1,
  VM_LOOP   = 2,
  VM_16BIT  = 4
};

#ifdef USE_JACK
using sample_t = jack_default_audio_sample_t;
#endif

static bool snd_initmixer();
static void snd_uninitmixer();
static void snd_mixdata(Uint8 *dest, unsigned bytes);
static void snd_mixer_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);


// Lowlevel mixing functions
#ifdef USE_JACK
static void snd_jack_postprocess(Sint32 *src, sample_t* dest, unsigned samples);
#endif
static void snd_float_postprocess(Sint32 *src, float* dest, unsigned samples);
static void snd_16bit_postprocess(Sint32 *src, Sint16 *dest, unsigned samples);
static void snd_8bit_postprocess(Sint32 *src, Uint8 *dest, unsigned samples);

Player snd_player = nullptr;
bool snd_sndinitted = false;
int snd_bpmcount;
int snd_bpmtempo = 125;
unsigned snd_mixmode;
unsigned snd_mixrate;

static CustomMixer snd_custommixer = nullptr;
static unsigned snd_buffersize;
static unsigned snd_framesize;
static Sint32 *snd_clipbuffer = nullptr;
SDL_AudioStream *stream = nullptr;

#ifdef USE_JACK
static bool use_jack = true;
static bool use_jack_audio = false;

static jack_client_t* client;
static jack_port_t* output_port;
#endif

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

    client = jack_client_open("loadtracker", JackNoStartServer, &status);
    if (client == 0)
    {
        ltlog::warning("Failed to create jack client");
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

bool snd_init(unsigned mixrate, unsigned numsids)
{
    // If user wants to re-initialize, shutdown first

    snd_uninit();

#ifdef USE_JACK
    if (use_jack) {
        snd_init_jack();
        if (use_jack_audio) return true;
    }
#endif

    // Check for illegal config
    if (!mixrate)
    {
        snd_uninit();
        ltlog::warning("Sampling rate not specified");
        return false;
    }

    SDL_AudioSpec spec;
    spec.freq = mixrate;
    spec.format = SDL_AUDIO_S16;
    spec.channels = numsids;

    // Init tempo count

    snd_bpmcount = 0;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, snd_mixer_callback, nullptr);

    if (!stream)
    {
        snd_uninit();
        ltlog::warning("Cannot open sound device", SDL_GetError());
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
        ltlog::warning("Invalid sound format");
        return false;
    }

    int sample_frames;
    if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &spec, &sample_frames))
    {
        snd_uninit();
        ltlog::warning("Cannot get device format", SDL_GetError());
        return false;
    }

    snd_buffersize = sample_frames;
    snd_mixrate = spec.freq;

    // Allocate mixer tables
    if (!snd_initmixer())
    {
        return false;
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
}

void snd_setcustommixer(CustomMixer custommixer)
{
    snd_custommixer = custommixer;
}

void snd_setplayer(Player player)
{
    snd_player = player;
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
    if (!snd_clipbuffer)
    {
        snd_uninit();
        ltlog::error("Out of memory");
        return false;
    }

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
    if (!snd_custommixer)
    {
        std::memset(dest, 0, bytes*sizeof(Uint8));
        return;
    }
    unsigned mixsamples = bytes / snd_framesize;
    unsigned clipsamples = mixsamples;
    if (snd_mixmode & STEREO) clipsamples <<= 1;
    Sint32 *clipptr = snd_clipbuffer;
#ifdef USE_JACK
    if (use_jack_audio) {
        clipsamples = bytes / sizeof(sample_t);
        mixsamples = clipsamples;
    }
#endif

    if (snd_player) // Must the player be called?
    {
        while (mixsamples)
        {
            if (!snd_bpmcount)
            {
                // Call player
                snd_player();
                // Reset tempocounter
                snd_bpmcount = ((snd_mixrate * 5) >> 1) / snd_bpmtempo;
            }

            int musicsamples = mixsamples;
            if (musicsamples > snd_bpmcount) musicsamples = snd_bpmcount;
            snd_bpmcount -= musicsamples;
            snd_custommixer(clipptr, musicsamples);
            if (snd_mixmode & STEREO) clipptr += musicsamples * 2;
            else clipptr += musicsamples;
            mixsamples -= musicsamples;
        }
    }
    else
    {
        snd_custommixer(clipptr, mixsamples);
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

int clip(int sample)
{
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    return sample;
}

#ifdef USE_JACK
static void snd_jack_postprocess(Sint32* src, sample_t* dest, unsigned samples) {
    while (samples--)
    {
        int sample = clip(*src++);
        *dest++ = sample / 32768.0;
    }
}
#endif

static void snd_float_postprocess(Sint32* src, float* dest, unsigned samples) {
    while (samples--)
    {
        int sample = clip(*src++);
        *dest++ = sample / 32768.0f;
    }
}

static void snd_16bit_postprocess(Sint32 *src, Sint16 *dest, unsigned samples)
{
    while (samples--)
    {
        int sample = clip(*src++);
        *dest++ = sample;
    }
}

static void snd_8bit_postprocess(Sint32 *src, Uint8 *dest, unsigned samples)
{
    while (samples--)
    {
        int sample = clip(*src++);
        *dest++ = (sample >> 8) + 128;
    }
}

unsigned getmixrate()
{
    return snd_mixrate;
}

void snd_play()
{
    //SDL_ClearAudioStream(stream);
    SDL_ResumeAudioStreamDevice(stream);
}

void snd_stop()
{
    SDL_PauseAudioStreamDevice(stream);
}
