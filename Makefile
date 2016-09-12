###########################################################################
# Makefile for ViamSonus-1.1
# Author: J. Ian Lindsay
# Date:   2015.03.06
#
###########################################################################
FIRMWARE_NAME      = viamsonus

MCU                = cortex-m4
CPU_SPEED          = 96000000
OPTIMIZATION       = -Os
C_STANDARD         = gnu99
CPP_STANDARD       = gnu++11

WHO_I_AM       = $(shell whoami)
HOME_DIRECTORY = /home/$(WHO_I_AM)
ARDUINO_PATH   = $(HOME_DIRECTORY)/arduino
export TEENSY_PATH    = $(ARDUINO_PATH)/hardware/teensy/avr/


###########################################################################
# Variables for the firmware compilation...
###########################################################################
WHERE_I_AM         = $(shell pwd)
TOOLCHAIN          = $(ARDUINO_PATH)/hardware/tools/arm
TEENSY_LOADER_PATH = $(ARDUINO_PATH)/hardware/tools/teensy_loader_cli
FORMAT             = ihex

export CC      = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
export CXX     = $(TOOLCHAIN)/bin/arm-none-eabi-g++
export AR      = $(TOOLCHAIN)/bin/arm-none-eabi-ar
export AS      = $(TOOLCHAIN)/bin/arm-none-eabi-as
export OBJCOPY = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy
export SZ      = $(TOOLCHAIN)/bin/arm-none-eabi-size
export MAKE    = $(shell which make)

# This is where we will store compiled libs and the final output.
export OUTPUT_PATH  = $(WHERE_I_AM)/build



###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES     = -iquote. -iquotesrc/
INCLUDES    += -I./ -Isrc/
INCLUDES    += -I$(TEENSY_PATH)/libraries -I$(ARDUINO_PATH)/libraries
INCLUDES    += -I$(TEENSY_PATH)/cores/teensy3
INCLUDES    += -I$(WHERE_I_AM)/confs
INCLUDES    += -I$(WHERE_I_AM)/lib/ManuvrOS/ManuvrOS
INCLUDES    += -I$(WHERE_I_AM)/lib
INCLUDES    += -I$(WHERE_I_AM)/lib/FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src
INCLUDES    += -I$(WHERE_I_AM)/lib/FreeRTOS_Arduino/libraries/SdFat/src
INCLUDES    += -I$(WHERE_I_AM)/lib/Audio/utility
INCLUDES    += -I$(TEENSY_PATH)/libraries/SPI
INCLUDES    += -I$(TEENSY_PATH)/libraries/Wire
INCLUDES    += -I$(TEENSY_PATH)/libraries/EEPROM
INCLUDES    += -I$(TEENSY_PATH)/libraries/SD
INCLUDES    += -I$(TEENSY_PATH)/libraries/SerialFlash
INCLUDES    += -I$(WHERE_I_AM)/lib/mbedtls/include/

LD_FILE     = $(TEENSY_PATH)cores/teensy3/mk20dx256.ld

# Libraries to link
# TODO: CBOR-CPP is the _only_ thing requiring the standard library. Rectify...
LIBS = -lm -larm_cortexM4l_math -lmanuvr -lextras

# Wrap the include paths into the flags...
CFLAGS  = $(INCLUDES)
CFLAGS += -Wall

CFLAGS += -DF_CPU=$(CPU_SPEED)
CFLAGS += -mcpu=$(MCU)  -mthumb -D__MK20DX256__

CFLAGS += -fno-exceptions -ffunction-sections -fdata-sections
CFLAGS += -mlittle-endian
CFLAGS += -mfloat-abi=soft
CFLAGS += -DARDUINO=105 -nostdlib -DTEENSYDUINO=120
CFLAGS += -DUSB_VID=null -DUSB_PID=null -DUSB_SERIAL -DLAYOUT_US_ENGLISH

CPP_FLAGS  = -felide-constructors -fno-exceptions -fno-rtti


# Options that build for certain threading models (if any).
#MANUVR_OPTIONS += -D__MANUVR_FREERTOS
MANUVR_OPTIONS += -D__MANUVR_EVENT_PROFILER
MANUVR_OPTIONS += -D__MANUVR_CONSOLE_SUPPORT
MANUVR_OPTIONS += -DMANUVR_CBOR
MANUVR_OPTIONS += -DMANUVR_STORAGE

# Options for various security features.
ifeq ($(SECURE),1)
## mbedTLS will require this in order to use our chosen options.
#LIBS += $(OUTPUT_PATH)/libmbedtls.a
#LIBS += $(OUTPUT_PATH)/libmbedx509.a
#LIBS += $(OUTPUT_PATH)/libmbedcrypto.a
#MANUVR_OPTIONS += -D__MANUVR_MBEDTLS
#export MBEDTLS_CONFIG_FILE = $(WHERE_I_AM)/confs/mbedTLS_conf.h
#export SECURE=1
endif

# Debugging options...
ifeq ($(DEBUG),1)
MANUVR_OPTIONS += -D__MANUVR_DEBUG
#MANUVR_OPTIONS += -D__MANUVR_PIPE_DEBUG
endif


###########################################################################
# Source file definitions...
###########################################################################
CPP_SRCS  = src/main.cpp src/ViamSonus/ViamSonus.cpp


###########################################################################
# Rules for building the firmware follow...
###########################################################################
# Merge our choices and export them to the downstream Makefiles...
CFLAGS += $(MANUVR_OPTIONS) $(OPTIMIZATION)

export MANUVR_PLATFORM = TEENSY3
export CFLAGS
export CPP_FLAGS  += $(CFLAGS)


OBJS = $(SRCS:.c=.o)


.PHONY: lib $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf


all: lib $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf
	$(SZ) $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf


lib:
	mkdir -p $(OUTPUT_PATH)
	$(MAKE) -C lib


$(OUTPUT_PATH)/$(FIRMWARE_NAME).elf:
	$(CXX) -c $(CPP_FLAGS) $(CPP_SRCS) -std=$(CPP_STANDARD)
	$(CXX) -Wl,--gc-sections -T$(LD_FILE) -mcpu=$(MCU) -mthumb -o $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf *.o -L$(OUTPUT_PATH) $(LIBS)

	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf $(OUTPUT_PATH)/$(FIRMWARE_NAME).eep
	$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .lock -R .signature $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf $(OUTPUT_PATH)/$(FIRMWARE_NAME).hex


program: $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf
	$(TEENSY_LOADER_PATH) -mmcu=mk20dx128 -w -v $(OUTPUT_PATH)/$(FIRMWARE_NAME).hex


clean:
	rm -f *.d *.o *.su *~ $(OBJS)

fullclean: clean
	rm -rf doc/doxygen/*
	rm -rf $(OUTPUT_PATH)
	$(MAKE) clean -C lib
