/*
File:   StaticHub.h
Author: J. Ian Lindsay
Date:   2014.07.01


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


StaticHub is a generalized resource pool that is implemented as a Singleton.
The reason for this is that we want to avoid making individual classes singleton
while still retaining global-access and a one-instance-only instantiation.

This class is by necessity, application-specific. Re-use in other projects is not recommended.

StaticHub has these responsibilities:
  Responsibility #1:
    This class holds pointers to the classes that provide hardware-level services:
    * Device drivers
    * Display framebuffer
    * Logging
    * Scheduler
    * Host communication handles
    * And so on....
  Responsibility #2:
    Interrupt service routines that cannot (or should not) be encapsulated are collected here.


This class needs to be instantiated prior to lifting interrupt masks. It should
  never be torn down. There is no destructor. If this class instance is torn down,
  the program ceases to function in a unified way.

Once instantiated, calling the bootstrap() function will build all of the class instances
  that our program needs to have globally-accessible. As other classes come to life,
  they should call this class instance to find pointers to each driver they will need,
  cache those pointers, and then never ask for them again.
  
Because of its priveleged position in the program, StaticHub contains static pass-through
  functions to commonly needed services (such as the Logger). This allows us to avoid this
  sort of mess everywhere else in the program:

    #ifdef LOGGING_FACULTY
      extern Logger logger;
      function classname::log(char* message) {
          logger->log(message);
      }
    #else
      function classname::log(char* message) {
          printf("%s\n", message);
      }
    #endif

  ...instead converting that into something more like...

    s_hub->log(message);
    
  All of the preprocessor directives and graceful deprecation nonsense can be collected here,
  and it should be fairly easy for even a novice to replace the classes so wrapped.

Saving the externs lowers the maintenance burden somewhat, but also makes live objects easier
  to manage and profile, since we don't need to worry about destructors leaving stale references
  in other (sometimes badly-written) classes.
  
Initialization order matters a great deal. We should bring classes online in order of dependencies.
  The bootstrap() function should, as its last act, raise the BOOT_COMPLETED event. That way, any 
  dependancies that were unsatisfied on instantiation can be collected and used within the classes
  that require them.
*/


#ifndef __STATIC_HUB_H__
#define __STATIC_HUB_H__

  // System-level includes.
  #include <inttypes.h>
  #include <stdlib.h>
  #include <stdarg.h>
  #include <time.h>
  #include <string.h>
  #include <stdio.h>

  #define LOG_EMERG   0    /* system is unusable */
  #define LOG_ALERT   1    /* action must be taken immediately */
  #define LOG_CRIT    2    /* critical conditions */
  #define LOG_ERR     3    /* error conditions */
  #define LOG_WARNING 4    /* warning conditions */
  #define LOG_NOTICE  5    /* normal but significant condition */
  #define LOG_INFO    6    /* informational */
  #define LOG_DEBUG   7    /* debug-level messages */


  #include "FirmwareDefs.h"
  #include <ManuvrOS/Scheduler.h>
  #include <ManuvrOS/EventManager.h>
  #include "StringBuilder/StringBuilder.h"
  
#ifdef ARDUINO
  #include <Arduino.h>
#endif

class AudioRouter;

#ifdef __cplusplus
 extern "C" {
#endif 



/*
* These are just lables. We don't really ever care about the *actual* integers being defined here. Only
*   their consistency.
*/
#define RTC_STARTUP_UNINITED       0x00000000
#define RTC_STARTUP_UNKNOWN        0x23196400
#define RTC_OSC_FAILURE            0x23196401
#define RTC_STARTUP_GOOD_UNSET     0x23196402
#define RTC_STARTUP_GOOD_SET       0x23196403


/*
* These are constants where we care about the number.
*/
#define STATICHUB_RNG_CARRY_CAPACITY           10     // How many random numbers should StaticHub cache?



/*
* This is the actual class...
*/
class StaticHub : public EventReceiver {
  public:
    volatile static uint32_t millis_since_reset;
    volatile static uint8_t  watchdog_mark;

    static bool mute_logger;
    static StringBuilder log_buffer;

    StaticHub(void);
    static StaticHub* getInstance(void);
    int8_t bootstrap(void);
    
    // These are functions that should be reachable from everywhere in the application.
    volatile static void log(const char *fxn_name, int severity, const char *str, ...);  // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(int severity, const char *str);                             // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(const char *str);                                           // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(StringBuilder *str);

    // getPreference()
    // setPreference()
    
    /*
    * Nice utility functions.
    */
    static uint32_t randomInt(void);                                // Fetches one of the stored randoms and blocks until one is available.
    static volatile bool provide_random_int(uint32_t);              // Provides a new random to StaticHub from the RNG ISR.
    static volatile uint32_t getStackPointer(void);                 // Returns the value of the stack pointer and prints some data.
    
        
    bool setTimeAndDate(char*);   // Takes a string of the form given by RFC-2822: "Mon, 15 Aug 2005 15:52:01 +0000"   https://www.ietf.org/rfc/rfc2822.txt
    uint32_t currentTimestamp(void);         // Returns an integer representing the current datetime.
    void currentTimestamp(StringBuilder*);   // Same, but writes a string representation to the argument.
    void currentDateTime(StringBuilder*);    // Writes a human-readable datetime to the argument.
    

    // Call this to accumulate characters from the USB layer into a buffer.
    // Pass terminal=true to cause StaticHub to proc an accumulated command from the host PC.
    void feedUSBBuffer(uint8_t *buf, int len, bool terminal);


    /*
    * These are global resource accessor functions. They are called once from each class that
    *   requires them. That class can technically call this accessor for each use, but this should
    *   be discouraged, as the instances fetched by these functions should never change.
    */
    // Sensors...

    // fetchMicrophones(void);

    // Services...
    EventManager* fetchEventManager(void);
    Scheduler* fetchScheduler(void);
    
    AudioRouter* fetchAudioRouter(void);
    
    
    // Volatile statics that serve as ISRs...
    volatile static void advanceScheduler(void);
    
    
    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    const char* getReceiverName();
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);


    void disableLogCallback();


  protected:
    int8_t bootComplete();       // Called as a result of bootstrap completed being raised.

    
  private:
    volatile static StaticHub* INSTANCE;
    
    static volatile uint32_t next_random_int[STATICHUB_RNG_CARRY_CAPACITY];  // Stores the last 10 random numbers.
    
    bool     usb_string_waiting   = false;
    StringBuilder usb_rx_buffer;
    StringBuilder last_user_input;
    
    // Scheduler PIDs that will be heavilly used...
    uint32_t pid_log_moderator   = 0;  // Moderate the logs running into the USB line.
    uint32_t pid_ui_timeout      = 0;  // If the user is expected to do something, we won't wait forever...
    uint32_t pid_profiler_report = 0;  // Internal testing. Allows periodic profiler dumping.
    uint32_t pid_prog_run_delay  = 0;  // Give the programmer a chance to stop system init before it get unmanagable.

    bool bootstrap_completed       = false;
    
    uint32_t rtc_startup_state = RTC_STARTUP_UNINITED;  // This is how we know what state we found the RTC in.
    
    // Global system resource handles...
    EventManager event_manager;            // This is our asynchronous message queue. 
    Scheduler __scheduler;
    
    AudioRouter *audio_router = NULL;

    // These fxns do string conversion for integer type-codes, and are only useful for logging.
    const char* getRTCStateString(uint32_t code);
    
    void print_type_sizes(void);

    // These functions handle various stages of bootstrap...
    void gpioSetup(void) volatile;        // We call this once on bootstrap. Sets up GPIO not covered by other classes.
    void nvicConf(void) volatile;         // We call this once on bootstrap. Sets up IRQs not covered by other classes.
    void init_RNG(void) volatile;         // Fire up the random number generator.
    void initRTC(void) volatile;          // We call this once on bootstrap. Sets up the RTC.
    void initSchedules(void);    // We call this once on bootstrap. Sets up all schedules.
    
    void procDirectDebugInstruction(StringBuilder*);
    
    void off_class_interrupts(bool enable);
    void maskable_interrupts(bool enable);
};

#ifdef __cplusplus
}
#endif 
  

#endif
