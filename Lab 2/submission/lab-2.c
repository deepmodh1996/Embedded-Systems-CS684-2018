#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

#define Idle_State      0x01
#define Press_State     0x02
#define Release_State   0x03

int sw_state = Idle_State; // same switch state variable for both sw1 and sw2 because only one switch is used during one part of lab

uint8_t lab_part_number = 0;

uint8_t LED_color = 0;
uint32_t sw2_pressed_count = 0;

void setup(void)
{
    uint32_t ui32Period;

    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //unlock PF0 based on requirement
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK)= GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK)= 0;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    ui32Period = (SysCtlClockGet() / 100) / 2;
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A);
}


void pin_config(void)
{
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

unsigned char detectKeyPress(uint32_t port, uint8_t pin)
{
    int pin_pressed = (!(GPIOPinRead(port, pin) & pin)); // true if pin is pressed

    // implementing Switch debouncing state diagram
    switch(sw_state)
    {
    case Idle_State :
        if (pin_pressed)
        {
            sw_state = Press_State;
        }
        else
        {
            sw_state = Idle_State;
        }
        break;

    case Press_State :
        if (pin_pressed)
        {
            sw_state = Release_State;
            return 1; // key is still pressed, return flag = 1
        }
        else
        {
            sw_state = Idle_State;
        }
        break;

    case Release_State :
        if (pin_pressed)
        {
            sw_state = Release_State;
        }
        else
        {
            sw_state = Idle_State;
        }
        break;
    }

    return 0;
}

void change_color(uint8_t color)
{
    if (color == 2)
    {
        LED_color = 8;
    }

    if (color == 4)
    {
        LED_color = 2;
    }

    if (color == 8)
    {
        LED_color = 4;
    }

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, LED_color);
    return;
}



void Timer0IntHandler(void)
{
    // Clear the timer interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    if (lab_part_number == 1)
    {
        if (detectKeyPress(GPIO_PORTF_BASE, GPIO_PIN_4))
        {
            change_color(LED_color);
        }
    }

    if (lab_part_number == 2)
    {
        if (detectKeyPress(GPIO_PORTF_BASE, GPIO_PIN_0))
        {
            sw2_pressed_count++;
        }
    }
}



void lab2_part1()
{
    lab_part_number = 1;

    LED_color = 2;
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, LED_color);

    while(1)
    {
    }
}

void lab2_part2()
{
    lab_part_number = 2;

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);

    while(1)
    {
    }
}

int main(void)
{
    setup();
    pin_config();

    //lab2_part1();
    //lab2_part2();
}
