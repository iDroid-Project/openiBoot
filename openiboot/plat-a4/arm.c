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

	InvalidateCPUDataCache();
	ClearCPUInstructionCache();

	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_INSTRUCTIONCACHE);	// Enable instruction cache
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_DATACACHE);		// Enable data cache

	WriteControlRegisterConfigData((ReadControlRegisterConfigData()
		& ~(ARM11_Control_STRICTALIGNMENTCHECKING)));				// Disable strict alignment fault checking

	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | ARM11_Control_BRANCHPREDICTION); 	// Enable branch prediction
	WriteAuxiliaryControlRegister(ReadAuxiliaryControlRegister() | ARM_A8_AuxControl_SPECULATIVEACCESSAXI);
	return 0;
}

void arm_disable_caches() {
	CleanAndInvalidateCPUDataCache();
	ClearCPUInstructionCache();
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~ARM11_Control_INSTRUCTIONCACHE);	// Disable instruction cache
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~ARM11_Control_DATACACHE);		// Disable data cache
}
