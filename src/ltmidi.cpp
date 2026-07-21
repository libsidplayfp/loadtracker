/*
 * LoadTracker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"
#include "instr.h"
#include "ltlog.h"
#include "pattern.h"
#include "play.h"

#ifdef USE_MIDI_INPUT

#include <rtmidi/rtmidi_c.h>

RtMidiInPtr midi_device = nullptr;

int current_note_on = -1;

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

#endif

bool midi_init()
{
#ifdef USE_MIDI_INPUT
    RtMidiInPtr midi_device = rtmidi_in_create(RTMIDI_API_UNSPECIFIED, "loadtracker", 100);
    if (!midi_device->ok)
    {
        ltlog::warning("failed to activate midi: %s\n", midi_device->msg);
        return false;
    }

    unsigned int ports = rtmidi_get_port_count(midi_device);
    if (!ports)
    {
        ltlog::warning("No available MIDI ports");
        return false;
    }

    rtmidi_open_port(midi_device, 0, "midi_in");
    if (!midi_device->ok)
    {
        ltlog::warning("failed to open port", midi_device->msg);
        return false;
    }

    rtmidi_in_set_callback(midi_device, snd_midi_process, nullptr);
    if (!midi_device->ok)
    {
        ltlog::warning("failed to set midi callback", midi_device->msg);
        return false;
    }

    return true;
#endif
}

void midi_uninit()
{
#ifdef USE_MIDI_INPUT
    if (midi_device != nullptr)
        rtmidi_in_free(midi_device);
#endif
}
