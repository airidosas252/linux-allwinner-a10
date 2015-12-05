/****************************************************************************
 *  (c) Copyright 2007 Wi-Fi Alliance.  All Rights Reserved
 *
 *
 *  LICENSE
 *
 *  License is granted only to Wi-Fi Alliance members and designated
 *  contractors ($B!H(BAuthorized Licensees$B!I(B)..AN  Authorized Licensees are granted
 *  the non-exclusive, worldwide, limited right to use, copy, import, export
 *  and distribute this software:
 *  (i) solely for noncommercial applications and solely for testing Wi-Fi
 *  equipment; and
 *  (ii) solely for the purpose of embedding the software into Authorized
 *  Licensee$B!G(Bs proprietary equipment and software products for distribution to
 *  its customers under a license with at least the same restrictions as
 *  contained in this License, including, without limitation, the disclaimer of
 *  warranty and limitation of liability, below..AN  The distribution rights
 *  granted in clause
 *  (ii), above, include distribution to third party companies who will
 *  redistribute the Authorized Licensee$B!G(Bs product to their customers with or
 *  without such third party$B!G(Bs private label. Other than expressly granted
 *  herein, this License is not transferable or sublicensable, and it does not
 *  extend to and may not be used with non-Wi-Fi applications..AN  Wi-Fi Alliance
 *  reserves all rights not expressly granted herein..AN
 *.AN
 *  Except as specifically set forth above, commercial derivative works of
 *  this software or applications that use the Wi-Fi scripts generated by this
 *  software are NOT AUTHORIZED without specific prior written permission from
 *  Wi-Fi Alliance.
 *.AN
 *  Non-Commercial derivative works of this software for internal use are
 *  authorized and are limited by the same restrictions; provided, however,
 *  that the Authorized Licensee shall provide Wi-Fi Alliance with a copy of
 *  such derivative works under a perpetual, payment-free license to use,
 *  modify, and distribute such derivative works for purposes of testing Wi-Fi
 *  equipment.
 *.AN
 *  Neither the name of the author nor "Wi-Fi Alliance" may be used to endorse
 *  or promote products that are derived from or that use this software without
 *  specific prior written permission from Wi-Fi Alliance.
 *
 *  THIS SOFTWARE IS PROVIDED BY WI-FI ALLIANCE "AS IS" AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY, NON-INFRINGEMENT AND FITNESS FOR A.AN PARTICULAR PURPOSE,
 *  ARE DISCLAIMED. IN NO EVENT SHALL WI-FI ALLIANCE BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, THE COST OF PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE) ARISING IN ANY WAY OUT OF
 *  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. ******************************************************************************
 */


/*
 * File: wfa_ca.c
 *       This is the main program for Control Agent.
 *
 * Revision History:
 *   2006/06/01 -- BETA Release by qhu
 *   2006/06/13 -- 00.02 Release by qhu
 *   2006/06/30 -- 00.10 Release by qhu
 *   2006/07/10 -- 01.00 Release by qhu
 *   2006/09/01 -- 01.05 Release by qhu
 *   2007/01/11 -- 01.10 released by qhu
 *   2007/02/15 -- WMM beta released by qhu, mkaroshi
 *   2007/03/21 -- 01.40 WPA2 and Official WMM Beta release by qhu
 *   2007/04/20 -- 02.00 WPA2 and Official WMM release by qhu
 *   2007/08/15 --  02.10 WMM-Power Save release by qhu
 *   2007/10/10 --  02.20 Voice SOHO beta -- qhu
 *   2007/11/07 -- 02.30 Voice HSO -- qhu
 *      -- on the calls wfaCtrlSend(), the string len is replaced with strlen()
 */
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>

#include "wfa_debug.h"
#include "wfa_main.h"
#include "wfa_types.h"
#include "wfa_tlv.h"
#include "wfa_tg.h"
#include "wfa_cmds.h"
#include "wfa_sock.h"
#include "wfa_miscs.h"
#include "wfa_ca.h"
#include "wfa_agtctrl.h"
#include "wfa_agt.h"
#include "wfa_rsp.h"
#include "wfa_wmmps.h"


#define MAC_ADDR_LEN		18
#define DEVICE_NAME_LEN		20
#define TRANSPORT_TYPE_LEN	10
#define SERVER_IP_LEN		20
#define INTERFACE_NAME_LEN	31
#define RESPONSE_SIZE		256

char gnetIf[WFA_BUFF_32]; /* specify the interface to use */
int adj_latency;          /* adjust sleep time due to latency */
int isExit = 1;
int gRegSec = 1;         /* regularly periodical timeout */
int gtimeOut = 0;        /* timeout value for select call in usec */
int gSock = -1,xcSockfd = -1, gtgSend, gtgRecv, gtgTransac;
int rwl_wifi_flag = 0;	/*Flag to check wheather the transport is wifi */
BYTE *xcCmdBuf = NULL, *parmsVal = NULL, *trafficBuf = NULL, *respBuf = NULL;

/* stream table */
tgStream_t *gStreams;
struct timeval *toutvalp;

#ifdef WFA_WMM_EXT
/*
 * Thread Synchronize flags
 */
tgWMM_t wmm_thr[WFA_THREADS_NUM];
extern void *wfa_wmm_thread(void *thr_param);
extern void *wfa_wmmps_thread();
BOOL gtgStartSync = 0;       /* flag to sync End2End Time handshaking */
BOOL gtgFinishSync = 0;      /* flag to sync End2End Time handshaking */
double min_rttime = 0xFFFFFFFF;

#ifdef WFA_WMM_PS_EXT
BOOL       gtgWmmPS = 0;
unsigned long psTxMsg[WFA_BUFF_512];
unsigned long psRxMsg[WFA_BUFF_512];
extern int psSockfd ;
extern struct apts_msg apts_msgs[];
extern void BUILD_APTS_MSG(int msg, unsigned long *txbuf);
extern int wfaWmmPowerSaveProcess(int sockfd);
void *g_wl_handle = NULL;

int wl_set(void *wl, int cmd, void *buf, int len)
{
	return 0;
}
#endif /* WFA_WMM_PS_EXT */
#endif /* WFA_WMM_EXT */


#ifndef DONE
#define DONE 1
#endif

extern typeNameStr_t nameLocalStr[];
/* For integration */
extern dutCommandRespFuncPtr wfaCmdRespProcFuncTbl[];
extern void buildDutCommandRespProcessTable(void);
extern typeNameStr_t nameStr[];
extern char gRespStr[];
extern sockfd_t gCaSockfd;
extern unsigned short wfa_defined_debug;
extern char* rwl_client_path;
extern xcCommandFuncPtr gWfaCmdFuncTbl[]; /* command process functions */
extern char gCmdStr[];
extern dutCmdResponse_t gGenericResp;
extern int clock_drift_ps;
extern int error_check(int errno_defined);
/*
 * the output format can be redefined for file output.
 */
int
main(int argc, char *argv[])
{
	char command[WFA_BUFF_1K], respStr[WFA_BUFF_128], cmdName[WFA_BUFF_32];
	char *pcmdStr, *trafficPath;
	unsigned short myport;
	int maxfdn1 = -1, nfds, i, isFound, DutCmd, nbytes = 0, tag, slen;
	int cmdLen = WFA_BUFF_1K, respLen = 0;
	int errno_defined;
	int length_client_path;
	sockfd_t tmsockfd;
	BYTE xcCmdBuf[WFA_BUFF_1K], xcCmdTag, pcmdBuf[WFA_BUFF_1K];
	BYTE *respBuf = NULL, *parmsVal = NULL;
	FILE* fp = NULL;
	fd_set sockSet;
	char *rwl_exe_path;
	rwl_exe_path = malloc(WFA_BUFF_1K);
	
/* CA assumes that wl.exe will be stored in the current working directory */
	get_rwl_exe_path(rwl_exe_path, WFA_BUFF_1K);
	if (argc < 3) {
		DPRINT_ERR(WFA_ERR, "Usage: %s <control agent IP (Win XP)/interface(linux) > <control port no> <--socket/--dongle/--wifi> <serverIp  port/clientComport/serverMacaddr>\n", argv[0]);
		return 0;
	}

    /* isdigit() is not working with the build server tagged build 
	 * so isdigit is replaced with isNumber() function call */

	if (isNumber(argv[2]) == FALSE) {
		DPRINT_ERR(WFA_ERR, "incorrect port number\n");
		return 0;
	}

	myport = atoi(argv[2]);

	/* interface_validation call checks isString() for linux and
	 * isIPV4() for win32 OS
	 */
	if (interface_validation(argv[1]) == FALSE) {
		DPRINT_ERR(WFA_ERR, "incorrect network interface\n");
		return 0;
	}

	if((errno_defined = Start_Socket_Service()) != 0){
		DPRINT_ERR(WFA_ERR, "Start_Socket_Service failed\n");
		return 0;
	}

	strncpy(gnetIf, argv[1], INTERFACE_NAME_LEN); /* For integration */

	if ((rwl_client_path = malloc(WFA_BUFF_256))== NULL) {
		DPRINT_ERR(WFA_ERR, "malloc failed\r\n");
		if((errno_defined = Stop_Socket_Service()) != 0){
			DPRINT_ERR(WFA_ERR, "Stop_Socket_Service failed\n");
		}
		return 0;
	}

	/* get the command line args to get the rwl exe working on different
	 * transport links - socket/dongle/wifi */

	argv += 3;
	strcpy(rwl_client_path, rwl_exe_path);
	while (*argv) {
		strncat(rwl_client_path, " ", 1);
		strncat(rwl_client_path, *argv, strlen(*argv));
		*argv++;
	}
	length_client_path = strlen(rwl_client_path);
	/* Look for wifi transport */
	if(strstr(rwl_client_path, "--wifi") != NULL) {
		rwl_wifi_flag = 1; 
	} else {
		rwl_wifi_flag = 0;
	}
	/* Allocate buffers */
	if ((parmsVal = malloc(MAX_PARMS_BUFF))== NULL) {
		DPRINT_ERR(WFA_ERR, "malloc failed allocating parmsVal\n");
		if((errno_defined = Stop_Socket_Service()) != 0){
			DPRINT_ERR(WFA_ERR, "Stop_Socket_Service failed\n");
		}
		return 0;
	}

	if ((respBuf = malloc(WFA_BUFF_512))== NULL) {
		DPRINT_ERR(WFA_ERR, "malloc failed allocating respBuf\n");
		if((errno_defined = Stop_Socket_Service()) != 0){
			DPRINT_ERR(WFA_ERR, "Stop_Socket_Service failed\n");
		}
		free(parmsVal);
		return 0;
	}
	DPRINT_INFO(WFA_OUT, "rwl_client_path = %s\n", rwl_client_path);
	
	
	/* Create TCP socket for getting the commands from tc_cli */
	if ((tmsockfd = wfaCreateTCPServSock(myport))== -1) {
		DPRINT_ERR(WFA_ERR, "Failed to open socket\n");
	    if((errno_defined = Stop_Socket_Service()) != 0){
			DPRINT_ERR(WFA_ERR, "Stop_Socket_Service failed\n");
		}
		return 0;
	}

	maxfdn1 = tmsockfd + 1;
	FD_ZERO(&sockSet);

	for (;;)
	{
		FD_ZERO(&sockSet);
		FD_SET(tmsockfd, &sockSet);
		maxfdn1 = tmsockfd + 1;

		if (gCaSockfd != -1) {
			FD_SET(gCaSockfd, &sockSet);
			if (maxfdn1 < (int)gCaSockfd)
				maxfdn1 = gCaSockfd + 1;
		}
		/*
		* The timer will be set for transaction traffic if no echo is back
		* The timeout from the select call force to send a new packet
		*/
		nfds = 0;
		if ((nfds = select(maxfdn1, &sockSet, NULL, NULL, NULL)) < 0) {
			if (error_check(errno_defined))
				continue;
			else
				DPRINT_WARNING(WFA_WNG, "select error %i", errno_defined);
		}

		DPRINT_INFO(WFA_OUT, "new event \n");

		if (FD_ISSET(tmsockfd, &sockSet)) {
			gCaSockfd = wfaAcceptTCPConn(tmsockfd);
			DPRINT_INFO(WFA_OUT, "accept new connection\n");
				FD_SET(gCaSockfd, &sockSet);
		}


		if (gCaSockfd > 0 && FD_ISSET(gCaSockfd, &sockSet)) {
			memset(xcCmdBuf, 0, WFA_BUFF_1K);
			memset(gRespStr, 0, WFA_BUFF_512);
			nbytes = wfaCtrlRecv(gCaSockfd, xcCmdBuf);
			if (nbytes <= 0) {
				asd_shutDown(gCaSockfd);
				asd_closeSocket(gCaSockfd);
				gCaSockfd = -1;
				continue;
			}

			memset(respStr, 0, WFA_BUFF_128);
			sprintf(respStr, "status,RUNNING\r\n");
			wfaCtrlSend(gCaSockfd, (BYTE *)respStr, strlen(respStr));
			/* WFA Comment :having this is for slowing down unexpected 
			 * output result on CLI command sometimes 
			 */
			asd_sleep(1);
			DPRINT_INFO(WFA_OUT, "%s\n", respStr);
			DPRINT_INFO(WFA_OUT, "message %s %i\n", xcCmdBuf, nbytes);
			slen = (int )strlen((char *)xcCmdBuf);
			strncpy(command, (char*)xcCmdBuf, strlen((char*)xcCmdBuf));
		   /* The carriage return and newline character need to be
			* removed before sending the command to the DUT for the TCL 
			* scripts to run correctly.
			*/
			strtok(command, "\r\n");
			DPRINT_INFO(WFA_OUT, "last %x last-1  %x last-2 %x last-3 %x\n", cmdName[slen], cmdName[slen-1], cmdName[slen-2], cmdName[slen-3]);

			xcCmdBuf[slen-3] = '\0';

			isFound = 0;
			DutCmd = 0;
			/* tokenize for the command name. Rest of the command buffer
		 	 * is copied to pcmdStr for command processing later.
		 	 */
			memcpy(cmdName, strtok_r((char *)xcCmdBuf, ",", (char **)&pcmdStr), 32);
			i = 0;
			/* Check if we need to execute the command using rwl client */
			while (nameStr[i].type != -1) {
				if ((strcmp(nameStr[i].name, cmdName) == 0)) {
					DutCmd = 1; /* Execute the command on the server e.g tg commands */
					break;
				}
				i++;
			}

			/* Search for the command to be executed using rwl client */
			if (!DutCmd) {
				i = 0;
				while (nameLocalStr[i].type != -1) {
					if ((strcmp(nameLocalStr[i].name, cmdName) == 0)) {
						/* Found a command to be executed using rwl client */
						isFound = 1;
						break;
					}
					i++;
				}
			} /* !DutCmd */

			DPRINT_INFO(WFA_OUT, "cmdName is %s\n", cmdName);

			memset(pcmdBuf, 0, WFA_BUFF_1K);

			/* Check for the valid command and the valid arguements for the command 
			 * and return STATUS INVLID for commands that do not exist or if the arguements
			 * for the command are invalid.
			 */
			if ((!isFound && !DutCmd)
				|| (DutCmd && (nameStr[i].cmdProcFunc(pcmdStr, pcmdBuf, &cmdLen)==FALSE))
			    || (!DutCmd && (nameLocalStr[i].cmdProcFunc(pcmdStr, pcmdBuf, &cmdLen)==FALSE))) {
					asd_sleep(1);
					sprintf(respStr, "status,INVALID\r\n");
					wfaCtrlSend(gCaSockfd, (BYTE *)respStr, WFA_BUFF_128);/* Buffer size modified from strlen(respStr) on 21/11/07 */
					DPRINT_WARNING(WFA_WNG, "Incorrect command syntax\n");
					continue;
			} 
		   /*
			* Decode the command that is parsed above to find the actual function pointer
			* that needs to be executed. (decode for xcCmdTag )
			* Commands that use rwl client are processed here.
			*/
			if (!DutCmd && isFound) {
				/* reset two commond storages used by control functions */
				wfaDecodeTLV(pcmdBuf, cmdLen, &xcCmdTag, &cmdLen, parmsVal);
				memset(respBuf, 0, WFA_BUFF_512);
				respLen = 0;

				/* reset two commond storages used by control functions */
				memset(gCmdStr, 0, WFA_CMD_STR_SZ);
				/* command process function defined in wfa_cs.c or wfa_tg.c */
				gWfaCmdFuncTbl[xcCmdTag](cmdLen, parmsVal, &respLen, (BYTE *)respBuf);

				tag = ((wfaTLV *)respBuf)->tag;

				DPRINT_INFO(WFA_OUT, "bytes=%i,%i,%x %x %x %x \n", ((wfaTLV *)respBuf)->tag,((wfaTLV *)respBuf)->len, *(respBuf+4), *(respBuf+5), *(respBuf+6), *(respBuf+7));

				DPRINT_INFO(WFA_OUT, "tag %i \n", tag-WFA_STA_COMMANDS_END);

				/* Response for the executed command is updated using the below function pointer
				 * table
				 */
				if ((tag != 0 && tag < WFA_STA_RESPONSE_END) &&
					wfaCmdRespProcFuncTbl[tag-WFA_STA_COMMANDS_END] != NULL) {
					wfaCmdRespProcFuncTbl[tag-WFA_STA_COMMANDS_END](respBuf);
				} else {
					DPRINT_WARNING(WFA_WNG, "function not defined\n");
					memset(respBuf, 0, sizeof (respBuf));
					memset(pcmdBuf, 0, sizeof (pcmdBuf));
				}
			} else {
				/* Commands that need to be executed at DUT are processed here
				 * TG commands for example
				 */
				 /*In case of Multiple streams the commands will have space after each 
				  * streamid. So Look for \n for the end of command.
				  */
				strtok(command, "\n");
				trafficPath = malloc(WFA_BUFF_1K);
				strcpy(trafficPath, rwl_client_path);

				/* e.g wl --socket <IP Addr> <Port no> asd ca_get_version */
				strncat(trafficPath, " asd ", 5);
				strncat(trafficPath, command, strlen(command));
				strncat(trafficPath, " > ", 3);
				if ((fp = asd_cmd_exec(trafficPath)) == NULL)
				continue;
				memset(trafficPath, 0, WFA_BUFF_1K);

				if (fread(trafficPath, sizeof(char), RESPONSE_SIZE, fp) <= 0)
					strcpy(trafficPath, "status,ERROR\r\n");
				
				DPRINT_INFO(WFA_OUT, "%s %d\n", trafficPath, strlen(trafficPath));

				wfaCtrlSend(gCaSockfd, (BYTE *)trafficPath, strlen(trafficPath));
				file_cleanup(fp);
												
				memset(command, 0, strlen(command));
				free(trafficPath);

			} /* DutCmd */
		} /* done with  if(gCaSockfd) */
	} /* for */
    if((errno_defined = Stop_Socket_Service()) != 0){
		DPRINT_ERR(WFA_ERR, "Stop_Socket_Service failed\n");
	}
	asd_closeSocket(gCaSockfd);
	return 0;
}
