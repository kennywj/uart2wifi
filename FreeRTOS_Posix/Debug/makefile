################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include ParTest/subdir.mk
-include FreeRTOS_Kernel/subdir.mk
-include FreeRTOS_Kernel/portable/MemMang/subdir.mk
-include FreeRTOS_Kernel/portable/GCC/Posix/subdir.mk
-include FileIO/subdir.mk
-include Common_Demo/Minimal/subdir.mk
-include Common_Demo/Full/subdir.mk
-include AsyncIO/subdir.mk
-include shell/subdir.mk
-include uart2wifi/subdir.mk
-include lwip/subdir_api.mk
-include lwip/subdir_core.mk
-include lwip/subdir_ipv4.mk
-include lwip/subdir_ipv6.mk
-include lwip/subdir_if.mk
-include lwip/subdir_if_ppp.mk
-include lwip/subdir_if_polarssl.mk
-include port/subdir_port.mk
-include fatfs/subdir_fat.mk
-include fatfs/subdir_fat_option.mk
-include fatfs/subdir_fat_port.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: FreeRTOS_Posix

# Tool invocations
FreeRTOS_Posix: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -pthread -g -o"u2w" $(OBJS) $(USER_OBJS) $(LIBS) -lrt -lm
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) u2w
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
