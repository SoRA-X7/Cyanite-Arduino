/*
Nintendo Switch Fightstick - Proof-of-Concept

Based on the LUFA library's Low-Level Joystick Demo
	(C) Dean Camera
Based on the HORI's Pokken Tournament Pro Pad design
	(C) HORI

This project implements a modified version of HORI's Pokken Tournament Pro Pad
USB descriptors to allow for the creation of custom controllers for the
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the Pokken
Tournament Pro Pad as a Pro Controller. Physical design limitations prevent
the Pokken Controller from functioning at the same level as the Pro
Controller. However, by default most of the descriptors are there, with the
exception of Home and Capture. Descriptor modification allows us to unlock
these buttons for our use.
*/

/** \file
 *
 *  Main source file for the Joystick demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include <LUFA/Drivers/Peripheral/Serial.h>
#include "Joystick.h"

#define MAX_BUFFER 32
char commands[MAX_BUFFER];
int cmdIndex = 0;
int cmdLen = 0;
int count = 0;
bool active = false;
bool holdDown = false;
bool optimize = false;

char b[MAX_BUFFER];
uint8_t l = 0;
ISR(USART1_RX_vect)
{
	char c = fgetc(stdin);
	if (Serial_IsSendReady())
	{
		printf("%c", c);
	}
	if (c == '\r')
	{
		memcpy(commands, b, MAX_BUFFER);
		cmdIndex = 0;
		count = -1;
		active = true;
		cmdLen = l;
		holdDown = false;
		l = 0;
		memset(b, 0, sizeof(b));
	}
	else if (c == '!')
	{
		optimize = true;
	}
	else if (c == '?')
	{
		optimize = false;
	}
	else if (c != '\n' && l < MAX_BUFFER)
	{
		b[l++] = c;
	}
}

// Main entry point.
int main(void)
{
	Serial_Init(9600, false);
	Serial_CreateStream(NULL);

	sei();
	UCSR1B |= (1 << RXCIE1);

	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	for (;;)
	{
		// We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void)
{
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.

	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void)
{
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void)
{
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void)
{

	// We can handle two control requests: a GetReport and a SetReport.
	switch (USB_ControlRequest.bRequest)
	{
	// GetReport is a request for data from the device.
	case HID_REQ_GetReport:
		if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
		{
			// We'll create an empty report.
			USB_JoystickReport_Input_t JoystickInputData;
			// We'll then populate this report with what we want to send to the host.
			GetNextReport(&JoystickInputData);
			// Since this is a control endpoint, we need to clear up the SETUP packet on this endpoint.
			Endpoint_ClearSETUP();
			// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
			Endpoint_Write_Control_Stream_LE(&JoystickInputData, sizeof(JoystickInputData));
			// We then acknowledge an OUT packet on this endpoint.
			Endpoint_ClearOUT();
		}

		break;
	case HID_REQ_SetReport:
		if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
		{
			// We'll create a place to store our data received from the host.
			USB_JoystickReport_Output_t JoystickOutputData;
			// Since this is a control endpoint, we need to clear up the SETUP packet on this endpoint.
			Endpoint_ClearSETUP();
			// With our report available, we read data from the control stream.
			Endpoint_Read_Control_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData));
			// We then send an IN packet on this endpoint.
			Endpoint_ClearIN();
		}

		break;
	}
}

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void)
{
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	// We'll start with the OUT endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
	// We'll check to see if we received something on the OUT endpoint.
	if (Endpoint_IsOUTReceived())
	{
		// If we did, and the packet has data, we'll react to it.
		if (Endpoint_IsReadWriteAllowed())
		{
			// We'll create a place to store our data received from the host.
			USB_JoystickReport_Output_t JoystickOutputData;
			// We'll then take in that data, setting it up in our storage.
			Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL);
			// At this point, we can react to this data.
			// However, since we're not doing anything with this data, we abandon it.
		}
		// Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
		Endpoint_ClearOUT();
	}

	// We'll then move on to the IN endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
	// We first check to see if the host is ready to accept data.
	if (Endpoint_IsINReady())
	{
		// We'll create an empty report.
		USB_JoystickReport_Input_t JoystickInputData;
		// We'll then populate this report with what we want to send to the host.
		GetNextReport(&JoystickInputData);
		// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
		Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL);
		// We then send an IN packet on this endpoint.
		Endpoint_ClearIN();

		/* Clear the report data afterwards */
		// memset(&JoystickInputData, 0, sizeof(JoystickInputData));
	}
}

#define optimizable (optimize && cmdIndex + 1 < cmdLen)

#define ECHOES 2
int echoes = 0;
USB_JoystickReport_Input_t last_report;
// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t *const ReportData)
{
	/* Clear the report contents */
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
	ReportData->LX = STICK_CENTER;
	ReportData->LY = STICK_CENTER;
	ReportData->RX = STICK_CENTER;
	ReportData->RY = STICK_CENTER;
	ReportData->HAT = HAT_CENTER;
	ReportData->Button |= SWITCH_RELEASE;

	// Repeat ECHOES times the last report
	if (echoes > 0)
	{
		memcpy(ReportData, &last_report, sizeof(USB_JoystickReport_Input_t));
		echoes--;
		return;
	}

	// If completed, do nothing
	if (!active)
		goto fin;

	// If the last command was SoftDrop, keep pressing down
	if (holdDown)
	{
		ReportData->HAT = HAT_BOTTOM;
		goto fin;
	}
	count += 1;
	if (count % 2 != 0)
		goto fin;
	cmdIndex = count / 2;
	if (cmdIndex > cmdLen)
	{
		active = false;
		goto fin;
	}

	switch (commands[cmdIndex])
	{
	case 'H':
		ReportData->Button |= SWITCH_R;
		_delay_ms(85);
		break;
	case '<':
		ReportData->HAT = HAT_LEFT;
		if (!optimizable)
			break;

		switch (commands[cmdIndex + 1])
		{
		case 'A':
			ReportData->Button |= SWITCH_A;
			cmdIndex++;
			count += 2;
			break;
		case 'B':
			ReportData->Button |= SWITCH_B;
			cmdIndex++;
			count += 2;
			break;
		default:
			break;
		}

		break;
	case '>':
		ReportData->HAT = HAT_RIGHT;
		if (!optimizable)
			break;
		switch (commands[cmdIndex + 1])
		{
		case 'A':
			ReportData->Button |= SWITCH_A;
			cmdIndex++;
			count += 2;
			break;
		case 'B':
			ReportData->Button |= SWITCH_B;
			cmdIndex++;
			count += 2;
			break;
		default:
			break;
		}

		break;
	case 'D':
		ReportData->HAT = HAT_TOP;
		break;
	case 'S':
		holdDown = true;
		ReportData->HAT = HAT_BOTTOM;
		break;
	case 'A':
		ReportData->Button |= SWITCH_A;
		if (!optimizable)
			break;
		if (commands[cmdIndex + 1] == 'D')
		{
			ReportData->HAT = HAT_TOP;
			cmdIndex++;
			count += 2;
		}

		break;
	case 'B':
		ReportData->Button |= SWITCH_B;
		if (!optimizable)
			break;
		if (commands[cmdIndex + 1] == 'D')
		{
			ReportData->HAT = HAT_TOP;
			cmdIndex++;
			count += 2;
		}

		break;

	// Other buttons which are unnecessary to play PPT
	case 'd':
		ReportData->HAT = HAT_BOTTOM;
		break;
	case 'R':
		ReportData->Button |= SWITCH_R;
		break;
	case 'L':
		ReportData->Button |= SWITCH_L;
		break;
	case 'r':
		ReportData->Button |= SWITCH_ZR;
		break;
	case 'l':
		ReportData->Button |= SWITCH_ZL;
		break;
	case 'Y':
		ReportData->Button |= SWITCH_Y;
		break;
	case 'X':
		ReportData->Button |= SWITCH_X;
		break;
	case '+':
		ReportData->Button |= SWITCH_START;
		break;
	case '-':
		ReportData->Button |= SWITCH_SELECT;
		break;
	case 'c':
		ReportData->Button |= SWITCH_CAPTURE;
		break;
	case 'h':
		ReportData->Button |= SWITCH_HOME;
		break;
	}

fin:
	// Prepare to echo this report
	memcpy(&last_report, ReportData, sizeof(USB_JoystickReport_Input_t));
	echoes = ECHOES;
}
