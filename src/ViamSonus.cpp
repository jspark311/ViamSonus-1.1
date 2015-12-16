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

#include "FirmwareDefs.h"
#include <ManuvrOS/Platform/Platform.h>
#include <ManuvrOS/Kernel.h>
#include <DataStructures/StringBuilder.h>

#include <ManuvrOS/Drivers/ManuvrableNeoPixel/ManuvrableNeoPixel.h>
#include <ManuvrOS/Drivers/AudioRouter/AudioRouter.h>
#include <ManuvrOS/Drivers/LightSensor/LightSensor.h>
#include <ManuvrOS/Drivers/ADCScanner/ADCScanner.h>
#include "Encoder/Encoder.h"

//#include <Audio/utility/dspinst.h>
#include <Audio/Audio.h>

#define HOST_BAUD_RATE  115200
#define NEOPIXEL_PIN  11

Kernel* kernel = NULL;
Encoder* encoder_stack = NULL;;
    AudioRouter *audio_router = NULL;
    ManuvrableNeoPixel* strip = NULL;
    LightSensor* light_sensor = NULL;
    ADCScanner*  adc_scanner  = NULL;


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

int last_touch_read[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long last_encoder_read = 0;


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


void scan_buttons() {
  int current_touch_read[12] = {
    touchRead(0),
    touchRead(1),
    touchRead(15),
    touchRead(16),
    touchRead(17),
    touchRead(18),
    touchRead(19),
    touchRead(22),
    touchRead(23),
    touchRead(25),
    touchRead(32),
    touchRead(33)
  };
  
  long current_encoder_read = encoder_stack->read();
  if (current_encoder_read) {
    last_encoder_read += current_encoder_read;
    ManuvrEvent* event = EventManager::returnEvent((current_encoder_read > 0) ? VIAM_SONUS_MSG_ENCODER_UP : VIAM_SONUS_MSG_ENCODER_DOWN);
    EventManager::staticRaiseEvent(event);
    StringBuilder local_log;
    local_log.concatf("Encoder changed. %lu\n", current_encoder_read);
    StaticHub::log(&local_log);
  }
  
  for (int i = 0; i < 12; i++) {
    // Real cheesy. Going to look for a doubling until I write calibration code.
    if (last_touch_read[i] == 0) {
      // First run. Do nothing.
    }
    else if (last_touch_read[i]*2 < current_touch_read[i]) {
      // BIG rise
      ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_USER_BUTTON_PRESS);
      event->addArg((uint8_t) i);
      EventManager::staticRaiseEvent(event);
      StringBuilder local_log;
      local_log.concatf("Button %d pressed.\n", i);
      StaticHub::log(&local_log);
    }
    else if (current_touch_read[i]*2 < last_touch_read[i]) {
      // BIG fall
      ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_USER_BUTTON_RELEASE);
      event->addArg((uint8_t) i);
      EventManager::staticRaiseEvent(event);
      StringBuilder local_log;
      local_log.concatf("Button %d released.\n", i);
      StaticHub::log(&local_log);
    }
    else {
      // Steady-state. Nominal case.
    }
    last_touch_read[i] = current_touch_read[i];
  }
}




void setup() {
  Serial.begin(HOST_BAUD_RATE);   // USB
  pinMode(13, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(4,  OUTPUT);
  
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  
  pinMode(14, INPUT);

  Kernel __kernel;  // Instance the kernel.
  kernel = &__kernel;

  AudioMemory(2);
  
  //analogReadRes(BEST_ADC_PRECISION);  // All ADC channels shall be 10-bit.
  analogReadAveraging(32);            // And maximally-smoothed by the hardware (32).
  analogWriteResolution(12);   // Setup the DAC.

  // Setup the first i2c adapter and Subscribe it to Kernel.
  I2CAdapter i2c(1);

  __kernel.subscribe((EventReceiver*) &i2c);

  const uint8_t SWITCH_ADDR = 0x76;
  const uint8_t POT_0_ADDR  = 0x50;
  const uint8_t POT_1_ADDR  = 0x51;
  audio_router = new AudioRouter((I2CAdapter*) i2c, SWITCH_ADDR, POT_0_ADDR, POT_1_ADDR); 

  encoder_stack = new Encoder(2, 3);
  strip = new ManuvrableNeoPixel(80, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
  light_sensor = new LightSensor();
  adc_scanner = new ADCScanner();
  adc_scanner->addADCPin(A10);
  adc_scanner->addADCPin(A11);
  adc_scanner->addADCPin(A12);
  adc_scanner->addADCPin(A13);
  adc_scanner->addADCPin(A15);
  adc_scanner->addADCPin(A16);
  adc_scanner->addADCPin(A17);
  adc_scanner->addADCPin(A6);
  adc_scanner->addADCPin(A7);
  adc_scanner->addADCPin(A20);

  __kernel.subscribe((EventReceiver*) adc_scanner);
  __kernel.subscribe((EventReceiver*) audio_router);
  __kernel.subscribe((EventReceiver*) strip);
  __kernel.subscribe((EventReceiver*) light_sensor);


  __kernel.createSchedule(40, -1, false, logo_fade);
  __kernel.createSchedule(100,  -1, false, scan_buttons);
  
  __kernel.bootstrap();
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

