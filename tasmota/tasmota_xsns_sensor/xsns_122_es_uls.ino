#ifdef USE_RS485
#ifdef USE_ES_ULS

#define XSNS_122 122
#define XRS485_21 21

struct ESULSt
{
    bool valid = false;
    float display_value = 0.0;
    float temperature = 0.0;
    char name[11] = "ULTRASONIC";
}ESULS;

#define ULS_ADDRESS_ID 0x01
#define ULS_ADDRESS_DISPLAY_VALUE 0x0000 
//#define ULS_ADDRESS_DISPLAY_VALUE_2 0x0001
#define ULS_ADDRESS_TEMPERATURE 0x0002
//#define ULS_ADDRESS_TEMPERATURE_2 0x0003
#define ULS_ADDRESS_SUPPLY_VOLTAGE 0x0004
//#define ULS_ADDRESS_SUPPLY_VOLTAGE_2 0x0005
#define ULS_ADDRESS_OPERATING_TIME 0x0006
//#define ULS_ADDRESS_OPERATING_TIME_2 0x0007
#define ULS_ADDRESS_LIQUID_LEVEL 0x0008           //---- 1: liquid level, 0: object level
//#define ULS_ADDRESS_LIQUID_LEVEL_2 0x0009
#define ULS_ADDRESS_MOUNTING_HEIGHT 0x000A
//#define ULS_ADDRESS_MOUNTING_HEIGHT_2 0x000B
#define ULS_ADDRESS_DEVICE_ADDRESS 0x0014
//#define ULS_ADDRESS_DEVICE_ADDRESS_2 0x0015
#define ULS_ADDRESS_BAUD_RATE 0x0016
//#define ULS_ADDRESS_BAUD_RATE_2 0x0017
#define ULS_ADDRESS_UNITS 0x002C
//#define ULS_ADDRESS_UNITS_2 0x002D
#define ULS_ADDRESS_MODE 0x000E
//#define ULS_ADDRESS_MODE_2 0x000F

#define ULS_FUNCTION_CODE_READ 0x03
#define ULS_FUNCTION_CODE_WRITE 0x10


#define ULS_ADDRESS_ID_FLOAT 1.0f
#define ULS_MOUNTING_HEIGHT 2.0f
#define ULS_SETTING_MODE 1.0f

bool ESULSisConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus -> Send(ULS_ADDRESS_ID, ULS_FUNCTION_CODE_READ, ULS_ADDRESS_DEVICE_ADDRESS, 2);

    delay(200);

    RS485.Rs485Modbus -> ReceiveReady();

    uint8_t buffer[16];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,4);

    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("Ultrasonic error %u"), error);
        return false;
    }
    else
    {
        uint16_t check_ULS_1 = (buffer[3] << 8) | buffer[4];
        uint16_t check_ULS_2 = (buffer[5] << 8) | buffer[6];
        uint32_t check_ULS = (check_ULS_1 << 16) | check_ULS_2;
        float address_float;
        memcpy(&address_float, &check_ULS, sizeof(float));
        if(fabs(address_float - ULS_ADDRESS_ID_FLOAT) < 0.0001f) return true;
    }
    return false;
}

void SettingMountingHeight(float mounting_height)
{
    //float mounting_height = ULS_MOUNTING_HEIGHT;
    uint32_t mounting_height_raw;
    memcpy(&mounting_height_raw, &mounting_height, sizeof(float));
    uint16_t data_mounting_height[2] = {(uint16_t)(mounting_height_raw >> 16),
                                        (uint16_t)(mounting_height_raw & 0xFFFF)};

    RS485.Rs485Modbus->Send(ULS_ADDRESS_ID, ULS_FUNCTION_CODE_WRITE, ULS_ADDRESS_MOUNTING_HEIGHT, 0x02, data_mounting_height);

    delay(200);
    RS485.Rs485Modbus->ReceiveReady();
    uint8_t buffer[12];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 4);
    if (error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("Setting for MOUNTING HEIGHT has error %d"), error);
    }
    else
    {
        if (buffer[1] == 0x90)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Error Message for MOUNTING HEIGHT: %d"), buffer[2]);
        }
        else
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Setting for MOUNTING HEIGHT successfully"));
        }
    }
}

void SettingMode(float mode)
{   
    uint32_t mode_raw;
    memcpy(&mode_raw, &mode, sizeof(float));
    uint16_t data_mode[2] = {(uint16_t)(mode_raw >> 16),
                             (uint16_t)(mode_raw & 0xFFFF)};

    RS485.Rs485Modbus -> Send(ULS_ADDRESS_ID, ULS_FUNCTION_CODE_WRITE, ULS_ADDRESS_MODE, 0x02, data_mode);

    delay(200);

    RS485.Rs485Modbus -> ReceiveReady();

    uint8_t buffer[12];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer, 4);
    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("Setting for MODE has error %u"), error);
    }
    else
    {
        if(buffer[1] == 0x90)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Error Message for MODE: %u"), buffer[2]);
        }
        else
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Setting for MODE successfully"));
        }
    }
}

void SettingUints(float units)
{
    uint32_t units_raw;
    memcpy(&units_raw, &units, sizeof(float));
    uint16_t data_units[2] = {(uint16_t)(units_raw >> 16),
                              (uint16_t)(units_raw & 0xFFFF)};

    RS485.Rs485Modbus -> Send(ULS_ADDRESS_ID, ULS_FUNCTION_CODE_WRITE, ULS_ADDRESS_UNITS, 0x02, data_units);

    delay(200);

    RS485.Rs485Modbus -> ReceiveReady();

    uint8_t buffer[12];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,4);
    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("Setting for UNITS has error %u"), error);
    }
    else
    {
        if(buffer[1] == 0x90)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Error Message for UNITS: %u"),buffer[2]);
        }
        else
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Setting for UNITS successfully"));
        }
    }
}

void ESULSInit(void)
{
    if(!RS485.active) return;

    ESULS.valid = ESULSisConnected();
    //if(!ESULS.valid) TasmotaGlobal.restart_flag = 2;
    if(ESULS.valid)
    {
        Rs485SetActiveFound(ULS_ADDRESS_ID, ESULS.name);

        //Setting for Liquid level or Object Level
        SettingMode(ULS_SETTING_MODE);

        //Setting for Mounting height to measure liquid
        SettingMountingHeight(ULS_MOUNTING_HEIGHT);
    }
    AddLog(LOG_LEVEL_INFO, PSTR(ESULS.valid ? "Ultrasonic is connected" : "Ultrasonic is not connected"));
}

void ULSReadData(void)
{
    if(ESULS.valid == false) return;

    if(isWaitingResponse(ULS_ADDRESS_ID)) return;

    if(RS485.requestSent[ULS_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus -> Send(ULS_ADDRESS_ID, ULS_FUNCTION_CODE_READ, ULS_ADDRESS_DISPLAY_VALUE, 0x04);
        RS485.requestSent[ULS_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if((RS485.requestSent[ULS_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        if(RS485.Rs485Modbus -> ReceiveReady())
        {
            uint8_t buffer[12];
            uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,8);

            if(error)
            {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus ULS error: %u"), error);
            }
            else
            {
                uint32_t display_value_raw = ((buffer[3] << 24) | (buffer[4] << 16) | (buffer[5] << 8) | buffer[6]);
                uint32_t temperature_raw = ((buffer[7]) << 24 |
                                            (buffer[8]) << 16 |
                                            (buffer[9]) << 8  |
                                            buffer[10]);
                memcpy(&ESULS.temperature, &temperature_raw, sizeof(float));
                memcpy(&ESULS.display_value, &display_value_raw, sizeof(float)); 
            }
            RS485.requestSent[ULS_ADDRESS_ID] = 0;
            RS485.lastRequestTime = 0;
        }
    }
}

const char HTTP_SNS_ES_ULS_TEMP[] PROGMEM = "{s} Ultrasonic Temperature {m} %.2f°C";
const char HTTP_SNS_ES_ULS_VALUE[] PROGMEM = "{s} Ultrasonic Liquid Level {m} %.3fm";

#define D_JSON_ES_ULS_LIQUID_LEVEL "Ultrasonic Liquid Level"
#define D_JSON_ES_ULS_TEMPERATURE "Ultrasonic Temperature"

void ESULSShow(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"),ESULS.name);
        ResponseAppend_P(PSTR(
                "\"" D_JSON_ES_ULS_TEMPERATURE "\":%.2f,"
                "\"" D_JSON_ES_ULS_LIQUID_LEVEL"\":%.2f"),
                        ESULS.temperature, ESULS.display_value);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_ES_ULS_TEMP, ESULS.temperature);
        WSContentSend_PD(HTTP_SNS_ES_ULS_VALUE, ESULS.display_value);
    }
#endif
}


bool Xsns122(uint32_t function)
{
    if(!Rs485Enabled(XRS485_21)) return false;
    
    bool result = false;
    if(FUNC_INIT == function)
    {
        ESULSInit();
    }
    else if (ESULS.valid)
    {
        switch(function)
        {
        case FUNC_EVERY_250_MSECOND:
            ULSReadData();
            break;
        case FUNC_JSON_APPEND:
            ESULSShow(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            ESULSShow(0);
            break;
#endif
        }
    }
    return result;
}


#endif
#endif