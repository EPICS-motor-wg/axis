/*
  FILENAME... IcePAPController.cpp
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <iocsh.h>
#include <epicsThread.h>

#include <asynOctetSyncIO.h>

#include "asynAxisController.h"
#include "asynAxisAxis.h"

#include <epicsExport.h>
#include "IcePAP.h"

/** Creates a new IcePAPController object.
 * \param[in] portName          The name of the asyn port that will be created for this driver
 * \param[in] MotorPortName     The name of the drvAsynSerialPort that was created previously to connect to the IcePAP controller
 * \param[in] numAxes           The number of axes that this controller supports
 * \param[in] movingPollPeriod  The time between polls when any axis is moving
 * \param[in] idlePollPeriod    The time between polls when no axis is moving
 */
IcePAPController::IcePAPController(const char *portName, const char *MotorPortName, int numAxes,
                                   double movingPollPeriod,double idlePollPeriod)
  :  asynAxisController(portName, numAxes, NUM_VIRTUAL_MOTOR_PARAMS,
                        0, // No additional interfaces beyond those in base class
                        0, // No additional callback interfaces beyond those in base class
                        ASYN_CANBLOCK | ASYN_MULTIDEVICE,
                        1, // autoconnect
                        0, 0)  // Default priority and stack size
{
  asynStatus status;

  createParam(EthercatMCErrString,           asynParamInt32,       &EthercatMCErr_);
  /* Per axis */
  createParam(EthercatMCVelActString,        asynParamFloat64,     &EthercatMCVelAct_);
  createParam(EthercatMCVel_RBString,        asynParamFloat64,     &EthercatMCVel_RB_);
  createParam(EthercatMCAcc_RBString,        asynParamFloat64,     &EthercatMCAcc_RB_);
  createParam(EthercatMCDec_RBString,        asynParamFloat64,     &EthercatMCDec_RB_);

  createParam(EthercatMCErrRstString,        asynParamInt32,       &EthercatMCErrRst_);
  /* Connect to IcePAP controller */
  status = pasynOctetSyncIO->connect(MotorPortName, 0, &pasynUserController_, NULL);
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
              "cannot connect to motor controller\n");
  }
  startPoller(movingPollPeriod, idlePollPeriod, 2);
}


/** Creates a new IcePAPController object.
 * Configuration command, called directly or from iocsh
 * \param[in] portName          The name of the asyn port that will be created for this driver
 * \param[in] MotorPortName  The name of the drvAsynIPPPort that was created previously to connect to the IcePAP controller
 * \param[in] numAxes           The number of axes that this controller supports (0 is not used)
 * \param[in] movingPollPeriod  The time in ms between polls when any axis is moving
 * \param[in] idlePollPeriod    The time in ms between polls when no axis is moving
 */
extern "C" int IcePAPCreateController(const char *portName, const char *MotorPortName, int numAxes,
                                      int movingPollPeriod, int idlePollPeriod)
{
  new IcePAPController(portName, MotorPortName, 1+numAxes, movingPollPeriod/1000., idlePollPeriod/1000.);
  return(asynSuccess);
}

/** Writes a string to the controller and reads a response.
 * Disconnects in case of error
 */
asynStatus IcePAPController::writeReadOnErrorDisconnect(void)
{
  size_t nwrite = 0;
  asynStatus status;
  int eomReason;
  size_t outlen = strlen(outString_);
  size_t nread;
  status = pasynOctetSyncIO->writeRead(pasynUserController_, outString_, outlen,
                                       inString_, sizeof(inString_),
                                       DEFAULT_CONTROLLER_TIMEOUT,
                                       &nwrite, &nread, &eomReason);
  if (status == asynTimeout) {
    asynInterface *pasynInterface = NULL;
    asynCommon     *pasynCommon = NULL;
    asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "out=%s status=asynTimeout (%d)\n",
              outString_, (int)status);
    pasynInterface = pasynManager->findInterface(pasynUserController_,
                                                 asynCommonType,
                                                 0 /* FALSE */);
    if (pasynInterface) {
      pasynCommon = (asynCommon *)pasynInterface->pinterface;
      status = pasynCommon->disconnect(pasynInterface->drvPvt,
                                       pasynUserController_);
      handleStatusChange(asynError);
      if (status != asynSuccess) {
        asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "out=%s status=%s (%d)\n",
                  outString_, pasynManager->strStatus(status), (int)status);
      }
    } else {
      asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "pasynInterface=%p pasynCommon=%p\n",
                pasynInterface, pasynCommon);
    }
    return asynError; /* TimeOut -> Error */
  }
  return status;
}

asynStatus IcePAPController::writeOnErrorDisconnect(void)
{
  size_t nwrite = 0;
  asynStatus status;
  size_t outlen = strlen(outString_);
  status = pasynOctetSyncIO->write(pasynUserController_, outString_, outlen,
                                   DEFAULT_CONTROLLER_TIMEOUT, &nwrite);
  if (status == asynTimeout) {
    asynInterface *pasynInterface = NULL;
    asynCommon     *pasynCommon = NULL;
    asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "out=%s status=asynTimeout (%d)\n",
              outString_, (int)status);
    pasynInterface = pasynManager->findInterface(pasynUserController_,
                                                 asynCommonType,
                                                 0 /* FALSE */);
    if (pasynInterface) {
      pasynCommon = (asynCommon *)pasynInterface->pinterface;
      status = pasynCommon->disconnect(pasynInterface->drvPvt,
                                       pasynUserController_);
      handleStatusChange(asynError);
      if (status != asynSuccess) {
        asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "out=%s status=%s (%d)\n",
                  outString_, pasynManager->strStatus(status), (int)status);
      }
    } else {
      asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "pasynInterface=%p pasynCommon=%p\n",
                pasynInterface, pasynCommon);
    }
    return asynError; /* TimeOut -> Error */
  }
  return status;
}

void IcePAPController::handleStatusChange(asynStatus status)
{
  int i;
  for (i=0; i<numAxes_; i++) {
    IcePAPAxis *pAxis=getAxis(i);
    if (!pAxis) continue;
    pAxis->handleStatusChange(status);
  }
}

/** Reports on status of the driver
 * \param[in] fp The file pointer on which report information will be written
 * \param[in] level The level of report detail desired
 *
 * If details > 0 then information is printed about each axis.
 * After printing controller-specific information it calls asynAxisController::report()
 */
void IcePAPController::report(FILE *fp, int level)
{
  fprintf(fp, "Twincat motor driver %s, numAxes=%d, moving poll period=%f, idle poll period=%f\n",
          this->portName, numAxes_, movingPollPeriod_, idlePollPeriod_);

  // Call the base class method
  asynAxisController::report(fp, level);
}

/** Returns a pointer to an IcePAPAxis object.
 * Returns NULL if the axis number encoded in pasynUser is invalid.
 * \param[in] pasynUser asynUser structure that encodes the axis index number. */
IcePAPAxis* IcePAPController::getAxis(asynUser *pasynUser)
{
  return static_cast<IcePAPAxis*>(asynAxisController::getAxis(pasynUser));
}

/** Returns a pointer to an IcePAPAxis object.
 * Returns NULL if the axis number encoded in pasynUser is invalid.
 * \param[in] axisNo Axis index number. */
IcePAPAxis* IcePAPController::getAxis(int axisNo)
{
  return static_cast<IcePAPAxis*>(asynAxisController::getAxis(axisNo));
}


asynStatus IcePAPController::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int function = pasynUser->reason;
  IcePAPAxis *pAxis;
  pAxis = getAxis(pasynUser);
  if (!pAxis) return asynError;

  (void)pAxis->setIntegerParam(function, value);
  return asynAxisController::writeInt32(pasynUser, value);
}

/** Code for iocsh registration */
static const iocshArg IcePAPCreateControllerArg0 = {"Port name", iocshArgString};
static const iocshArg IcePAPCreateControllerArg1 = {"EPICS ASYN TCP motor port name", iocshArgString};
static const iocshArg IcePAPCreateControllerArg2 = {"Number of axes", iocshArgInt};
static const iocshArg IcePAPCreateControllerArg3 = {"Moving poll period (ms)", iocshArgInt};
static const iocshArg IcePAPCreateControllerArg4 = {"Idle poll period (ms)", iocshArgInt};
static const iocshArg * const IcePAPCreateControllerArgs[] = {&IcePAPCreateControllerArg0,
                                                              &IcePAPCreateControllerArg1,
                                                              &IcePAPCreateControllerArg2,
                                                              &IcePAPCreateControllerArg3,
                                                              &IcePAPCreateControllerArg4};
static const iocshFuncDef IcePAPCreateControllerDef = {"IcePAPCreateController", 5, IcePAPCreateControllerArgs};
static void IcePAPCreateContollerCallFunc(const iocshArgBuf *args)
{
  IcePAPCreateController(args[0].sval, args[1].sval, args[2].ival, args[3].ival, args[4].ival);
}


/* IcePAPCreateAxis */
static const iocshArg IcePAPCreateAxisArg0 = {"Controller port name", iocshArgString};
static const iocshArg IcePAPCreateAxisArg1 = {"Axis number", iocshArgInt};
static const iocshArg IcePAPCreateAxisArg2 = {"axisFlags", iocshArgInt};
static const iocshArg IcePAPCreateAxisArg3 = {"axisOptionsStr", iocshArgString};
static const iocshArg * const IcePAPCreateAxisArgs[] = {&IcePAPCreateAxisArg0,
                                                        &IcePAPCreateAxisArg1,
                                                        &IcePAPCreateAxisArg2,
                                                        &IcePAPCreateAxisArg3};
static const iocshFuncDef IcePAPCreateAxisDef = {"IcePAPCreateAxis", 4, IcePAPCreateAxisArgs};
static void IcePAPCreateAxisCallFunc(const iocshArgBuf *args)
{
  IcePAPCreateAxis(args[0].sval, args[1].ival, args[2].ival, args[3].sval);
}

static void IcePAPControllerRegister(void)
{
  iocshRegister(&IcePAPCreateControllerDef, IcePAPCreateContollerCallFunc);
  iocshRegister(&IcePAPCreateAxisDef,       IcePAPCreateAxisCallFunc);
}

extern "C" {
  epicsExportRegistrar(IcePAPControllerRegister);
}
