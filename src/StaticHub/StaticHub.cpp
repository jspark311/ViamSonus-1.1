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
  }
}
