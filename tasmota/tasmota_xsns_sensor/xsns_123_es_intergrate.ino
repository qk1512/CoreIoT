#ifdef USE_RS485
#ifdef USE_ES_SENSOR

#define XSNS_123 123
#define XRS485_22 22

#define ES_SENSOR_ADDRESS_ID 0x15
#define ES_SENSOR_ADDRESS_HUMIDITY 0x01F4
#define ES_SENSOR_ADDRESS_TEMPERATURE 0x01F5
#define ES_SENSOR_ADDRESS_NOISE 0x01F6
#define ES_SENSOR_ADDRESS_PM2_5 0x01F7
#define ES_SENSOR_ADDRESS_PM_10 0x01F8
#define ES_SENSOR_ADDRESS_ATMOSPHERE 0x01F9
#define ES_SENSOR_ADDRESS_AMBIENT_LIGHT 0x01FA
//#define ES_SENSOR_ADDRESS_AMBIENT_LIGHT_2 0x01FB
#define ES_SENSOR_DEVICE_ADDRESS 0x07D0
#define ES_SENSOR_DEVICE_BAUDRATE 0x07D1

#define ES_SENSOR_FUNCTION_CODE 0x03

struct ES_SENSORt
{
    bool valid = false;
    float humidity = 0.0;
    float temperature = 0.0;
    float noise_value = 0.0;
    int PM2_5 = 0;
    int PM10 = 0;
    float atmosphere_pressure = 0.0;
    int ambient_light = 0;
    char name[10] = "ES SENSOR";
}ESSENSOR;

bool ESSENSORisConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus -> Send(ES_SENSOR_ADDRESS_ID, ES_SENSOR_FUNCTION_CODE, (0x07 << 8) | 0xD0, 0x01);
    delay(200);

    RS485.Rs485Modbus -> ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,2);
    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("error %u"), error);
        return false;
    }
    else
    {
        uint16_t check_ESSENSOR = (buffer[3] << 8) | buffer[4];
        if(check_ESSENSOR == ES_SENSOR_ADDRESS_ID) return true;
    }
    return false;
}

void ESSENSORInit(void)
{
    if(!RS485.active) return;
    ESSENSOR.valid = ESSENSORisConnected();
    //if(!ESSENSOR.valid) TasmotaGloabl.restart_flag = 2;
    if(ESSENSOR.valid) Rs485SetActiveFound(ES_SENSOR_ADDRESS_ID, ESSENSOR.name);
    AddLog(LOG_LEVEL_INFO, PSTR(ESSENSOR.valid ? "ES Sensor is connected" : "ES Sensor is not detected"));
}

void ESSENSORReadData(void)
{
    if(ESSENSOR.valid == false) return;

    if(isWaitingResponse(ES_SENSOR_ADDRESS_ID)) return;

    if(RS485.requestSent[ES_SENSOR_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus -> Send(ES_SENSOR_ADDRESS_ID, ES_SENSOR_FUNCTION_CODE, ES_SENSOR_ADDRESS_HUMIDITY, 8);
        RS485.requestSent[ES_SENSOR_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if((RS485.requestSent[ES_SENSOR_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        if(RS485.Rs485Modbus -> ReceiveReady())
        {
            uint8_t buffer[25]; //(3 header + 8*2 data + 2CRC)
            uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,8);

            if(error)
            {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus ES Sensor error: %u"), error);
            }
            else
            {
                uint16_t humidity_raw = (buffer[3] << 8) | buffer[4];
                ESSENSOR.humidity = humidity_raw /10.0;

                uint16_t temperature_raw = (buffer[5] << 8) | buffer[6];
                ESSENSOR.temperature = temperature_raw/10.0;

                uint16_t noise_value_raw = (buffer[7] << 8) | buffer[8];
                ESSENSOR.noise_value = noise_value_raw / 10.0;

                uint16_t PM2_5_raw = (buffer[9] << 8) | buffer[10];
                ESSENSOR.PM2_5 = PM2_5_raw;

                uint16_t PM10_raw = (buffer[11] << 8) | buffer[12];
                ESSENSOR.PM10 = PM10_raw;
                
                uint16_t atmosphere_pressure_raw = (buffer[13] << 8) | buffer[14];
                ESSENSOR.atmosphere_pressure = atmosphere_pressure_raw / 10.0;

                uint32_t ambient_light_raw = ((buffer[15] << 24 ) | (buffer[16] << 16) |
                                                buffer[17] << 6 | buffer[18]); 
                ESSENSOR.ambient_light = ambient_light_raw / 10.0;
            }
            RS485.requestSent[ES_SENSOR_ADDRESS_ID] = 0;
            RS485.lastRequestTime = 0;
        }
    }
}

const char HTTP_SNS_ES_SENSOR_TEMPERATURE[] PROGMEM = "{s} Temperature {m} %.1f°C";
const char HTTP_SNS_ES_SENSOR_HUMIDITY[] PROGMEM = "{s} Humidity {m} %.1f%RH";
const char HTTP_SNS_ES_SENSOR_NOISE[] PROGMEM = "{s} Noise value {m} %.1fdB";
const char HTTP_SNS_ES_SENSOR_PM_2_5[] PROGMEM = "{s} PM2.5 {m} %dppm";
const char HTTP_SNS_ES_SENSOR_PM10[] PROGMEM = "{s} PM10 {m} %dppm";
const char HTTP_SNS_ES_SENSOR_ATMOSPHERE[] PROGMEM = "{s} Atmosphere pressure {m} %.1fkPa";
const char HTTP_SNS_ES_SENSOR_AMBIENT[] PROGMEM = "{s} Ambient Light{m} %dLux";

#define D_JSON_ES_SENSOR_TEMPERATURE "ES Sensor Temperature"
#define D_JSON_ES_SENSOR_HUMIDITY "ES Sensor Humidity"
#define D_JSON_ES_SENSOR_NOISE "ES Sensor Noise"
#define D_JSON_ES_SENSOR_PM_2_5 "ES Sensor PM2.5"
#define D_JSON_ES_SENSOR_PM10 "ES Sensor PM10"
#define D_JSON_ES_SENSOR_ATMOSPHERE "ES Sensor Atmosphere"
#define D_JSON_ES_SENSOR_AMBIENT "ES Sensor Ambient"

void ESSENSORShow(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), ESSENSOR.name);
        ResponseAppend_P(PSTR(
            "\"" D_JSON_ES_SENSOR_HUMIDITY "\":%.1f," 
            "\"" D_JSON_ES_SENSOR_TEMPERATURE "\":%.1f,"
            "\"" D_JSON_ES_SENSOR_NOISE "\":%.1f,"
            "\"" D_JSON_ES_SENSOR_PM_2_5 "\":%d,"
            "\"" D_JSON_ES_SENSOR_PM10 "\":%d," 
            "\"" D_JSON_ES_SENSOR_ATMOSPHERE "\":%.1f,"
            "\"" D_JSON_ES_SENSOR_AMBIENT "\":%d"
            ),
            ESSENSOR.humidity,
            ESSENSOR.temperature,
            ESSENSOR.noise_value,
            ESSENSOR.PM2_5,
            ESSENSOR.PM10,
            ESSENSOR.atmosphere_pressure,
            ESSENSOR.ambient_light);
        ResponseJsonEnd();
    }
    else
#ifdef USE_WEBSERVER
    {
        WSContentSend_PD(HTTP_SNS_ES_SENSOR_TEMPERATURE, ESSENSOR.temperature);
        WSContentSend_PD(HTTP_SNS_ES_SENSOR_HUMIDITY, ESSENSOR.humidity);
        WSContentSend_PD(HTTP_SNS_ES_SENSOR_NOISE, ESSENSOR.noise_value);
        WSContentSend_PD(HTTP_SNS_ES_SENSOR_PM_2_5, ESSENSOR.PM2_5);
        WSContentSend_PD(HTTP_SNS_ES_SENSOR_PM10, ESSENSOR.PM10);
        WSContentSend_PD(HTTP_SNS_ES_SENSOR_ATMOSPHERE, ESSENSOR.atmosphere_pressure);
        WSContentSend_PD(HTTP_SNS_ES_SENSOR_AMBIENT, ESSENSOR.ambient_light);
    }
#endif
}

bool Xsns123(uint32_t function)
{
    if(!Rs485Enabled(XRS485_22)) return false;

    bool result = false;
    if(FUNC_INIT == function)
    {
        ESSENSORInit();
    }
    else if(ESSENSOR.valid)
    {
        switch (function)
        {
        case FUNC_EVERY_250_MSECOND:
            ESSENSORReadData();
            break;
        case FUNC_JSON_APPEND:
            ESSENSORShow(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            ESSENSORShow(0);
            break;
#endif
        }
    }
    return result;
}

#endif
#endif