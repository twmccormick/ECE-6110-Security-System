################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/vl53l0x/vl53l0x_api.c \
../Core/Src/vl53l0x/vl53l0x_api_calibration.c \
../Core/Src/vl53l0x/vl53l0x_api_core.c \
../Core/Src/vl53l0x/vl53l0x_api_ranging.c \
../Core/Src/vl53l0x/vl53l0x_api_strings.c \
../Core/Src/vl53l0x/vl53l0x_platform_log.c \
../Core/Src/vl53l0x/vl53l0x_tof.c 

OBJS += \
./Core/Src/vl53l0x/vl53l0x_api.o \
./Core/Src/vl53l0x/vl53l0x_api_calibration.o \
./Core/Src/vl53l0x/vl53l0x_api_core.o \
./Core/Src/vl53l0x/vl53l0x_api_ranging.o \
./Core/Src/vl53l0x/vl53l0x_api_strings.o \
./Core/Src/vl53l0x/vl53l0x_platform_log.o \
./Core/Src/vl53l0x/vl53l0x_tof.o 

C_DEPS += \
./Core/Src/vl53l0x/vl53l0x_api.d \
./Core/Src/vl53l0x/vl53l0x_api_calibration.d \
./Core/Src/vl53l0x/vl53l0x_api_core.d \
./Core/Src/vl53l0x/vl53l0x_api_ranging.d \
./Core/Src/vl53l0x/vl53l0x_api_strings.d \
./Core/Src/vl53l0x/vl53l0x_platform_log.d \
./Core/Src/vl53l0x/vl53l0x_tof.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/vl53l0x/vl53l0x_api.o: ../Core/Src/vl53l0x/vl53l0x_api.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32L475xx -c -I../Components/hts221/ -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/vl53l0x/vl53l0x_api.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/vl53l0x/vl53l0x_api_calibration.o: ../Core/Src/vl53l0x/vl53l0x_api_calibration.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32L475xx -c -I../Components/hts221/ -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/vl53l0x/vl53l0x_api_calibration.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/vl53l0x/vl53l0x_api_core.o: ../Core/Src/vl53l0x/vl53l0x_api_core.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32L475xx -c -I../Components/hts221/ -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/vl53l0x/vl53l0x_api_core.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/vl53l0x/vl53l0x_api_ranging.o: ../Core/Src/vl53l0x/vl53l0x_api_ranging.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32L475xx -c -I../Components/hts221/ -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/vl53l0x/vl53l0x_api_ranging.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/vl53l0x/vl53l0x_api_strings.o: ../Core/Src/vl53l0x/vl53l0x_api_strings.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32L475xx -c -I../Components/hts221/ -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/vl53l0x/vl53l0x_api_strings.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/vl53l0x/vl53l0x_platform_log.o: ../Core/Src/vl53l0x/vl53l0x_platform_log.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32L475xx -c -I../Components/hts221/ -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/vl53l0x/vl53l0x_platform_log.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/vl53l0x/vl53l0x_tof.o: ../Core/Src/vl53l0x/vl53l0x_tof.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32L475xx -c -I../Components/hts221/ -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/Src/vl53l0x/vl53l0x_tof.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

