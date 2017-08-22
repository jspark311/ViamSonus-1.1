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

###########################################################################
# Environmental awareness...
###########################################################################
WHO_I_AM       = $(shell whoami)
HOME_DIRECTORY = /home/$(WHO_I_AM)
ARDUINO_PATH   = $(HOME_DIRECTORY)/arduino
export TEENSY_PATH    = $(ARDUINO_PATH)/hardware/teensy/avr/

# This is where we will store compiled libs and the final output.
export BUILD_ROOT   = $(shell pwd)
export OUTPUT_PATH  = $(BUILD_ROOT)/build

TOOLCHAIN          = $(ARDUINO_PATH)/hardware/tools/arm
TEENSY_LOADER_PATH = $(BUILD_ROOT)/tools/teensy_loader_cli/teensy_loader_cli
FORMAT             = ihex

export CC      = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
export CXX     = $(TOOLCHAIN)/bin/arm-none-eabi-g++
export AR      = $(TOOLCHAIN)/bin/arm-none-eabi-ar
export AS      = $(TOOLCHAIN)/bin/arm-none-eabi-as
export OBJCOPY = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy
export SZ      = $(TOOLCHAIN)/bin/arm-none-eabi-size
export MAKE    = $(shell which make)


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
CXXFLAGS    = -felide-constructors -fno-exceptions -fno-rtti
CFLAGS       = -Wall -nostdlib
LIBS         = -lm -larm_cortexM4l_math -lmanuvr -lextras
LD_FILE      = $(TEENSY_PATH)cores/teensy3/mk20dx256.ld

INCLUDES     = -iquote. -iquotesrc/
INCLUDES    += -I./ -Isrc/
INCLUDES    += -I$(TEENSY_PATH)/libraries -I$(ARDUINO_PATH)/libraries
INCLUDES    += -I$(TEENSY_PATH)/cores/teensy3
INCLUDES    += -I$(TEENSY_PATH)/libraries/EEPROM
INCLUDES    += -I$(TEENSY_PATH)/libraries/SPI
INCLUDES    += -I$(TEENSY_PATH)/libraries/Wire
INCLUDES    += -I$(TEENSY_PATH)/libraries/SD
INCLUDES    += -I$(TEENSY_PATH)/libraries/SerialFlash
INCLUDES    += -I$(BUILD_ROOT)/lib
INCLUDES    += -I$(BUILD_ROOT)/confs
INCLUDES    += -I$(BUILD_ROOT)/lib/ManuvrOS/ManuvrOS
INCLUDES    += -I$(BUILD_ROOT)/lib/Audio/utility
INCLUDES    += -I$(BUILD_ROOT)/lib/mbedtls/include

CFLAGS += -DF_CPU=$(CPU_SPEED)
CFLAGS += -mcpu=$(MCU)  -mthumb -D__MK20DX256__

CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -mlittle-endian
CFLAGS += -mfloat-abi=soft
CFLAGS += -DARDUINO=105 -DTEENSYDUINO=120
CFLAGS += -DUSB_VID=null -DUSB_PID=null -DUSB_SERIAL -DLAYOUT_US_ENGLISH

export MANUVR_PLATFORM = TEENSY3

###########################################################################
# Option conditionals
###########################################################################

# Options that build for certain threading models (if any).
ifeq ($(THREADS),1)
INCLUDES       += -I$(BUILD_ROOT)/lib/FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src
INCLUDES       += -I$(BUILD_ROOT)/lib/FreeRTOS_Arduino/libraries/SdFat/src
MANUVR_OPTIONS += -D__MANUVR_FREERTOS
export THREADS=1
endif

# Options for various security features.
ifeq ($(SECURE),1)
MANUVR_OPTIONS += -DWITH_MBEDTLS
# mbedTLS will require this in order to use our chosen options.
#LIBS  = -lm -larm_cortexM4l_math
LIBS += $(OUTPUT_PATH)/libmbedtls.a
LIBS += $(OUTPUT_PATH)/libmbedx509.a
LIBS += $(OUTPUT_PATH)/libmbedcrypto.a
#LIBS += -lmanuvr -lextras
MANUVR_OPTIONS += -DMBEDTLS_CONFIG_FILE='<mbedTLS_conf.h>'
export SECURE=1
endif


###########################################################################
# Source file definitions...
###########################################################################
SOURCES_CPP  = src/main.cpp src/ViamSonus/ViamSonus.cpp


###########################################################################
# exports, consolidation....
###########################################################################
OBJS  = $(SOURCES_C:.c=.o) $(SOURCES_CPP:.cpp=.o)

# Merge our choices and export them to the downstream Makefiles...
CFLAGS += $(MANUVR_OPTIONS) $(OPTIMIZATION) $(INCLUDES)

export CFLAGS
export CXXFLAGS  += $(CFLAGS)


###########################################################################
# Rules for building the firmware follow...
###########################################################################

.PHONY:  lib $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf

all:  lib $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf

%.o : %.cpp
	$(CXX) -std=$(CPP_STANDARD) $(OPTIMIZATION) $(CXXFLAGS) -c -o $@ $^

%.o : %.c
	$(CC) -std=$(C_STANDARD) $(OPTIMIZATION) $(CFLAGS) -c -o $@ $^

lib: $(OBJS)
	mkdir -p $(OUTPUT_PATH)
	$(MAKE) -C lib

$(OUTPUT_PATH)/$(FIRMWARE_NAME).elf:
	$(CXX) $(OBJS) -Wl,--gc-sections -T$(LD_FILE) -mcpu=$(MCU) -mthumb -o $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf -L$(OUTPUT_PATH) $(LIBS)
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf $(OUTPUT_PATH)/$(FIRMWARE_NAME).eep
	$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .lock -R .signature $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf $(OUTPUT_PATH)/$(FIRMWARE_NAME).hex
	$(SZ) $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf

program: $(OUTPUT_PATH)/$(FIRMWARE_NAME).elf
	$(TEENSY_LOADER_PATH) -mmcu=mk20dx128 -w -v $(OUTPUT_PATH)/$(FIRMWARE_NAME).hex

clean:
	rm -f *.d *.o *.su *~ $(OBJS)

fullclean: clean
	rm -rf $(OUTPUT_PATH)
	$(MAKE) clean -C lib
