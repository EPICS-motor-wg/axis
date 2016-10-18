drvAsynIPPortConfigure("$(ASYN_PORT)","$(IPADDR):$(IPPORT)",0,0,0)
asynOctetSetOutputEos("$(ASYN_PORT)", -1, ";\n")
asynOctetSetInputEos("$(ASYN_PORT)", -1, ";\n")
eemcuCreateController("$(MOTOR_PORT)", "$(ASYN_PORT)", "32", "200", "1000")
asynSetTraceMask("$(ASYN_PORT)", -1, 0x41)
asynSetTraceIOMask("$(ASYN_PORT)", -1, 2)
asynSetTraceInfoMask("$(ASYN_PORT)", -1, 15)
