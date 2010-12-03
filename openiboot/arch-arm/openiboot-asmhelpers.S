#include "hardware/platform.h"
#include "hardware/arm.h"

.include "openiboot.S.h"

.global IncrementCriticalLock
.global DecrementCriticalLock
.global EnterCriticalSection
.global LeaveCriticalSection

.global EnableCPUIRQ
.global EnableCPUFIQ
.global DisableCPUIRQ
.global DisableCPUFIQ

@
@	Coprocessor manipulation
@

.global ReadControlRegisterConfigData
.global WriteControlRegisterConfigData
.global ReadAuxiliaryControlRegister
.global WriteAuxiliaryControlRegister
.global WriteDomainAccessControlRegister
.global ReadDataFaultStatusRegister
.global ReadFaultAddressRegister
.global WritePeripheralPortMemoryRemapRegister
.global GiveFullAccessCP10CP11
.global EnableVFP
.global WaitForInterrupt
.global WriteTranslationTableBaseRegister0
.global ReadTranslationTableBaseRegister0

.global InvalidateUnifiedTLBUnlockedEntries
.global ClearCPUInstructionCache
.global CleanDataCacheLineMVA
.global CleanCPUDataCache
.global InvalidateCPUDataCache
.global CleanAndInvalidateCPUDataCache
.global ClearCPUCaches

.global CallArm
.global CallThumb

.global Reboot
.global EndlessLoop

.global SwapTask
.global CurrentRunning

.text

@
@	Utility functions
@

.code 16
 .thumb_func
IncrementCriticalLock:
	LDR	R3, =CurrentRunning
	LDR	R2, [R3]
	LDR	R3, [R2,#TaskDescriptor.criticalSectionNestCount]
	ADD	R3, #1
	STR	R3, [R2,#TaskDescriptor.criticalSectionNestCount]
	BX	LR


.thumb_func
DecrementCriticalLock:
	LDR	R3, =CurrentRunning
	LDR	R2, [R3]
	LDR	R3, [R2,#TaskDescriptor.criticalSectionNestCount]
	SUB	R3, #1
	STR	R3, [R2,#TaskDescriptor.criticalSectionNestCount]
	BX	LR

.thumb_func
EnterCriticalSection:
	PUSH	{LR}

	LDR	R0, =DisableCPUIRQ
	BLX	R0
	LDR	R0, =DisableCPUFIQ
	BLX	R0

	LDR	R1, =CurrentRunning
	LDR	R2, [R1]
	LDR	R0, [R2,#TaskDescriptor.criticalSectionNestCount]
	ADD	R0, #1
	STR	R0, [R2,#TaskDescriptor.criticalSectionNestCount]

	POP	{PC}

.thumb_func
LeaveCriticalSection:
	PUSH	{LR}

	LDR	R1, =CurrentRunning
	LDR	R2, [R1]
	LDR	R0, [R2,#TaskDescriptor.criticalSectionNestCount]
	SUB	R0, #1
	STR	R0, [R2,#TaskDescriptor.criticalSectionNestCount]
	BNE	LeaveCriticalSection_return				@ check the Zero flag set by the previous SUB operation
									@ remember in thumb mode, everything has set flag set

	LDR	R0, =EnableCPUFIQ
	BLX	R0
	LDR	R0, =EnableCPUIRQ
	BLX	R0

LeaveCriticalSection_return:
	POP	{PC}

.thumb_func
CallArm:
	BX	R0

.code 32


CallThumb:
	BX	R0

@
@	FIQ/IRQ Control
@

EnableCPUIRQ:
	MRS	R0, CPSR
	BIC	R0, R0,	#ARM11_CPSR_IRQDISABLE
	MSR	CPSR_c,	R0
	BX	LR

EnableCPUFIQ:
	MRS	R0, CPSR
	BIC	R0, R0,	#ARM11_CPSR_FIQDISABLE
	MSR	CPSR_c,	R0
	BX	LR

DisableCPUIRQ:
	MRS	R0, CPSR
	ORR	R0, R0,	#ARM11_CPSR_IRQDISABLE
	MSR	CPSR_c,	R0
	BX	LR

DisableCPUFIQ:
	MRS	R0, CPSR
	ORR	R0, R0,	#ARM11_CPSR_FIQDISABLE
	MSR	CPSR_c,	R0
	BX	LR

@
@	Coprocessor manipulation
@

ReadControlRegisterConfigData:
	MRC	p15, 0,	R0, c1, c0, 0
	BX	LR

WriteControlRegisterConfigData:
	MCR	p15, 0,	R0, c1, c0, 0
	BX	LR

ReadAuxiliaryControlRegister:
	MRC	p15, 0,	R0, c1, c0, 1
	BX	LR

WriteAuxiliaryControlRegister:
	MCR	p15, 0,	R0, c1, c0, 1
	BX	LR

WriteDomainAccessControlRegister:
	MCR	p15, 0,	R0, c3, c0
	BX	LR

ReadDataFaultStatusRegister:
	MRC	p15, 0,	R0, c5, c0, 1
	BX	LR

ReadFaultAddressRegister:
	MRC	p15, 0,	R0, c6, c0, 1
	BX	LR

WritePeripheralPortMemoryRemapRegister:
	MCR	p15, 0,	R0, c15, c2, 4
	BX	LR

GiveFullAccessCP10CP11:
	MRC	p15, 0,	R0, c1, c0, 2
	MOV	R1, #(ARM11_AccessControl_CP10_ALL | ARM11_AccessControl_CP11_ALL)
	ORR	R0, R0,	R1
	MCR	p15, 0,	R0, c1, c0, 2
	BX	LR

EnableVFP:					@ FIXME: Not working yet without VFP
	FMRX	R2, FPEXC
	ORR	R2, R2,	ARM11_VFP_Enable
	FMXR	FPEXC, R2
	BX	LR

WaitForInterrupt:
	MOV	R0, #0
	MCR	p15, 0,	R0, c7, c0, 4
	BX	LR

@
@	MMU management
@

WriteTranslationTableBaseRegister0:
	MCR	p15, 0,	R0, c2, c0
	BX LR

ReadTranslationTableBaseRegister0:
	MRC	p15, 0,	R0, c2, c0
	BX LR

InvalidateUnifiedTLBUnlockedEntries:
	MOV	R0, #0
	MCR	p15, 0,	R0, c8, c7
	BX	LR

@
@	Cache management
@

ClearCPUInstructionCache:
	MOV	R0, #0
	MCR	p15, 0,	R0, c7, c5
	NOP
	NOP
	NOP
	NOP
	BLX	LR

CleanDataCacheLineMVA:
	MCR	p15, 0,	R0, c7, c10, 1
	BX	LR

CleanCPUDataCache:
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	MOV	R0, #0
	MCR	p15, 0,	R0, c7, c10
#else
	MOV	R2, #0x2000
CleanCPUDataCache_1:
	SUB	R2, R2, #0x40
	MOV	R1, #0
CleanCPUDataCache_2:
	SUBS	R1, R1, #0x40000000
	ORR	R0, R1, R2
	MCR	p15, 0, R0, c7, c10, 2
	BNE	CleanCPUDataCache_2
	CMP	R2, #0
	BNE	CleanCPUDataCache_1
	MOV	R0, #0
#endif
	MCR	p15, 0,	R0, c7, c10, 4	@ Data synchronization barrier
	BX	LR

InvalidateCPUDataCache:
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	MOV	R0, #0
	MCR	p15, 0,	R0, c7, c6
#else
	MOV	R2, #0x2000
InvalidateCPUDataCache_1:
	SUB	R2, R2, #0x40
	MOV	R1, #0
InvalidateCPUDataCache_2:
	SUBS	R1, R1, #0x40000000
	ORR	R0, R1, R2
	MCR	p15, 0, R0, c7, c6, 2
	BNE	InvalidateCPUDataCache_2
	CMP	R2, #0
	BNE	InvalidateCPUDataCache_1
#endif
	BX	LR

CleanAndInvalidateCPUDataCache:
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	MOV	R0, #0
	MCR	p15, 0,	R0, c7, c14
#else
	MOV	R2, #0x2000
CleanAndInvalidateCPUDataCache_1:
	SUB	R2, R2, #0x40
	MOV	R1, #0
CleanAndInvalidateCPUDataCache_2:
	SUBS	R1, R1, #0x40000000
	ORR	R0, R1, R2
	MCR	p15, 0, R0, c7, c14, 2
	BNE	CleanAndInvalidateCPUDataCache_2
	CMP	R2, #0
	BNE	CleanAndInvalidateCPUDataCache_1
	MOV	R0, #0
#endif
	MCR	p15, 0,	R0, c7, c10, 4	@ Data synchronization barrier
	BX	LR

ClearCPUCaches:
	STMFD	SP!, {LR}
	BL	CleanCPUDataCache
	BL	ClearCPUInstructionCache
	LDMFD	SP!, {LR}
	BX	LR

Reboot:
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	LDR	R0, =WDT_CTRL
	MOV	R1, #WDT_ENABLE
	STR	R1, [R0]
#else
	LDR     R1, =0xBF10202C
	LDR     R3, =0xBF102024
	MOVS    R0, #0
	MOVS    R2, #1
	STR     R0, [R1]
	STR     R2, [R3]
	LDR     R2, =0xBF102020
	MOV     R3, #0x80000000
	STR     R3, [R2]
	MOVS    R3, #4
	STR     R3, [R1]
	STR     R0, [R2]
#endif
	B	Reboot

@
@ Tasks
@

SwapTask:
	@ Store current stuff
	LDR R1, =CurrentRunning
	LDR R1, [R1]
	ADD R1, #TaskDescriptor.savedRegisters
	STR R4, [R1, #TaskRegisterState.r4]
	STR R5, [R1, #TaskRegisterState.r5]
	STR R6, [R1, #TaskRegisterState.r6]
	STR R7, [R1, #TaskRegisterState.r7]
	STR R8, [R1, #TaskRegisterState.r8]
	STR R9, [R1, #TaskRegisterState.r9]
	STR R10, [R1, #TaskRegisterState.r10]
	STR R11, [R1, #TaskRegisterState.r11]
	STR SP, [R1, #TaskRegisterState.sp]
	STR LR, [R1, #TaskRegisterState.lr]
	
	@ Load new task
	ADD R1, R0, #TaskDescriptor.savedRegisters
	LDR R4, [R1, #TaskRegisterState.r4]
	LDR R5, [R1, #TaskRegisterState.r5]
	LDR R6, [R1, #TaskRegisterState.r6]
	LDR R7, [R1, #TaskRegisterState.r7]
	LDR R8, [R1, #TaskRegisterState.r8]
	LDR R9, [R1, #TaskRegisterState.r9]
	LDR R10, [R1, #TaskRegisterState.r10]
	LDR R11, [R1, #TaskRegisterState.r11]
	LDR SP, [R1, #TaskRegisterState.sp]
	LDR LR, [R1, #TaskRegisterState.lr]

	@ Store new 'current' task
	LDR R1, =CurrentRunning
	STR R0, [R1]	
	BX LR

.thumb_func
.global task_stop
.global StartTask

StartTask:
	@ R4 = Task PTR
	@ R5 = Argument

	BL LeaveCriticalSection
	
	MOV R0, R5
	BLX R4

	BL task_stop

	@ We shouldn't get here, because the main thread is
	@ never started. So umm, what do we do? Just let
	@ the IP roam into the data segment? Sounds good. -- Ricky26

	B Reboot

EndlessLoop:
	B	EndlessLoop

.data

CurrentRunning:
	.word	0x0

