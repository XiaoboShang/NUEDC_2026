################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
targetConfigs/Debug/%.o: ../targetConfigs/Debug/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"G:/electronic/ti/ccs2050/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"G:/NUEDC_2026/code/ccs_workspace/11_PID_CAR/user_driver" -I"G:/NUEDC_2026/code/ccs_workspace/11_PID_CAR" -I"G:/NUEDC_2026/code/ccs_workspace/11_PID_CAR/Debug" -I"G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" -I"G:/electronic/ti/ccs2050/mspm0_sdk_2_11_00_07/source" -g -Wall -MMD -MP -MF"targetConfigs/Debug/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


