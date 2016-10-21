#Makefile at top of application tree
# "#!" marks lines that can be uncommented.

TOP = .
include $(TOP)/configure/CONFIG

DIRS += configure axisApp
axisApp_DEPEND_DIRS   = configure

# To build axis examples;
# 1st - uncomment lines below.
# 2nd - uncomment required support module lines at the bottom of
#       <axis>/configure/RELEASE.
# 3rd - make clean uninstall
# 4th - make

#!DIRS += axisExApp iocBoot
#!axisExApp_DEPEND_DIRS = axisApp
#!iocBoot_DEPEND_DIRS    = axisExApp

include $(TOP)/configure/RULES_TOP
