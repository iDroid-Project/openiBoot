#include "commands.h"
#include "util.h"
#include "h2fmi.h"
#include "ftl.h"

uint32_t VFL_ReadBlockDriverSign(uint32_t* buffer, uint32_t bytesToRead)
{
	uint32_t result;

	h2fmi_set_encryption(1);
	result = get_scfg_info(0, NULL, buffer, bytesToRead, "NANDDRIVERSIGN\0\0", 16, 0, 0);
	if(result)
		return result;

	bufferPrintf("vfl: didn't work 1\r\n");

	h2fmi_set_encryption(0);
	result = get_scfg_info(0, NULL, buffer, bytesToRead, "NANDDRIVERSIGN\0\0", 16, 0, 0);
	bufferPrintf("vfl: didn't work 2\r\n");
	if(result)
		bufferPrintf("fil: Disable data whitening\r\n");
	else
		h2fmi_set_encryption(1);

	return result;
}

uint32_t VFL_ReadBlockZeroSign(uint32_t* buffer, uint32_t bytesToRead)
{
	// Sorry, OldStyle not yet supported.
	system_panic("vfl: Not yet.");
	return 0;
}
