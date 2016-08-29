/*
 * Arduinoboy
 * http://trash80.com
 * Copyright (c) 2016 Timothy Lamb
 *
 * This file is part of Arduinoboy.
 *
 * Arduinoboy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arduinoboy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

 #include "LSDJMidiout.h"

void LSDJMidioutClass::begin()
{
    gameboy->setOutputMode();
    sequencerStarted = false;
    for(int i=0;i<NUM_MIDIOUT;i++) lastNote[i] = -1;
}

void LSDJMidioutClass::update()
{

    int data = gameboy->receiveByteClocked();
    if(data > 0x6f) {
        switch(data){
            case 0x7F:
                //clock tick
                callback->sendTransportClock();
            break;
            case 0x7E:
                //stop
                allNotesOff();
                callback->sendTransportStop();
            break;
            case 0x7D:
                //start
                callback->sendTransportStart();
            break;
            default:
                command = data - 0x70;
        }
    } else if(data != -1 && command != -1) {
        // data contains a value to be performed on the command
        if(command < 4) {
            //Note message
            noteMessage(command, data);
        } else if (command < 8) {
            //Control change message
            command-=4;
            controlChangeMessage(command, data);
        } else if(command< 0x0C) {
            //Program change message
            command -=8;
            programChangeMessage(command, data);
        }
        command = -1;
    }
}

void LSDJMidioutClass::noteMessage(uint8_t chan, uint8_t data)
{
    if(data) {
        if(lastNote[chan] >= 0) {
            callback->sendNoteOff(channel[chan], lastNote[chan], 0x40);
        }
        callback->sendNoteOn(channel[chan], data, 0x7F);
        lastNote[chan] = data;
    } else if (lastNote[chan]>=0) {
        callback->sendNoteOff(channel[chan], lastNote[chan], 0x40);
        lastNote[chan] = -1;
    }
}

void LSDJMidioutClass::allNotesOff()
{
    for(uint8_t chan = 0; chan < 4; chan++) {
        if(lastNote[chan] >= 0) {
            callback->sendNoteOff(channel[chan], lastNote[chan], 0x40);
            lastNote[chan] = -1;
        }
        callback->sendControlChange(channel[chan], 123, 0);
    }
}

void LSDJMidioutClass::controlChangeMessage(uint8_t chan, uint8_t data)
{
    uint8_t value = data;

    if(ccMode[chan]) {
        if(ccScaling[chan]) {
            value = (value & 0x0F)*8;
        }
        data = (data>>4) & 0x07;
        callback->sendControlChange(channel[chan], cc[chan][data], value);
    } else {
        if(ccScaling[chan]) {
            value = (uint8_t) ((((float)data) / 0x6f) * 0x7f);
        }
        callback->sendControlChange(channel[chan], cc[chan][0], value);
    }
}

void LSDJMidioutClass::programChangeMessage(uint8_t chan, uint8_t data)
{
    callback->sendProgramChange(channel[chan], data);
}