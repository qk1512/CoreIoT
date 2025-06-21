#ifdef USE_RS485
#ifdef USE_WDS

#define XSNS_119 119
#define XRS485_01 1
struct WDSt
{
    bool valid = false;
    int wind_direction = 0;
    char name[15] = "WIND DIRECTION";
} WDS;

#define WDS_ADDRESS_ID 0x01
#define WDS_ADDRESS_WIND_DIRECTION 0x0001
#define WDS_FUNCTION_CODE 0x03
#define WDS_TIMEOUT 150

bool WDSisConnected()
{
    if (!RS485.active) return false;

    RS485.Rs485Modbus->Send(WDS_ADDRESS_ID, WDS_FUNCTION_CODE, (0x01 << 8) | 0x00 , 0x01);

    delay(200);

    RS485.Rs485Modbus -> ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);
    if (error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("error %d"), error);
        return false;
    }
    else
    {
        if (buffer[0] == WDS_ADDRESS_ID) return true;
    }
    return false;
}

void WDSInit(void)
{
    if (!RS485.active)
    {
        return;
    }

    WDS.valid = WDSisConnected();
    if (WDS.valid) Rs485SetActiveFound(WDS_ADDRESS_ID, WDS.name);
    AddLog(LOG_LEVEL_INFO, PSTR(WDS.valid ? "Wind Direction is connected" : "Wind Direction is not detected"));
}

void WDSReadData(void)
{

    if (WDS.valid == false)
        return;

    if (isWaitingResponse(WDS_ADDRESS_ID))
        return;

    if (RS485.requestSent[WDS_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus->Send(WDS_ADDRESS_ID, WDS_FUNCTION_CODE, WDS_ADDRESS_WIND_DIRECTION, 1);
        RS485.requestSent[WDS_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[WDS_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        if (RS485.Rs485Modbus->ReceiveReady())
        {
            uint8_t buffer[8];
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

            if (error)
            {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus WDS Error: %u"), error);
            }
            else if (buffer[0] == WDS_ADDRESS_ID) // Ensure response is from the correct slave ID
            {
                uint16_t wind_directionRaw = (buffer[3] << 8) | buffer[4];
                WDS.wind_direction = wind_directionRaw / 10.0;
            }
            RS485.requestSent[WDS_ADDRESS_ID] = 0;
            RS485.lastRequestTime = 0;
        }
    }
}

const char HTTP_SNS_WDS[] PROGMEM = "{s} Wind Direction {m} %d";
#define D_JSON_WDS "WindDirection"

void WDSShow(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), WDS.name);
        ResponseAppend_P(PSTR("\"" D_JSON_WDS "\":%d"), WDS.wind_direction);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_WDS, WDS.wind_direction);
    }
#endif
}

bool Xsns119(uint32_t function)
{
    if (!Rs485Enabled(XRS485_01))
        return false;

    bool result = false;
    if (FUNC_INIT == function)
    {
        WDSInit();
    }
    else if (WDS.valid)
    {
        switch (function)
        {
        case FUNC_EVERY_250_MSECOND:
            WDSReadData();
            break;
        case FUNC_JSON_APPEND:
            WDSShow(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            WDSShow(0);
            break;
#endif
        }
    }
    return result;
}

#endif
#endif