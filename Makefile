# Otherwise use Makefile

# Must check if some updated axisRecord.dbd.pod
build: documentation/axisRecord.html


documentation/axisRecord.html: axisApp/AxisSrc/axisRecord.dbd.pod ./axisApp/tools/gen_axisRecord_html.sh
	./axisApp/tools/gen_axisRecord_html.sh

#Use either the Makefile for EPICS, or the one for
# ESS EPICS ENVIRONMENT

ifdef EPICS_ENV_PATH
ifeq ($(EPICS_MODULES_PATH),/opt/epics/modules)
ifeq ($(EPICS_BASES_PATH),/opt/epics/bases)
include Makefile.EEE
else
include Makefile.epics
endif
else
include Makefile.epics
endif
else
include Makefile.epics
endif
