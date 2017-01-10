# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/packet-buffer/packet-buffer.c \
../utils/ringbuf/ringbuf.c

OBJS += \
./src/packet-buffer/packet-buffer.o \
../utils/ringbuf/ringbuf.o

# Each subdirectory must supply rules for building sources it contributes
src/packet-buffer/%.o: ../src/packet-buffer/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -fPIC -rdynamic -std=gnu99 -DDEBUG=$(DEBUGOPT) -O0 -g3 -Wall -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


