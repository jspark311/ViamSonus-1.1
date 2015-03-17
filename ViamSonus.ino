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
#include "ManuvrOS/XenoSession/XenoSession.h"
#include <Encoder.h>   // Bleh.... more Arduino brain-damage...

#include <Adafruit_GFX.h>

#define HOST_BAUD_RATE  115200



/****************************************************************************************************
* Entry-point for teensy3...                                                                        *
****************************************************************************************************/

IntervalTimer timer0;               // Scheduler
StaticHub* sh;

Scheduler*    scheduler     = NULL;
EventManager* event_manager = NULL;

XenoSession *sess = NULL;

uint8_t analog_write_val = 0;
int8_t direction = 1;   // Zero for hold, 1 for brighten, -1 for darken.

void logo_fade() {
  if (direction < 0) analog_write_val--;
  else if (direction > 0) analog_write_val++;
  else return;
  
  if (0 == analog_write_val) {
    direction = direction * -1;
  }
  else if (255 == analog_write_val) {
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

  //analogReadRes(BEST_ADC_PRECISION);  // All ADC channels shall be 10-bit.
  analogReadAveraging(32);            // And maximally-smoothed by the hardware (32).

  sh = StaticHub::getInstance();
  
  scheduler     = sh->fetchScheduler();
  event_manager = sh->fetchEventManager();
  
  scheduler->createSchedule(40,  -1, false, logo_fade);

  timer0.begin(timerCallbackScheduler, 1000);   // Turn on the periodic interrupts...
  
  sess = new XenoSession();
  sess->markSessionConnected(true);
  
  sei();      // Enable interrupts and let the madness begin.
}





void loop() {
  unsigned char* ser_buffer = (unsigned char*) alloca(255);
  int bytes_read = 0;
  //StaticHub::watchdog_mark = 42;  // The period (in ms) of our clock punch. 
  

  while (1) {   // Service this loop for-ev-ar
    while (Serial.available() < 1) {
      event_manager->procIdleFlags();
      scheduler->serviceScheduledEvents();
    }

    // Zero the buffer.
    bytes_read = 0;
    for (int i = 0; i < 255; i++) *(ser_buffer+i) = 0;
    
    while (Serial.available()) {
      *(ser_buffer+bytes_read++) = Serial.read();
    }
    
//    sh->feedUSBBuffer((uint8_t*) &c, 1, (c == '\r' || c == '\n'));
    sess->bin_stream_rx(ser_buffer, bytes_read);
  }
}


