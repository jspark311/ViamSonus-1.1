###########################################################################
# Makefile for ViamSonus-1.1
# Author: J. Ian Lindsay
# Date:   2015.03.06
#
###########################################################################


# Environmental awareness...
###########################################################################
SHELL          = /bin/sh
WHO_I_AM       = $(shell whoami)
WHERE_I_AM     = $(shell pwd)
HOME_DIRECTORY = /home/$(WHO_I_AM)
ARDUINO_PATH   = $(HOME_DIRECTORY)/arduino
TEENSY_PATH    = $(ARDUINO_PATH)/hardware/teensy/avr/


# Variables for the firmware compilation...
###########################################################################
FIRMWARE_NAME      = viamsonus

TOOLCHAIN          = $(ARDUINO_PATH)/hardware/tools/arm
C_CROSS            = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
CPP_CROSS          = $(TOOLCHAIN)/bin/arm-none-eabi-g++
OBJCOPY            = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy
SZ_CROSS           = $(TOOLCHAIN)/bin/arm-none-eabi-size
TEENSY_LOADER_PATH = $(ARDUINO_PATH)/hardware/tools/teensy_loader_cli

OUTPUT_PATH        = build


MCU                = cortex-m4
CPU_SPEED          = 96000000
OPTIMIZATION       = -Os
C_STANDARD         = gnu99
CPP_STANDARD       = gnu++11
FORMAT             = ihex


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES     = -iquote. -iquotesrc/ 
INCLUDES    += -I./ -I$(TEENSY_PATH)libraries -I$(ARDUINO_PATH)/libraries
INCLUDES    += -I$(TEENSY_PATH)cores/teensy3
INCLUDES    += -I./ -Isrc/
INCLUDES    += -Isrc/FreeRTOS_Arduino/FreeRTOS_ARM/src/
INCLUDES    += -Isrc/FreeRTOS_Arduino/SdFat/src/

LD_FILE     = $(TEENSY_PATH)cores/teensy3/mk20dx256.ld

# Libraries to link
LIBS = -lm -lstdc++ -larm_cortexM4l_math -lc

# Wrap the include paths into the flags...
CFLAGS = $(INCLUDES)
CFLAGS += $(OPTIMIZATION) -Wall

CFLAGS += -DF_CPU=$(CPU_SPEED)
CFLAGS += -mcpu=$(MCU)  -mthumb -D__MK20DX256__

CFLAGS += -fno-exceptions -ffunction-sections -fdata-sections
CFLAGS += -mlittle-endian 
CFLAGS += -mfloat-abi=soft

CFLAGS += -DARDUINO=105 -nostdlib -DTEENSYDUINO=117

CFLAGS += -DUSB_VID=null -DUSB_PID=null -DUSB_SERIAL -DLAYOUT_US_ENGLISH

CPP_FLAGS = -std=$(CPP_STANDARD) 
CPP_FLAGS += -felide-constructors -fno-exceptions -fno-rtti


# Options that build for certain threading models (if any).
MANuVR_OPTIONS += -D__MANUVR_DEBUG
MANuVR_OPTIONS += -D__MANUVR_FREERTOS

CFLAGS += $(MANuVR_OPTIONS) 

CPP_FLAGS += $(CFLAGS)



###########################################################################
# Are we on a 64-bit system? If so, we'll need to specify
#   that we want a 32-bit build...
# Thanks, estabroo...
# http://www.linuxquestions.org/questions/programming-9/how-can-make-makefile-detect-64-bit-os-679513/
###########################################################################
LBITS = $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
  TARGET_WIDTH = -m32
else
  TARGET_WIDTH =
endif



###########################################################################
# Source file definitions...
###########################################################################

MANUVROS_SRCS = src/DataStructures/*.cpp src/ManuvrOS/*.cpp src/ManuvrOS/XenoSession/*.cpp src/ManuvrOS/ManuvrMsg/*.cpp src/ManuvrOS/Transports/*.cpp
SENSOR_SRCS   = src/ManuvrOS/Drivers/SensorWrapper/*.cpp 
I2C_DRIVERS   = src/ManuvrOS/Drivers/i2c-adapter/*.cpp src/ManuvrOS/Drivers/DeviceWithRegisters/DeviceRegister.cpp src/ManuvrOS/Drivers/DeviceWithRegisters/DeviceWithRegisters.cpp

I2C_VIAM_SONUS_DRIVERS  =  $(I2C_DRIVERS) src/ManuvrOS/Drivers/ISL23345/*.cpp src/ManuvrOS/Drivers/ADG2128/*.cpp src/ManuvrOS/Drivers/AudioRouter/*.cpp 

AUDIO_SOURCES = $(TEENSY_PATH)libraries/Audio/*.cpp $(TEENSY_PATH)libraries/Audio/*.c $(TEENSY_PATH)libraries/Audio/utility/*.c

CPP_SRCS  = $(MANUVROS_SRCS) $(SENSOR_SRCS) $(I2C_VIAM_SONUS_DRIVERS)
CPP_SRCS += src/Drivers/ADCScanner/*.cpp src/Drivers/LightSensor/*.cpp
CPP_SRCS += src/ManuvrOS/Drivers/ManuvrableNeoPixel/*.cpp

FREERTOS_C_SRC     = src/FreeRTOS_Arduino/FreeRTOS_ARM/src/FreeRTOS_ARM.c
FREERTOS_CPP_SRC  += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/assertMsg.cpp
FREERTOS_CPP_SRC  += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/basic_io_arm.cpp
FREERTOS_C_SRC    += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/utility/croutine.c
FREERTOS_C_SRC    += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/utility/event_groups.c
FREERTOS_C_SRC    += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/utility/heap_3.c
FREERTOS_C_SRC    += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/utility/list.c
FREERTOS_C_SRC    += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/utility/port.c
FREERTOS_C_SRC    += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/utility/queue.c
FREERTOS_C_SRC    += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/utility/tasks.c
FREERTOS_C_SRC    += src/FreeRTOS_Arduino/FreeRTOS_ARM/src/utility/timers.c


SRCS   = src/ViamSonus.cpp $(CPP_SRCS) $(AUDIO_SOURCES)



###########################################################################
# Rules for building the firmware follow...
###########################################################################

.PHONY: lib $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf


all: lib $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf
	$(SZ_CROSS) $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf


lib:
	mkdir -p $(OUTPUT_PATH)
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)libraries/Time/DateStrings.cpp -o $(OUTPUT_PATH)/DateStrings.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)libraries/Time/Time.cpp -o $(OUTPUT_PATH)/Time.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)libraries/SPI/SPI.cpp -o $(OUTPUT_PATH)/SPI.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)libraries/i2c_t3/i2c_t3.cpp -o $(OUTPUT_PATH)/SPI.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)libraries/Encoder/Encoder.cpp -o $(OUTPUT_PATH)/Encoder.cpp.o 
	
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/usb_dev.c -o $(OUTPUT_PATH)/usb_dev.c.o
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/usb_seremu.c -o $(OUTPUT_PATH)/usb_seremu.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/keylayouts.c -o $(OUTPUT_PATH)/keylayouts.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/serial2.c -o $(OUTPUT_PATH)/serial2.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/analog.c -o $(OUTPUT_PATH)/analog.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/usb_serial.c -o $(OUTPUT_PATH)/usb_serial.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/usb_desc.c -o $(OUTPUT_PATH)/usb_desc.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/eeprom.c -o $(OUTPUT_PATH)/eeprom.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/usb_mem.c -o $(OUTPUT_PATH)/usb_mem.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/serial1.c -o $(OUTPUT_PATH)/serial1.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/math_helper.c -o $(OUTPUT_PATH)/math_helper.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/serial3.c -o $(OUTPUT_PATH)/serial3.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/pins_teensy.c -o $(OUTPUT_PATH)/pins_teensy.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/mk20dx128.c -o $(OUTPUT_PATH)/mk20dx128.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/touch.c -o $(OUTPUT_PATH)/touch.c.o 
	$(C_CROSS)   -c $(CFLAGS) $(TEENSY_PATH)cores/teensy3/nonstd.c -o $(OUTPUT_PATH)/nonstd.c.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/Print.cpp -o $(OUTPUT_PATH)/Print.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/DMAChannel.cpp -o $(OUTPUT_PATH)/DMAChannel.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/yield.cpp -o $(OUTPUT_PATH)/yield.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/WString.cpp -o $(OUTPUT_PATH)/WString.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/WMath.cpp -o $(OUTPUT_PATH)/WMath.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/usb_inst.cpp -o $(OUTPUT_PATH)/usb_inst.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/Tone.cpp -o $(OUTPUT_PATH)/Tone.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/Stream.cpp -o $(OUTPUT_PATH)/Stream.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/avr_emulation.cpp -o $(OUTPUT_PATH)/avr_emulation.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/new.cpp -o $(OUTPUT_PATH)/new.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/HardwareSerial1.cpp -o $(OUTPUT_PATH)/HardwareSerial1.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/HardwareSerial2.cpp -o $(OUTPUT_PATH)/HardwareSerial2.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/HardwareSerial3.cpp -o $(OUTPUT_PATH)/HardwareSerial3.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/main.cpp -o $(OUTPUT_PATH)/main.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/IntervalTimer.cpp -o $(OUTPUT_PATH)/IntervalTimer.cpp.o 
	$(CPP_CROSS) -c $(CPP_FLAGS) $(TEENSY_PATH)cores/teensy3/AudioStream.cpp -o $(OUTPUT_PATH)/AudioStream.cpp.o 


$(OUTPUT_PATH)/$(FIRMWARE_NAME).elf:
	$(shell mkdir $(OUTPUT_PATH))

	$(CPP_CROSS) -c $(CPP_FLAGS) $(SRCS)
	$(CPP_CROSS) $(OPTIMIZATION) -Wl,--gc-sections -T$(LD_FILE) -mcpu=$(MCU) -mthumb -o $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf $(OUTPUT_PATH)/*.o *.o -L$(OUTPUT_PATH) $(LIBS)

	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf $(OUTPUT_PATH)/$(FIRMWARE_NAME).eep 
	$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .lock -R .signature $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf $(OUTPUT_PATH)/$(FIRMWARE_NAME).hex



program: $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf
	$(TEENSY_LOADER_PATH) -mmcu=mk20dx128 -w -v $(OUTPUT_PATH)/$(FIRMWARE_NAME).hex


fullclean:
	rm -f *.d *.o *.su *~
	rm -rf doc/doxygen/*
	rm -rf $(OUTPUT_PATH)

clean:
	rm -f *.d *.o *.su *~
	rm -rf $(OUTPUT_PATH)

doc:
	mkdir -p doc/doxygen/
	doxygen Doxyfile

stats:
	find ./src -type f \( -name \*.cpp -o -name \*.h \) -exec wc -l {} +
