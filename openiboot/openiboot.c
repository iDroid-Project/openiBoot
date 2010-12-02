#include "openiboot.h"
#include "platform.h"
#include "tasks.h"
#include "util.h"

int globalFtlHasBeenRestored = 0; 
int received_file_size;

void OpenIBootStart()
{
	platform_init();

	init_boot_modules();
	init_modules();

	//startScripting("openiboot"); //start script mode if there is a file
	bufferPrintf("  ___                   _ ____              _   \r\n");
	bufferPrintf(" / _ \\ _ __   ___ _ __ (_) __ )  ___   ___ | |_ \r\n");
	bufferPrintf("| | | | '_ \\ / _ \\ '_ \\| |  _ \\ / _ \\ / _ \\| __|\r\n");
	bufferPrintf("| |_| | |_) |  __/ | | | | |_) | (_) | (_) | |_ \r\n");
	bufferPrintf(" \\___/| .__/ \\___|_| |_|_|____/ \\___/ \\___/ \\__|\r\n");
	bufferPrintf("      |_|                                       \r\n");
	bufferPrintf("\r\n");
	bufferPrintf("version: %s\r\n", OPENIBOOT_VERSION_STR);
	DebugPrintf("                    DEBUG MODE\r\n");

//#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
//	audiohw_postinit();
//#endif

	tasks_run(); // Runs forever.

	exit_modules();

	platform_shutdown();
}
