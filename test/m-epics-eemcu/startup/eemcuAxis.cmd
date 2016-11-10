# @field AXISCONFIG
# @type  STRING
# @field PREFIX
# @type  STRING
# @field MOTOR_NAME
# @type  STRING
# @field MOTOR_PORT
# @type  STRING
# @field AXIS_NO
# @type  STRING
# @field DESC
# @type  STRING
# @field PREC
# @type  INTEGER
# @field VELO
# @type  FLOAT
# @field JVEL
# @type  FLOAT
# @field JAR
# @type  FLOAT
# @field ACCL
# @type  FLOAT
# @field MRES
# @type  FLOAT
# @field DLLM
# @type  FLOAT
# @field DHLM
# @type  FLOAT
# @field HOMEPROC
# @type  INTEGER

eemcuCreateAxis("$(MOTOR_PORT)", "$(AXIS_NO)", "6", "$(AXISCONFIG)")

dbLoadRecords("eemcu.template", "PREFIX=$(PREFIX), MOTOR_NAME=$(MOTOR_NAME), R=$(R), MOTOR_PORT=$(MOTOR_PORT), ASYN_PORT=$(ASYN_PORT), AXIS_NO=$(AXIS_NO), DESC=$(DESC), PREC=$(PREC), VELO=$(VELO), JVEL=$(JVEL), JAR=$(JAR), ACCL=$(ACCL), MRES=$(MRES), DLLM=$(DLLM), DHLM=$(DHLM), HOMEPROC=$(HOMEPROC)")


