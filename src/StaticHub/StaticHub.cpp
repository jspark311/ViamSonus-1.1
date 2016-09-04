

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

  }
}
