#!/bin/sh
(
  cd nose_tests/ &&
  ./runTests.sh IOC:m1 "$@"
)
