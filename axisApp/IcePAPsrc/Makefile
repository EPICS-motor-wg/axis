TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = IcePAP
# IcePAP.dbd will be created and installed
DBD += IcePAP.dbd

# IcePAP.dbd will be made up from these files:
IcePAP_DBD += base.dbd
IcePAP_DBD += axisSupport.dbd

# Include dbd files from all support applications:
#IcePAP_DBD += xxx.dbd
IcePAP_DBD += asyn.dbd
IcePAP_DBD += drvAsynIPPort.dbd
IcePAP_DBD += IcePAPController.dbd

# Add source files
IcePAP_SRCS += IcePAPController.cpp
IcePAP_SRCS += IcePAPAxis.cpp

# Add all the support libraries needed by this IOC
IcePAP_LIBS += asyn
IcePAP_LIBS += axis

# IcePAP_registerRecordDeviceDriver.cpp derives from IcePAP.dbd
IcePAP_SRCS += IcePAP_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
IcePAP_SRCS_DEFAULT += IcePAPMain.cpp
IcePAP_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#IcePAP_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
IcePAP_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

