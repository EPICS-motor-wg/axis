TOP = ..
include $(TOP)/configure/CONFIG

# AxisSrc is required for all motor record configurations.  It MUST appear
# 1st in this list of directories.

DIRS += AxisSrc

# Select/deselect individual device driver modules by removing/adding a
# "#" comment.

# All the following modules require ASYN.
ifdef ASYN

DIRS += OmsAsynSrc
OmsAsynSrc_DEPEND_DIRS   = AxisSrc

#DIRS += MotorSimSrc
#MotorSimSrc_DEPEND_DIRS = AxisSrc

DIRS += NewportSrc
NewportSrc_DEPEND_DIRS = AxisSrc

DIRS += PIGCS2Src
PIGCS2Src_DEPEND_DIRS = AxisSrc

#DIRS += NewFocusSrc
#NewFocusSrc_DEPEND_DIRS = AxisSrc

#DIRS += AcsTech80Src
#AcsTech80Src_DEPEND_DIRS = AxisSrc

#DIRS += OrielSrc
#OrielSrc_DEPEND_DIRS = AxisSrc

#DIRS += ThorLabsSrc
#ThorLabsSrc_DEPEND_DIRS = AxisSrc

#DIRS += SmartMotorSrc
#SmartAxisSrc_DEPEND_DIRS = AxisSrc

#DIRS += PiJenaSrc
#PiJenaSrc_DEPEND_DIRS = AxisSrc

#DIRS += KohzuSrc
#KohzuSrc_DEPEND_DIRS = AxisSrc

#DIRS += AttocubeSrc
#AttocubeSrc_DEPEND_DIRS = AxisSrc

#DIRS += AerotechSrc
#AerotechSrc_DEPEND_DIRS = AxisSrc

# The Hytec support requires ipac
ifdef IPAC
DIRS += HytecSrc
HytecSrc_DEPEND_DIRS = AxisSrc
endif

DIRS += ACRSrc
ACRSrc_DEPEND_DIRS = AxisSrc

DIRS += SmarActMCSSrc
SmarActMCSSrc_DEPEND_DIRS = AxisSrc

DIRS += NPointSrc
NPointSrc_DEPEND_DIRS = AxisSrc

DIRS += MicronixSrc
MicronixSrc_DEPEND_DIRS = AxisSrc

DIRS += PhytronSrc
PhytronSrc_DEPEND_DIRS = AxisSrc

endif

# Install the edl files
#DIRS += opi

ifdef MX
DIRS += MXmotorSrc
endif

DIRS += Db

include $(TOP)/configure/RULES_DIRS
