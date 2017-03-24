# Makefile
TOP = ../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# The following are used for debugging messages.
#!USR_CXXFLAGS += -DDEBUG

DBDINC += axisRecord.h
DBD += axisSupport.dbd

INC += axis.h

INC += axis_interface.h
INC += axis_priv.h
INC += paramLib.h
INC += asynAxisController.h
INC += asynAxisAxis.h


LIBRARY_IOC += axis

# The following are required for all axis record configurations.
axis_SRCS += axisRecord.cc
axis_SRCS += axisUtilAux.cc
axis_SRCS += axisUtil.cc

axis_SRCS += devAxisAsyn.c
axis_SRCS += paramLib.c
axis_SRCS += asynAxisController.cpp
axis_SRCS += asynAxisAxis.cpp
axis_LIBS += asyn

axis_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# Parallel build sometimes fails.
# Make dependences on axisRecord.h explicit.
axisRecord$(OBJ):  $(COMMON_DIR)/axisRecord.h
axisdevCom$(OBJ):  $(COMMON_DIR)/axisRecord.h
devAxisAsyn$(OBJ): $(COMMON_DIR)/axisRecord.h
