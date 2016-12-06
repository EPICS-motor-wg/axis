# axis

EPICS module: axis

Currently this is a fork of the motorRecord, with the goal to do
further development, cleanup and API extensions.
The baseline is after R6-9 and after the conversion from svn to Git.


https://github.com/epics-modules/motor/commit/60a2298b225583f5fa6a6576f7088bb9db661e16

## Having the Simulation Up and Running

Open a terminal window and execute the following commands:

```
cd <local-repository>/axis/test
./run-EthercatMC-simulator.sh
```

Open another terminal window and execute the following commands:

```
cd <local-repository>/axis
make install
cd test
./run-EthercatMC-ioc.sh SolAxis
```

Pressing RETURN then executing `dbl` will show the list of PVs available.

### Running the Tests

Open one more terminal window and execute the following commands:

```
cd <local-repository>/axis/test
./run-Record-tests.sh IOC:m1
```