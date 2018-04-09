################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
portable/heap_4.obj: ../portable/heap_4.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/Users/andrewkuklinski/workspace_v8/hw5freertosQuestion4" --include_path="C:/ti/TivaWare_C_Series-2.1.4.178/driverlib/ccs/Debug" --include_path="c:/TI/TivaWare_C_Series-2.1.4.178" --include_path="C:/FreeRTOSv10.0.1/FreeRTOS/Source/portable/CCS/ARM_CM4F" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.1.LTS/include" --include_path="C:/FreeRTOSv10.0.1/FreeRTOS/Source/include" --define=ccs="ccs" --define=PART_TM4C1294NCPDT -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="portable/heap_4.d_raw" --obj_directory="portable" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


