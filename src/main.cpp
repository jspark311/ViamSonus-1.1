/*
File:   main.cpp
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

     __     ___                 ____
     \ \   / (_) __ _ _ __ ___ / ___|  ___  _ __  _   _ ___
      \ \ / /| |/ _` | '_ ` _ \\___ \ / _ \| '_ \| | | / __|
       \ V / | | (_| | | | | | |___) | (_) | | | | |_| \__ \
        \_/  |_|\__,_|_| |_| |_|____/ \___/|_| |_|\__,_|___/

Supported build targets: Teensy3 and Raspi.
*/

#include <DataStructures/StringBuilder.h>
#include <Platform/Platform.h>

#include <Drivers/ManuvrableNeoPixel/ManuvrableNeoPixel.h>
#include <Drivers/AudioRouter/AudioRouter.h>
#include <Drivers/LightSensor/LightSensor.h>
#include <Drivers/ADCScanner/ADCScanner.h>
#include <Transports/ManuvrSerial/ManuvrSerial.h>
#include <XenoSession/Console/ManuvrConsole.h>

//#include <Audio/utility/dspinst.h>
#include "arm_math.h"

#include <Audio/Audio.h>
#include "Encoder/Encoder.h"

#define HOST_BAUD_RATE  115200
#define NEOPIXEL_PIN  11



// These are only here until they are migrated to each receiver that deals with them.
const MessageTypeDef message_defs_viam_sonus[] = {
  {  VIAM_SONUS_MSG_ENCODER_UP       , 0x0000,  "ENCODER_UP",    ManuvrMsg::MSG_ARGS_NONE },   // The encoder on the front panel was incremented.
  {  VIAM_SONUS_MSG_ENCODER_DOWN     , 0x0000,  "ENCODER_DOWN",  ManuvrMsg::MSG_ARGS_NONE },   // The encoder on the front panel was decremented.

  /* ViamSonus has neopixels and a light-level sensor. */
  {  MANUVR_MSG_NEOPIXEL_REFRESH     , 0x0000,  "NEOPIXEL_REFRESH",  ManuvrMsg::MSG_ARGS_NONE }, // Cause any neopixel classes to refresh their strands.
  {  MANUVR_MSG_DIRTY_FRAME_BUF      , 0x0000,  "DIRTY_FRAME_BUF",   ManuvrMsg::MSG_ARGS_NONE }, // Something changed the framebuffer and we need to redraw.

  /* ViamSonus has hardware buttons... */
  {  MANUVR_MSG_USER_BUTTON_PRESS    , MSG_FLAG_EXPORTABLE,  "USER_BUTTON_PRESS",    ManuvrMsg::MSG_ARGS_NONE },   // The user pushed a button with the given integer code.
  {  MANUVR_MSG_USER_BUTTON_RELEASE  , MSG_FLAG_EXPORTABLE,  "USER_BUTTON_RELEASE",  ManuvrMsg::MSG_ARGS_NONE },   // The user released a button with the given integer code.
};


StringBuilder local_log;

Kernel*             kernel        = NULL;
Encoder*            encoder_stack = NULL;
AudioRouter*        audio_router  = NULL;
ManuvrableNeoPixel* strip         = NULL;
LightSensor*        light_sensor  = NULL;
ADCScanner*         adc_scanner   = NULL;


/****************************************************************************************************
* Audio test code.                                                                                  *
****************************************************************************************************/
//AudioSynthToneSweep myEffect;
//AudioOutputAnalog   audioOutput;
//
//// The tone sweep goes to left and right channels
//AudioConnection c1(myEffect, 0, audioOutput, 0);
//AudioConnection c2(myEffect, 0, audioOutput, 1);
//
//float t_ampx = 0.8;
//int t_lox = 10;
//int t_hix = 22000;
//// Length of time for the sweep in seconds
//float t_timex = 10;




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
    ManuvrMsg* event = Kernel::returnEvent((current_encoder_read > 0) ? VIAM_SONUS_MSG_ENCODER_UP : VIAM_SONUS_MSG_ENCODER_DOWN);
    Kernel::staticRaiseEvent(event);

    local_log.concatf("Encoder changed. %lu\n", current_encoder_read);
    Kernel::log(&local_log);
  }

  for (int i = 0; i < 12; i++) {
    // Real cheesy. Going to look for a doubling until I write calibration code.
    if (last_touch_read[i] == 0) {
      // First run. Do nothing.
    }
    else if (last_touch_read[i]*2 < current_touch_read[i]) {
      // BIG rise
      ManuvrMsg* event = Kernel::returnEvent(MANUVR_MSG_USER_BUTTON_PRESS);
      event->addArg((uint8_t) i);
      Kernel::staticRaiseEvent(event);

      local_log.concatf("Button %d pressed.\n", i);
      Kernel::log(&local_log);
    }
    else if (current_touch_read[i]*2 < last_touch_read[i]) {
      // BIG fall
      ManuvrMsg* event = Kernel::returnEvent(MANUVR_MSG_USER_BUTTON_RELEASE);
      event->addArg((uint8_t) i);
      Kernel::staticRaiseEvent(event);

      local_log.concatf("Button %d released.\n", i);
      Kernel::log(&local_log);
    }
    else {
      // Steady-state. Nominal case.
    }
    last_touch_read[i] = current_touch_read[i];
  }
}


void setup() {
  // One of the first things we need to do is populate the EventManager with all of the
  // message codes that come with this firmware.
  int mes_count = sizeof(message_defs_viam_sonus) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(message_defs_viam_sonus, mes_count);

  platform.platformPreInit();
  kernel = platform.kernel();

  //analogReadRes(BEST_ADC_PRECISION);  // All ADC channels shall be 10-bit.
  analogReadAveraging(32);            // And maximally-smoothed by the hardware (32).
  analogWriteResolution(12);   // Setup the DAC.

  gpioDefine(13, OUTPUT);
  gpioDefine(11, OUTPUT);
  gpioDefine(4,  OUTPUT);
  gpioDefine(2,  INPUT_PULLUP);
  gpioDefine(3,  INPUT_PULLUP);
  gpioDefine(14, INPUT);
}


void loop() {
  //AudioMemory(2);

  // Setup the first i2c adapter and Subscribe it to Kernel.
  I2CAdapter i2c(1, 30, 29);

  kernel->subscribe((EventReceiver*) &i2c);

  const uint8_t SWITCH_ADDR = 0x76;
  const uint8_t POT_0_ADDR  = 0x50;
  const uint8_t POT_1_ADDR  = 0x51;
  audio_router = new AudioRouter((I2CAdapter*) &i2c, SWITCH_ADDR, POT_0_ADDR, POT_1_ADDR);

  encoder_stack = new Encoder(2, 3);
  strip = new ManuvrableNeoPixel(80, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
  light_sensor = new LightSensor(A0);
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

  kernel->subscribe((EventReceiver*) adc_scanner);
  kernel->subscribe((EventReceiver*) audio_router);
  kernel->subscribe((EventReceiver*) strip);
  kernel->subscribe((EventReceiver*) light_sensor);

  platform.bootstrap();

  kernel->createSchedule(40, -1, false, logo_fade)->enableSchedule(true);
  kernel->createSchedule(100,  -1, false, scan_buttons)->enableSchedule(true);

  ManuvrSerial  _console_xport("U", HOST_BAUD_RATE);  // Indicate USB.
  kernel->subscribe((EventReceiver*) &_console_xport);
  ManuvrConsole _console((BufferPipe*) &_console_xport);
  kernel->subscribe((EventReceiver*) &_console);

  while (1) {
    kernel->procIdleFlags();
  }
}


#if defined(__MANUVR_LINUX)
  // For linux builds, we provide a shunt into the loop function.
  int main(int argc, char *argv[]) {
    setup();
    loop();
  }
#endif
