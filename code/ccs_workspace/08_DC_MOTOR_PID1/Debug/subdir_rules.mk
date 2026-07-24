################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-170778174: ../empty.syscfg
	@echo 'SysConfig - building file: "$<"'
	"G:/electronic/ti/ccs2050/sysconfig_1.26.2/sysconfig_cli.bat" -s "G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/.metadata/product.json" --script "G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1/empty.syscfg" -o "." --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-170778174 ../empty.syscfg
device.opt: build-170778174
device.cmd.genlibs: build-170778174
ti_msp_dl_config.c: build-170778174
ti_msp_dl_config.h: build-170778174
Event.dot: build-170778174

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"G:/electronic/ti/ccs2050/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1/user_driver" -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1" -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1/Debug" -I"G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" -I"G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"G:/electronic/ti/ccs2050/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1/user_driver" -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1" -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1/Debug" -I"G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" -I"G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"G:/electronic/ti/ccs2050/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1/user_driver" -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1" -I"G:/NUEDC_2026/code/ccs_workspace/08_DC_MOTOR_PID1/Debug" -I"G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" -I"G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


