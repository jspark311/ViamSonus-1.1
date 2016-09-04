# This is not a CHANGELOG
It is a verbose running commentary with no relationship to versioning.
File should be read top-to-bottom for chronological order.
The contents of this file will be periodically archived and purged to keep the log related to the "current events" of the code base.

_---J. Ian Lindsay 2016.08.27_

------

### 2016.09.03

After a long period of neglect, I feel the need to fix ViamSonus and bring it into
alignment with the last year of changes to Manuvr. Most of this is related to Makefiles
and build options. But some of Manuvr's drivers were never updated.
Still unable to build. Not done modernizing...

Hey! I finished, and it's still today (23:33).
Committing...

------

### 2016.09.04
Still at it. Cleaning up loose-ends and knocking down TODO's in Manuvr.
Neopixels behave abnormally. Fixed a serious bug in Teensy3 platform, as well
  as a few in the Kernel.

     text    data     bss     dec     hex
    84040    3608    5128   92776   16a68   Current build size with DEBUG and no RTOS.
    84776    3608    5128   93512   16d48   Extensions to NeoPixel.
    84768    3608    5128   93504   16d40   Small fixes. About to optimize...
    84808    3608    5128   93544   16d68   Removal of pointless heap usage.
