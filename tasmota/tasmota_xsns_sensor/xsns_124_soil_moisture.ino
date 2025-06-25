/*
  xsns_120_soilmoisture.ino - Soil Moisture Sensor using Modbus support for Tasmota

  Copyright (C) 2023  Norbert Richter

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
#ifdef USE_SOILMOISTURE

#define XSNS_124 124
#define XRS485_11 11
/*
Inquiry Frame
--------------------------------------------------------------------------------------
Address Code |Function Code |Reg start address |Reg length |CRC_L |CRC_H  |
-------------|--------------|------------------|-----------|------|-------|-----------
1byte        |1byte         |2bytes            |2bytest    |1byte |1byte  |
--------------------------------------------------------------------------------------

Answer Frame
-------------------------------------------------------------------------------------------------------
Address Code |Function Code |Enumber of bytes |Data area |Second Data Area |Nth Data Area |Check code
-------------|--------------|-----------------|----------|-----------------|--------------|------------
1byte        |1byte         |1byte            |2bytes    |2bytes           |2bytes        |2bytes
-------------------------------------------------------------------------------------------------------

--------------------------------------------------------
PH Values (unit 0.01pH)          |0006H                |
---------------------------------|---------------------|
Soil Moisture(unit 0.1%RH)       |0012H                |
---------------------------------|---------------------|
Soil Temperature(unit 0.1°C)     |0013H                |
---------------------------------|---------------------|
Soil Conductivity(unit 1us/cm)   |0015H                |
---------------------------------|---------------------|
Soil Nitrogen (unit mg/kg)       |001EH                |
---------------------------------|---------------------|
Soil Phosphorus (unit mg/kg)     |001FH                |
---------------------------------|---------------------|
Soil Potassium (unit mg/kg)      |0020H                |
--------------------------------------------------------

This transmitter only uses function code 0x03 (read register data)
*/

struct SoilMoisture
{
    float temperature = 0;
    float PH_values = 0;
    float soil_moisture = 0;
    int soil_conductivity = 0;
    int soil_nitrogen = 0;
    int soil_phosphorus = 0;
    int soil_potassium = 0;

    char name[14] = "SOIL MOISTURE";
    bool valid = false;
} SM_sensor;

#define SM_ADDRESS_ID 0x1E
#define SM_ADDRESS_CHECK 0x0100
#define SM_FUNCTION_CODE 0x03

bool SMisConnected()
{
    if (!RS485.active)
        return false;
    RS485.Rs485Modbus->Send(0x1E, SM_FUNCTION_CODE, (0x01 << 8) | 0x00, 0x01);

    delay(200);

    RS485.Rs485Modbus->ReceiveReady();
    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);
    if (error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("error %d"), error);
        return false;
    }
    else
    {
        uint16_t check_SM = (buffer[3] << 8) | buffer[4];
        if (check_SM == SM_ADDRESS_ID) return true;
    }
    return false;
}

// static int count_restart = 0;
void SMInit(void)
{
    if (!RS485.active)
    {
        return;
    }
    SM_sensor.valid = SMisConnected();

    //if (!SM_sensor.valid) TasmotaGlobal.restart_flag = 2;

    if (SM_sensor.valid) Rs485SetActiveFound(SM_ADDRESS_ID, SM_sensor.name);
    AddLog(LOG_LEVEL_INFO, PSTR(SM_sensor.valid ? "Soil Moisture is connected" : "Soil Moisture is not detected"));
}

void SMReadData(void)
{

    if (SM_sensor.valid == false) return;

    if (isWaitingResponse(SM_ADDRESS_ID)) return;

    static const struct
    {
        uint16_t regAddr; // Start register address
        uint8_t regCount; // Number of registers to read
    } modbusRequests[] = {
        {0x0006, 1}, // PH Value
        {0x0012, 2}, // Soil Moisture & Temperature
        {0x0015, 1}, // Soil Conductivity
        {0x001E, 3}  // Nitrogen, Phosphorus, Potassium
    };

    static uint8_t requestIndex = 0;

    if (RS485.requestSent[SM_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus->Send(SM_ADDRESS_ID, SM_FUNCTION_CODE, modbusRequests[requestIndex].regAddr, modbusRequests[requestIndex].regCount);
        RS485.requestSent[SM_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[SM_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        if (RS485.Rs485Modbus->ReceiveReady())
        {
            uint8_t buffer[10]; // Max expected response size
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

            if (error)
            {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus Error: %u"), error);
            }
            else
            {
                switch (requestIndex)
                {
                case 0: // PH Value
                    SM_sensor.PH_values = ((buffer[3] << 8) | buffer[4]) / 100.0;
                    break;
                case 1: // Soil Moisture & Temperature
                    SM_sensor.soil_moisture = ((buffer[3] << 8) | buffer[4]) / 10.0;
                    SM_sensor.temperature = ((buffer[5] << 8) | buffer[6]) / 10.0;
                    break;
                case 2: // Soil Conductivity
                    SM_sensor.soil_conductivity = (buffer[3] << 8) | buffer[4];
                    break;
                case 3: // Nitrogen, Phosphorus, Potassium
                    SM_sensor.soil_nitrogen = (buffer[3] << 8) | buffer[4];
                    SM_sensor.soil_phosphorus = (buffer[5] << 8) | buffer[6];
                    SM_sensor.soil_potassium = (buffer[7] << 8) | buffer[8];
                    break;
                }
            }
            requestIndex = (requestIndex + 1) % (sizeof(modbusRequests) / sizeof(modbusRequests[0]));

            RS485.requestSent[SM_ADDRESS_ID] = 0;
            RS485.lastRequestTime = 0;
        }
    }
}

const char HTTP_SNS_SM_PH[] PROGMEM = "{s} SOIL MOISTURE PH Values {m} %.1f pH";
const char HTTP_SNS_SM_MOISTURE[] PROGMEM = "{s} SOIL MOISTURE Soil Moisture {m} %.1f %% RH";
const char HTTP_SNS_SM_TEMP[] PROGMEM = "{s} SOIL MOISTURE Soil Temperature {m} %.1f °C";
const char HTTP_SNS_SM_CONDUCTIVITY[] PROGMEM = "{s} SOIL MOISTURE Soil Conductivity {m} %d us/cm";
const char HTTP_SNS_SM_NITRO[] PROGMEM = "{s} SOIL MOISTURE Soil Nitrogen {m} %d mg/kg";
const char HTTP_SNS_SM_PHOSPHORUS[] PROGMEM = "{s} SOIL MOISTURE Soil Phosphorus {m} %d mg/kg";
const char HTTP_SNS_SM_POTASSIUM[] PROGMEM = "{s} SOIL MOISTURE Soil Potassium {m} %d mg/kg";

#define D_JSON_SOIL_PH_VALUE "PHValue"
#define D_JSON_SOIL_MOISTURE "Moisture"
#define D_JSON_SOIL_TEMPERATURE "Temperature"
#define D_JSON_SOIL_CONDUCTIVITY "Conductivity"
#define D_JSON_SOIL_NITROGEN "Nitrogen"
#define D_JSON_SOIL_PHOSPHORUS "Phosphorus"
#define D_JSON_SOIL_POTASSIUM "Potassium"

void SMShow(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), SM_sensor.name);

        ResponseAppend_P(PSTR("\"" D_JSON_SOIL_PH_VALUE "\":%.1f,"), SM_sensor.PH_values);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL_MOISTURE "\":%.1f,"), SM_sensor.soil_moisture);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL_TEMPERATURE "\":%.1f,"), SM_sensor.temperature);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL_CONDUCTIVITY "\":%d,"), SM_sensor.soil_conductivity);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL_NITROGEN "\":%d,"), SM_sensor.soil_nitrogen);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL_PHOSPHORUS "\":%d,"), SM_sensor.soil_phosphorus);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL_POTASSIUM "\":%d"), SM_sensor.soil_potassium);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_SM_PH, SM_sensor.PH_values);
        WSContentSend_PD(HTTP_SNS_SM_MOISTURE, SM_sensor.soil_moisture);
        WSContentSend_PD(HTTP_SNS_SM_TEMP, SM_sensor.temperature);
        WSContentSend_PD(HTTP_SNS_SM_CONDUCTIVITY, SM_sensor.soil_conductivity);
        WSContentSend_PD(HTTP_SNS_SM_NITRO, SM_sensor.soil_nitrogen);
        WSContentSend_PD(HTTP_SNS_SM_PHOSPHORUS, SM_sensor.soil_phosphorus);
        WSContentSend_PD(HTTP_SNS_SM_POTASSIUM, SM_sensor.soil_potassium);

#endif
    }
}

bool Xsns124(uint32_t function)
{
    if (!Rs485Enabled(XRS485_11))
        return false;
    bool result = false;
    if (FUNC_INIT == function)
    {
        SMInit();
    }
    else if (SM_sensor.valid)
    {
        switch (function)
        {
        case FUNC_EVERY_250_MSECOND:
            SMReadData();
            break;
        case FUNC_JSON_APPEND:
            SMShow(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            SMShow(0);
            break;
#endif
        }
    }
    return result;
}

#endif
#endif