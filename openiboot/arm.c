#include "hardware/arm.h"
#include "arm.h"
#include "openiboot-asmhelpers.h"

int arm_setup() {
	CleanAndInvalidateCPUDataCache();
	ClearCPUInstructionCache();

	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~(ARM11_Control_INSTRUCTIONCACHE));	// Disable instruction cache
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~(ARM11_Control_DATACACHE));		// Disable data cache

	GiveFullAccessCP10CP11();
	EnableVFP();

#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	// Map the peripheral port of size 128 MB to 0x38000000
	WritePeripheralPortMemoryRemapRegister(PeripheralPort | ARM11_PeripheralPortSize128MB);
#endif

	InvalidateCPUDataCache();
	ClearCPUInstructionCache();

	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_INSTRUCTIONCACHE);	// Enable instruction cache
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_DATACACHE);		// Enable data cache

#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	WriteControlRegisterConfigData((ReadControlRegisterConfigData()
		& ~(ARM11_Control_STRICTALIGNMENTCHECKING))				// Disable strict alignment fault checking
		| ARM11_Control_UNALIGNEDDATAACCESS);					// Enable unaligned data access operations
#else
	WriteControlRegisterConfigData((ReadControlRegisterConfigData()
		& ~(ARM11_Control_STRICTALIGNMENTCHECKING)));				// Disable strict alignment fault checking
#endif


	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_BRANCHPREDICTION); 	// Enable branch prediction

#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	// Enable return stack, dynamic branch prediction, static branch prediction
	WriteAuxiliaryControlRegister(ReadAuxiliaryControlRegister()
		| ARM11_AuxControl_RETURNSTACK
		| ARM11_AuxControl_DYNAMICBRANCHPREDICTION
		| ARM11_AuxControl_STATICBRANCHPREDICTION);
#else
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
