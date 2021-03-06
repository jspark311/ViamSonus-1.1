     __     ___                 ____                        
     \ \   / (_) __ _ _ __ ___ / ___|  ___  _ __  _   _ ___ 
      \ \ / /| |/ _` | '_ ` _ \\___ \ / _ \| '_ \| | | / __|
       \ V / | | (_| | | | | | |___) | (_) | | | | |_| \__ \
        \_/  |_|\__,_|_| |_| |_|____/ \___/|_| |_|\__,_|___/


Viam Sonus *("Sound Route")* is an IoT audio multiplexer implemented on top of a Teensy3.1 running ManuvrOS.


----------------------
####What is in this repository:
**./3DModels**: Blender models of the physical pieces that comprise a finished unit.

**./doc**:  Documentation related to this project.
    
**./KiCAD**:  KiCAD project data for building a unit.

**./src**:  Original (unless otherwise specified) source code.


----------------------
####Building the device firmware 

I got sick of tracking the inconsistent behavior of the Arduino IDE, and so Viam Sonus was re-tooled to be buildable with a makefile against the Arduino libraries.

This project has ManuvrOS as a dependency. You will first need to add to this project's source tree:

    git clone https://github.com/Manuvr/ManuvrOS
    cd ManuvrOS*
    mv ManuvrOS StringBuilder DataStructures ..

You will also need the [Teensy3 enhanced i2c library](https://github.com/nox771/i2c_t3). Follow those installation proceedures to get better i2c support. Ultimately, this needs to be fixed in ManuvrOS.

One last step. You will need the [teensy_loader_cli program](https://github.com/PaulStoffregen/teensy_loader_cli) program to flash the controller with the new build. Build it, and place the binary in a place you deem logical, and then update the Makefile to reflect your choice.

The Makefile might need to be changed to reflect the location of your Arduino environment location.


----------------------
####Building the device firmware for a RasPi

This project relies on certain capabilities of the Teensy3.1, but a feature-restricted RasPi build is possible in-principle. If there enough interest, I will post instructions for it.


----------------------
Bitcoin accelerates feature requests and bug fixes. :-)

**16SeqCTSkx7tUj9kXKyzYuVMQ4ghs2HNK**


----------------------
####License
Original code is GPL v2. 

Code adapted from others' work inherrits their license terms, which were preserved in the commentary where it applies. 


----------------------
####Cred:
The ASCII art in this file was generated by [this most-excellent tool](http://patorjk.com/software/taag).

Some of the hardware drivers are adaptions from Adafruit or Teensyduino code. This is noted in each specific class so derived.

