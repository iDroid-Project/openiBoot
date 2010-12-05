#include "openiboot.h"
#include "commands.h"

initfn_t init_boot_sentinal __attribute__ ((section(BOOT_MODULE_INIT_SECTION))) = NULL;
initfn_t init_sentinal __attribute__ ((section(MODULE_INIT_SECTION))) = NULL;
exitfn_t exit_sentinal __attribute__ ((section(MODULE_EXIT_SECTION))) = NULL;
OPIBCommand *command_sentinal __attribute__ ((section(".commands"))) = NULL;
