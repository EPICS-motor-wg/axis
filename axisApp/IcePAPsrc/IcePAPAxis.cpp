/*
  FILENAME... IcePAPAxis.cpp
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <epicsThread.h>

#include "IcePAP.h"

#ifndef ASYN_TRACE_INFO
#define ASYN_TRACE_INFO      0x0040
#endif

/* Page 21 */
#define STATUS_BIT_0         (1<<0)
#define STATUS_BIT_1         (1<<1)
#define STATUS_BIT_2         (1<<2)
#define STATUS_BIT_3         (1<<3)

#define STATUS_BITS_DISABLE   ((1<<4) || (1<<5) || (1<<6))

#define STATUS_BIT_7         (1<<7)
#define STATUS_BIT_8         (1<<8)
#define STATUS_BIT_READY     (1<<9)
#define STATUS_BIT_MOVING    (1<<10)
#define STATUS_BIT_SETTLING  (1<<11)
#define STATUS_BIT_OUTOFWIN  (1<<12)
#define STATUS_BIT_13        (1<<13)
#define STATUS_BIT_14        (1<<14)
#define STATUS_BIT_15        (1<<15)
#define STATUS_BIT_16        (1<<16)
#define STATUS_BIT_17        (1<<17)
#define STATUS_BIT_LIMIT_POS (1<<18)
#define STATUS_BIT_LIMIT_NEG (1<<19)
#define STATUS_BIT_HSIGNAL   (1<<20)
#define STATUS_BIT_POWERON   (1<<23)
#define STATUS_BIT_14        (1<<14)
#define STATUS_BIT_15        (1<<15)
#define STATUS_BIT_16        (1<<16)
#define STATUS_BIT_17        (1<<17)
#define STATUS_BIT_18        (1<<14)
#define STATUS_BIT_19        (1<<15)
#define STATUS_BIT_20        (1<<20)
#define STATUS_BIT_21        (1<<21)
#define STATUS_BIT_22        (1<<22)
#define STATUS_BIT_23        (1<<23)
#define STATUS_BIT_24        (1<<24)
#define STATUS_BIT_25        (1<<25)
#define STATUS_BIT_26        (1<<26)
#define STATUS_BIT_27        (1<<27)
#define STATUS_BIT_28        (1<<28)
#define STATUS_BIT_29        (1<<29)
#define STATUS_BIT_30        (1<<30)
#define STATUS_BIT_31        (1<<31)

//
// These are the IcePAPAxis methods
//

/** Creates a new IcePAPAxis object.
 * \param[in] pC Pointer to the IcePAPController to which this axis belongs.
 * \param[in] axisNo Index number of this axis, range 1 to pC->numAxes_. (0 is not used)
 *
 *
 * Initializes register numbers, etc.
 */
IcePAPAxis::IcePAPAxis(IcePAPController *pC, int axisNo,
                       int axisFlags, const char *axisOptionsStr)
  : asynAxisAxis(pC, axisNo),
    pC_(pC)
{
  memset(&drvlocal, 0, sizeof(drvlocal));
  drvlocal.errbuf[1] = ' '; /* trigger setStringParam(pC_->eemcuErrMsg_ */
  drvlocal.cfg.axisFlags = axisFlags;
  if (axisFlags & AMPLIFIER_ON_FLAG_USING_CNEN) {
    setIntegerParam(pC->motorStatusGainSupport_, 1);
  }
  if (axisOptionsStr && axisOptionsStr[0]) {
    const char * const encoder_is_str = "encoder=";

    char *pOptions = strdup(axisOptionsStr);
    char *pThisOption = pOptions;
    char *pNextOption = pOptions;

    while (pNextOption && pNextOption[0]) {
      pNextOption = strchr(pNextOption, ';');
      if (pNextOption) {
        *pNextOption = '\0'; /* Terminate */
        pNextOption++;       /* Jump to (possible) next */
      }
      if (!strncmp(pThisOption, encoder_is_str, strlen(encoder_is_str))) {
        pThisOption += strlen(encoder_is_str);
        drvlocal.cfg.externalEncoderStr = strdup(pThisOption);
        setIntegerParam(pC->motorStatusHasEncoder_, 1);
      }
    }
    free(pOptions);
  }

  pC_->wakeupPoller();
}


extern "C" int IcePAPCreateAxis(const char *IcePAPName, int axisNo,
                                int axisFlags, const char *axisOptionsStr)
{
  IcePAPController *pC;

  pC = (IcePAPController*) findAsynPortDriver(IcePAPName);
  if (!pC)
  {
    printf("Error port %s not found\n", IcePAPName);
    return asynError;
  }
  pC->lock();
  new IcePAPAxis(pC, axisNo, axisFlags, axisOptionsStr);
  pC->unlock();
  return asynSuccess;
}

/** Connection status is changed, the dirty bits must be set and
 *  the values in the controller must be updated
 * \param[in] AsynStatus status
 *
 * Sets the dirty bits
 */
void IcePAPAxis::handleStatusChange(asynStatus newStatus)
{
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
            "IcePAPAxis::handleStatusChange status=%s (%d)\n",
            pasynManager->strStatus(newStatus), (int)newStatus);
  if (newStatus == asynSuccess) {
    if (drvlocal.cfg.axisFlags & AMPLIFIER_ON_FLAG_CREATE_AXIS) {
      /* Enable the amplifier when the axis is created,
         but wait until we have a connection to the controller.
         After we lost the connection, Re-enable the amplifier
         See AMPLIFIER_ON_FLAG */
      enableAmplifier(1);
    }
  }
}

/** Reports on status of the axis
 * \param[in] fp The file pointer on which report information will be written
 * \param[in] level The level of report detail desired
 *
 * After printing device-specific information calls asynAxisAxis::report()
 */
void IcePAPAxis::report(FILE *fp, int level)
{
  if (level > 0) {
    fprintf(fp, "  axis %d\n", axisNo_);
  }

  // Call the base class method
  asynAxisAxis::report(fp, level);
}


/** Writes a command to the axis, and expects a logical ack from the controller
 * Outdata is in pC_->outString_
 * Indata is in pC_->inString_
 * The communiction is logged ASYN_TRACE_INFO
 *
 * When the communictaion fails ot times out, writeReadOnErrorDisconnect() is called
 */
asynStatus IcePAPAxis::writeReadACK(void)
{
  asynStatus status;
  int eemcuErr = 0;
  status = pC_->getIntegerParam(axisNo_, pC_->eemcuErr_, &eemcuErr);
  if (status || eemcuErr) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "eemcuErr=%d status=%s (%d)\n",
              eemcuErr,
              pasynManager->strStatus(status), (int)status);
    return status;
  }
  status = pC_->writeReadOnErrorDisconnect();
  switch (status) {
    case asynError:
      return status;
    case asynSuccess:
      if (!strstr(pC_->inString_, "OK")) {
        status = asynError;
        asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "out=%s in=%s return=%s (%d)\n",
                  pC_->outString_, pC_->inString_,
                  pasynManager->strStatus(status), (int)status);
        setIntegerParam(pC_->eemcuErr_, 1);
        setStringParam(pC_->eemcuErrMsg_, pC_->inString_);

        return status;
      }
    default:
      break;
  }
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "out=%s in=%s status=%s (%d) oldPosition=%d\n",
            pC_->outString_, pC_->inString_,
            pasynManager->strStatus(status), (int)status,
            drvlocal.lastpoll.motorPosition);
  return status;
}


/** Sets an integer or boolean value on an axis
 * the values in the controller must be updated
 * \param[in] name of the variable to be updated
 * \param[in] value the (integer) variable to be updated
 *
 */
asynStatus IcePAPAxis::setValueOnAxis(const char* var)
{
  sprintf(pC_->outString_, "#%d:%s", axisNo_, var);
  return writeReadACK();
}

/** Sets an integer or boolean value on an axis
 * the values in the controller must be updated
 * \param[in] name of the variable to be updated
 * \param[in] value the (integer) variable to be updated
 *
 */
asynStatus IcePAPAxis::setValueOnAxis(const char* var, const char *value)
{
  sprintf(pC_->outString_, "#%d:%s %s", axisNo_, var, value);
  return writeReadACK();
}

/** Sets an integer or boolean value on an axis
 * the values in the controller must be updated
 * \param[in] name of the variable to be updated
 * \param[in] value the (integer) variable to be updated
 *
 */
asynStatus IcePAPAxis::setValueOnAxis(const char* var, int value)
{
  sprintf(pC_->outString_, "#%d:%s %d", axisNo_, var, value);
  return writeReadACK();
}

/** Gets a stringfrom an axis
 * \param[in] name of the variable to be retrieved
 * \param[in] pointer to the string result
 *
 */
asynStatus IcePAPAxis::getValueFromAxis(const char* var,
                                        unsigned len, char *value)
{
  char format_string[128];
  asynStatus comStatus;

  memset(value, 0, len);
  sprintf(pC_->outString_, "%d:?%s", axisNo_, var);
  sprintf(format_string, "%d:?%s %%%uc", axisNo_, var, len-1);
  comStatus = pC_->writeReadOnErrorDisconnect();
  if (comStatus) {
    return comStatus;
  } else {
    int nvals = sscanf(pC_->inString_, format_string, value);
    if (nvals != 1) {
      asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "nvals=%d format_string=\"%s\" command=\"%s\" response=\"%s\"\n",
                nvals, format_string, pC_->outString_, pC_->inString_);
      return asynError;
    }
  }
  return asynSuccess;
}

/** Gets an integer or boolean value from an axis
 * \param[in] name of the variable to be retrieved
 * \param[in] pointer to the integer result
 *
 */
asynStatus IcePAPAxis::getValueFromAxis(const char* var, int *value)
{
  char format_string[100];
  asynStatus comStatus;
  int res;

  sprintf(pC_->outString_, "%d:?%s", axisNo_, var);
  sprintf(format_string, "%d:?%s %%d", axisNo_, var);
  comStatus = pC_->writeReadOnErrorDisconnect();
  if (comStatus) {
    return comStatus;
  } else {
    int nvals = sscanf(pC_->inString_, format_string, &res);
    if (nvals != 1) {
    }
    if (nvals != 1) {
      asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "nvals=%d format_string=\"%s\" command=\"%s\" response=\"%s\"\n",
                nvals, format_string, pC_->outString_, pC_->inString_);
      return asynError;
    }
  }
  *value = res;
  return asynSuccess;
}

/** Gets an integer or boolean value from an axis
 * \param[in] name of the variable to be retrieved
 * \param[in] pointer to the integer result
 *
 */
asynStatus IcePAPAxis::getFastValueFromAxis(const char* var, const char *extra, int *value)
{
  char format_string[100];
  asynStatus comStatus;
  int res;

  sprintf(pC_->outString_, "?%s %s %d", var, extra, axisNo_);
  sprintf(format_string, "?%s %%d", var);
  comStatus = pC_->writeReadOnErrorDisconnect();
  if (comStatus) {
    return comStatus;
  } else {
    int nvals = sscanf(pC_->inString_, format_string, &res);
    if (nvals != 1) {
    }
    if (nvals != 1) {
      asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "nvals=%d format_string=\"%s\" command=\"%s\" response=\"%s\"\n",
                nvals, format_string, pC_->outString_, pC_->inString_);
      return asynError;
    }
  }
  *value = res;
  return asynSuccess;
}

/** Move the axis to a position, either absolute or relative
 * \param[in] position in mm
 * \param[in] relative (0=absolute, otherwise relative)
 * \param[in] minimum velocity, mm/sec
 * \param[in] maximum velocity, mm/sec
 * \param[in] acceleration, seconds to maximum velocity
 *
 */
asynStatus IcePAPAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration)
{
  asynStatus status = asynSuccess;
  drvlocal.lastCommandIsHoming = 0;
  if (status == asynSuccess) status = stopAxisInternal(__FUNCTION__, 0);
  if (status == asynSuccess) status = setValueOnAxis("VELOCITY", (int)maxVelocity);
  if (status == asynSuccess) status = setValueOnAxis(relative ? "RMOVE" : "MOVE", (int)position);

  return status;
}


/** Home the motor, search the home position
 * \param[in] minimum velocity, mm/sec
 * \param[in] maximum velocity, mm/sec
 * \param[in] acceleration, seconds to maximum velocity
 * \param[in] forwards (0=backwards, otherwise forwards)
 *
 */
asynStatus IcePAPAxis::home(double minVelocity, double maxVelocity, double acceleration, int forwards)
{
  asynStatus status = asynSuccess;

  drvlocal.lastCommandIsHoming = 1;
  /* The controller will do the home search, and change its internal
     raw value to what we specified in fPosition. Use 0 */
  if (status == asynSuccess) status = stopAxisInternal(__FUNCTION__, 0);
  if ((drvlocal.cfg.axisFlags & AMPLIFIER_ON_FLAG_WHEN_HOMING) &&
      (status == asynSuccess)) status = enableAmplifier(1);
  if (status == asynSuccess) status = setValueOnAxis("HOME", -1);
  return status;
}


/** jog the the motor, search the home position
 * \param[in] minimum velocity, mm/sec (not used)
 * \param[in] maximum velocity, mm/sec (positive or negative)
 * \param[in] acceleration, seconds to maximum velocity
 *
 */
asynStatus IcePAPAxis::moveVelocity(double minVelocity, double maxVelocity, double acceleration)
{
  asynStatus status = asynSuccess;
  drvlocal.lastCommandIsHoming = 0;
  if (status == asynSuccess) setValueOnAxis("JOG", (int)maxVelocity);
  return status;
}


asynStatus IcePAPAxis::resetAxis(void)
{
  asynStatus status = asynSuccess;
  setIntegerParam(pC_->eemcuErr_, 0);
  setStringParam(pC_->eemcuErrMsg_, "");

  return status;
}

/** Enable the amplifier on an axis
 *
 */
asynStatus IcePAPAxis::enableAmplifier(int on)
{
  return setValueOnAxis("POWER", on ? "ON" : "OFF");
}

/** Stop the axis
 *
 */
asynStatus IcePAPAxis::stopAxisInternal(const char *function_name, double acceleration)
{
  asynStatus status = setValueOnAxis("STOP"); /* Page 127 */
  if (status == asynSuccess) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "stopAxisInternal(%d) (%s)\n",  axisNo_, function_name);
  }
  return status;
}

/** Stop the axis, called by motor Record
 *
 */
asynStatus IcePAPAxis::stop(double acceleration )
{
  return stopAxisInternal(__FUNCTION__, acceleration);
}


/** Polls the axis.
 * This function reads the motor position, the limit status, the home status, the moving status,
 * and the drive power-on status.
 * and then calls callParamCallbacks() at the end.
 * \param[out] moving A flag that is set indicating that the axis is moving (true) or done (false). */
asynStatus IcePAPAxis::poll(bool *moving)
{
  st_axis_status_type st_axis_status;
  asynStatus comStatus;
  int hasEncoder = 0;
  int encPosition;
  int nowMoving = 0;
  int nvals = 10110;
  int motor_axis_no = 0;

  *pC_->outString_ = '\0';
  *pC_->inString_ = '\0';
  memset(&st_axis_status, 0, sizeof(st_axis_status));

  /* Phase 1: read the Axis status */
  sprintf(pC_->outString_, "%d:?STATUS", axisNo_);
  comStatus = pC_->writeReadController();
  if (comStatus) goto badpollall;
  nvals = sscanf(pC_->inString_,
                 "%d:?STATUS %x",
                 &motor_axis_no,
                 &st_axis_status.status);
  if (nvals != 2) goto badpollall;
  if (axisNo_ != motor_axis_no) goto badpollall;
  setIntegerParam(pC_->motorStatusProblem_, 0); //st_axis_status.status & STATUS_BITS_DISABLE);
  setIntegerParam(pC_->motorStatusAtHome_, st_axis_status.status & STATUS_BIT_HSIGNAL);
  if (drvlocal.lastCommandIsHoming) {
    setIntegerParam(pC_->motorStatusLowLimit_, 0);
    setIntegerParam(pC_->motorStatusHighLimit_, 0);
  } else {
    setIntegerParam(pC_->motorStatusLowLimit_, st_axis_status.status & STATUS_BIT_LIMIT_NEG);
    setIntegerParam(pC_->motorStatusHighLimit_, st_axis_status.status & STATUS_BIT_LIMIT_POS);
  }

  setIntegerParam(pC_->motorStatusPowerOn_, st_axis_status.status & STATUS_BIT_POWERON);

  /* Phase 2: read the Axis (readback) position */
  comStatus = getFastValueFromAxis("FPOS", "", &st_axis_status.motorPosition);
  if (comStatus) goto badpollall;
  /* Use previous motorPosition and current motorPosition to calculate direction.*/
  if (st_axis_status.motorPosition > drvlocal.lastpoll.motorPosition) {
    setIntegerParam(pC_->motorStatusDirection_, 1);
  } else if (st_axis_status.motorPosition < drvlocal.lastpoll.motorPosition) {
    setIntegerParam(pC_->motorStatusDirection_, 0);
  }
  drvlocal.lastpoll.motorPosition = st_axis_status.motorPosition;
  setDoubleParam(pC_->motorPosition_, st_axis_status.motorPosition);

  if (drvlocal.cfg.externalEncoderStr) {
    comStatus = getFastValueFromAxis("FPOS", drvlocal.cfg.externalEncoderStr, &encPosition);
    if (!comStatus) {
      hasEncoder = 1;
      setDoubleParam(pC_->motorEncoderPosition_, encPosition);
    }
  }

  if (drvlocal.lastpoll.status != st_axis_status.status) {
    unsigned int status = st_axis_status.status;
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "poll(%d) 0x%x %s%s%s%s%s%s%s%s %s=%d\n",
              axisNo_, st_axis_status.status,
              status & STATUS_BIT_READY ?     "READY " : "      ",
              status & STATUS_BIT_MOVING ?    "MOVIN " : "      ",
              status & STATUS_BIT_SETTLING ?  "SETTL " : "      ",
              status & STATUS_BIT_OUTOFWIN ?  "OOWIN " : "      ",
              status & STATUS_BIT_POWERON ?   "PWR-ON  " : "PWR-OFF ",
              status & STATUS_BIT_LIMIT_POS ? "LIM-P " : " ",
              status & STATUS_BIT_LIMIT_NEG ? "LIM-N " : " ",
              status & STATUS_BIT_HSIGNAL ?   "HOME  " : " ",
              hasEncoder ? "REP" : "RMP",
              hasEncoder ? encPosition : st_axis_status.motorPosition);
    drvlocal.lastpoll.status = st_axis_status.status;
  }

  /* Phase 3: is motor moving */
  if (st_axis_status.status & (STATUS_BIT_MOVING | STATUS_BIT_SETTLING)) {
    nowMoving = 1;
  }

  setIntegerParam(pC_->motorStatusMoving_, nowMoving);
  setIntegerParam(pC_->motorStatusDone_, !nowMoving);
  *moving = nowMoving ? true : false;

  /* Phase 4: Is the axis homed */
  {
    const char *found_str = "FOUND";
    int homed = 0;
    char homeencbuf[128];
    asynStatus comStatus;

    comStatus = getValueFromAxis("HOMESTAT", (int)sizeof(homeencbuf), &homeencbuf[0]);
    if (!comStatus) {
      if (!strncmp(homeencbuf, found_str, strlen(found_str))) {
        homed = 1;
      }
    }
    setIntegerParam(pC_->motorStatusHomed_, homed);
  }

  if (memcmp(st_axis_status.errbuf, drvlocal.errbuf, sizeof(drvlocal.errbuf))) {
    setIntegerParam(pC_->eemcuErr_, 0);
    setIntegerParam(pC_->eemcuErrId_, 0);
    setStringParam(pC_->eemcuErrMsg_, st_axis_status.errbuf);
    memcpy(drvlocal.errbuf, st_axis_status.errbuf, sizeof(drvlocal.errbuf));
  }

  callParamCallbacks();
  return asynSuccess;

  badpollall:
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
            "out=%s in=%s return=%s (%d)\n",
            pC_->outString_, pC_->inString_,
            pasynManager->strStatus(comStatus), (int)comStatus);

  setIntegerParam(pC_->motorStatusProblem_, 1);
  callParamCallbacks();
  return asynError;
}

asynStatus IcePAPAxis::setIntegerParam(int function, int value)
{
  asynStatus status;
  if (function == pC_->motorClosedLoop_) {
    if (drvlocal.cfg.axisFlags & AMPLIFIER_ON_FLAG_USING_CNEN) {
      (void)enableAmplifier(value);
    }
#ifdef eemcuErrRstString
  } else if (function == pC_->eemcuErrRst_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "setIntegerParam(%d ErrRst_)=%d\n", axisNo_, value);
    if (value) {
      resetAxis();
    }
  }
#endif

  //Call base class method
  status = asynAxisAxis::setIntegerParam(function, value);
  return status;
}

asynStatus IcePAPAxis::setStringParam(int function, const char *value)
{
  asynStatus status = asynSuccess;
#ifdef eemcuErrMsgString
  if (function == pC_->eemcuErrMsg_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "setStringParam(%d eemcuErrMsg_)=%s\n", axisNo_, value);
  }
#endif

  /* Call base class method */
  status = asynAxisAxis::setStringParam(function, value);
  return status;
}
