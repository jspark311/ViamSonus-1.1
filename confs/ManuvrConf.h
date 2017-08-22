/*
File:   ManuvrConf.h
Author: J. Ian Lindsay
Date:   2015.03.01


This is one of the files that the application author is required to provide.
This is where definition of (application or device)-specific parameters ought to go.
*/

#ifndef __MANUVR_FIRMWARE_DEFS_H
#define __MANUVR_FIRMWARE_DEFS_H


/*******************************************************************************
* Required fields...                                                           *
*******************************************************************************/

/*
* Particulars of this platform.
*/
#define PLATFORM_RNG_CARRY_CAPACITY       10    // How many random numbers should be cached? Must be > 0.
#define MANUVR_PLATFORM_TIMER_PERIOD_MS   10    // What is the granularity of our scheduler?
#define MANUVR_SUPPORT_I2C
#define MANUVR_STORAGE

/*
* Kernel options.
*/
#define EVENT_MANAGER_PREALLOC_COUNT     8
#define SCHEDULER_MAX_SKIP_BEFORE_RESET  20

/*
* Particulars of this Manuvrable.
*/
// This is the string that identifies this Manuvrable to other Manuvrables. In MHB's case, this
//   will select the mEngine.
#define FIRMWARE_NAME            "ViamSonus"    // This will select Manuvr's debug engine in MHB.

// This would be the version of the Manuvrable's firmware (this program).
#define VERSION_STRING               "0.1.1"

// Hardware is versioned. Manuvrables that are strictly-software should say -1 here.
#define HW_VERSION_STRING               "1"

// The version of Manuvr's protocol we are using.
#define PROTOCOL_VERSION                  1
#define PROTOCOL_MTU                   2000    // See MTU notes above....

#define MANUVR_CBOR


/*
* Debugging and console options.
*/
#define MANUVR_CONSOLE_SUPPORT
#define MANUVR_DEBUG
#define MANUVR_EVENT_PROFILER



/* AudioRouter functionality */
  #define VIAM_SONUS_MSG_ENCODER_UP            0x9010 //
  #define VIAM_SONUS_MSG_ENCODER_DOWN          0x9011 //

#endif  // __MANUVR_FIRMWARE_DEFS_H
