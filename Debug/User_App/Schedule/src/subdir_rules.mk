################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
User_App/Schedule/src/%.obj: ../User_App/Schedule/src/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"D:/ti/CCS1110/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 --fp_mode=relaxed --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/device/driverlib" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/User_App/Schedule/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/User_App/WatchDog/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/User_App/InterruptCfg/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/User_App/LoopCtrl/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/Platform/Common/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/User_App/Comm/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/User_App/StateMachine/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/device/common/include" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/device/driverlib/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/device/headers/include" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/Broad/inc" --include_path="D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026/Drivers/inc" --include_path="D:/ti/CCS1110/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=_FLASH --define=DEBUG --define=CPU1 --define=F2837xS -g --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="User_App/Schedule/src/$(basename $(<F)).d_raw" --obj_directory="User_App/Schedule/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


