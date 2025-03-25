/*

##    This file is pam_mhcrfid 0.1.4
##
##    pam_mhcrfid is free software; you can redistribute it and/or modify
##    it under the terms of the GNU General Public License as published by
##    the Free Software Foundation; version 2 of the License.
##
##    pam_mhcrfid is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##    You should have received a copy of the GNU General Public License
##    along with pam_mhcrfid; if not, write to the Free Software
##    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

visit cmatthew.net for all information

*/
#include <security/pam_modules.h>
#include <security/_pam_macros.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>

#include <phidget21.h>

#define PAM_SM_AUTH
//#define PAM_SM_ACCOUNT
//#define PAM_SM_SESSION
//#define PAM_SM_PASSWORD

//----------- PhIDGETS STUF START-----------//

//handle
CPhidgetRFIDHandle rfid = 0;

//tags
char lt[11], at[11];

//turn on antenna when attached
int AttachHandler(CPhidgetHandle RFID, void *userptr)
{
	CPhidgetRFID_setAntennaOn(rfid, 1);
	return 0;
}

//keep quiet
int DetachHandler(CPhidgetHandle RFID, void *userptr)
{
	return 0;
}

//keep quiet
int ErrorHandler(CPhidgetHandle RFID, void *userptr, int ErrorCode, const char *unknown)
{
	return 0;
}

//keep quiet
int OutputChangeHandler(CPhidgetRFIDHandle RFID, void *usrptr, int Index, int State)
{
	return 0;
}

//turn on led and write read tag to str
int TagHandler(CPhidgetRFIDHandle RFID, void *usrptr, unsigned char *buf)
{
	//turn on the Onboard LED
	//CPhidgetRFID_setLEDOn(RFID, 1);

	//external led output 1
	CPhidgetRFID_setOutputState(rfid, 1, 1);

	//external +5v output 0
	//CPhidgetRFID_setOutputState(rfid, 0, 1);

	//print tag value to screen debug
	//fprintf(stderr, "Got: %02x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

	//FILE *fp0;
	//fp0 = fopen ("/etc/lasttag","w");

	//print tag value to file old
	//fprintf(fp0, "%02x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
	//fclose(fp0);

	//print tag value to str
	sprintf(lt, "%02x%02x%02x%02x%02x", buf[0], buf[1], buf[2], buf[3], buf[4]);

	//debug
	//fprintf(stderr, "lt read id: %s\n", lt);

	return 0;
}

//turn off led
int TagLostHandler(CPhidgetRFIDHandle RFID, void *usrptr, unsigned char *buf)
{
	//turn off the Onboard LED
	//CPhidgetRFID_setLEDOn(RFID, 0);

	//external led output 1
	CPhidgetRFID_setOutputState(rfid, 1, 0);

	//external +5v output 0
	//CPhidgetRFID_setOutputState(rfid, 0, 0);

	//printf("Lost: %02x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

	return 0;
}

/*
int CPhidgetRFID_getLastTag(CPhidgetRFIDHandle RFID, unsigned char *buff)
{

	return 0;

}

int CPhidgetRFID_getTagStatus(CPhidgetRFIDHandle RFID, int *present)
{

	return 0;

}
*/

//keep quiet
int display_properties(CPhidgetRFIDHandle RFID)
{
	return 0;
}

//read pause close
int mhc_rfidread()
{
	CPhidgetRFID_create(&rfid);

	CPhidget_set_OnAttach_Handler((CPhidgetHandle)rfid, AttachHandler, NULL);
	CPhidget_set_OnDetach_Handler((CPhidgetHandle)rfid, DetachHandler, NULL);
	CPhidget_set_OnError_Handler((CPhidgetHandle)rfid, ErrorHandler, NULL);
	CPhidgetRFID_set_OnOutputChange_Handler(rfid, OutputChangeHandler, NULL);
	CPhidgetRFID_set_OnTag_Handler(rfid, TagHandler, NULL);
	CPhidgetRFID_set_OnTagLost_Handler(rfid, TagLostHandler, NULL);

	CPhidget_open((CPhidgetHandle)rfid, -1);

	//wait 3 sec to place tag
	sleep(3);

	//we have read antenna off
	CPhidgetRFID_setAntennaOn(rfid, 0);

	//turn of led just in case
	CPhidgetRFID_setOutputState(rfid, 1, 0);

	//delete and close
	CPhidget_close((CPhidgetHandle)rfid);
	CPhidget_delete((CPhidgetHandle)rfid);

	return 0;
}

//----------- PhIDGETS STUF END-----------//

PAM_EXTERN
int pam_sm_authenticate(pam_handle_t *pamh, int flags,
		int argc, const char **argv)
{
int retval;
const char *service;
const char *user;

openlog("[pam_mhcrfid]", LOG_PID, LOG_AUTH);

	//get service
	retval = pam_get_item(pamh, PAM_SERVICE,
			(const void **)(const void *)&service);

	if (retval != PAM_SUCCESS)
	{
		syslog(LOG_WARNING, "unable to retrieve the PAM service name.\n");
		return (PAM_AUTH_ERR);
		closelog();
	}

	//get user
	if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS || !user || !*user)
	{
		syslog(LOG_WARNING, "unable to retrieve the PAM user name.\n");
		return (PAM_AUTH_ERR);
		closelog();
	}

	//tell who and what to screen debug
	//fprintf(stderr, "mhc rfid: authentication request for user \"%s\" (%s)\n", user, service);

	//tell who and what to syslog
	syslog(LOG_WARNING, "authentication request for user \"%s\" (%s)\n", user, service);
	closelog();

	//read tag from reader
	mhc_rfidread();

	FILE *fp2;
	char file[100];

	//lets see if its root or user to call pam
	switch (strcmp ("root",user))
	{

	//pam is called by root
	case 0:

	//authtag file for root
	fp2 = fopen ("/root/.authtag","r");

		//check if authtag file exist to avid errors
		if (fp2==NULL)
		{
		syslog(LOG_WARNING,"unable to open authtag file");
		closelog();
		return (PAM_SERVICE_ERR);
		}
	break;

	//pam is called by user
	case 1:

	strcpy(file, "/home/");
	strcat(file, user);
	strcat(file, "/.authtag");
	
	//authtag file for user
	fp2 = fopen (file,"r");

		//check if authtag file exist to avid errors
		if (fp2==NULL)
		{
		syslog(LOG_WARNING,"unable to open authtag file");
		closelog();
		return (PAM_SERVICE_ERR);
		}
	break;
	}

	//print authtag to str
	sprintf(at, "%s", fgets(at, 11 , fp2));
	//debug
	//fprintf(stderr, "at for: %s is: %s\n", user, at);

	if ((strcmp (lt,at)) == 0)
	{

	//debug
	//fprintf(stderr, "lt is: %s and at is: %s\n", lt, at);

	//winner to syslog
	syslog(LOG_WARNING, "authentication granted for user \"%s\" (%s)\n", user, service);
	closelog();

	return (PAM_SUCCESS);
	}

	else 

	//debug
	//fprintf(stderr, "lt is: %s and at is: %s\n", lt, at);

	//looser to syslog
	syslog(LOG_WARNING, "authentication failure for user \"%s\" (%s)\n", user, service);
	closelog();

	return (PAM_AUTH_ERR);
}

PAM_EXTERN
int pam_sm_setcred(pam_handle_t *pamh,int flags,int argc
    ,const char **argv)
{
  //syslog(LOG_WARNING, "pam_sm_setcred called\n");
  return PAM_SUCCESS;
}

/*
PAM_EXTERN
int pam_sm_acct_mgmt(pam_handle_t *pamh,int flags,int argc
    ,const char **argv)
{
  return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_chauthtok(pam_handle_t *pamh,int flags,int argc
    ,const char **argv)
{
  return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_open_session(pam_handle_t *pamh,int flags,int argc
    ,const char **argv)
{
  return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_close_session(pam_handle_t *pamh,int flags,int argc
    ,const char **argv)
{
  return PAM_SUCCESS;
}
*/

#ifdef PAM_STATIC

struct pam_module _pam_mhcrfid_modstruct = {
	"pam_mhcrfid",
	pam_sm_authenticate,
	pam_sm_setcred,
	NULL,
	NULL,
	NULL,
	NULL
};

#endif
