/*

##    This file is pam_mhcrfid 0.1
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

pam_mhcrfid provides an easy way to auth with your gnu/linux box and phidgets rfid reader.

it simply read the tag value from a phidgets rfid reader and write it to a file, then compare it with authorized tag value in another file...

in my case use /etc/lasttag and /etc/authtag, both files should be root read only and contains tag hex value.. last tag gets cleaned every auth success.. authtag must be created by user..

pam_mhcrfid is for use on a single user personal machine, for people like me too lazy to type in pwds. with the same tag u'll get auth for all services like gdm, su, system-config utils and so on, depending on your personal pam configuration...

i'm sure that this code is really bad written because I'm really noob in programming, and i would like to give credits to all people that i copied from like web site C tutorials and other pam modules..

fell free to contact me for any info, comments and improvement of this code.

visit cmatthew.net for all information

to compile:

gcc -Wall -fPIC -c pam_mhcrfid.c

gcc -shared -o pam_mhcrfid.so pam_mhcrfid.o -lpam -lm -lphidget21

to install:

copy files to /lib/security/ or /lib64/security/

add to /etc/pam.d/system-auth or to pam service you want to use:

auth sufficient pam_mhcrfid.so 

note:

you should SUID kcheckpass if use kde for screensaver unlock

*/
#include <security/pam_modules.h>
#include <security/_pam_macros.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include <phidget21.h>

#define PAM_SM_AUTH
//#define PAM_SM_ACCOUNT
//#define PAM_SM_SESSION
//#define PAM_SM_PASSWORD

//----------- PhIDGETS STUF START-----------//

//handle
CPhidgetRFIDHandle rfid = 0;

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

//turn on led and write read tag to file
int TagHandler(CPhidgetRFIDHandle RFID, void *usrptr, unsigned char *buf)
{
	//turn on the Onboard LED
	//CPhidgetRFID_setLEDOn(RFID, 1);

	//external led output 1
	CPhidgetRFID_setOutputState(rfid, 1, 1);

	//external +5v output 0
	//CPhidgetRFID_setOutputState(rfid, 0, 1);

	fprintf(stderr, "Got: %02x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

	FILE *fp0;

	fp0 = fopen ("/etc/lasttag","w");

	fprintf(fp0, "%02x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

	fclose(fp0);

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

openlog("[pam_mhcrfid]", LOG_NDELAY, LOG_AUTHPRIV);

	//get service
	retval = pam_get_item(pamh, PAM_SERVICE,
			(const void **)(const void *)&service);

	if (retval != PAM_SUCCESS)
	{
		syslog(LOG_WARNING, "unable to retrieve the PAM service name.\n");
		return (PAM_AUTH_ERR);
	}

	//get user
	if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS || !user || !*user)
	{
		syslog(LOG_WARNING, "unable to retrieve the PAM user name.\n");
		return (PAM_AUTH_ERR);
	}

	//tell who and what
	fprintf(stderr, "mhc rfid: authentication request for user \"%s\" (%s)\n", user, service);

	//read tag
	mhc_rfidread();

	FILE *fp1, *fp2, *fp3;
	char ch1, ch2, same;
	unsigned long l;

	//last tag file
	fp1 = fopen ("/etc/lasttag","r");
	//auth tag file
	fp2 = fopen ("/etc/authtag","r");

	l = 0;
	same = 1;

	//compare tags in files
	while(!feof(fp1))
	{
	ch1 = fgetc(fp1);
	
		if(ferror(fp1))
		{
		exit(1);
		}

	ch2 = fgetc(fp2);

		if(ferror(fp2))
		{
		exit(1);
		}

		if(ch1 != ch2)
		{

		//looser
		fprintf(stderr, "mhc rfid: Looser!\n");

		//cloese files
		fclose(fp1);
		fclose(fp2);

		return (PAM_AUTH_ERR);

		same = 0;
		break;
		}
	l++;
  	}

	if(same)

	//winner
	fprintf(stderr, "mhc rfid: Winner!\n");

	//close files
	fclose(fp1);
	fclose(fp2);

	//reopen lasttag file and delete tag value
	fp3 = fopen ("/etc/lasttag","w");

	fprintf(fp3, "LinuxRocks\n");

	//close file
	fclose(fp3);

	return (PAM_SUCCESS);
}

PAM_EXTERN
int pam_sm_setcred(pam_handle_t *pamh,int flags,int argc
    ,const char **argv)
{
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
