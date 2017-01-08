# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/vpb/common.c \
../src/vpb/tpl.c \
../src/vpb/con-manager.c


OBJS += \
./src/vpb/common.o \
./src/vpb/tpl.o \
./src/vpb/con-manager.o


# Each subdirectory must supply rules for building sources it contributes
src/vpb/%.o: ../src/vpb/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -fPIC -rdynamic -std=gnu99 -DDEBUG=$(DEBUGOPT) -O0 -g3 -Wall -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


