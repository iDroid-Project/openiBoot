#include "openiboot.h"
#include "openiboot-asmhelpers.h"
#include "hardware/arm.h"
#include "hardware/platform.h"
#include "arm.h"

int arm_setup() {
	CleanAndInvalidateCPUDataCache();
	ClearCPUInstructionCache();

	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~(ARM11_Control_INSTRUCTIONCACHE));	// Disable instruction cache
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~(ARM11_Control_DATACACHE));		// Disable data cache

	GiveFullAccessCP10CP11();
	EnableVFP();

#ifdef ARM11
	// Map the peripheral port of size 128 MB to 0x38000000
	WritePeripheralPortMemoryRemapRegister(PeripheralPort | ARM11_PeripheralPortSize128MB);
#endif

	InvalidateCPUDataCache();
	ClearCPUInstructionCache();

	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_INSTRUCTIONCACHE);	// Enable instruction cache
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_DATACACHE);		// Enable data cache

#ifdef ARM11
	WriteControlRegisterConfigData((ReadControlRegisterConfigData()
		& ~(ARM11_Control_STRICTALIGNMENTCHECKING))				// Disable strict alignment fault checking
		| ARM11_Control_UNALIGNEDDATAACCESS);					// Enable unaligned data access operations
#endif

#ifdef ARM_A8
	WriteControlRegisterConfigData((ReadControlRegisterConfigData()
		& ~(ARM11_Control_STRICTALIGNMENTCHECKING)));				// Disable strict alignment fault checking
#endif


	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_BRANCHPREDICTION); 	// Enable branch prediction

#ifdef ARM11
	// Enable return stack, dynamic branch prediction, static branch prediction
	WriteAuxiliaryControlRegister(ReadAuxiliaryControlRegister()
		| ARM11_AuxControl_RETURNSTACK
		| ARM11_AuxControl_DYNAMICBRANCHPREDICTION
		| ARM11_AuxControl_STATICBRANCHPREDICTION);
#endif

#ifdef ARM_A8
	WriteAuxiliaryControlRegister(ReadAuxiliaryControlRegister() | ARM_A8_AuxControl_SPECULATIVEACCESSAXI);
#endif
	return 0;
}

void arm_disable_caches() {
	CleanAndInvalidateCPUDataCache();
	ClearCPUInstructionCache();
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~ARM11_Control_INSTRUCTIONCACHE);	// Disable instruction cache
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~ARM11_Control_DATACACHE);		// Disable data cache
}

void DataCacheOperation(uint8_t mode, uint32_t buffer, uint32_t size) {
	if (mode & (1 << 3)) {
		if (!buffer && (mode & 1) && !size)
			CleanCPUDataCache();
		return;
	}
	EnterCriticalSection();
	if (!buffer && size <= 1 && !(size & 1)) {
		if ((mode & 3) == 1) {
			CleanCPUDataCache();
		} else if ((mode & 3) == 3) {
			CleanAndInvalidateCPUDataCache();
		}
	} else {
		uint32_t buffer_end = buffer + size;
		if ((mode & 3) == 1) {
			buffer &= 0xFFFFFFC0;
			while (buffer < buffer_end) {
				CleanDataCacheLineMVA(buffer);
				buffer += 64;
			}
		} else if ((mode & 3) == 2) {
			while (buffer < buffer_end) {
				InvalidateDataCacheLineMVA(buffer);
				buffer += 64;
			}
		} else if ((mode & 3) == 3) {
			buffer &= 0xFFFFFFC0;
			while (buffer < buffer_end) {
				CleanAndInvalidateDataCacheLineMVA(buffer);
				buffer += 64;
			}
		}
	}
	if (mode & 4)
		DataSynchronizationBarrier();
	LeaveCriticalSection();
}
