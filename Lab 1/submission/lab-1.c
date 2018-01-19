/*

* Author: Texas Instruments

* Editted by: Parin Chheda
          ERTS Lab, CSE Department, IIT Bombay

* Description: This code structure will assist you in completing Lab 1
* Filename: lab-1.c

* Functions: setup(), config(), main()


*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_ints.h"
#include <time.h>


/*

* Function Name: setup()

* Input: none

* Output: none

* Description: Set crystal frequency,enable GPIO Peripherals and unlock Port F pin 0 (PF0)

* Example Call: setup();

*/

uint32_t sw2status = 0;

void setup(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    
	//unlock PF0 based on requirement
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK)= GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK)= 0;
}

/*

* Function Name: pin_config()

* Input: none

* Output: none

* Description: Set Port F Pin 1, Pin 2, Pin 3 as output. On this pin Red, Blue and Green LEDs are connected.
			   Set Port F Pin 0 and 4 as input, enable pull up on both these pins.

* Example Call: pin_config();

*/

void pin_config(void)
{
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

int switch_pressed_debounced(uint32_t port, uint8_t pin)
{
    if (!(GPIOPinRead(port, pin) & pin))
    {
        SysCtlDelay(670000); // 50 ms delay
        if (!(GPIOPinRead(port, pin) & pin))
        {
            return 1;
        }
    }
    return 0;
}

uint8_t change_color(uint8_t LED_color)
{
    if (LED_color == 2)
    {
        return 8;
    }

    if (LED_color == 4)
    {
        return 2;
    }

    return 4;
}

void lab1_part1()
{
    uint8_t LED_color = 2;

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);

    while (1) {
        if (switch_pressed_debounced(GPIO_PORTF_BASE, GPIO_PIN_4)){
            // set color
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, LED_color);

            while (switch_pressed_debounced(GPIO_PORTF_BASE, GPIO_PIN_4))
            {
                continue;
            }

            // change color
            LED_color = change_color(LED_color);
        }
        else{
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
        }
    }
}

void lab1_part2()
{
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);

    while (1) {
        if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) & GPIO_PIN_0))
        {
            sw2status++;
        }
    }
}

void lab1_part3()
{
    uint8_t LED_color = 2;
    uint32_t delay = 3700000;
    bool show_LED = 1;

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);

    while (1) {
        if (switch_pressed_debounced(GPIO_PORTF_BASE, GPIO_PIN_4))
        {
           // switch 1 is pressed, change delay
           if (delay == 3700000*4)
           {
                delay = 3700000;
           }
           else
           {
               delay *= 2;
           }
        }

        if (switch_pressed_debounced(GPIO_PORTF_BASE, GPIO_PIN_0))
        {
            // switch 2 is pressed, change color
            LED_color = change_color(LED_color);
        }

        if (show_LED)
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, LED_color);
            SysCtlDelay(delay);
            show_LED = 0;
        }
        else
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
            SysCtlDelay(delay);
            show_LED = 1;
        }
    }
}

int main(void)
{
    setup();
    pin_config();

    //lab1_part1();
    //lab1_part2();
    //lab1_part3();
}
