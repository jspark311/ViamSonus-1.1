/*
File:   FirmwareDefs.h
Author: J. Ian Lindsay
Date:   2015.03.01

This is one of the files that the application author is required to provide. This is where definition of
  (application or device)-specific event codes ought to go. We also define some fields that will be used
  during communication with other devices, so some things here are mandatory.

*/

#ifndef __FIRMWARE_DEFS_H
#define __FIRMWARE_DEFS_H

/*
* Macros we will use in scattered places...
*/
#ifndef max
    #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
    #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

/*
* These are required fields.
*
* PROTOCOL_MTU is required for constraining communication length due to memory restrictions at
*   one-or-both sides. Since the protocol currently supports up to (2^24)-1 bytes in a single transaction,
*   a microcontroller would want to limit it's counterparty's use of precious RAM. PROTOCOL_MTU, therefore,
*   determines the effective maximum packet size for this device.
*/
#define PROTOCOL_MTU              20000                  // See MTU notes above....
#define VERSION_STRING            "0.1.0"                // We should be able to communicate version so broken behavior can be isolated.
#define HW_VERSION_STRING         "0"                    // Revision-0 hardwaare (engineering prototype).
#define IDENTITY_STRING           "ViamSonus"            // Might also be a hash....
#define EXTENDED_DETAIL_STRING    ""                     // Optional. User-defined.


/* AudioRouter functionality */
  #define VIAM_SONUS_MSG_ENABLE_ROUTING        0x9000 // 
  #define VIAM_SONUS_MSG_DISABLE_ROUTING       0x9001 // 
  #define VIAM_SONUS_MSG_NAME_INPUT_CHAN       0x9002 // 
  #define VIAM_SONUS_MSG_NAME_OUTPUT_CHAN      0x9003 // 
  #define VIAM_SONUS_MSG_DUMP_ROUTER           0x9004 // 
  #define VIAM_SONUS_MSG_OUTPUT_CHAN_VOL       0x9005 // 
  #define VIAM_SONUS_MSG_UNROUTE               0x9006 // 
  #define VIAM_SONUS_MSG_ROUTE                 0x9007 // 
  #define VIAM_SONUS_MSG_PRESERVE_ROUTES       0x9008 // 
  #define VIAM_SONUS_MSG_GROUP_CHANNELS        0x9009 // 
  #define VIAM_SONUS_MSG_UNGROUP_CHANNELS      0x900A // 

  #define VIAM_SONUS_MSG_ENCODER_UP            0x9010 // 
  #define VIAM_SONUS_MSG_ENCODER_DOWN          0x9011 // 
  #define VIAM_SONUS_MSG_USER_BUTTON_PRESS     0x9012 // 
  #define VIAM_SONUS_MSG_USER_BUTTON_RELEASE   0x9013 // 
  #define VIAM_SONUS_MSG_NEOPIXEL_REFRESH      0x9020 // 
  #define VIAM_SONUS_MSG_AMBIENT_LIGHT_LEVEL   0x9030 // 
  #define VIAM_SONUS_MSG_ADC_SCAN              0x9040 // 


#ifdef __cplusplus
extern "C" {
#endif

volatile void jumpToBootloader(void);
volatile void reboot(void);

#ifdef __cplusplus
}
#endif

#endif
