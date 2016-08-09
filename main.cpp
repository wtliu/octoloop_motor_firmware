/*
 * main.c
 */
#include "commondef.h"

void InitConsole(void) // Debug logging
{
	// Enable GPIO port A which is used for UART0 pins.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	// Configure the pin muxing for UART0 functions on port A0 and A1.
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	// Enable UART0 so that we can configure the clock.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	// Use the internal 16MHz oscillator as the UART clock source.
	UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
	// Select the alternate (UART) function for these pins.
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	// Initialize the UART for console I/O.
	UARTStdioConfig(0, 115200, 16000000);
}

volatile bool g_bErrFlag = 0;
volatile uint32_t g_ui32MsgCount = 0;

void canHandler(void) {
	uint32_t ui32Status;
	// Read the CAN interrupt status to find the cause of the interrupt
	ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
	// If the cause is a controller status interrupt, then get the status
	if(ui32Status == CAN_INT_INTID_STATUS)
	{
		// Read the controller status.  This will return a field of status
		// error bits that can indicate various errors.  Error processing
		// is not done in this example for simplicity.  Refer to the
		// API documentation for details about the error status bits.
		// The act of reading this status will clear the interrupt.  If the
		// CAN peripheral is not connected to a CAN bus with other CAN devices
		// present, then errors will occur and will be indicated in the
		// controller status.
		ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

		// Set a flag to indicate some errors may have occurred.
		g_bErrFlag = 1;
	}
	// Check if the cause is message object 1, which what we are using for
	// sending messages.
	else if(ui32Status == 1)
	{
		// Getting to this point means that the TX interrupt occurred on
		// message object 1, and the message TX is complete.  Clear the
		// message object interrupt.
		CANIntClear(CAN0_BASE, 1);
		// Increment a counter to keep track of how many messages have been
		// sent.  In a real application this could be used to set flags to
		// indicate when a message is sent.
		g_ui32MsgCount++;

		// Since the message was sent, clear any error flags.
		g_bErrFlag = 0;
	}
	// Otherwise, something unexpected caused the interrupt.  This should
	// never happen.
	else
	{
		// Spurious interrupt handling can go here.
	}
}
void
SimpleDelay(void)
{
	//
	// Delay cycles for 1 second
	//
	SysCtlDelay(80000000 / 3 ); //50ms
}

int main(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); //80 Mhz clock cycle
#ifdef DEBUG	// Set up the serial console to use for displaying messages
	InitConsole();
//	printf("Hello: there!\n");
	printf("Current,Target,Speed,Time\n");
#endif
	SysTickPeriodSet(80000000); // The period should be equal to the system clock time to ensure the systick values are in clock cycle units
	SysTickEnable();

	/*** Init classes, variables ***/
	AMSPositionEncoder cAMSPositionEncoder;
	Params cParams;
	PID cPID(0.73, 100, -100, 0.0061, 0.000455, 0.00182);//0.0031
	MotorDriver5015a cMotorDriver5015a;
	TempArduino cTempArduino;

	uint16_t current_position=0;
	uint16_t target_position = 9805;
	cParams.setTargetPos(target_position);
	float speed;
	uint32_t prevTime = SysTickValueGet(); // clock cycles
	uint32_t currTime;

//	float speeds[] = {5.0f,15.0f,10.0f,5.0f,0.0f,10.0f};
//	int i=0;
	while(1) {



		/*********************************************
		 *******USING AMS ENCODER********
		 **********************************************/
		//		// Read the current position from the encoder and udpate the params class
		//		current_position = cAMSPositionEncoder.getPosition();
		//		cParams.setCurrentPos(current_position);
		//#ifdef DEBUG
		//		//		UARTprintf("Current Position: %d\n",current_position);
		//		printf("Current Position: %d\n",current_position);
		//#endif
		//
		//		//	 Read the target position from the Params class
		//		//		target_position = cParams.getTargetPos();
		//#ifdef DEBUG
		//		//UARTprintf("Target Position: %d\n", target_position);
		//		printf("Target Position: %d\n", target_position);
		//#endif

		/*********************************************
		 *******USING TEMP ARDUINO ********
		 **********************************************/

		//Read the current position
		current_position = cTempArduino.getPositionEncoderPosition();
		cParams.setCurrentPos(current_position);
#ifdef DEBUG
		printf("%d,", current_position);
#endif
		//Read the target position from the Params class
		target_position = cParams.getTargetPos();
#ifdef DEBUG
		printf("%d,", target_position);
#endif

		/*** Call PID class main function and get PWM speed as the output ***/
		speed = cPID.calculate(target_position, current_position);
#ifdef DEBUG
		//		UARTprintf("Speed: %d\n", (int)speed);
		printf("%d,", (int)speed);
#endif
//

//		speed =speeds[i];
//		if(i>5)
//			i=0;
//		else
//			i++;

		float spd = fabsf(speed);
		cMotorDriver5015a.setSpeed(spd);

		if (speed > 0)
			cMotorDriver5015a.setDirection(MotorDriver5015a::CLOCKWISE);
		else
			cMotorDriver5015a.setDirection(MotorDriver5015a::ANTICLOCKWISE);



		currTime = SysTickValueGet(); // clock cycles
#ifdef DEBUG
		printf("%d\n", currTime - prevTime);
#endif
		prevTime = currTime;
		SimpleDelay();

	}

	return 0;
}

