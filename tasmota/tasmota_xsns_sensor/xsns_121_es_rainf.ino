#ifdef USE_RS485
#ifdef USE_ES_RAINF

#define XSNS_121 121
#define XRS485_20 20
struct ESRAINFt
{
    bool valid = false;
    float rainfall_value = 0.0;
    char name[9] = "RAINFALL";
}ESRAINF;

#define ES_RAINF_ADDRESS_ID 0x41
#define ES_RAINF_ADDRESS_RAINFALL 0x0000
#define ES_RAINF_FUNCTION_CODE 0x03

bool ESRAINFisConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus -> Send(ES_RAINF_ADDRESS_ID, ES_RAINF_FUNCTION_CODE, (0x07 << 8) | 0xD0, 1);
    //RS485.Rs485Modbus -> (0x41, 0x03, 0x00, 0x01);
    
    delay(200);

    //flush();

    RS485.Rs485Modbus -> ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,8);
    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("error %d"), error);
        return false;
    }
    else
    {
        uint16_t check_ESRAINF = (buffer[3] << 8) | buffer[4];
        if(check_ESRAINF == ES_RAINF_ADDRESS_ID) return true;
    }
    return false;
}

void ESRAINFInit(void)
{
    if(!RS485.active) return;

    ESRAINF.valid = ESRAINFisConnected();
    if(!ESRAINF.valid) TasmotaGlobal.restart_flag = 2;
    if(ESRAINF.valid) Rs485SetActiveFound(ES_RAINF_ADDRESS_ID, ESRAINF.name);
    AddLog(LOG_LEVEL_INFO, PSTR(ESRAINF.valid ? "Rainfall is connected" : "Rainfall is not detected"));
}

uint32_t time_reset_rainfall = 0;
bool reset_rainfall = false;

bool ESRAINFReset(void)
{
    TIME_T now;
    BreakTime(LocalTime(), now);
    if(now.hour == 23 && now.minute == 59 && reset_rainfall == false)
    {   
        uint16_t value_reset = 90;
        RS485.Rs485Modbus->Send(ES_RAINF_ADDRESS_ID, 0x06, ES_RAINF_ADDRESS_RAINFALL, 1, &value_reset);
        delay(200);
        RS485.Rs485Modbus->ReceiveReady();
        AddLog(LOG_LEVEL_INFO, PSTR("Reset rainfall value"));
        reset_rainfall = true;
        return true;
    }
    
    if(now.hour == 1 && now.minute == 0) reset_rainfall = false;
    return false;
}

/* void CheckResetByTime()
{   
    if(time_reset_rainfall < 120) return;
    TIME_T now;
    BreakTime(LocalTime(), now);
    AddLog(LOG_LEVEL_INFO,PSTR("hour: %d, minute: %d, seconds: %d"), now.hour, now.minute, now.second);
    time_reset_rainfall = 0;
    
} */

void ESRAINFReadData(void)
{
    if (ESRAINF.valid == false) return;

    if (isWaitingResponse(ES_RAINF_ADDRESS_ID)) return;

    if(ESRAINFReset()) return;

    if (RS485.requestSent[ES_RAINF_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus->Send(ES_RAINF_ADDRESS_ID, ES_RAINF_FUNCTION_CODE, ES_RAINF_ADDRESS_RAINFALL, 1);
        RS485.requestSent[ES_RAINF_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[ES_RAINF_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        if (RS485.Rs485Modbus->ReceiveReady())
        {
            uint8_t buffer[8];
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

            if (error)
            {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus Rainfall Error: %u"), error);
            }
            else if (buffer[0] == ES_RAINF_ADDRESS_ID) // Ensure response is from the correct slave ID
            {
                uint16_t rainfall_valueRaw = (buffer[3] << 8) | buffer[4];
                ESRAINF.rainfall_value = rainfall_valueRaw / 10.0;
            }
            RS485.requestSent[ES_RAINF_ADDRESS_ID] = 0;
            RS485.lastRequestTime = 0;
        }
    }
}

const char HTTP_SNS_ES_RAINF[] PROGMEM = "{s} Rainfall Value {m} %.1fmm";
#define D_JSON_ES_RAINF "Rainfall"

void ESRAINFShow(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), ESRAINF.name);
        ResponseAppend_P(PSTR("\"" D_JSON_ES_RAINF "\":%.1f"), ESRAINF.rainfall_value);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_ES_RAINF, ESRAINF.rainfall_value);
    }
#endif
}

bool Xsns121(uint32_t function)
{
    if (!Rs485Enabled(XRS485_20)) return false;

    bool result = false;
    if (FUNC_INIT == function)
    {
        ESRAINFInit();
    }
    else if (ESRAINF.valid)
    {
        switch (function)
        {
        case FUNC_EVERY_250_MSECOND:
            ESRAINFReadData();
            break;
        case FUNC_JSON_APPEND:
            ESRAINFShow(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            ESRAINFShow(0);
            break;
#endif
        }
    }
    return result;
}

#endif
#endif
