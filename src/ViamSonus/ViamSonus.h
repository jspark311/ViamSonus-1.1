/*
File:   ViamSonus.h
Author: J. Ian Lindsay
Date:   2016.09.11

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef __VIAM_SONUS_MODULE_H__
#define __VIAM_SONUS_MODULE_H__

#include <Platform/Platform.h>

/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define VIAMSONUS_FLAG_RESERVED_7       0x01    //
#define VIAMSONUS_FLAG_RESERVED_6       0x02    //
#define VIAMSONUS_FLAG_RESERVED_5       0x04    //
#define VIAMSONUS_FLAG_RESERVED_4       0x08    //
#define VIAMSONUS_FLAG_RESERVED_3       0x10    //
#define VIAMSONUS_FLAG_RESERVED_2       0x20    //
#define VIAMSONUS_FLAG_RESERVED_1       0x40    //
#define VIAMSONUS_FLAG_RESERVED_0       0x80    //


class ViamSonus : public EventReceiver {
  public:
    ViamSonus();
    ViamSonus(Argument*);
    ~ViamSonus();

    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);
    int8_t erConfigure(Argument*);


  protected:
    int8_t bootComplete();


  private:
};

#endif   // __VIAM_SONUS_MODULE_H__
