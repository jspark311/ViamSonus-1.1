/*
File:   StaticHub.cpp
Author: J. Ian Lindsay
Date:   2014.07.01


Copyright (C) 2014 J. Ian Lindsay
All rights reserved.

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
#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"
#include <ManuvrOS/EventManager.h>
#include <ManuvrOS/XenoSession/XenoSession.h>

#include <sys/time.h>
#include <unistd.h>

#include "ManuvrOS/Drivers/ManuvrableNeoPixel/ManuvrableNeoPixel.h"
#include "Drivers/AudioRouter/AudioRouter.h"
#include "Drivers/LightSensor/LightSensor.h"
#include "Drivers/ADCScanner/ADCScanner.h"


#include "Encoder/Encoder.h"


// Externs and prototypes...
// I really don't like doing this, but haven't found a better way to get good ISR behavior.
extern volatile I2CAdapter* i2c;

uint32_t profiler_mark_0 = 0;
uint32_t profiler_mark_1 = 0;

#define NEOPIXEL_PIN  11

Encoder* encoder_stack = NULL;;


extern XenoSession *sess;


  
/****************************************************************************************************
* Scheduler callbacks. Please note that these are NOT part of StaticHub.                            *
****************************************************************************************************/

/*
* Scheduler callback
* This gets called periodically to moderate traffic volume across the USB link
*   until I figure out how to sink a hook into the DMA interrupt to take its place.
*/
void stdout_funnel() {
  if (StaticHub::log_buffer.count()) {
    if (!StaticHub::getInstance()->getVerbosity()) {
      StaticHub::log_buffer.clear();
    }
    else {
      #ifdef ARDUINO
      if (!StaticHub::mute_logger) {
        Serial.print((char*) StaticHub::log_buffer.string());
      }
      #else
      if (!StaticHub::mute_logger) printf(StaticHub::log_buffer.position(0));
      #endif
      StaticHub::log_buffer.drop_position(0);
    }
  }
  else {
    StaticHub::getInstance()->disableLogCallback();
  }
}



/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/
volatile StaticHub* StaticHub::INSTANCE = NULL;
volatile uint32_t StaticHub::next_random_int[STATICHUB_RNG_CARRY_CAPACITY];
volatile uint32_t StaticHub::millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  StaticHub::watchdog_mark = 42;

bool StaticHub::mute_logger = false;


// These are only here until they are migrated to each receiver that deals with them.
const MessageTypeDef message_defs[] = {
  {  VIAM_SONUS_MSG_ENABLE_ROUTING        , 0x0000,               "ENABLE_ROUTING",        ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_DISABLE_ROUTING       , 0x0000,               "DISABLE_ROUTING",       ManuvrMsg::MSG_ARGS_NONE }, // 
  {  VIAM_SONUS_MSG_NAME_INPUT_CHAN       , 0x0000,               "NAME_INPUT_CHAN",       ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_NAME_OUTPUT_CHAN      , 0x0000,               "NAME_OUTPUT_CHAN",      ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_DUMP_ROUTER           , 0x0000,               "DUMP_ROUTER",           ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_OUTPUT_CHAN_VOL       , 0x0000,               "OUTPUT_CHAN_VOL",       ManuvrMsg::MSG_ARGS_U8_U8 }, // Either takes a global volume, or a volume and a specific channel.
  {  VIAM_SONUS_MSG_UNROUTE               , 0x0000,               "UNROUTE",               ManuvrMsg::MSG_ARGS_U8   }, // Unroutes the given channel, or all channels.
  {  VIAM_SONUS_MSG_ROUTE                 , 0x0000,               "ROUTE",                 ManuvrMsg::MSG_ARGS_U8_U8 }, // Routes the input to the output.
  {  VIAM_SONUS_MSG_PRESERVE_ROUTES       , 0x0000,               "PRESERVE_ROUTES",       ManuvrMsg::MSG_ARGS_NONE }, //
  {  VIAM_SONUS_MSG_GROUP_CHANNELS        , 0x0000,               "GROUP_CHANNELS",        ManuvrMsg::MSG_ARGS_U8_U8 }, // Pass two output channels to group them (stereo).
  {  VIAM_SONUS_MSG_UNGROUP_CHANNELS      , 0x0000,               "UNGROUP_CHANNELS",      ManuvrMsg::MSG_ARGS_NONE }, // Pass a group ID to free the channels it contains, or no args to ungroup everything.
  {  VIAM_SONUS_MSG_ENCODER_UP            , 0x0000,               "ENCODER_UP",            ManuvrMsg::MSG_ARGS_U8 },   // The encoder on the front panel was incremented.
  {  VIAM_SONUS_MSG_ENCODER_DOWN          , 0x0000,               "ENCODER_DOWN",          ManuvrMsg::MSG_ARGS_U8 },   // The encoder on the front panel was decremented.
  {  VIAM_SONUS_MSG_NEOPIXEL_REFRESH      , MSG_FLAG_IDEMPOTENT,  "NEOPIXEL_REFRESH",      ManuvrMsg::MSG_ARGS_NONE }, // Something wrote to the neopixel "frame buffer".
  {  VIAM_SONUS_MSG_AMBIENT_LIGHT_LEVEL   , MSG_FLAG_IDEMPOTENT,  "AMBIENT_LIGHT_LEVEL",   ManuvrMsg::MSG_ARGS_U16 }, // Something wrote to the neopixel "frame buffer".
  {  VIAM_SONUS_MSG_ADC_SCAN              , MSG_FLAG_IDEMPOTENT,  "ADC_SCAN",              ManuvrMsg::MSG_ARGS_NONE }, // It is time to scan the ADC channels.
};



/****************************************************************************************************
* Functions that convert from #define codes to something readable by a human...                     *
****************************************************************************************************/
const char* StaticHub::getRTCStateString(uint32_t code) {
  switch (code) {
    case RTC_STARTUP_UNKNOWN:     return "RTC_STARTUP_UNKNOWN";
    case RTC_OSC_FAILURE:         return "RTC_OSC_FAILURE";
    case RTC_STARTUP_GOOD_UNSET:  return "RTC_STARTUP_GOOD_UNSET";
    case RTC_STARTUP_GOOD_SET:    return "RTC_STARTUP_GOOD_SET";
    default:                      return "RTC_STARTUP_UNDEFINED";
  }
}



/****************************************************************************************************
* Logging members...                                                                                *
****************************************************************************************************/
StringBuilder StaticHub::log_buffer;

/*
* Logger pass-through functions. Please mind the variadics...
*/
volatile void StaticHub::log(int severity, const char *str) {
  if (!INSTANCE->verbosity) return;
  if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  log_buffer.concat(str);
}

volatile void StaticHub::log(const char *str) {
  if (!INSTANCE->verbosity) return;
  if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  log_buffer.concat(str);
}

volatile void StaticHub::log(const char *fxn_name, int severity, const char *str, ...) {
  if (!INSTANCE->verbosity) return;
  //log_buffer.concat("%d  %s:\t", severity, fxn_name);
  if (log_buffer.count() == 0) {
    if (NULL != INSTANCE) {
       if (INSTANCE->pid_log_moderator) {
         ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
       }
     }
  }
  log_buffer.concatf("%d  %s:\t", severity, fxn_name);
  va_list marker;
  
  va_start(marker, str);
  log_buffer.concatf(str, marker);
  va_end(marker);
}

volatile void StaticHub::log(StringBuilder *str) {
  if (!INSTANCE->verbosity) return;
  if (log_buffer.count() == 0) ((StaticHub*) INSTANCE)->__scheduler.enableSchedule(INSTANCE->pid_log_moderator);
  log_buffer.concatHandoff(str);
}


void StaticHub::disableLogCallback(){
	__scheduler.disableSchedule(pid_log_moderator);
}



/****************************************************************************************************
* Various small utility functions...                                                                *
****************************************************************************************************/

/**
* Sometimes we question the size of the stack.
*
* @return the stack pointer at call time.
*/
volatile uint32_t StaticHub::getStackPointer(void) {
  uint32_t test;
  test = (uint32_t) &test;  // Store the pointer.
  return test;
}


/**
* Called by the RNG ISR to provide new random numbers to StaticHub.
*
* @param    nu_rnd The supplied random number.
* @return   True if the RNG should continue supplying us, false if it should take a break until we need more.
*/
volatile bool StaticHub::provide_random_int(uint32_t nu_rnd) {
  for (uint8_t i = 0; i < STATICHUB_RNG_CARRY_CAPACITY; i++) {
    if (next_random_int[i] == 0) {
      next_random_int[i] = nu_rnd;
      return (i == STATICHUB_RNG_CARRY_CAPACITY-1) ? false : true;
    }
  }
  return false;
}


/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (next_random_int != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t StaticHub::randomInt(void) {
	uint8_t i = 0;
	uint8_t t = 0;
	while (next_random_int[i] == 0) {
		// Spin inside this array until we get a random number (anything nonzero).
		i = (i+1) % STATICHUB_RNG_CARRY_CAPACITY;
		t++;
		if (t > 100) {
		  EventManager::raiseEvent(MANUVR_MSG_RNG_BUFFER_EMPTY, NULL);
			return 0;
		}
	}
	uint32_t return_value = next_random_int[i];
	next_random_int[i] = 0;
	
	// We should enable the RNG again to prevent us from exhausting our cache.
	#ifdef STM32F4XX
    RNG_ITConfig(ENABLE);
    RNG_Cmd(ENABLE);
  #endif

	return return_value;
}


/*
* Given an RFC2822 datetime string, decompose it and set the time and date.
* We would prefer RFC2822, but we should try and cope with things like missing
*   time or timezone.
* Returns false if the date failed to set. True if it did.
*/
bool StaticHub::setTimeAndDate(char* nu_date_time) {
  return false;
}


/*
* Returns an integer representing the current datetime.
*/
uint32_t StaticHub::currentTimestamp(void) {
	uint32_t return_value = 0;
	return return_value;
}

/*
* Same, but writes a string representation to the argument.
*/
void StaticHub::currentTimestamp(StringBuilder* target) {
}

/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void StaticHub::currentDateTime(StringBuilder* target) {
	if (target != NULL) {
	  #ifdef STM32F4XX
		  RTC_TimeTypeDef RTC_TimeStructure;
		  RTC_DateTypeDef RTC_DateStructure;
		  RTC_GetTimeStamp(RTC_Format_BIN, &RTC_TimeStructure, &RTC_DateStructure);
		  
		  TM_RTC_Time_t current_datetime;
		  TM_RTC_GetDateTime(&current_datetime, TM_RTC_Format_BIN);
		  target->concatf("%02d.%02d.%04d %02d:%02d:%02d  Unix: %u\n",
                  current_datetime.date,
                  current_datetime.month,
                  current_datetime.year + 2000,
                  current_datetime.hours,
                  current_datetime.minutes,
                  current_datetime.seconds,
                  current_datetime.unix);
		  
		  target->concatf("%d-%d-%dT", RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date);
		  target->concatf("%d:%d:%d+00:00", RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
		#else
		  target->concat("<UNSUPPORTED PLATFORM>\n");
		#endif
	}
}





/****************************************************************************************************
* Functions that deal with clock and mode switching...                                              *
****************************************************************************************************/

/*
* Call this with a boolean to enable or disable maskable interrupts globally.
* NOTE: This includes USB and SysTick. So no host communication, and no scheduler.
*       Events ought to still work, however.
*/
void StaticHub::maskable_interrupts(bool enable) {
	if (enable) {
		StaticHub::log("All interrupts enabled\n");
		asm volatile ("cpsie i");
	}
	else {
		StaticHub::log("All interrupts masked\n");
		asm volatile ("cpsid i");
		EventManager::raiseEvent(MANUVR_MSG_INTERRUPTS_MASKED, NULL);
	}
}


/*
* From this fxn will grow the IRQ aspect of our mode-switching (power use).
*   It would be prefferable to contain this machinery within the classes that
*   each block pertains to, but for debugging sake, they are centrallized here
*   for the moment.
* The problem is related to hard faults when the peripherals are allowed to bring
*   their own interrupts online. Solve that before trying to make this pretty.
*/
void StaticHub::off_class_interrupts(bool enable) {
}


volatile void jumpToBootloader(void) {
  _reboot_Teensyduino_();
}


volatile void reboot(void) {
}



/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
 ▀▀▀▀█░█▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀  ▀▀▀▀█░█▀▀▀▀ 
     ▐░▌     ▐░▌▐░▌    ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌ ▐░▌   ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌  ▐░▌  ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌   ▐░▌ ▐░▌     ▐░▌          ▐░▌     
     ▐░▌     ▐░▌    ▐░▌▐░▌     ▐░▌          ▐░▌     
 ▄▄▄▄█░█▄▄▄▄ ▐░▌     ▐░▐░▌ ▄▄▄▄█░█▄▄▄▄      ▐░▌     
▐░░░░░░░░░░░▌▐░▌      ▐░░▌▐░░░░░░░░░░░▌     ▐░▌     
 ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀▀       ▀     

StaticHub bootstrap and setup fxns. This code is only ever called to initiallize this or that thing,
  and then becomes inert.
****************************************************************************************************/

int last_touch_read[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long last_encoder_read = 0;

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


/*
* When the system comes to life, we will want to setup some periodic schedules.
* Even for schedules that are meant to be one-shots, we will try to avoid a malloc()/free()
*   cycle if it's going to be a regular event. Just pre-build some schedules that we know
*   we will need and let them sit dormant until needed.
*/
void StaticHub::initSchedules(void) {
  pid_profiler_report = __scheduler.createSchedule(10000,  -1, false, &__scheduler, new ManuvrEvent(MANUVR_MSG_SCHED_DUMP_META));
  __scheduler.disableSchedule(pid_profiler_report);

  // This schedule marches the data into the USB VCP at a throttled rate.
  pid_log_moderator = __scheduler.createSchedule(2,  -1, false, stdout_funnel);
  __scheduler.delaySchedule(pid_log_moderator, 1000);
  
  
  __scheduler.createSchedule(100,  -1, false, scan_buttons);
  // Automatically start things running after 3 seconds.
}


/*
* Init the RNG. Short and sweet.
*/
void StaticHub::init_RNG(void) volatile {
  // Zero the random number cache.
  for (uint8_t i = 0; i < STATICHUB_RNG_CARRY_CAPACITY; i++) next_random_int[i] = 0;
  
  #ifdef ARDUINO
    randomSeed(analogRead(0));
  #endif
}


/*
* Setup the realtime clock module.
* Informed by code here:
* http://autoquad.googlecode.com/svn/trunk/onboard/rtc.c
*/
void StaticHub::initRTC(void) volatile {
}



/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup 
*   in the class that deals with them. An example of this would be the CPLD pins (GPIO
*   case) or the i2c pins (AF case).
* Pending peripheral-level init of pins, we should just enable everything and let 
*   individual classes work out their own requirements.
* Since we are using a 100-pin QFP part, we don't have ports higher than E.
*/
void StaticHub::gpioSetup() volatile {
}



/*
* The last step in our bootstrap is to call this function. By this point, all peripherals and GPIO
*   should have been claimed and setup. Now we need to conf the interrupts we want.
*/
void StaticHub::nvicConf() volatile {
}

  
ManuvrableNeoPixel* strip = NULL;
LightSensor* light_sensor = NULL;
ADCScanner*  adc_scanner  = NULL;

/*
* The primary init function. Calling this will bring the entire system online if everything is
*   working nominally.
*/
int8_t StaticHub::bootstrap() {
  log_buffer.concatf("\n\n%s v%s    Build date: %s %s\nBootstrap beginning...\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);
  
  event_manager.subscribe((EventReceiver*) &__scheduler);    // Subscribe the Scheduler.
  event_manager.subscribe((EventReceiver*) this);            // Subscribe StaticHub as top priority in EventManager.
  
  /*
  If we are going to use 50MHz GPIO rates, we should probably also turn on the I/O
  compensation cell to reduce noise.
  */
  this->gpioSetup();                                 // Setup the GPIO ports
  this->initRTC();                                   // Instantiate timebase

  // Setup the first i2c adapter and Subscribe it to EventManager.
  i2c = new I2CAdapter(1);
  event_manager.subscribe((EventReceiver*) i2c);

  const uint8_t SWITCH_ADDR = 0x76;
  const uint8_t POT_0_ADDR  = 0x50;
  const uint8_t POT_1_ADDR  = 0x51;
  audio_router = new AudioRouter((I2CAdapter*) i2c, SWITCH_ADDR, POT_0_ADDR, POT_1_ADDR); 

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
  
  event_manager.subscribe((EventReceiver*) strip);
  event_manager.subscribe((EventReceiver*) audio_router);
  event_manager.subscribe((EventReceiver*) light_sensor);
  event_manager.subscribe((EventReceiver*) adc_scanner);

  
  initSchedules();   // We know we will need some schedules...

  off_class_interrupts(true);  // Now configure interrupts, lift interrupt masks, and let the madness begin.
  
  ManuvrEvent *boot_completed_ev = EventManager::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  boot_completed_ev->priority = EVENT_PRIORITY_HIGHEST;
  raiseEvent(boot_completed_ev);
  return 0;
}


/*
* All external access to StaticHub's non-static members should obtain it's reference via this fxn...
*   Note that services that are dependant on SH during the bootstrap phase should have an SH reference
*   passed into their constructors, rather than forcing them to call this and risking an infinite 
*   recursion.
*/
StaticHub* StaticHub::getInstance() {
	if (StaticHub::INSTANCE == NULL) {
	  // TODO: remove last traces of this pattern.
	  // This should never happen, but if it does, it will crash us for sure.
		StaticHub::INSTANCE = new StaticHub();
		((StaticHub*) StaticHub::INSTANCE)->bootstrap();
	}
	// And that is how the singleton do...
	return (StaticHub*) StaticHub::INSTANCE;
}



StaticHub::StaticHub() {
  StaticHub::INSTANCE = this;
  for (MessageTypeDef mes_type : message_defs) {
    ManuvrMsg::message_defs_extended.insert(&mes_type);
  }

  encoder_stack = new Encoder(2, 3);
}




/****************************************************************************************************
* Resource fetch functions...                                                                       *
****************************************************************************************************/

Scheduler*    StaticHub::fetchScheduler(void) {     return &__scheduler;     }
EventManager* StaticHub::fetchEventManager(void) {  return &event_manager;   }
AudioRouter*  StaticHub::fetchAudioRouter(void) {   return audio_router;     }


bool power_mode = false;


/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ 
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌          
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀ 
* 
* These are overrides from EventReceiver interface...
****************************************************************************************************/

/*
* Some peripherals and operations need a bit of time to complete. This function is called from a
*   one-shot schedule and performs all of the cleanup for latent consequences of bootstrap().
*/
int8_t StaticHub::bootComplete() {
  //off_class_interrupts(true);  // Now configure interrupts, lift interrupt masks, and let the madness begin.
	return 0;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument 
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We 
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the EventManager
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t StaticHub::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    case MANUVR_MSG_SYS_BOOT_COMPLETED:
      StaticHub::log("StaicHub: Boot complete.\n");
      bootstrap_completed = true;
      break;
    default:
      break;
  }
  
  return return_value;
}



int8_t StaticHub::notify(ManuvrEvent *active_event) {
  StringBuilder output;
  int8_t return_value = 0;
  uint32_t temp_uint_32;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_INTERRUPTS_MASKED:
      break;

    // RETAIN CASES BELOW
    // RETAIN CASES BELOW
    // RETAIN CASES BELOW
    case MANUVR_MSG_SYS_USB_DISCONNECT:
    case MANUVR_MSG_SYS_USB_SUSPEND:
    case MANUVR_MSG_SYS_USB_RESET:
      mute_logger = true;
      log_buffer.clear();
      return_value++;
      break;
    case MANUVR_MSG_SYS_USB_CONFIGURED:
    case MANUVR_MSG_SYS_USB_RESUME:
    case MANUVR_MSG_SYS_USB_CONNECT:
      mute_logger = false;
      return_value++;
      break;

    case MANUVR_MSG_USER_DEBUG_INPUT:
      last_user_input.concatHandoff(&usb_rx_buffer);
      procDirectDebugInstruction(&last_user_input);
      return_value++;
      break;
    case MANUVR_MSG_SYS_REBOOT:
      maskable_interrupts(false);
      reboot();
      break;
    case MANUVR_MSG_SYS_BOOTLOADER:
      maskable_interrupts(false);
      jumpToBootloader();
      break;
    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      {
        if (NULL != sess) {
          StringBuilder temp_sb;
          int mess_len = sess->nextMessage(&temp_sb);
          if (mess_len > 0) {
            Serial.write(temp_sb.string(), temp_sb.length());
          }
        }
        
      }
      break;
      
    case MANUVR_MSG_USER_BUTTON_PRESS:
      if (active_event->argCount() > 0) {
        uint8_t button;
        if (0 == active_event->getArgAs(&button)) {
          switch (button) {
            case 2:
              return_value++;
              power_mode = !power_mode;
              break;
            case 3:
              return_value++;
              break;
            case 4:
              return_value++;
              break;
            case 5:
              return_value++;
              break;
          }
        }
      }
      break;

    case VIAM_SONUS_MSG_ADC_SCAN:
      if (power_mode) {
        uint8_t* npfb = strip->getPixels();
        for (uint8_t i = 0; i < 10; i++) {
          switch ((adc_scanner->getSample(i) - 512) / 64) {
            case 0:
              for (uint8_t x = 0; x < 24; x++) *(npfb++) = 0;
              break;
            case 1:
              *(npfb++) = 0;
              *(npfb++) = 0xFF;
              *(npfb++) = 0;
              for (uint8_t x = 0; x < 21; x++) *(npfb++) = 0;
              break;
            case 2:
              for (uint8_t x = 0; x < 3; x++)  *(npfb++) = 0xFF;
              *(npfb++) = 0;
              *(npfb++) = 0xFF;
              *(npfb++) = 0;
              for (uint8_t x = 0; x < 18; x++) *(npfb++) = 0;
              break;
            case 3:
              for (uint8_t x = 0; x < 6; x++)  *(npfb++) = 0xFF;
              *(npfb++) = 0;
              *(npfb++) = 0xFF;
              *(npfb++) = 0;
              for (uint8_t x = 0; x < 15; x++) *(npfb++) = 0;
              break;
            case 4:
              for (uint8_t x = 0; x < 9; x++)  *(npfb++) = 0xFF;
              *(npfb++) = 0;
              *(npfb++) = 0xFF;
              *(npfb++) = 0;
              for (uint8_t x = 0; x < 12; x++) *(npfb++) = 0;
              break;
            case 5:
              for (uint8_t x = 0; x < 12; x++)  *(npfb++) = 0xFF;
              *(npfb++) = 0;
              *(npfb++) = 0xFF;
              *(npfb++) = 0;
              for (uint8_t x = 0; x < 9; x++) *(npfb++) = 0;
              break;
            case 6:
              for (uint8_t x = 0; x < 15; x++)  *(npfb++) = 0xFF;
              *(npfb++) = 0;
              *(npfb++) = 0xFF;
              *(npfb++) = 0;
              for (uint8_t x = 0; x < 6; x++) *(npfb++) = 0;
              break;
            case 7:
              for (uint8_t x = 0; x < 18; x++)  *(npfb++) = 0xFF;
              *(npfb++) = 0;
              *(npfb++) = 0xFF;
              *(npfb++) = 0;
              break;
          }
        }
        strip->show();
        return_value++;
      }
      break;

    /* Something is trying to start a program. */
    case MANUVR_MSG_PROGRAM_START:
      {
        //const char *temp_ptr = NULL; 
        //if (0 == active_event->getArgAs(&temp_ptr)) {
        //  output.concatf("Starting demo program %s.\n", ((NULL != temp_ptr) ? temp_ptr : "<UNSPECIFIED>"));
        //  ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_SYS_LOG_VERBOSITY);
        //  event->addArg((uint8_t) 0);
        //  raiseEvent(event);
        //  
        //}
      }
      return_value++;
      break;

    case MANUVR_MSG_RNG_BUFFER_EMPTY:
      output.concatf("RNG underrun.\n");
      break;

    case MANUVR_MSG_SYS_ISSUE_LOG_ITEM:
      {
        StringBuilder *log_item;
        if (0 == active_event->getArgAs(&log_item)) {
          log_buffer.concatHandoff(log_item);
        }
        __scheduler.enableSchedule(pid_log_moderator);
      }
      break;
      
      
    // TODO: Wrap into SELF_DESCRIBE
    //case MANUVR_MSG_VERSION_FIRMWARE:
    //  if (0 == active_event->args.size()) {
    //    StringBuilder *str_ver = new StringBuilder("ViamSonus v");
    //    str_ver->concat(VERSION_STRING);
    //    active_event->addArg(str_ver);
    //    return_value++;
    //  }
    //  break;
      
      
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  if (output.length() > 0) {    StaticHub::log(&output);  }
  return return_value;
}






/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄       ▄            ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄  
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌          ▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░▌ 
 ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌     ▐░▌          ▐░█▀▀▀▀▀▀▀█░▌▐░▌░▌     ▐░▌▐░█▀▀▀▀▀▀▀█░▌
     ▐░▌     ▐░▌          ▐░▌       ▐░▌     ▐░▌          ▐░▌       ▐░▌▐░▌▐░▌    ▐░▌▐░▌       ▐░▌
     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌     ▐░▌          ▐░█▄▄▄▄▄▄▄█░▌▐░▌ ▐░▌   ▐░▌▐░▌       ▐░▌
     ▐░▌     ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌          ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌▐░▌       ▐░▌
     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀█░█▀▀      ▐░▌          ▐░█▀▀▀▀▀▀▀█░▌▐░▌   ▐░▌ ▐░▌▐░▌       ▐░▌
     ▐░▌               ▐░▌▐░▌     ▐░▌       ▐░▌          ▐░▌       ▐░▌▐░▌    ▐░▌▐░▌▐░▌       ▐░▌
 ▄▄▄▄█░█▄▄▄▄  ▄▄▄▄▄▄▄▄▄█░▌▐░▌      ▐░▌      ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌       ▐░▌▐░▌     ▐░▐░▌▐░█▄▄▄▄▄▄▄█░▌
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌     ▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░▌      ▐░░▌▐░░░░░░░░░░▌ 
 ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀       ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀ 

Interrupt service routine support functions...                                                                    
A quick note is in order. These functions are static class members that are called directly from  
  the ISRs in stm32f4xx_it.c. They are not themselves ISRs. Keep them as short as possible.                                     

TODO: Keep them as short as possible.                                                             
****************************************************************************************************/


/*
* Called from the SysTick handler once per ms.
*/
volatile void StaticHub::advanceScheduler(void) {
	if (0 == (millis_since_reset++ % watchdog_mark)) {
	  /* If it's time to punch the watchdog, do so. Doing it in the ISR will prevent a
	       long-running operation from causing a reset. This way, the only thing that will
	       cause a watchdog reset is if this ISR fails to fire. */
	  //WWDG_SetCounter(127);
	}
	// TODO: advanceScheduler() can be made *much* faster with a bit of re-work in the Scheduler class.
	if (NULL != INSTANCE) ((StaticHub*) INSTANCE)->__scheduler.advanceScheduler();
}


/*
* This is called from the USB peripheral. It is called when the short static
* character array that forms the USB rx buffer is either filled up, or we see
* a new-line character on the wire.
*/
void StaticHub::feedUSBBuffer(uint8_t *buf, int len, bool terminal) {
	usb_rx_buffer.concat(buf, len);
	
	if (terminal) {
		// If the ISR saw a CR or LF on the wire, we tell the parser it is ok to
		// run in idle time.
    ManuvrEvent* event = EventManager::returnEvent(MANUVR_MSG_USER_DEBUG_INPUT);
    event->specific_target = (EventReceiver*) this;
    raiseEvent(event);
	}
}



/****************************************************************************************************
 ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄   ▄         ▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░░░░░░░░░░░▌
▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ 
▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌▐░▌ ▄▄▄▄▄▄▄▄ 
▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░▌▐░░░░░░░░▌
▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░▌ ▀▀▀▀▀▀█░▌
▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
 ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀ 
 
Code in here only exists for as long as it takes to debug something. Don't write against these fxns.
****************************************************************************************************/

/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* StaticHub::getReceiverName() {  return "StaticHub";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void StaticHub::printDebug(StringBuilder* output) {
	if (NULL == output) return;
  uint32_t initial_sp = getStackPointer();
	uint32_t final_sp = getStackPointer();
	
	EventReceiver::printDebug(output);
	output->concatf("--- %s v%s    Build date: %s %s\n\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);
	output->concatf("---\n--- bootstrap_completed  %s\n", (bootstrap_completed) ? "yes" : "no");
	output->concatf("---\n--- stack grows %s\n", (final_sp > initial_sp) ? "up" : "down");
	output->concatf("--- getStackPointer()   0x%08x\n---\n", getStackPointer());

	output->concatf("--- millis()            0x%08x\n", millis());
	output->concatf("--- micros()            0x%08x\n", micros());
	currentDateTime(output);
	output->concatf("\nrtc_startup_state %s\n", getRTCStateString(rtc_startup_state));
}



void StaticHub::print_type_sizes(void) {
	StringBuilder temp("---< Type sizes >-----------------------------\n");
	temp.concatf("Elemental structures:\n");
	temp.concatf("\tStringBuilder         %d\n", sizeof(StringBuilder));
	temp.concatf("\tLinkedList<void*>     %d\n", sizeof(LinkedList<void*>));
	temp.concatf("\tPriorityQueue<void*>   %d\n", sizeof(PriorityQueue<void*>));
	
	temp.concatf("Peripheral sensor package:\n");
	
	temp.concatf("i2c machinary:\n");
	temp.concatf("\tI2CQueuedOperation    %d\n", sizeof(I2CQueuedOperation));
	temp.concatf("\tI2CDevice (Registers) %d\n", sizeof(I2CDeviceWithRegisters));
	temp.concatf("\tI2CDevice             %d\n", sizeof(I2CDevice));
	temp.concatf("\tDeviceRegister        %d\n", sizeof(DeviceRegister));

	temp.concatf(" Core singletons:\n");
	temp.concatf("\t StaticHub             %d\n", sizeof(StaticHub));
	temp.concatf("\t Scheduler             %d\n", sizeof(Scheduler));
	temp.concatf("\t EventManager          %d\n", sizeof(EventManager));
	temp.concatf("\t I2CAdapter            %d\n", sizeof(I2CAdapter));

	temp.concatf(" Messaging components:\n");
	temp.concatf("\t ManuvrEvent      %d\n", sizeof(ManuvrEvent));
	temp.concatf("\t ManuvrMsg        %d\n", sizeof(ManuvrMsg));
	temp.concatf("\t Argument              %d\n", sizeof(Argument));
	temp.concatf("\t SchedulerItem         %d\n", sizeof(ScheduleItem));
	temp.concatf("\t TaskProfilerData      %d\n", sizeof(TaskProfilerData));

	StaticHub::log(&temp);
}




void StaticHub::procDirectDebugInstruction(StringBuilder* input) {
  char *str = (char *) input->string();
  char c = *(str);
  uint8_t temp_byte = 0;        // Many commands here take a single integer argument.
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  ManuvrEvent *event = NULL;  // Pitching events is a common thing in this fxn...
  
  StringBuilder parse_mule;
  
  switch (c) {
    case 'B':
      if (temp_byte == 128) {
        EventManager::raiseEvent(MANUVR_MSG_SYS_BOOTLOADER, NULL);
      }
      local_log.concatf("Will only jump to bootloader if the number '128' follows the command.\n");
      break;
    case 'b':
      if (temp_byte == 128) {
        EventManager::raiseEvent(MANUVR_MSG_SYS_REBOOT, NULL);
      }
      local_log.concatf("Will only reboot if the number '128' follows the command.\n");
      break;

    case 'f':  // FPU benchmark
      {
        float a = 1.001;
        long time_var2 = millis();
        for (int i = 0;i < 1000000;i++) {
          a += 0.01 * sqrtf(a);
        }
        local_log.concatf("Running floating-point test...\nTime:      %d ms\n", millis() - time_var2);
        local_log.concatf("Value:     %.5f\nFPU test concluded.\n", (double) a);
      }
      break;

    case '6':        // Read so many random integers...
      { // TODO: I don't think the RNG is ever being turned off. Save some power....
        temp_byte = (temp_byte == 0) ? STATICHUB_RNG_CARRY_CAPACITY : temp_byte;
        for (uint8_t i = 0; i < temp_byte; i++) {
          uint32_t x = StaticHub::randomInt();
          if (x) {
            local_log.concatf("Random number: 0x%08x\n", x);
          }
          else {
            local_log.concatf("Restarting RNG\n");
            init_RNG();
          }
        }
      }
      break;


    case 'u':
      switch (temp_byte) {
        case 1:
          EventManager::raiseEvent(MANUVR_MSG_SELF_DESCRIBE, NULL);
          break;
        case 3:
          EventManager::raiseEvent(MANUVR_MSG_LEGEND_MESSAGES, NULL);
          break;
        default:
          break;
      }
      break;

    case 'y':    // Power mode.
      switch (temp_byte) {
        case 255:
          break;
        default:
          event = EventManager::returnEvent(MANUVR_MSG_SYS_POWER_MODE);
          event->addArg((uint8_t) temp_byte);
          raiseEvent(event);
          local_log.concatf("Power mode is now %d.\n", temp_byte);
          break;
      }
      break;
      
      
    case 'i':   // StaticHub debug prints.
      if (1 == temp_byte) {
        local_log.concat("EventManager profiling enabled.\n");
        event_manager.profiler(true);
      }
      else if (2 == temp_byte) {
        event_manager.printDebug(&local_log);
      }
      else if (3 == temp_byte) {
        print_type_sizes();
      }
      else if (5 == temp_byte) {
        event_manager.clean_first_discard();
        local_log.concat("EventManager cleaned the head of the discard queue.\n");
      }
      else if (6 == temp_byte) {
        local_log.concat("EventManager profiling disabled.\n");
        event_manager.profiler(false);
      }
      else if (10 == temp_byte) {
        local_log.concat("Sending boot complete...\n");
        EventManager::raiseEvent(MANUVR_MSG_SYS_BOOT_COMPLETED, this);  // Bootstrap complete
      }
      else {
        printDebug(&local_log);
      }
      break;
      
    case 'S':
      break;
   
    case 'o':
      if (temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_PROFILER_STOP);
        event->addArg(temp_byte);

        local_log.concatf("Stopped profiling schedule %d\n", temp_byte);
        event->printDebug(&local_log);
      }
      break;
    case 'O':
      if (temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_PROFILER_START);
        event->addArg(temp_byte);

        local_log.concatf("Now profiling schedule %d\n", temp_byte);
        event->printDebug(&local_log);
      }
      break;

    case 'P':
      if (1 == temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_DUMP_META);
        raiseEvent(event);   // Raise an event with a message. No need to clean it up.
      }
      else if (2 == temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_DUMP_SCHEDULES);
        raiseEvent(event);   // Raise an event with a message. No need to clean it up.
      }
      else if (3 == temp_byte) {
        event = new ManuvrEvent(MANUVR_MSG_SCHED_PROFILER_DUMP);
        raiseEvent(event);   // Raise an event with a message. No need to clean it up.
      }
      else if (4 == temp_byte) {
        //ManuvrEvent *deferred = new ManuvrEvent(MANUVR_MSG_SCHED_DUMP_META);
        //event = new ManuvrEvent(MANUVR_MSG_SCHED_DEFERRED_EVENT);
        //event->addArg(deferred);
        //EventManager::raiseEvent(event);   // Raise an event with a message. No need to clean it up.
      }
      else {
        if (__scheduler.scheduleEnabled(pid_profiler_report)) {
          __scheduler.disableSchedule(pid_profiler_report);
          local_log.concatf("Scheduler profiler dump disabled.\n");
        }
        else {
          __scheduler.enableSchedule(pid_profiler_report);
          __scheduler.delaySchedule(pid_profiler_report, 10);   // Run the schedule immediately.
          local_log.concatf("Scheduler profiler dump enabled.\n");
        }
      }
      break;


    case 'v':           // Set log verbosity.
      parse_mule.concat(str);
      local_log.concatf("parse_mule split (%s) into %d positions. \n", str, parse_mule.split(" "));
      parse_mule.drop_position(0);
      
      event = new ManuvrEvent(MANUVR_MSG_SYS_LOG_VERBOSITY);
      switch (parse_mule.count()) {
        case 2:
          event->specific_target = event_manager.getSubscriberByName((const char*) (parse_mule.position_trimmed(1)));
          local_log.concatf("Directing verbosity change to %s.\n", (NULL == event->specific_target) ? "NULL" : event->specific_target->getReceiverName());
        case 1:
          event->addArg((uint8_t) parse_mule.position_as_int(0));
          break;
        default:
          break;
      }
      raiseEvent(event);
      break;
	  
    case 'z':
      input->cull(1);
      ((I2CAdapter*) i2c)->procDirectDebugInstruction(input);
	    break;
    case 'm':
      input->cull(1);
      break;
    case 'l':
      input->cull(1);
      break;

    default:
      break;
  }

  if (local_log.length() > 0) {    StaticHub::log(&local_log);  } // Cheap trick to trigger the logger.
  last_user_input.clear();
}

