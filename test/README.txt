The test directory, what is is about ?

In order to run test, you need 3 things:

- A motion controller with at least one axis
- An EPICS IOC
- A test suite

The motion controller
  Either you have a HW test setup somewhere, or you can use the
  simulator.

The EPICS IOC
  Many people already have some EPICS installation.
  If not, you can run:
  git clone https://github.com/EuropeanSpallationSource/MCAG_setupMotionDemo.git
  
  Then you need the axisRecord and a driver for your motion controller.
  This is where m-epics-eemcu comes in as an example.
  It is a copy of the driver written for the motorRecord.
  On the long term I will change the structure and integrate it in the
  same way we have integrated the other drivers, we are not there yet.
  
The test suite
  The test suite that we have today is under nose_tests/
  See http://nose.readthedocs.io/en/latest/
  The nice thing with nose is that you can add printouts
  in the test code, which nose hides from you until
  a test case fails.
  Then the whole log output is presented to you.

  If you have other tests, consider to send us a pull request, please.


All in all you need 3 terminal windows to run the 3 scripts in parallel:

./runEtherCAT-simulator.sh
./runEtherCAT-IOC.sh
./runEtherCAT-tests.sh
