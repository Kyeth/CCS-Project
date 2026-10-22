################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
ASM_SRCS += \
../device/common/source/F2837xS_CodeStartBranch.asm \
../device/common/source/F2837xS_DBGIER.asm \
../device/common/source/F2837xS_usDelay.asm 

C_SRCS += \
../device/common/source/F2837xS_Adc.c \
../device/common/source/F2837xS_CpuTimers.c \
../device/common/source/F2837xS_DefaultISR.c \
../device/common/source/F2837xS_Dma.c \
../device/common/source/F2837xS_ECap.c \
../device/common/source/F2837xS_EPwm.c \
../device/common/source/F2837xS_EQep.c \
../device/common/source/F2837xS_Emif.c \
../device/common/source/F2837xS_Gpio.c \
../device/common/source/F2837xS_I2C.c \
../device/common/source/F2837xS_Mcbsp.c \
../device/common/source/F2837xS_PieCtrl.c \
../device/common/source/F2837xS_PieVect.c \
../device/common/source/F2837xS_Sci.c \
../device/common/source/F2837xS_Spi.c \
../device/common/source/F2837xS_SysCtrl.c \
../device/common/source/F2837xS_TempSensorConv.c \
../device/common/source/F2837xS_Upp.c \
../device/common/source/F2837xS_can.c \
../device/common/source/F2837xS_sci_io.c \
../device/common/source/F2837xS_sdfm_drivers.c \
../device/common/source/F2837xS_struct.c \
../device/common/source/device.c 

C_DEPS += \
./device/common/source/F2837xS_Adc.d \
./device/common/source/F2837xS_CpuTimers.d \
./device/common/source/F2837xS_DefaultISR.d \
./device/common/source/F2837xS_Dma.d \
./device/common/source/F2837xS_ECap.d \
./device/common/source/F2837xS_EPwm.d \
./device/common/source/F2837xS_EQep.d \
./device/common/source/F2837xS_Emif.d \
./device/common/source/F2837xS_Gpio.d \
./device/common/source/F2837xS_I2C.d \
./device/common/source/F2837xS_Mcbsp.d \
./device/common/source/F2837xS_PieCtrl.d \
./device/common/source/F2837xS_PieVect.d \
./device/common/source/F2837xS_Sci.d \
./device/common/source/F2837xS_Spi.d \
./device/common/source/F2837xS_SysCtrl.d \
./device/common/source/F2837xS_TempSensorConv.d \
./device/common/source/F2837xS_Upp.d \
./device/common/source/F2837xS_can.d \
./device/common/source/F2837xS_sci_io.d \
./device/common/source/F2837xS_sdfm_drivers.d \
./device/common/source/F2837xS_struct.d \
./device/common/source/device.d 

OBJS += \
./device/common/source/F2837xS_Adc.obj \
./device/common/source/F2837xS_CodeStartBranch.obj \
./device/common/source/F2837xS_CpuTimers.obj \
./device/common/source/F2837xS_DBGIER.obj \
./device/common/source/F2837xS_DefaultISR.obj \
./device/common/source/F2837xS_Dma.obj \
./device/common/source/F2837xS_ECap.obj \
./device/common/source/F2837xS_EPwm.obj \
./device/common/source/F2837xS_EQep.obj \
./device/common/source/F2837xS_Emif.obj \
./device/common/source/F2837xS_Gpio.obj \
./device/common/source/F2837xS_I2C.obj \
./device/common/source/F2837xS_Mcbsp.obj \
./device/common/source/F2837xS_PieCtrl.obj \
./device/common/source/F2837xS_PieVect.obj \
./device/common/source/F2837xS_Sci.obj \
./device/common/source/F2837xS_Spi.obj \
./device/common/source/F2837xS_SysCtrl.obj \
./device/common/source/F2837xS_TempSensorConv.obj \
./device/common/source/F2837xS_Upp.obj \
./device/common/source/F2837xS_can.obj \
./device/common/source/F2837xS_sci_io.obj \
./device/common/source/F2837xS_sdfm_drivers.obj \
./device/common/source/F2837xS_struct.obj \
./device/common/source/F2837xS_usDelay.obj \
./device/common/source/device.obj 

ASM_DEPS += \
./device/common/source/F2837xS_CodeStartBranch.d \
./device/common/source/F2837xS_DBGIER.d \
./device/common/source/F2837xS_usDelay.d 

OBJS__QUOTED += \
"device\common\source\F2837xS_Adc.obj" \
"device\common\source\F2837xS_CodeStartBranch.obj" \
"device\common\source\F2837xS_CpuTimers.obj" \
"device\common\source\F2837xS_DBGIER.obj" \
"device\common\source\F2837xS_DefaultISR.obj" \
"device\common\source\F2837xS_Dma.obj" \
"device\common\source\F2837xS_ECap.obj" \
"device\common\source\F2837xS_EPwm.obj" \
"device\common\source\F2837xS_EQep.obj" \
"device\common\source\F2837xS_Emif.obj" \
"device\common\source\F2837xS_Gpio.obj" \
"device\common\source\F2837xS_I2C.obj" \
"device\common\source\F2837xS_Mcbsp.obj" \
"device\common\source\F2837xS_PieCtrl.obj" \
"device\common\source\F2837xS_PieVect.obj" \
"device\common\source\F2837xS_Sci.obj" \
"device\common\source\F2837xS_Spi.obj" \
"device\common\source\F2837xS_SysCtrl.obj" \
"device\common\source\F2837xS_TempSensorConv.obj" \
"device\common\source\F2837xS_Upp.obj" \
"device\common\source\F2837xS_can.obj" \
"device\common\source\F2837xS_sci_io.obj" \
"device\common\source\F2837xS_sdfm_drivers.obj" \
"device\common\source\F2837xS_struct.obj" \
"device\common\source\F2837xS_usDelay.obj" \
"device\common\source\device.obj" 

C_DEPS__QUOTED += \
"device\common\source\F2837xS_Adc.d" \
"device\common\source\F2837xS_CpuTimers.d" \
"device\common\source\F2837xS_DefaultISR.d" \
"device\common\source\F2837xS_Dma.d" \
"device\common\source\F2837xS_ECap.d" \
"device\common\source\F2837xS_EPwm.d" \
"device\common\source\F2837xS_EQep.d" \
"device\common\source\F2837xS_Emif.d" \
"device\common\source\F2837xS_Gpio.d" \
"device\common\source\F2837xS_I2C.d" \
"device\common\source\F2837xS_Mcbsp.d" \
"device\common\source\F2837xS_PieCtrl.d" \
"device\common\source\F2837xS_PieVect.d" \
"device\common\source\F2837xS_Sci.d" \
"device\common\source\F2837xS_Spi.d" \
"device\common\source\F2837xS_SysCtrl.d" \
"device\common\source\F2837xS_TempSensorConv.d" \
"device\common\source\F2837xS_Upp.d" \
"device\common\source\F2837xS_can.d" \
"device\common\source\F2837xS_sci_io.d" \
"device\common\source\F2837xS_sdfm_drivers.d" \
"device\common\source\F2837xS_struct.d" \
"device\common\source\device.d" 

ASM_DEPS__QUOTED += \
"device\common\source\F2837xS_CodeStartBranch.d" \
"device\common\source\F2837xS_DBGIER.d" \
"device\common\source\F2837xS_usDelay.d" 

C_SRCS__QUOTED += \
"../device/common/source/F2837xS_Adc.c" \
"../device/common/source/F2837xS_CpuTimers.c" \
"../device/common/source/F2837xS_DefaultISR.c" \
"../device/common/source/F2837xS_Dma.c" \
"../device/common/source/F2837xS_ECap.c" \
"../device/common/source/F2837xS_EPwm.c" \
"../device/common/source/F2837xS_EQep.c" \
"../device/common/source/F2837xS_Emif.c" \
"../device/common/source/F2837xS_Gpio.c" \
"../device/common/source/F2837xS_I2C.c" \
"../device/common/source/F2837xS_Mcbsp.c" \
"../device/common/source/F2837xS_PieCtrl.c" \
"../device/common/source/F2837xS_PieVect.c" \
"../device/common/source/F2837xS_Sci.c" \
"../device/common/source/F2837xS_Spi.c" \
"../device/common/source/F2837xS_SysCtrl.c" \
"../device/common/source/F2837xS_TempSensorConv.c" \
"../device/common/source/F2837xS_Upp.c" \
"../device/common/source/F2837xS_can.c" \
"../device/common/source/F2837xS_sci_io.c" \
"../device/common/source/F2837xS_sdfm_drivers.c" \
"../device/common/source/F2837xS_struct.c" \
"../device/common/source/device.c" 

ASM_SRCS__QUOTED += \
"../device/common/source/F2837xS_CodeStartBranch.asm" \
"../device/common/source/F2837xS_DBGIER.asm" \
"../device/common/source/F2837xS_usDelay.asm" 


