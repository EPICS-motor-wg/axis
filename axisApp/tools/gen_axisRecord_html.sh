#!/bin/sh

# script to update axisRecord.html from axisApp/AxisSrc/axisRecord.dbd.pod
# This needs base R3.15.4 or higher
# For the moment, we assume that there is a (shell) environment variable
# like this
# EPICS_BASE=/opt/epics/bases/base-3.15.4
# EPICS_HOST_ARCH=centos7-x86_64
# If there is tools/dbdToHtml.pl
#

# pathes and file names
AXISPWD=$PWD
AXISDBD=$AXISPWD/axisApp/AxisSrc/axisRecord.dbd
AXISRECORDHTML=axisRecord.html

#perl and perl files
AXISPERL="perl -CSD "
DBDTOHTMLPL=$EPICS_BASE/bin/$EPICS_HOST_ARCH/dbdToHtml.pl
PODREMOVEPL=$EPICS_BASE/bin/$EPICS_HOST_ARCH/podRemove.pl

export AXISPWD AXISDBD AXISRECORDHTML AXISPERL DBDTOHTMLPL PODREMOVEPL

if test -x "$DBDTOHTMLPL"; then
(
  rm -f $AXISPWD/documentation/$AXISRECORDHTML &&
  if test -e "$AXISDBD"; then
    chmod u+w $AXISDBD
  fi &&
  cd $EPICS_BASE &&
  cmd=$(echo $AXISPERL $PODREMOVEPL -o $AXISDBD $AXISDBD.pod)
  echo $cmd &&
  eval $cmd &&
  cmd=$(echo $AXISPERL $DBDTOHTMLPL -I ./dbd/ $AXISDBD.pod)
  echo $cmd &&
  eval $cmd &&
  mv $AXISRECORDHTML $AXISPWD/documentation/$AXISRECORDHTML
)
else
  echo "Info: dbdToHtml.pl is only available in EPICS base since 15.4" &&
  echo "Info: You can ignore this message, if you didn't change $AXISDBD.pod" &&
  touch $AXISDBD &&
  touch $AXISPWD/documentation/$AXISRECORDHTML
fi &&
chmod -v ugo-w $AXISDBD
