/*

##    This file is mhc_rfid
##
##    mhc_rfid is free software; you can redistribute it and/or modify
##    it under the terms of the GNU General Public License as published by
##    the Free Software Foundation; version 2 of the License.
##
##    mhc_rfid is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##    You should have received a copy of the GNU General Public License
##    along with mhc_rfid; if not, write to the Free Software
##    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

mhc_rfid is a small program to test phidgets rfid reader, it will read tag to output hex value nad turn on external led for testing..

note that only external led will turn on with successfully read tag, not internal led or +5v output..

i would like to give credits to all people that i copied from like web site C tutorials, phidgets example code and code from phidgets forums...

fell free to contact me for any info, comments and improvement of this code...

visit cmatthew.net for all information

to compile:

cc -o mhc_rfid mhc_rfid.c -g -O0 -Wall  -lphidget21 -lm

*/
#include <stdio.h>
#include <phidget21.h>

CPhidgetRFIDHandle rfid = 0;

int AttachHandler(CPhidgetHandle RFID, void *userptr)
{
	int serialNo;
	const char *name;

	CPhidget_getDeviceName (RFID, &name);
	CPhidget_getSerialNumber(RFID, &serialNo);
	printf("%s %10d attached!\n", name, serialNo);

	CPhidgetRFID_setAntennaOn(rfid, 1);

	return 0;
}

int DetachHandler(CPhidgetHandle RFID, void *userptr)
{
	int serialNo;
	const char *name;

	CPhidget_getDeviceName (RFID, &name);
	CPhidget_getSerialNumber(RFID, &serialNo);
	printf("%s %10d detached!\n", name, serialNo);

	return 0;
}

int ErrorHandler(CPhidgetHandle RFID, void *userptr, int ErrorCode, const char *unknown)
{
	printf("Error handled. %i - %s\n", ErrorCode, unknown);

	return 0;
}

int OutputChangeHandler(CPhidgetRFIDHandle RFID, void *usrptr, int Index, int State)
{
	if(Index == 0 || Index == 1)
	{
		printf("Output: %d > State: %d\n", Index, State);
	}

	return 0;
}

int TagHandler(CPhidgetRFIDHandle RFID, void *usrptr, unsigned char *buf)
{
	//turn on the Onboard LED
	//CPhidgetRFID_setLEDOn(RFID, 1);

	//external led output 1
	CPhidgetRFID_setOutputState(rfid, 1, 1);

	//external +5v output 0
	//CPhidgetRFID_setOutputState(rfid, 0, 1);

	printf("Got: %02x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

	return 0;
}

int TagLostHandler(CPhidgetRFIDHandle RFID, void *usrptr, unsigned char *buf)
{
	//turn off the Onboard LED
	//CPhidgetRFID_setLEDOn(RFID, 0);

	//external led output 1
	CPhidgetRFID_setOutputState(rfid, 1, 0);

	//external +5v output 0
	//CPhidgetRFID_setOutputState(rfid, 0, 0);

	printf("Lost: %02x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

	return 0;
}

int display_properties(CPhidgetRFIDHandle RFID)
{
	int serialNo, version, numOutputs, antennaOn, LEDOn;
	const char* ptr;

	CPhidget_getDeviceType((CPhidgetHandle)RFID, &ptr);
	CPhidget_getSerialNumber((CPhidgetHandle)RFID, &serialNo);
	CPhidget_getDeviceVersion((CPhidgetHandle)RFID, &version);

	CPhidgetRFID_getNumOutputs (RFID, &numOutputs);
	CPhidgetRFID_getAntennaOn (RFID, &antennaOn);
	CPhidgetRFID_getLEDOn (RFID, &LEDOn);


	printf("%s\n", ptr);
	printf("Serial Number: %10d\nVersion: %8d\n", serialNo, version);
	printf("# Outputs: %d\n\n", numOutputs);
	printf("Antenna Status: %d\nOnboard LED Status: %d\n", antennaOn, LEDOn);

	return 0;
}

int mhc_rfid_simple()
{
	int result;
	const char *err;

	CPhidgetRFID_create(&rfid);

	CPhidget_set_OnAttach_Handler((CPhidgetHandle)rfid, AttachHandler, NULL);
	CPhidget_set_OnDetach_Handler((CPhidgetHandle)rfid, DetachHandler, NULL);
	CPhidget_set_OnError_Handler((CPhidgetHandle)rfid, ErrorHandler, NULL);

	CPhidgetRFID_set_OnOutputChange_Handler(rfid, OutputChangeHandler, NULL);

	CPhidgetRFID_set_OnTag_Handler(rfid, TagHandler, NULL);

	CPhidgetRFID_set_OnTagLost_Handler(rfid, TagLostHandler, NULL);

	CPhidget_open((CPhidgetHandle)rfid, -1);

	printf("Waiting for RFID to be attached....");
	if((result = CPhidget_waitForAttachment((CPhidgetHandle)rfid, 5000)))
	{
		CPhidget_getErrorDescription(result, &err);
		printf("Problem waiting for attachment: %s\n", err);

		return 0;
	}

	CPhidgetRFID_setAntennaOn(rfid, 0);

	display_properties(rfid);

	printf("Press any key to start.....\n");
	getchar();

	CPhidgetRFID_setAntennaOn(rfid, 1);

	printf("Antenna ON Reading.....\n");

	printf("Press any key to end.....\n");
	getchar();

	printf("Closing...\n");

	CPhidgetRFID_setAntennaOn(rfid, 0);

	CPhidgetRFID_setLEDOn(rfid, 0);

	CPhidgetRFID_setOutputState(rfid, 1, 0);

	CPhidget_close((CPhidgetHandle)rfid);
	CPhidget_delete((CPhidgetHandle)rfid);

	return 0;
}

int main(int argc, char* argv[])
{
	mhc_rfid_simple();

	return 0;
}
