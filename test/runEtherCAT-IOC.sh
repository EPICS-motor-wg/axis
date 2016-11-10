#!/bin/sh
(
  cd m-epics-eemcu/ &&
  ./doit.sh Sim2 "$@"
)
