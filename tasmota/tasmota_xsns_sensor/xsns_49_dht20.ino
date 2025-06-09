/*
  xsns_128_DHT20.ino - DHT20 temperature and humidity sensor support for Tasmota

  Copyright (C) 2025 Quang Khanh DO

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>

*/

/*
  This library is built based on https://github.com/RobTillaart/DHT20.git
  All functions of this library are used for educational purposes.
*/

#ifdef USE_I2C
#ifdef USE_DHT20
/*********************************************************************************************\
 * Sensirion I2C temperature and humidity sensors
 *
 * This driver supports the following sensors:
 * - DHT20 (address: 0x38)
\*********************************************************************************************/

#define XSNS_49 49
#define XI2C_93 93 // New define in I2CDevices.md

#define DHT20_OK 0
#define DHT20_ERROR_CHECKSUM -10
#define DHT20_ERROR_CONNECT -11
#define DHT20_MISSING_BYTES -12
#define DHT20_ERROR_BYTES_ALL_ZERO -13
#define DHT20_ERROR_READ_TIMEOUT -14
#define DHT20_ERRO_LASTREAD -15

#define DHT20_ADDR 0x38

struct DHT20t
{
    float temperature = NAN;
    float humidity = NAN;
    float humOffset = NAN;
    float tempOffset = NAN;
    char name[6] = "DHT20";

    uint8_t count = 0;
    uint8_t valid = 0;
    uint8_t status = 0;
    uint32_t _lastRequest = 0;
    uint32_t _lastRead = 0;
    uint8_t bits[7];

} DHT20;

int DHT20RequestData();
int DHT20ReadData();
bool DHT20Read();
bool DHT20Convert();

////////////////////////////////////////////////
//
//  STATUS
//

uint8_t DHT20ReadStatus()
{
    Wire.requestFrom(DHT20_ADDR, (uint8_t)1);
    delay(1);
    return (uint8_t)Wire.read();
}

bool DHT20isCalibrated()
{
    return (DHT20ReadStatus() & 0x08) == 0x08;
}

bool DHT20isMeasuring()
{
    return (DHT20ReadStatus() & 0x80) == 0x80;
}

bool DHT20isIdle()
{
    return (DHT20ReadStatus() & 0x80) == 0x00;
}

uint8_t DHT20ComputeCrc(uint8_t data[], uint8_t len)
{
    // Compute CRC as per datasheet
    uint8_t crc = 0xFF;

    for (uint8_t x = 0; x < len; x++)
    {
        crc ^= data[x];
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool DHT20isConnected()
{
    Wire.beginTransmission(DHT20_ADDR);
    int rv = Wire.endTransmission();
    return rv == 0;
}

bool DHT20Begin()
{
    return DHT20isConnected();
}

///////////////////////////////////////////////////
//
// READ THE SENSOR
//

bool DHT20Read(void)
{
    if (DHT20.valid)
    {
        DHT20.valid--;
    }

    if (millis() - DHT20._lastRead < 1000)
    {
        return false;
    }

    int status = DHT20RequestData();
    if (status < 0)
        return status;
    // wait for measurement ready
    uint32_t start = millis();
    while (DHT20isMeasuring())
    {
        if (millis() - start >= 1000)
        {
            return false;
        }
        yield;
    }
    // read the measurement
    status = DHT20ReadData();
    if (status < 0)
        return false;

    DHT20.valid = SENSOR_MAX_MISS;

    // CONVERT AND STORE
    DHT20.status = DHT20.bits[0];
    uint32_t raw = DHT20.bits[1];
    raw <<= 8;
    raw += DHT20.bits[2];
    raw <<= 4;
    raw += (DHT20.bits[3] >> 4);
    DHT20.humidity = raw * 9.5367431640625e-5; // ==> / 1048576.0 * 100%;

    raw = (DHT20.bits[3] & 0x0F);
    raw <<= 8;
    raw += DHT20.bits[4];
    raw <<= 8;
    raw += DHT20.bits[5];
    DHT20.temperature = raw * 1.9073486328125e-4 - 50; //  ==> / 1048576.0 * 200 - 50;

    if (isnan(DHT20.temperature) || isnan(DHT20.humidity))
        return false;

    // TEST CHECKSUM
    uint8_t crc = DHT20ComputeCrc(DHT20.bits, 6);
    if (crc != DHT20.bits[6])
        return false;

    return true;
}

int DHT20RequestData()
{
    Wire.beginTransmission(DHT20_ADDR);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    int rv = Wire.endTransmission();

    DHT20._lastRequest = millis();
    return rv;
}

int DHT20ReadData()
{
    const uint8_t length = 7;
    int bytes = Wire.requestFrom(DHT20_ADDR, length);

    if (bytes == 0)
        return DHT20_ERROR_CONNECT;
    if (bytes < length)
        return DHT20_MISSING_BYTES;

    bool allZero = true;
    for (int i = 0; i < bytes; i++)
    {
        DHT20.bits[i] = Wire.read();
        allZero = allZero && (DHT20.bits[i] == 0);
    }

    if (allZero)
        return DHT20_ERROR_BYTES_ALL_ZERO;

    DHT20._lastRead = millis();
    return bytes;
}

////////////////////////////////////////////////
//
//  TEMPERATURE & HUMIDITY & OFFSET
//

void DHT20Detect(void)
{
    if (!I2cSetDevice(DHT20_ADDR))
        return;

    AddLog(LOG_LEVEL_INFO, PSTR("Checking DHT20 at 0x38"));

    if (DHT20isConnected()) // Directly check if sensor responds
    {
        I2cSetActiveFound(DHT20_ADDR, DHT20.name);
        DHT20.count = 1;
        AddLog(LOG_LEVEL_INFO, PSTR("DHT20 detected!"));
    }
    else
    {
        AddLog(LOG_LEVEL_ERROR, PSTR("DHT20 NOT detected!"));
    }
}

void DHT20Show(bool json)
{
    if (DHT20.valid)
    {
        TempHumDewShow(json, (0 == TasmotaGlobal.tele_period), DHT20.name, DHT20.temperature, DHT20.humidity);
    }
}

void DHT20EverySecond(void)
{
    if (TasmotaGlobal.uptime & 1)
    {
        if (!DHT20Read())
        {
            AddLogMissed(DHT20.name, DHT20.valid);
        }
    }
}

bool Xsns49(uint32_t function)
{
    if (!I2cEnabled(XI2C_93))
    {
        // AddLog(LOG_LEVEL_INFO, PSTR("DHT20"));
        return false;
    }

    bool result = false;
    if (FUNC_INIT == function)
    {
        DHT20Detect();
    }
    else if (DHT20.count)
    {
        switch (function)
        {
        case FUNC_EVERY_SECOND:
            DHT20EverySecond();
            break;
        case FUNC_JSON_APPEND:
            DHT20Show(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            DHT20Show(0);
            break;
#endif
        }
    }
    return result;
}

#endif // USE_DHT20
#endif // USE_I2C