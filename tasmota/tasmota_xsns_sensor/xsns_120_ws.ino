#ifdef USE_RS485
#ifdef USE_WS

#define XSNS_120 120
#define XRS485_02 2
struct WSt
{
    bool valid = false;
    float wind_speed = 0;
    char name[11] = "WIND SPEED";
}WS;

#define WS_ADDRESS_ID 0x02
#define WS_ADDRESS_WIND_SPEED 0x0000
#define WS_FUNCTION_CODE 0x03
#define WS_TIMEOUT 150

bool WSisConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus->Send(WS_ADDRESS_ID, WS_FUNCTION_CODE, ((0x07 << 8) | 0xD0), 0x01);

    delay(200);

    RS485.Rs485Modbus -> ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer, 8);
    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("error %d"),error);
        return false;
    }
    else
    {
        uint16_t check_WS = (buffer[3] << 8) | buffer[4];
        if(check_WS == WS_ADDRESS_ID) return true;
    }
    return false;
}

void WSInit(void)
{
    if(!RS485.active) return;

    WS.valid = WSisConnected();
    if(!WS.valid) TasmotaGlobal.restart_flag = 2;
    if(WS.valid) Rs485SetActiveFound(WS_ADDRESS_ID, WS.name);
    AddLog(LOG_LEVEL_INFO, PSTR(WS.valid ? "Wind Speed is connected" : "Wind Speed is not connected"));
}

void WSReadData(void)
{
    if(WS.valid == false) return;

    if(isWaitingResponse(WS_ADDRESS_ID)) return;

    if(RS485.requestSent[WS_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus -> Send(WS_ADDRESS_ID, WS_FUNCTION_CODE, WS_ADDRESS_WIND_SPEED , 1);
        RS485.requestSent[WS_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[WS_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        if (RS485.Rs485Modbus->ReceiveReady())
        {
            uint8_t buffer[8];
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

            if (error)
            {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus WS Error: %u"), error);
            }
            else if (buffer[0] == WS_ADDRESS_ID) // Ensure response is from the correct slave ID
            {
                uint16_t wind_speedRaw = (buffer[3] << 8) | buffer[4];
                WS.wind_speed = wind_speedRaw / 10.0;
            }
            RS485.requestSent[WS_ADDRESS_ID] = 0;
            RS485.lastRequestTime = 0;
        }
    }
}

const char HTTP_SNS_WS[] PROGMEM = "{s} Wind Speed {m} %.1f m/s";
#define D_JSON_WS "WindSpeed"

void WSShow(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), WS.name);
        ResponseAppend_P(PSTR("\"" D_JSON_WS "\":%.1f"), WS.wind_speed);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_WS, WS.wind_speed);
    }
#endif
}

bool Xsns120(uint32_t function)
{
    if(!Rs485Enabled(XRS485_02)) return false;

    bool result = false;
    if(FUNC_INIT == function)
    {
        WSInit();
    }
    else if(WS.valid)
    {
        switch(function)
        {
        case FUNC_EVERY_250_MSECOND:
            WSReadData();
            break;
        case FUNC_JSON_APPEND:
            WSShow(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            WSShow(0);
            break;
#endif
        }
    }
    return result;
}

#endif 
#endif