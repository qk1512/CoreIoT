#ifdef USE_RS485
#ifdef USE_ES_SD

#define XSNS_118 118
#define XRS485_26 26

struct ESSDt
{
    bool valid = false;
    uint16_t alarm_check = 0;
    char name[5] = "ESSD";
} ESSD;

#define ESSD_ADDRESS_ID 0x01
#define ESSD_ADDRESS_CHECK 0x0100
#define ESSD_ADDRESS_ALARM_CHECK 0x0003
#define ESSD_FUNCTION_CODE 0x03
#define ESSD_TIMEOUT 200

bool ESSDisConnected()
{
    if (!RS485.active) return false;

    RS485.Rs485Modbus->Send(ESSD_ADDRESS_ID, 0x03, (0x07 << 8) | 0xD0, 0x01);

    delay(200);

    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

    if (error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("ESSD has error %d"), error);
        return false;
    }
    else
    {
        uint16_t check_ESSD = (buffer[3] << 8) | buffer[4];
        AddLog(LOG_LEVEL_INFO, PSTR("ADDRESS ID: %d"), check_ESSD);
        if (check_ESSD == ESSD_ADDRESS_ID) return true;
    }
    return false;
}

void ESSDInit()
{
    if (!RS485.active) return;
    ESSD.valid = ESSDisConnected();
    if(!ESSD.valid) TasmotaGlobal.restart_flag = 2;

    if (ESSD.valid)
        Rs485SetActiveFound(ESSD_ADDRESS_ID, ESSD.name);
    AddLog(LOG_LEVEL_INFO, PSTR(ESSD.valid ? "ESSD is connected" : "ESSD is not detected"));
}

void ESSDReadData()
{
    if (!ESSD.valid)
        return;

    if (isWaitingResponse(ESSD_ADDRESS_ID))
        return;

    if ((RS485.requestSent[ESSD_ADDRESS_ID] == 0) && (RS485.lastRequestTime == 0))
    {
        RS485.Rs485Modbus->Send(ESSD_ADDRESS_ID, ESSD_FUNCTION_CODE, ESSD_ADDRESS_ALARM_CHECK, 1);
        RS485.requestSent[ESSD_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[ESSD_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 150)
    {
        if (!RS485.Rs485Modbus->ReceiveReady()) return;
        uint8_t buffer[8];
        uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

        if (error)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Modbus ESSD error: %d"), error);
        }
        else
        {
            uint16_t alarm_checkRaw = (buffer[3] << 8) | buffer[4];
            ESSD.alarm_check = alarm_checkRaw;
        }
        RS485.requestSent[ESSD_ADDRESS_ID] = 0;
        RS485.lastRequestTime = 0;
    }
}

const char HTTP_SNS_ESSD[] PROGMEM = "{s} Alarm {m} %d";
#define D_JSON_ESSD "ESSD"

void ESSDShow(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), ESSD.name);
        ResponseAppend_P(PSTR("\"" D_JSON_ESSD "\":%d"), ESSD.alarm_check);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_ESSD, ESSD.alarm_check);
    }
#endif
}

bool Xsns118(uint32_t function)
{
    if (!Rs485Enabled(XRS485_26))
        return false;

    bool result = false;
    if (FUNC_INIT == function)
    {
        ESSDInit();
    }
    else if (ESSD.valid)
    {
        switch (function)
        {
        case FUNC_EVERY_250_MSECOND:
            ESSDReadData();
            break;
        case FUNC_JSON_APPEND:
            ESSDShow(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            ESSDShow(0);
            break;
#endif
        }
    }
    return result;
}

#endif
#endif