################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/LinkedList.c \
../src/client.c \
../src/packet.c \
../src/routed_LS.c \
../src/server.c 

OBJS += \
./src/LinkedList.o \
./src/client.o \
./src/packet.o \
./src/routed_LS.o \
./src/server.o 

C_DEPS += \
./src/LinkedList.d \
./src/client.d \
./src/packet.d \
./src/routed_LS.d \
./src/server.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


