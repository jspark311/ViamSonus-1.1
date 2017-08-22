###########################################################################
# Makefile for libraries
# Author: J. Ian Lindsay
###########################################################################
C_STANDARD      = gnu99
CPP_STANDARD    = gnu++11

SOURCES_C      =
SOURCES_CPP    =
LIBS_TO_BUILD  = libmanuvr.a
INCLUDES       = -Ii2c_t3

###########################################################################
# Source files
###########################################################################

FREERTOS_C_SRC     = FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/FreeRTOS_ARM.c
FREERTOS_C_SRC    += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/utility/croutine.c
FREERTOS_C_SRC    += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/utility/event_groups.c
FREERTOS_C_SRC    += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/utility/heap_3.c
FREERTOS_C_SRC    += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/utility/list.c
FREERTOS_C_SRC    += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/utility/port.c
FREERTOS_C_SRC    += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/utility/queue.c
FREERTOS_C_SRC    += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/utility/tasks.c
FREERTOS_C_SRC    += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/utility/timers.c

FREERTOS_CPP_SRC   = FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/assertMsg.cpp
FREERTOS_CPP_SRC  += FreeRTOS_Arduino/libraries/FreeRTOS_ARM/src/basic_io_arm.cpp

TEENSY3_C_SRC  = $(TEENSY_PATH)/cores/teensy3/usb_dev.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/usb_seremu.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/keylayouts.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/serial2.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/analog.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/usb_serial.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/usb_desc.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/eeprom.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/usb_mem.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/serial1.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/math_helper.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/serial3.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/pins_teensy.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/mk20dx128.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/touch.c
TEENSY3_C_SRC += $(TEENSY_PATH)/cores/teensy3/nonstd.c

TEENSY3_CPP_SRC  = $(TEENSY_PATH)/cores/teensy3/Print.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/DMAChannel.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/yield.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/WString.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/WMath.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/usb_inst.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/Tone.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/Stream.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/avr_emulation.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/new.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/HardwareSerial1.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/HardwareSerial2.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/HardwareSerial3.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/main.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/IntervalTimer.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/cores/teensy3/AudioStream.cpp

TEENSY3_CPP_SRC += $(TEENSY_PATH)/libraries/Time/DateStrings.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/libraries/Time/Time.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/libraries/SPI/SPI.cpp
TEENSY3_CPP_SRC += $(TEENSY_PATH)/libraries/Encoder/Encoder.cpp

# TODO: Until Teensyduino is pulled into /lib...
TEENSY3_CPP_SRC += $(TEENSY_PATH)libraries/i2c_t3/i2c_t3.cpp
#TEENSY3_CPP_SRC += i2c_t3/i2c_t3.cpp

TEENSY_AUDIO_C  = Audio/data_ulaw.c
TEENSY_AUDIO_C += Audio/data_waveforms.c
TEENSY_AUDIO_C += Audio/data_windows.c

TEENSY_AUDIO_CPP  = Audio/analyze_fft1024.cpp
TEENSY_AUDIO_CPP += Audio/analyze_fft256.cpp
TEENSY_AUDIO_CPP += Audio/analyze_notefreq.cpp
TEENSY_AUDIO_CPP += Audio/analyze_peak.cpp
TEENSY_AUDIO_CPP += Audio/analyze_print.cpp
TEENSY_AUDIO_CPP += Audio/analyze_rms.cpp
TEENSY_AUDIO_CPP += Audio/analyze_tonedetect.cpp
TEENSY_AUDIO_CPP += Audio/control_ak4558.cpp
TEENSY_AUDIO_CPP += Audio/control_cs4272.cpp
TEENSY_AUDIO_CPP += Audio/control_sgtl5000.cpp
TEENSY_AUDIO_CPP += Audio/control_wm8731.cpp
TEENSY_AUDIO_CPP += Audio/effect_bitcrusher.cpp
TEENSY_AUDIO_CPP += Audio/effect_chorus.cpp
TEENSY_AUDIO_CPP += Audio/effect_delay.cpp
TEENSY_AUDIO_CPP += Audio/effect_delay_ext.cpp
TEENSY_AUDIO_CPP += Audio/effect_envelope.cpp
TEENSY_AUDIO_CPP += Audio/effect_fade.cpp
TEENSY_AUDIO_CPP += Audio/effect_flange.cpp
TEENSY_AUDIO_CPP += Audio/effect_midside.cpp
TEENSY_AUDIO_CPP += Audio/effect_multiply.cpp
TEENSY_AUDIO_CPP += Audio/filter_biquad.cpp
TEENSY_AUDIO_CPP += Audio/filter_fir.cpp
TEENSY_AUDIO_CPP += Audio/filter_variable.cpp
TEENSY_AUDIO_CPP += Audio/input_adc.cpp
TEENSY_AUDIO_CPP += Audio/input_i2s.cpp
TEENSY_AUDIO_CPP += Audio/input_i2s_quad.cpp
TEENSY_AUDIO_CPP += Audio/mixer.cpp
TEENSY_AUDIO_CPP += Audio/output_dac.cpp
TEENSY_AUDIO_CPP += Audio/output_i2s.cpp
TEENSY_AUDIO_CPP += Audio/output_pt8211.cpp
TEENSY_AUDIO_CPP += Audio/output_pwm.cpp
TEENSY_AUDIO_CPP += Audio/output_spdif.cpp
TEENSY_AUDIO_CPP += Audio/play_memory.cpp
TEENSY_AUDIO_CPP += Audio/play_queue.cpp
#TEENSY_AUDIO_CPP += Audio/output_i2s_quad.cpp
TEENSY_AUDIO_CPP += Audio/play_sd_raw.cpp
TEENSY_AUDIO_CPP += Audio/play_sd_wav.cpp
TEENSY_AUDIO_CPP += Audio/play_serialflash_raw.cpp
TEENSY_AUDIO_CPP += Audio/record_queue.cpp
TEENSY_AUDIO_CPP += Audio/spi_interrupt.cpp
TEENSY_AUDIO_CPP += Audio/synth_dc.cpp
TEENSY_AUDIO_CPP += Audio/synth_karplusstrong.cpp
TEENSY_AUDIO_CPP += Audio/synth_pinknoise.cpp
TEENSY_AUDIO_CPP += Audio/synth_simple_drum.cpp
TEENSY_AUDIO_CPP += Audio/synth_sine.cpp
TEENSY_AUDIO_CPP += Audio/synth_tonesweep.cpp
TEENSY_AUDIO_CPP += Audio/synth_waveform.cpp
TEENSY_AUDIO_CPP += Audio/synth_whitenoise.cpp


###########################################################################
# Option conditionals
###########################################################################
ifeq ($(THREADS),1)
	SOURCES_CPP += $(FREERTOS_CPP_SRC)
	SOURCES_C   += $(FREERTOS_C_SRC)
endif

ifeq ($(SECURE),1)
	LIBS_TO_BUILD += mbedtls.a
endif


###########################################################################
# Parameter unification and make targets
###########################################################################
SOURCES_CPP += $(TEENSY3_CPP_SRC) $(TEENSY_AUDIO_CPP)
SOURCES_C   += $(TEENSY3_C_SRC) $(TEENSY_AUDIO_C)

LOCAL_C_FLAGS   = $(CFLAGS) $(INCLUDES) -nostdlib
LOCAL_CXX_FLAGS = $(CXXFLAGS) $(INCLUDES)

OBJS  = $(SOURCES_C:.c=.o) $(SOURCES_CPP:.cpp=.o)


all: $(LIBS_TO_BUILD) $(OBJS)
	$(AR) -rvs $(OUTPUT_PATH)/libextras.a $(OBJS)

%.o : %.c
	$(CC) $(INCLUDES) $(LOCAL_C_FLAGS) -std=$(C_STANDARD) -c $< -o $@

%.o : %.cpp
	$(CXX) $(INCLUDES) $(LOCAL_CXX_FLAGS) -std=$(CPP_STANDARD) -c $< -o $@

libmanuvr.a:
	$(MAKE) -C ManuvrOS/ManuvrOS

mbedtls.a:
	$(MAKE) lib -C mbedtls
	cp -RP mbedtls/library/*.a $(OUTPUT_PATH)

clean:
	rm -f $(OBJS) $(OUTPUT_PATH)/libextras.a
	$(MAKE) clean -C mbedtls
	$(MAKE) clean -C ManuvrOS/ManuvrOS
