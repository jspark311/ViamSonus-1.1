#!/bin/bash
#
# This script is meant to go fetch the most recent versions of various libraries that
#   ManuvrOS has been written against. None of this is strictly required for a basic build,
#   but most real-world applications will want at least one of them.

# Make the lib directory...
mkdir lib

# FreeRTOS...
rm -rf lib/FreeRTOS_Arduino
git clone https://github.com/greiman/FreeRTOS-Arduino lib/FreeRTOS_Arduino

# mbedTLS...
rm -rf lib/mbedtls
git clone https://github.com/ARMmbed/mbedtls.git lib/mbedtls

# Teensyduino libraries...
git clone https://github.com/PaulStoffregen/Audio.git lib/Audio
git clone https://github.com/PaulStoffregen/i2c_t3.git lib/i2c_t3
