#ifndef OPENIBOOT_S_H
#define OPENIBOOT_S_H

#
#	Structures
#

# InterruptHandler
.equ InterruptHandler.handler, 0x0
.equ InterruptHandler.token, (InterruptHandler.handler + 0x4)
.equ InterruptHandler.useEdgeIC, (InterruptHandler.token + 0x4)

# TaskDescriptor
.equ TaskDescriptor.identifier1, 0x0
.equ TaskDescriptor.taskList, (TaskDescriptor.identifier1 + 0x4)
.equ TaskDescriptor.runqueueList, (TaskDescriptor.taskList + 0x8)
.equ TaskDescriptor.state, (TaskDescriptor.runqueueList + 0x8)
.equ TaskDescriptor.criticalSectionNestCount, (TaskDescriptor.state + 0x4)
.equ TaskDescriptor.savedRegisters, (TaskDescriptor.criticalSectionNestCount + 0x4)
.equ TaskDescriptor.sleepEvent, (TaskDescriptor.savedRegisters + 0x28)
.equ TaskDescriptor.linked_list_3, (TaskDescriptor.sleepEvent + 0x20)
.equ TaskDescriptor.exitState, (TaskDescriptor.linked_list_3 + 0x8)
.equ TaskDescriptor.taskRoutine, (TaskDescriptor.exitState + 0x4)
.equ TaskDescriptor.unknown_passed_value, (TaskDescriptor.taskRoutine + 0x4)
.equ TaskDescriptor.storage, (TaskDescriptor.unknown_passed_value + 0x4)
.equ TaskDescriptor.storageSize, (TaskDescriptor.storage + 0x4)
.equ TaskDescriptor.taskName, (TaskDescriptor.storageSize + 0x4)
.equ TaskDescriptor.identifier2, (TaskDescriptor.taskName + 0x10)
.equ TaskDescriptorIdentifier1, 0x7461736b
.equ TaskDescriptorIdentifier2,	0x74736b32

# TaskRegisterState
.equ TaskRegisterState.r4,		0x00
.equ TaskRegisterState.r5,		0x04
.equ TaskRegisterState.r6,		0x08
.equ TaskRegisterState.r7,		0x0c
.equ TaskRegisterState.r8,		0x10
.equ TaskRegisterState.r9,		0x14
.equ TaskRegisterState.r10,		0x18
.equ TaskRegisterState.r11,		0x1c
.equ TaskRegisterState.sp,		0x20
.equ TaskRegisterState.lr,		0x24

#endif

