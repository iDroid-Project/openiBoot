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

#ifndef ARM_A8
	// Map the peripheral port of size 128 MB to 0x38000000
	WritePeripheralPortMemoryRemapRegister(PeripheralPort | ARM11_PeripheralPortSize128MB);
#endif

	InvalidateCPUDataCache();
	ClearCPUInstructionCache();

	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_INSTRUCTIONCACHE);	// Enable instruction cache
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_DATACACHE);		// Enable data cache

#ifndef ARM_A8
	WriteControlRegisterConfigData((ReadControlRegisterConfigData()
		& ~(ARM11_Control_STRICTALIGNMENTCHECKING))				// Disable strict alignment fault checking
		| ARM11_Control_UNALIGNEDDATAACCESS);					// Enable unaligned data access operations
#endif

#ifdef ARM11
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
