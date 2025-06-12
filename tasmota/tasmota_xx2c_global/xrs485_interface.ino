/*
  xrs485_interface.ino - RS485 interface support for Tasmota

  Copyright (C) 2021  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_RS485

#ifdef XFUNC_PTR_IN_ROM
const uint8_t kRs485List[] PROGMEM = {
#else
const uint8_t kRs485List[] = {
#endif

#ifdef XRS485_01
    XRS485_01,
#endif

#ifdef XRS485_02
    XRS485_02,
#endif

#ifdef XRS485_03
    XRS485_03,
#endif

#ifdef XRS485_04
    XRS485_04,
#endif

#ifdef XRS485_05
    XRS485_05,
#endif

#ifdef XRS485_06
    XRS485_06,
#endif

#ifdef XRS485_07
    XRS485_07,
#endif

#ifdef XRS485_08
    XRS485_08,
#endif

#ifdef XRS485_09
    XRS485_09,
#endif

#ifdef XRS485_10
    XRS485_10,
#endif

#ifdef XRS485_11
    XRS485_11,
#endif

#ifdef XRS485_12
    XRS485_13,
#endif

#ifdef XRS485_13
    XRS485_13,
#endif

#ifdef XRS485_14
    XRS485_14,
#endif

#ifdef XRS485_15
    XRS485_15,
#endif

#ifdef XRS485_16
    XRS485_16,
#endif

#ifdef XRS485_17
    XRS485_17,
#endif

#ifdef XRS485_18
    XRS485_18,
#endif

#ifdef XRS485_19
    XRS485_19,
#endif

#ifdef XRS485_20
    XRS485_20,
#endif

#ifdef XRS485_21
    XRS485_21,
#endif

#ifdef XRS485_22
    XRS485_22,
#endif

#ifdef XRS485_23
    XRS485_23,
#endif

#ifdef XRS485_24
    XRS485_24,
#endif

#ifdef XRS485_25
    XRS485_25,
#endif

#ifdef XRS485_26
    XRS485_26,
#endif

#ifdef XRS485_27
    XRS485_27,
#endif

#ifdef XRS485_28
    XRS485_28,
#endif

#ifdef XRS485_29
    XRS485_29,
#endif

#ifdef XRS485_30
    XRS485_30,
#endif

#ifdef XRS485_31
    XRS485_31,
#endif

#ifdef XRS485_32
    XRS485_32,
#endif

#ifdef XRS485_33
    XRS485_33,
#endif

#ifdef XRS485_34
    XRS485_34,
#endif

#ifdef XRS485_35
    XRS485_35,
#endif

#ifdef XRS485_36
    XRS485_36,
#endif

#ifdef XRS485_37
    XRS485_37,
#endif

#ifdef XRS485_38
    XRS485_38,
#endif

#ifdef XRS485_39
    XRS485_39,
#endif

#ifdef XRS485_40
    XRS485_40
#endif
};

bool Rs485Enabled(uint32_t rs485_index)
{
    return (TasmotaGlobal.rs485_enabled && bitRead(Settings -> rs485_drivers[rs485_index/32], rs485_index % 32));
}

void Rs485DriverState(void)
{
    ResponseAppend_P(PSTR("\""));
    for(uint8_t i = 0; i < sizeof(kRs485List); i++)
    {
#ifdef XFUNC_PTR_IN_ROM
        uint32_t rs485_driver_id = pgm_read_byte(kRs485List + i);
#else   
        uint32_t rs485_driver_id = kRs485List[i];
#endif
        bool disabled = false;
        if(rs485_driver_id < MAX_RS485_DRIVERS)
        {
            disabled = !bitRead(Settings -> rs485_drivers[rs485_driver_id / 32], rs485_driver_id %32);
        }
        ResponseAppend_P(PSTR("%s%s%d"), (i) ? "," : "", (disabled) ? "!" : "", rs485_driver_id);
    }
    ResponseAppend_P(PSTR("\""));
}

#endif