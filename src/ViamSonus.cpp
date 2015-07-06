/*
File:   ViamSonus.ino
Author: J. Ian Lindsay
Date:   2014.05.19

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


*/

#include "StaticHub/StaticHub.h"
#include "Drivers/AudioRouter/AudioRouter.h"
//#include "ManuvrOS/XenoSession/XenoSession.h"

#include <Audio/Audio.h>

//#include <Adafruit_GFX.h>

#define HOST_BAUD_RATE  115200




/****************************************************************************************************
* Audio test code.                                                                                  *
****************************************************************************************************/
AudioSynthToneSweep myEffect;
AudioOutputAnalog   audioOutput; 

// The tone sweep goes to left and right channels
AudioConnection c1(myEffect, 0, audioOutput, 0);
AudioConnection c2(myEffect, 0, audioOutput, 1);


float t_ampx = 0.8;
int t_lox = 10;
int t_hix = 22000;
// Length of time for the sweep in seconds
float t_timex = 10;




/****************************************************************************************************
* Entry-point for teensy3...                                                                        *
****************************************************************************************************/

IntervalTimer timer0;               // Scheduler
StaticHub*    sh            = NULL;
Scheduler*    scheduler     = NULL;
EventManager* event_manager = NULL;
//XenoSession *sess = NULL;





uint8_t analog_write_val = 0;
int8_t direction = 1;   // Zero for hold, 1 for brighten, -1 for darken.

void logo_fade() {
  if (direction < 0) analog_write_val--;
  else if (direction > 0) analog_write_val++;
  else return;
  
  if (0 == analog_write_val) {
    direction = direction * -1;
  }
  else if (200 == analog_write_val) {
    direction = direction * -1;
  }
  analogWrite(4, analog_write_val);
}


void timerCallbackScheduler() {  if (scheduler) scheduler->advanceScheduler(); }



void setup() {
  Serial.begin(HOST_BAUD_RATE);                           // Setup host communication.
  pinMode(13, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(4,  OUTPUT);
  
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  
  pinMode(14, INPUT);

  AudioMemory(2);
  
  //analogReadRes(BEST_ADC_PRECISION);  // All ADC channels shall be 10-bit.
  analogReadAveraging(32);            // And maximally-smoothed by the hardware (32).
  analogWriteResolution(12);   // Setup the DAC.

  sh = StaticHub::getInstance();
  sh->bootstrap();

  scheduler     = sh->fetchScheduler();
  event_manager = sh->fetchEventManager();
  
  scheduler->createSchedule(40,  -1, false, logo_fade);

  timer0.begin(timerCallbackScheduler, 1000);   // Turn on the periodic interrupts...
  
//  sess = new XenoSession();
//  sess->markSessionConnected(true);
  sei();
}





void loop() {
  unsigned char* ser_buffer = (unsigned char*) alloca(255);
  int bytes_read = 0;

  while (1) {   // Service this loop for-ev-ar
    while (Serial.available() < 1) {
      event_manager->procIdleFlags();
      scheduler->serviceScheduledEvents();
	  
      if(! myEffect.isPlaying()) {
        myEffect.play(t_ampx,t_hix,t_lox,t_timex);
        int temp_x = t_hix;
        t_hix = t_lox;
        t_lox = temp_x;
      }
    }

    // Zero the buffer.
    bytes_read = 0;
    for (int i = 0; i < 255; i++) *(ser_buffer+i) = 0;
    char c;
    while (Serial.available()) {
      c = Serial.read();
      *(ser_buffer+bytes_read++) = c;
    }

    sh->feedUSBBuffer(ser_buffer, bytes_read, (c == '\r' || c == '\n'));
//    sess->bin_stream_rx(ser_buffer, bytes_read);
  }
}

