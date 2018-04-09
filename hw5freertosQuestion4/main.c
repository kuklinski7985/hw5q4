#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "conversion.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

uint32_t SysClkFreq;
static uint8_t led1Value = 0;
static uint8_t led2Value = 0;

void my_UARTprintString(char * input_string);
void veventsTask(void *pvParameters);
void vledBlink1(void *pvParameters);
void vledBlink2(void *pvParameters);

static void led1CallBackFxn (TimerHandle_t xTimer);
static void led2CallBackFxn (TimerHandle_t xTimer);

#define led1_TIMER_PERIOD       pdMS_TO_TICKS(500)
#define led2_TIMER_PERIOD       pdMS_TO_TICKS(250)

#define TOGGLE_LED          0x02
#define LOG_STRING          0x04

QueueHandle_t task3Q = NULL;

TaskHandle_t eventsTaskHandle;  //task handle for the signal notification task

int main(void)
{
    //creating timer handles and verables for the timer start function
    TimerHandle_t led1Timer, led2Timer;
    BaseType_t led1TimerStarted, led2TimerStarted,

    /*sets the clock freq
     *(external crystal freq | use external crystal | PLL source | PLL VCO to 320Mhz), desired freq
     */
    SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_320), 120000000);

    //USRLED 1 and 2 are connected to port N, turns it on
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //turning on URAT0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0|GPIO_PIN_1);
    //setting pins for RX / TX
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, SysClkFreq, 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    my_UARTprintString("Project for: Andrew Kuklinski | 4-8-2018");
    UARTCharPut(UART0_BASE, 10);    //carriage return
    UARTCharPut(UART0_BASE, 13);    //new line
    //configures Port N, pins 0 and 1 as output

    //GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
    //writes 0x0 to both pins turning them off
    //GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0|GPIO_PIN_1, 0x01);
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);

    led1Timer = xTimerCreate("led1timer",led1_TIMER_PERIOD,pdTRUE,0,led1CallBackFxn);
    led2Timer = xTimerCreate("led2timer",led2_TIMER_PERIOD,pdTRUE,0,led2CallBackFxn);

    task3Q = xQueueCreate(25,32);
    if(task3Q == NULL)
    {
        my_UARTprintString("Queue Create Failed!");
        UARTCharPut(UART0_BASE, 13);
        UARTCharPut(UART0_BASE, 10);
    }

    if((led1Timer != NULL) && (led2Timer != NULL))
    {
        led1TimerStarted = xTimerStart(led1Timer,0);
        led2TimerStarted = xTimerStart(led2Timer,0);
        if((led1TimerStarted == pdPASS) && (led2TimerStarted == pdPASS))
        {
            my_UARTprintString("Timers 1 and 2 successfull");
            UARTCharPut(UART0_BASE, 13);
            UARTCharPut(UART0_BASE, 10);
        }

    }


    BaseType_t eventsTask = xTaskCreate(veventsTask, "eventsTask", 2000, NULL, 2, &eventsTaskHandle);
    if(eventsTask == pdFAIL)
    {
        my_UARTprintString("Task eventsTask has failed");
    }

    BaseType_t led1 = xTaskCreate(vledBlink1, "ledBlink1", 1000, NULL, 1, NULL);
    if(led1 == pdFAIL)
    {
        my_UARTprintString("Task ledBlink1 has failed");
    }

    BaseType_t led2 = xTaskCreate(vledBlink2, "ledBlink2", 1000, NULL, 1, NULL);
    if(led2 == pdFAIL)
    {
        my_UARTprintString("Task ledBlink2 has failed");
    }


    vTaskStartScheduler();
    //uint32_t counter = 0;
    //uint8_t counter_buff[16] = {0};
    while(1)
    {
        /*GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 1);
        //(120000000cycle/sec) * (1 call/3 cycle) * (1 sec / 2 cycle) = 20000000calls/cycle
        SysCtlDelay(2000000);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1,0);
        SysCtlDelay(2000000);
        counter++;

        my_itoa(counter, counter_buff,10);
        my_UARTprintString("LED Counter: ");
        my_UARTprintString((char*)counter_buff);
        UARTCharPut(UART0_BASE, 13);    //carriage return
        UARTCharPut(UART0_BASE, 10);*/
    }
}


void my_UARTprintString(char * input_string)
{
    uint8_t string_length = strlen(input_string);
    uint8_t i;
    for(i=0; i<string_length; i++)
    {
        UARTCharPut(UART0_BASE, input_string[i]);
    }

}

void veventsTask(void *pvParameters)
{
    uint32_t receivedValue;
    char receivedString;
    BaseType_t receivedQueueInfo;
    for(;;)
    {
        uint32_t receivedEvents = xTaskNotifyWait(0,0, &receivedValue,portMAX_DELAY);
        if(receivedValue == TOGGLE_LED)
        {
            my_UARTprintString("LED Toggling");
            UARTCharPut(UART0_BASE, 13);
            UARTCharPut(UART0_BASE, 10);
            led1Value = !led1Value;
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, led1Value);
        }
        if(receivedValue == LOG_STRING)
        {
            receivedQueueInfo = xQueueReceive(task3Q, &receivedString,portMAX_DELAY);
            if(receivedQueueInfo == pdPASS)
            {
                my_UARTprintString(&receivedString);
                UARTCharPut(UART0_BASE, 13);
                UARTCharPut(UART0_BASE, 10);
            }
        }
    }
}


void vledBlink1(void *pvParameters)
{
    for(;;)
    {

    }
}

void vledBlink2(void *pvParameters)
{
    for(;;)
    {

    }
}

static void led1CallBackFxn (TimerHandle_t xTimer)
{
    //led1Value = !led1Value;
    //GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, led1Value);
    //my_UARTprintString("CallBack LED1 is working");
    //UARTCharPut(UART0_BASE, 13);
    //UARTCharPut(UART0_BASE, 10);
    xTaskNotify(eventsTaskHandle, TOGGLE_LED, eSetValueWithoutOverwrite);
}

static void led2CallBackFxn (TimerHandle_t xTimer)
{
    char task3message[64];
    static uint16_t led2counter = 0;
    led2Value = (!led2Value);
    if(led2Value != 0)
    {
        led2Value++;
    }
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, led2Value);
    //my_UARTprintString("CallBack LED2 is working also");
    //UARTCharPut(UART0_BASE, 13);
    //UARTCharPut(UART0_BASE, 10);

    sprintf(task3message, "Task3 Counter Value: %d",led2counter);
    xQueueSend(task3Q, task3message, portMAX_DELAY);

    xTaskNotify(eventsTaskHandle, LOG_STRING, eSetValueWithoutOverwrite);
    led2counter++;
}








