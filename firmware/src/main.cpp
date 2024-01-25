/*******************************************************************************
  Main Source File
*******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <string.h>
#include "definitions.h"                // SYS function prototypes
#include "temphum11.h"
#include "servo.h"
#include "rgbled.h"
#include "bluesmirf.h"

/* RTC Time period match values for input clock of 1 KHz */
#define PERIOD_500MS                            512
#define PERIOD_1S                               1024
#define PERIOD_2S                               2048
#define PERIOD_4S                               4096

#define SERVO_FAN               SERVO_MOTOR_3
#define SERVO_CELL              SERVO_MOTOR_1
#define SERVO_DOOR              SERVO_MOTOR_16
#define SERVO_DOOR_MIN          0
#define SERVO_DOOR_MAX          125


static volatile bool isRTCExpired = false;
static volatile bool isUSARTTxComplete = true;
static uint8_t uartTxBuffer[100] = {0};

static BlueSmirf bs;
static bool doorIsOpen = false;

static void EIC_User_Handler(uintptr_t context)
{
}

static void rtcEventHandler (RTC_TIMER32_INT_MASK intCause, uintptr_t context)
{
    if (intCause & RTC_MODE0_INTENSET_CMP0_Msk)
    {            
        isRTCExpired    = true;
    }
}

static void usartDmaChannelHandler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle)
{
    if (event == DMAC_TRANSFER_EVENT_COMPLETE)
    {
        isUSARTTxComplete = true;
    }
}

static void mains_switch(bool on)
{
    on? PS_ON_Clear() : PS_ON_Set();
    bs.setMains(on);
    
    //update status LED
}

static void fan_switch(bool on)
{
    servo_set_position(SERVO_FAN, on? 90 : 0);
    bs.setFan(on);
    
    //update status LED
}

static void cell_switch(bool on)
{
    servo_set_position(SERVO_CELL, on? 0 : 90);
    bs.setCell(on);

    //update status LED
}

static void door_init()
{
    servo_set_position(SERVO_DOOR, SERVO_DOOR_MAX);
}

static void door_open(bool open)
{
    if (doorIsOpen == open)
        return;
    
    uint8_t start = open? SERVO_DOOR_MAX : SERVO_DOOR_MIN;
    uint8_t end = open? SERVO_DOOR_MIN : SERVO_DOOR_MAX;
    uint8_t step = open? -1 : 1;
    
    uint8_t curr = start;
    while (curr != end)
    {
        servo_set_position(SERVO_DOOR, curr);
        SYSTICK_DelayMs(100);
        curr += step;
    }
    
    doorIsOpen = open;
}

static void print(uint8_t* txBuffer)
{
    isUSARTTxComplete = false;

    DMAC_ChannelTransfer(DMAC_CHANNEL_0, txBuffer, \
        (const void *)&(SERCOM5_REGS->USART_INT.SERCOM_DATA), \
        strlen((const char*)txBuffer));
}

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    DMAC_ChannelCallbackRegister(DMAC_CHANNEL_0, usartDmaChannelHandler, 0);
    EIC_CallbackRegister(EIC_PIN_15,EIC_User_Handler, 0);
    RTC_Timer32CallbackRegister(rtcEventHandler, 0);

    sprintf((char*)uartTxBuffer, "COLD CASE Terminal\r\n");
    print(uartTxBuffer);
    
    SYSTICK_TimerStart();
    
    RTC_Timer32Compare0Set(PERIOD_500MS);
    RTC_Timer32Start();

    RGBLed rgbLed;
    rgbLed.init();

    temphum11_default_cfg();

    bs.init();

    servo_init();
    servo_default_cfg();
    servo_soft_reset();
    
    mains_switch(true);
    cell_switch(false);
    fan_switch(false);
    door_init();

    SYSTICK_DelayMs(1000);
    mains_switch(false);

//#define TEST_DOOR
#ifdef TEST_DOOR
    PS_ON_Clear();
    while (1)
    {
        door_open(true);
        SYSTICK_DelayMs(3000);
        door_open(false);
        SYSTICK_DelayMs(3000);
    }
#endif

//#define TEST_RGB    
#ifdef TEST_RGB
    uint32_t pwm = 0;
    while (1)
    {
        rgbLed.updateFromTemp(pwm);
    pwm = (pwm+1) % 20;
            
    SYSTICK_DelayMs(500);
    }
#endif
        
    unsigned long psTick = 0;
    unsigned long psOnTick = 0;
    unsigned long cellOffTick = 0;
    int psSwitchPrev = 1;
    
    while ( true )
    {
        bs.update();
        
        CommandEnum cmd = bs.command();
        if (cmd == Command_Open)
        {
            print((uint8_t*)">>>>>> DOOR OPEN \r\n");

            int mainsOn = bs.mains();
            if (!mainsOn)
                mains_switch(true);

            door_open(true);

            if (!mainsOn)
                mains_switch(false);
        }
        else if (cmd == Command_Close)
        {
            print((uint8_t*)">>>>>> DOOR CLOSE \r\n");
            
            int mainsOn = bs.mains();
            if (!mainsOn)
                mains_switch(true);
            
            door_open(false);

            if (!mainsOn)
                mains_switch(false);
        }
        
        if (bs.manual())
        {
            sprintf((char*)uartTxBuffer, ">>>>>> MANUAL MODE Mains %d, Fan %d, Cell %d\r\n", bs.mainsOn(), bs.fanOn(), bs.cellOn());
            print(uartTxBuffer);

            fan_switch(bs.fanOn());
            cell_switch(bs.cellOn());
            mains_switch(bs.mainsOn());
        }
        
        int psSwitch = PS_SW_Get();
        if (psSwitch != psSwitchPrev)
        {
            if (psSwitch == 0)
            {
                if (psTick != 0)
                {
                    unsigned long delta = SYSTICK_GetTickCounter() - psTick;
                    if (delta > 1000)
                    {
                        if (!bs.mains())
                        {
                            psOnTick = SYSTICK_GetTickCounter();
                        }
                        else
                        {
                            fan_switch(false);
                            cell_switch(false);
                            SYSTICK_DelayMs(500);
                            
                            psOnTick = 0;
                        }
                        
                        psTick = 0;
                        psSwitchPrev = psSwitch;

                        sprintf((char*)uartTxBuffer, ">>>>>> SWITCHING %s\r\n", PS_ON_Get()? "ON" : "OFF");
                        print(uartTxBuffer);

                        mains_switch(!bs.mains());
                    }
                }
                else
                {
                    psTick = SYSTICK_GetTickCounter();
                }
            }
            else
            {
                psTick = 0;
                psSwitchPrev = psSwitch;
            }
        }
        
        if (psOnTick != 0)
        {
            unsigned long delta = SYSTICK_GetTickCounter() - psOnTick;
            if (delta > 2000)
            {
#ifdef TEST_POWERON                
                sprintf((char*)uartTxBuffer, ">>>>>> SELF TEST\r\n");
                print(uartTxBuffer);
                SYSTICK_DelayMs(500);

                // self test
                sprintf((char*)uartTxBuffer, ">>>>>> SELF TEST: FAN ON\r\n");
                print(uartTxBuffer);
                rgbLed.update(255,0,0);
                fan_switch(true);
                SYSTICK_DelayMs(3000);

                sprintf((char*)uartTxBuffer, ">>>>>> SELF TEST: CELL ON\r\n");
                print(uartTxBuffer);
                rgbLed.update(0,255,0);
                cell_switch(true);
                SYSTICK_DelayMs(3000);

                sprintf((char*)uartTxBuffer, ">>>>>> SELF TEST: CELL OFF\r\n");
                print(uartTxBuffer);
                rgbLed.update(0,0,255);
                cell_switch(false);
                SYSTICK_DelayMs(3000);

                sprintf((char*)uartTxBuffer, ">>>>>> SELF TEST: FAN OFF\r\n");
                print(uartTxBuffer);
                fan_switch(false);
                SYSTICK_DelayMs(3000);

                sprintf((char*)uartTxBuffer, ">>>>>> SELF TEST: DOOR OPEN\r\n");
                print(uartTxBuffer);
                door_open(true);
                SYSTICK_DelayMs(3000);

                sprintf((char*)uartTxBuffer, ">>>>>> SELF TEST: DOOR CLOSE\r\n");
                print(uartTxBuffer);
                door_open(false);
                SYSTICK_DelayMs(3000);

                sprintf((char*)uartTxBuffer, ">>>>>> SELF TEST COMPLETED\r\n");
                print(uartTxBuffer);
#endif
                
                rgbLed.update(0,0,0);
                
                psOnTick = 0;
            }
        }

        if (isRTCExpired == true)
        {
            isRTCExpired = false;
            LED0_Toggle();

            float h = temphum11_get_humidity();
            float t = temphum11_get_temperature(TEMPHUM11_TEMP_IN_CELSIUS);

            if (bs.mains())
            {
                // misuro T frigo
                if (t < bs.temperatureSetpoint())
                {
                    // cell off, keep the fan running for another 30 seconds
                    cell_switch(false);
                    cellOffTick = SYSTICK_GetTickCounter();
                }
                else if (t > bs.temperatureSetpoint()+1.0)
                {
                    cell_switch(true);
                    cellOffTick = 0;

                    fan_switch(true);
                }

                if (cellOffTick != 0)
                {
                    unsigned long delta = SYSTICK_GetTickCounter() - cellOffTick;
                    if (delta > 30000)
                    {
                       fan_switch(false); 
                       cellOffTick = 0;
                    }
                }
            }
            
            // update RGB LED
            rgbLed.updateFromTemp(t, bs.temperatureSetpoint());

            bs.setTemperature(t);
            bs.setHumidity((int)h);
            
            sprintf((char*)uartTxBuffer, "Temp=%f, Hum=%f, SW=%d, Conn=%d, Mains=%d, Fan=%d, Cell=%d, SP: %f\r\n", t, h, psSwitch, 
                    bs.connected(), bs.mains(), bs.fan(), bs.cell(), bs.temperatureSetpoint());
            print(uartTxBuffer);
        }
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}
/*******************************************************************************
 End of File
*/

