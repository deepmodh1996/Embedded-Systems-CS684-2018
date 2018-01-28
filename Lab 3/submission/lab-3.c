#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"

#define PWM_FREQUENCY 50

typedef enum {PIN_0_STATE_INDEX, PIN_4_STATE_INDEX} swState_i;
typedef enum {IDLE_STATE, PRESS_STATE, RELEASE_STATE} bounce_state;
typedef enum {AUTO, MANUAL_R, MANUAL_G, MANUAL_B, SERVO} mode;

bounce_state sw_state[3] = {IDLE_STATE, IDLE_STATE, IDLE_STATE};
uint32_t swNumRelease[3] = {0, 0, 0};

uint32_t timer0IntFreq = 100; // 100Hz

uint32_t ui32Load;
uint32_t colorWheelTheta = 0;	// 360*4=1440 degrees make one circle
uint32_t dTheta = 200; 			// 25 degrees per sec (25*4)

uint8_t ui8AdjustR = 250;
uint8_t ui8AdjustG = 10;
uint8_t ui8AdjustB = 10;
uint8_t ui8Adjust = 75;

mode currMode = AUTO;
bool firstPressDetected = false;

void setup(void) {
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	//unlock PF0 based on requirement
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK)= GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01 ;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK)= 0 ;
}

void pin_config(void) {
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PF1_M1PWM5);
	GPIOPinConfigure(GPIO_PF2_M1PWM6);
	GPIOPinConfigure(GPIO_PF3_M1PWM7);
	GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);
	GPIOPinConfigure(GPIO_PD0_M1PWM0);

	GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	uint32_t ui32PWMClock = SysCtlClockGet() / 64;
	ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
	PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);

	PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
	PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);
	PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);
	PWMGenEnable(PWM1_BASE, PWM_GEN_0);
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);
	PWMGenEnable(PWM1_BASE, PWM_GEN_3);
}

unsigned char detectKeyPress(uint32_t port, uint8_t pin, swState_i index) {
	unsigned char flag = 0;
	bool keyPressed = !GPIOPinRead(port, pin);

	// desciption of the switch debouncing state machine
	switch(sw_state[index]) {
		case IDLE_STATE:
			if(keyPressed){sw_state[index] = PRESS_STATE;} else {sw_state[index] = IDLE_STATE;} break;
		case PRESS_STATE:
			if(keyPressed){sw_state[index] = RELEASE_STATE; flag = 1;} else {sw_state[index] = IDLE_STATE;} break;
		case RELEASE_STATE:
			if(keyPressed){sw_state[index] = RELEASE_STATE; swNumRelease[index]+=1; if(swNumRelease[index] > 100){flag=2;}} else {sw_state[index] = IDLE_STATE; swNumRelease[index]=0;} break;
	}
	return flag;
}

void timer0IntSetup(void) {
	// enable Timer0 peripherals
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	// set timer for 100Hz at 50% duty cycle
	uint32_t ui32Period = (SysCtlClockGet() / timer0IntFreq) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	// enable the interrupt
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	// enable the timer
	TimerEnable(TIMER0_BASE, TIMER_A);
}

void setNextColor(void) {
	colorWheelTheta += (dTheta / timer0IntFreq);
	if (colorWheelTheta >= 1440) {colorWheelTheta %= 1440;}
	uint8_t section = colorWheelTheta / 240;
	switch(section) {
		case 0: ui8AdjustG = 10+colorWheelTheta; ui8AdjustR = 250; ui8AdjustB = 10; break;
		case 1: ui8AdjustR = 10+240-(colorWheelTheta-(section*240)); ui8AdjustG = 250; ui8AdjustB = 10; break;
		case 2: ui8AdjustB = 10+colorWheelTheta-(section*240); ui8AdjustG = 250; ui8AdjustR = 10; break;
		case 3: ui8AdjustG = 10+240-(colorWheelTheta-(section*240)); ui8AdjustB = 250; ui8AdjustR = 10; break;
		case 4: ui8AdjustR = 10+colorWheelTheta-(section*240); ui8AdjustB = 250; ui8AdjustG = 10; break;
		case 5: ui8AdjustB = 10+240-(colorWheelTheta-(section*240)); ui8AdjustR = 250; ui8AdjustG = 10; break;
	}
}

void setPulseWidth(void) {
	if (currMode == SERVO) {
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, ui8Adjust * ui32Load / 1000);
		ui8AdjustR = 1; ui8AdjustB = 1; ui8AdjustG = 1;
	}
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8AdjustR * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8AdjustB * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8AdjustG * ui32Load / 1000);
}

void Timer0IntHandler(void) {
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	unsigned char detectSW1 = detectKeyPress(GPIO_PORTF_BASE, GPIO_PIN_4, PIN_4_STATE_INDEX);
	unsigned char detectSW2 = detectKeyPress(GPIO_PORTF_BASE, GPIO_PIN_0, PIN_0_STATE_INDEX);
	bool modeChange = false;
	switch(detectSW2) {
		case 0: 
		case 1: firstPressDetected = false; break;
		case 2: 
			if (detectSW1 == 1) { modeChange = true;
				if (firstPressDetected) {currMode = MANUAL_B;} else {currMode = MANUAL_R;}
				firstPressDetected = true;
			} else if (detectSW1 == 2) {
				currMode = MANUAL_G; modeChange = true;
			}
			break;
	}

	if (!modeChange) {
		switch(currMode) {
			case AUTO: 
				if (detectSW1 == 1) {dTheta += timer0IntFreq;}
				if (detectSW2 == 1) {dTheta -= timer0IntFreq; if(dTheta < timer0IntFreq) {dTheta = timer0IntFreq;}}
				setNextColor();
				break;
			case MANUAL_R: ui8AdjustG = 10; ui8AdjustB = 10;
				if (detectSW1 == 1) {ui8AdjustR += 10; if(ui8AdjustR > 240) {ui8AdjustR = 240;}}
				if (detectSW2 == 1) {ui8AdjustR -= 10; if(ui8AdjustR < 10) {ui8AdjustR = 10;}}
				break;
			case MANUAL_G: ui8AdjustR = 10; ui8AdjustB = 10;
				if (detectSW1 == 1) {
					ui8AdjustG += 10;
					if(ui8AdjustG > 240) {ui8AdjustG = 240;}
				}
				if (detectSW2 == 1) {ui8AdjustG -= 10; if(ui8AdjustG < 10) {ui8AdjustG = 10;}}
				break;
			case MANUAL_B: ui8AdjustG = 10; ui8AdjustR = 10;
				if (detectSW1 == 1) {ui8AdjustB += 10; if(ui8AdjustB > 240) {ui8AdjustB = 240;}}
				if (detectSW2 == 1) {ui8AdjustB -= 10; if(ui8AdjustB < 10) {ui8AdjustB = 10;}}
				break;
			case SERVO:
				if (detectSW1 == 1) {ui8Adjust += 5; if(ui8Adjust > 120) {ui8Adjust = 120;}}
				if (detectSW2 == 1) {ui8Adjust -= 5; if(ui8Adjust < 30) {ui8Adjust = 30;}}
				break;
		}
		setPulseWidth();
	}
}

int main(void) {
	setup();
	pin_config();
	timer0IntSetup();

	setPulseWidth();

	//currMode = AUTO;
	//currMode = SERVO;

	while(1)
	{
	}
}
