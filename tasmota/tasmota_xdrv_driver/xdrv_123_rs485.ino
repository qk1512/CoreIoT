#ifdef USE_RS485

#define XDRV_123 123

#include <TasmotaModbus.h>

#define MAX_SENSORS 100

struct RS485t
{
    bool active = false;
    uint8_t tx = 0;
    uint8_t rx = 0;
    TasmotaModbus *Rs485Modbus = nullptr;
    //uint32_t sensor_active[4];
    bool requestSent[MAX_SENSORS] = {false};
    uint32_t _active[3];
    uint8_t sensor_id = 1;
    uint32_t lastRequestTime; 
}RS485;

void Rs485Init(void)
{
    RS485.active = false;
    if(PinUsed(GPIO_RS485_RX) && PinUsed(GPIO_RS485_TX))
    {
        if(RS485.active)
        {
            AddLog(LOG_LEVEL_ERROR, "RS485: RS485 serial can be configured only on 1 time");
            
        }
        if(TasmotaGlobal.rs485_enabled)
        {
            AddLog(LOG_LEVEL_ERROR, "RS485: RS485 serial failed because RX/TX already configured");
        }
        else
        {
            RS485.rx = Pin(GPIO_RS485_RX);
            RS485.tx = Pin(GPIO_RS485_TX);
            RS485.active = true;
        }
    }

    if(RS485.active)
    {
        RS485.Rs485Modbus = new TasmotaModbus(RS485.rx,RS485.tx);
        uint8_t result = RS485.Rs485Modbus -> Begin();
        if(result)
        {
            if (2 == result)
            {
                ClaimSerial();
            }
            TasmotaGlobal.rs485_enabled = true;
            AddLog(LOG_LEVEL_INFO, PSTR("RS485: RS485 using GPIO%i(RX) and GPIO%i(TX)"),RS485.rx,RS485.tx);
        }
        else
        {
            delete RS485.Rs485Modbus;
            RS485.Rs485Modbus = nullptr;
            RS485.active = false;
        }

    }
}

bool isWaitingResponse(int sensor_id)
{
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if(RS485.requestSent[sensor_id]) continue;
        if(RS485.requestSent[i]) return true;
    }
    return false;
}

void advanceSensorID()
{
    RS485.sensor_id++;
    if (RS485.sensor_id > MAX_SENSORS)
    {
        RS485.sensor_id = 1;
    }
}
void Rs485SetActive(uint32_t addr)
{
    addr &= 0x77;
    RS485._active[addr/32] |= (1 << (addr %32));
}

bool Rs485Active(uint32_t addr)
{
    addr &= 0x7F;
    return (RS485._active[addr/32] & (1 << (addr %32)));
}
void Rs485SetActiveFound(uint32_t addr, const char *types)
{
    Rs485SetActive(addr);
    AddLog(LOG_LEVEL_INFO, PSTR("RS485: %s found at 0x%02x"), types, addr);
}


bool Xdrv123(uint32_t function)
{
    bool result = false;
    if(FUNC_PRE_INIT == function)
    {
        Rs485Init();
    }
    else if(RS485.active)
    {
        switch (function)
        {
            case FUNC_ACTIVE:
                result = true;
                break;
        }
    }
    return result;
}

#endif