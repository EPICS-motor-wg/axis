/*
FILENAME...     axisUtil.cc
USAGE...        Axis Record Utility Support.

*/


/*
*  axisUtil_A.c -- version 1.0
*
*  Original Author: Kevin Peterson (kmpeters@anl.gov)
*  Date: December 11, 2002
*
*  UNICAT
*  Advanced Photon Source
*  Argonne National Laboratory
*
*  Current Author: Ron Sluiter
*
* Modification Log:
* -----------------
* .01 05-10-07 rls - Bug fix for motorUtilInit()'s PVNAME_SZ error check using
*                    an uninitialized variable.
*                  - Added redundant initialization error check. 
* .02 03-11-08 rls - 64 bit compatability.
*                  - add printChIDlist() to iocsh.
*/

#include <stdio.h>
#include <string.h>
#include <cadef.h>
#include <dbDefs.h>
#include <epicsString.h>
#include <cantProceed.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <errlog.h>

#include <axis.h>

#define TIMEOUT 60 /* seconds */

/* ----- External Declarations ----- */
extern char **getAxisList();
/* ----- --------------------- ----- */

/* ----- Function Declarations ----- */
RTN_STATUS axisUtilInit(char *);
static int axisUtil_task(void *);
static chid getChID(char *);
static long pvMonitor(int, chid, int);
static void dmov_handler(struct event_handler_args);
static void allstop_handler(struct event_handler_args);
static void stopAll(chid, char *);
static int axisMovingCount();
static void moving(int, chid, short);
/* ----- --------------------- ----- */


typedef struct axis_pv_info
{
    char name[PVNAME_SZ];      /* pv names limited to 60 chars + term. in dbDefs.h */
    chid chid_dmov;     /* Channel id for <axis name>.DMOV */
    chid chid_stop;     /* Channel id for <axis name>.STOP */
    int in_motion;
    int index;          /* Call to ca_add_event() must have ptr to argument. */
} Axis_pv_info;


/* ----- Global Variables ----- */
int axisUtil_debug = 0;
int numAxiss = 0;
/* ----- ---------------- ----- */

/* ----- Local Variables  ----- */
static Axis_pv_info *axisArray;
static char **axislist = 0;
static char *vme;
static int old_numAxissMoving = 0;
static short old_alldone_value = 1;
static chid chid_allstop, chid_moving, chid_alldone, chid_movingdiff;
/* ----- ---------------- ----- */


RTN_STATUS axisUtilInit(char *vme_name)
{
    RTN_STATUS status = OK;
    static bool initialized = false;	/* axisUtil initialized indicator. */
    
    if (initialized == true)
    {
        printf( "axisUtil already initialized. Exiting\n");
        return ERROR;
    }

    if (strlen(vme_name) > PVNAME_SZ - 7 )
    {
        printf( "axisUtilInit: Prefix %s has more than %d characters. Exiting\n",
                vme_name, PVNAME_SZ - 7 );
        return ERROR;
    }

    initialized = true;
    vme = epicsStrDup(vme_name);

    epicsThreadCreate((char *) "axisUtil", epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      (EPICSTHREADFUNC) axisUtil_task, (void *) NULL);
    return(status);
}


static int axisUtil_task(void *arg)
{
    char temp[PVNAME_STRINGSZ+5];
    int itera, status;
    epicsEventId wait_forever;

    SEVCHK(ca_context_create(ca_enable_preemptive_callback),
           "axisUtil: ca_context_create() error");

    axislist = getAxisList();
    if (axisUtil_debug)
        errlogPrintf("There are %i axiss\n", numAxiss);
    
    if (numAxiss > 0)
    {
        axisArray = (Axis_pv_info *) callocMustSucceed(numAxiss,
                                   sizeof(Axis_pv_info), "axisUtil:init()");
   
        /* setup $(P)moving */
        strcpy(temp, vme);
        strcat(temp, "moving.VAL");
        chid_moving = getChID(temp);

        /* setup $(P)alldone */
        strcpy(temp, vme);
        strcat(temp, "alldone.VAL");
        chid_alldone = getChID(temp);

        /* setup $(P)movingDiff */
        strcpy(temp, vme);
        strcat(temp, "movingDiff.VAL");
        chid_movingdiff = getChID(temp);

	if (!chid_moving || !chid_alldone || !chid_movingdiff) {
	    errlogPrintf("Failed to connect to %smoving or %salldone or %smovingDiff.\n"
			 "Check prefix matches Db\n", vme, vme, vme);
	    ca_task_exit();
	    return ERROR;
	}

        /* loop over axiss in axislist and fill in axisArray */
        for (itera=0; itera < numAxiss; itera++)
        {
            axisArray[itera].index = itera;

            /* Setup .DMOVs */
            strcpy(axisArray[itera].name, axislist[itera]);
            strcpy(temp, axislist[itera]);
            strcat(temp, ".DMOV");
            axisArray[itera].chid_dmov = getChID(temp);
            status = pvMonitor(1, axisArray[itera].chid_dmov, itera);
                    
            /* Setup .STOPs */
            strcpy(temp, axislist[itera]);
            strcat(temp, ".STOP");
            axisArray[itera].chid_stop = getChID(temp);        
        }

        /* setup $(P)allstop */
        strcpy(temp, vme);
        strcat(temp, "allstop.VAL");
        chid_allstop = getChID(temp);
	if (!chid_allstop) {
	    errlogPrintf("Failed to connect to %sallstop\n",vme);
	} else {
	    status = pvMonitor(0, chid_allstop, -1);
	}
    }
    
    /* Wait on a (never signalled) event here, rather than suspending the
       thread, so as not to show up in the thread list as "SUSPENDED", which
       is usually a sign of a fault. */
    wait_forever = epicsEventCreate(epicsEventEmpty);
    if (wait_forever) {
	epicsEventMustWait(wait_forever);
    }
    return(ERROR);
}


static chid getChID(char *PVname)
{
    chid channelID = 0;
    int status;

    if (axisUtil_debug)
	errlogPrintf("getChID(%s)\n", PVname);

    /* In R3.14 ca_create_channel() will replace ca_search_and_connect() */
    status = ca_search_and_connect(PVname, &channelID, 0, 0);
    if (status == ECA_NORMAL) 
    {
	status = ca_pend_io(TIMEOUT);
    }

    if (status != ECA_NORMAL)
    {
        SEVCHK(status, "ca_search_and_connect");
        errlogPrintf("axisUtil.cc: getChID(%s) error: %i\n", PVname, status);
	channelID = 0;
    }
    return channelID;
}


static long pvMonitor(int eventType, chid channelID, int axis_index)
{
    int status; 

    /* Create monitor */
    if (eventType)      /* moving() */
        status = ca_add_event(DBR_SHORT, channelID, &dmov_handler,
                              &(axisArray[axis_index].index), 0);
    else                /* stopAll() */
        status = ca_add_event(DBR_STRING, channelID, &allstop_handler, 0, 0);
    status = ca_pend_io(TIMEOUT);

    if (status != ECA_NORMAL)
    {
        SEVCHK(status, "ca_add_event");
        ca_task_exit(); /* this is serious */
        return status;
    }

    return ECA_NORMAL;
}


static void allstop_handler(struct event_handler_args args)
{
    stopAll(args.chid, (char *) args.dbr);
}


static void stopAll(chid callback_chid, char *callback_value)
{
    int itera, status = 0;
    short val = 1, release_val = 0;
    
    if (callback_chid != chid_allstop)
        errlogPrintf("callback_chid = %p, chid_allstop = %p\n", callback_chid,
                      chid_allstop);
    
    if (strcmp(callback_value, "release") != 0)
    {
        /* if at least one axis is moving, then continue with stop all */
        if (axisMovingCount())
        {
            for(itera=0; itera < numAxiss; itera++)
	        /* Only stop a axis that is moving.  This should avoid problems caused by trying
		to stop axis records for which device and driver support have not been loaded.*/
                if (axisArray[itera].in_motion == 1)
		    ca_put(DBR_SHORT, axisArray[itera].chid_stop, &val);
            status = ca_flush_io(); 
        }

        /* reset allstop so that it may be called again */
        ca_put(DBR_SHORT, chid_allstop, &release_val);
        status = ca_flush_io();
        if (axisUtil_debug)
            errlogPrintf("reset allstop to \"release\"\n");
    }
    else if (axisUtil_debug)
	errlogPrintf("didn't need to reset allstop\n");
}


static void dmov_handler(struct event_handler_args args)
{
    moving(*((int *) args.usr), args.chid, *((short *) args.dbr));
}


static void moving(int callback_axis_index, chid callback_chid,
                   short callback_dmov)
{
    short new_alldone_value, done = 1, not_done = 0;
    int numAxissMoving, status;
    char diffChar;
    char diffStr[PVNAME_STRINGSZ+1];

    if (axisUtil_debug)            
        errlogPrintf("%s is %s\n", axisArray[callback_axis_index].name,
               (callback_dmov) ? "STOPPED" : "MOVING");

    if (callback_dmov)
    {                      
        axisArray[callback_axis_index].in_motion = 0;
        diffChar = '-';
    }
    else
    {
        axisArray[callback_axis_index].in_motion = 1;
        diffChar = '+';
    }
    
    numAxissMoving = axisMovingCount();

    new_alldone_value = (numAxissMoving) ? 0 : 1;
    
    /* check to see if $(P)alldone needs to be updated */
    if (new_alldone_value != old_alldone_value)
    {
        /* give $(P)alldone the appropriate value */
        if (numAxissMoving == 0)
        {
            if (axisUtil_debug)
                errlogPrintf("sending alldone = TRUE\n");

            ca_put(DBR_SHORT, chid_alldone, &done);
            old_alldone_value = new_alldone_value;
        }
        else
        {
            if (axisUtil_debug)
                errlogPrintf("sending alldone = FALSE\n");

            ca_put(DBR_SHORT, chid_alldone, &not_done);
            old_alldone_value = new_alldone_value;
        }
    }
    else if (axisUtil_debug)
	errlogPrintf("the alldone value remains the same.\n");

    /* check to see if $(P)moving needs to be updated */
    if (numAxissMoving != old_numAxissMoving)
    {
        if (axisUtil_debug)
            errlogPrintf("updating number of axiss moving\n");

        /* give $(P)moving the appropriate value */
        ca_put(DBR_LONG, chid_moving, &numAxissMoving);
	
	/* Tell which axis's dmov changed */
	sprintf(diffStr, "%c%s", diffChar, axisArray[callback_axis_index].name);
	ca_array_put(DBR_CHAR, strlen(diffStr)+1, chid_movingdiff, diffStr); 

        old_numAxissMoving = numAxissMoving;
    }
    else if (axisUtil_debug)
	errlogPrintf("the number of axiss moving remains the same.\n");
    
    /* send the ca_puts */
    status = ca_flush_io();
}


static int axisMovingCount()
{
    int itera, in_motion_count=0;

    for (itera=0; itera < numAxiss; itera++)
        in_motion_count += axisArray[itera].in_motion;

    return in_motion_count;
}


void listMovingAxiss()
{
    int itera;
  
    errlogPrintf("\nThe following axiss are moving:\n");
    
    for (itera=0; itera < numAxiss; itera++)
        if (axisArray[itera].in_motion == 1)
            errlogPrintf("%s, index = %i\n", axisArray[itera].name,
                   axisArray[itera].index);
}


void printChIDlist()
{
    int itera;

    for (itera=0; itera < numAxiss; itera++)
    {
        errlogPrintf("i = %i,\tname = %s\tchid_dmov = %p\tchid_stop = \
               %p\tin_motion = %i\tindex = %i\n", itera,
               axisArray[itera].name, axisArray[itera].chid_dmov,
               axisArray[itera].chid_stop, axisArray[itera].in_motion,
               axisArray[itera].index);
    }
    
    errlogPrintf("chid_allstop = %p\n", chid_allstop);
    errlogPrintf("chid_alldone = %p\n", chid_alldone);
    errlogPrintf("chid_moving = %p\n",  chid_moving);
}


extern "C"
{

static const iocshArg Arg = {"IOC name", iocshArgString};
static const iocshArg * const axisUtilArg[1]  = {&Arg};
static const iocshFuncDef axisUtilDef  = {"axisUtilInit", 1, axisUtilArg};

static void axisUtilCallFunc(const iocshArgBuf *args)
{
    axisUtilInit(args[0].sval);
}

static const iocshArg ArgP = {"Print axisUtil chid list", iocshArgString};
static const iocshArg * const printChIDArg[1]  = {&ArgP};
static const iocshFuncDef printChIDDef  = {"printChIDlist", 1, printChIDArg};

static void printChIDCallFunc(const iocshArgBuf *args)
{
    printChIDlist();
}

static const iocshArg ArgL = {"List moving axiss", iocshArgString};
static const iocshArg * const listMovingAxissArg[1]  = {&ArgL};
static const iocshFuncDef listMovingAxissDef  = {"listMovingAxiss", 1, listMovingAxissArg};

static void listMovingAxissCallFunc(const iocshArgBuf *args)
{
    listMovingAxiss();
}

static void axisUtilRegister(void)
{
    iocshRegister(&axisUtilDef,  axisUtilCallFunc);
    iocshRegister(&printChIDDef,  printChIDCallFunc);
    iocshRegister(&listMovingAxissDef,  listMovingAxissCallFunc);
}

epicsExportRegistrar(axisUtilRegister);
epicsExportAddress(int, axisUtil_debug);

} // extern "C"

