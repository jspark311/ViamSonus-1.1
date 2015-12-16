

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
  #define VIAM_SONUS_MSG_ADC_SCAN              0x9040 // 






// These are only here until they are migrated to each receiver that deals with them.
const MessageTypeDef message_defs_viam_sonus[] = {
  {  VIAM_SONUS_MSG_ADC_SCAN              , MSG_FLAG_IDEMPOTENT,  "ADC_SCAN",              ManuvrMsg::MSG_ARGS_NONE }, // It is time to scan the ADC channels.
  {  VIAM_SONUS_MSG_ENCODER_UP            , 0x0000,               "ENCODER_UP",            ManuvrMsg::MSG_ARGS_U8 },   // The encoder on the front panel was incremented.
  {  VIAM_SONUS_MSG_ENCODER_DOWN          , 0x0000,               "ENCODER_DOWN",          ManuvrMsg::MSG_ARGS_U8 },   // The encoder on the front panel was decremented.
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
  
  /* ViamSonus has neopixels and a light-level sensor. */
  {  MANUVR_MSG_NEOPIXEL_REFRESH     , MSG_FLAG_IDEMPOTENT,  "NEOPIXEL_REFRESH"     , ManuvrMsg::MSG_ARGS_NONE }, // Cause any neopixel classes to refresh their strands.
  {  MANUVR_MSG_DIRTY_FRAME_BUF      , 0x0000,               "DIRTY_FRAME_BUF"      , ManuvrMsg::MSG_ARGS_NONE }, // Something changed the framebuffer and we need to redraw.
  {  MANUVR_MSG_AMBIENT_LIGHT_LEVEL  , MSG_FLAG_IDEMPOTENT,  "LIGHT_LEVEL"          , ManuvrMsg::MSG_ARGS_U16  }, // Unitless light-level report.

  /* ViamSonus has hardware buttons... */
  {  MANUVR_MSG_USER_BUTTON_PRESS    , MSG_FLAG_EXPORTABLE,               "USER_BUTTON_PRESS",    ManuvrMsg::MSG_ARGS_U8 },   // The user pushed a button with the given integer code.
  {  MANUVR_MSG_USER_BUTTON_RELEASE  , MSG_FLAG_EXPORTABLE,               "USER_BUTTON_RELEASE",  ManuvrMsg::MSG_ARGS_U8 },   // The user released a button with the given integer code.
};




#ifdef __cplusplus
  extern "C" {
    //__attribute__((weak)) omit this as used in _write() in Print.cpp
  
    // this function overrides the one of the same name in Print.cpp
    int _write(int file, char *ptr, int len)
    {
        // send chars to zero or more outputs
        for (int i = 0; i < len; i++)  {
            Serial.print(ptr[i]);
        }
        return 0;
    }
  } // end extern "C" section
#endif


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


/*
* The primary init function. Calling this will bring the entire system online if everything is
*   working nominally.
*/
int8_t StaticHub::bootstrap() {
  // One of the first things we need to do is populate the EventManager with all of the
  // message codes that come with this firmware.
  int mes_count = sizeof(message_defs_viam_sonus) / sizeof(MessageTypeDef);
  for (int i = 0; i < mes_count; i++) {
    ManuvrMsg::message_defs_extended.insert(&message_defs_viam_sonus[i]);
  }

  return 0;
}



int8_t StaticHub::notify(ManuvrEvent *active_event) {
  StringBuilder output;
  int8_t return_value = 0;
  uint32_t temp_uint_32;
  
  switch (active_event->event_code) {
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
      
    case MANUVR_MSG_USER_BUTTON_PRESS:
      if (active_event->argCount() > 0) {
        uint8_t button;
        if (0 == active_event->getArgAs(&button)) {
          switch (button) {
            case 2:
              return_value++;
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
	  {
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

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  if (output.length() > 0) {    StaticHub::log(&output);  }
  return return_value;
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


    case 'a':
      input->cull(1);
      audio_router->procDirectDebugInstruction(input);
      break;

    default:
      break;
  }

  if (local_log.length() > 0) StaticHub::log(&local_log);
  last_user_input.clear();
}

