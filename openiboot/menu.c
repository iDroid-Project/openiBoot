#include "openiboot.h"
#include "lcd.h"
#include "util.h"
#include "framebuffer.h"
#include "buttons.h"
#include "timer.h"
#include "images/ConsolePNG.h"
#include "images/iPhoneOSPNG.h"
#include "images/AndroidOSPNG.h"
#include "images/ConsoleSelectedPNG.h"
#include "images/iPhoneOSSelectedPNG.h"
#include "images/AndroidOSSelectedPNG.h"
#include "images/HeaderPNG.h"
#include "images.h"
#include "actions.h"
#include "stb_image.h"
#include "pmu.h"
#include "nand.h"
#include "radio.h"
#include "hfs/fs.h"
#include "ftl.h"
#include "scripting.h"
#include "multitouch.h"
#include "nvram.h"
#include "tasks.h"

int globalFtlHasBeenRestored; /* global variable to tell wether a ftl_restore has been done*/
static TaskDescriptor menu_task;

void menu_draw()
{
	framebuffer_clear();

	int y = 0;
	BootEntry *entry = setup_root()->list_ptr.next;
	for(;entry != setup_root(); entry = entry->list_ptr.next)
	{
		if(entry == setup_current())
			framebuffer_setcolors(COLOR_BLACK, COLOR_WHITE);
		else
			framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);

		framebuffer_setloc(0, y);
		framebuffer_print_force(entry->title);
		y++;
	}

	if(setup_root() == setup_current())
		framebuffer_setcolors(COLOR_BLACK, COLOR_WHITE);
	else
		framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);

	framebuffer_setloc(0, y);
	framebuffer_print_force("Console");

	framebuffer_setloc(0, 47);
	framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);
	framebuffer_print_force(OPENIBOOT_VERSION_STR);
}

static void menu_run(uint32_t _V)
{
	while(TRUE)
	{
#ifdef BUTTONS_VOLDOWN
		if(buttons_is_pushed(BUTTONS_HOLD)
				|| !buttons_is_pushed(BUTTONS_VOLDOWN))
#else
		if(buttons_is_pushed(BUTTONS_HOLD))
#endif
		{
			setup_entry(setup_current()->list_ptr.next);

			menu_draw();

			udelay(200000);
		}

#ifdef BUTTONS_VOLUP
		if(!buttons_is_pushed(BUTTONS_VOLUP))
		{
			setup_entry(setup_current()->list_ptr.prev);

			menu_draw();

			udelay(200000);
		}
#endif

		if(buttons_is_pushed(BUTTONS_HOME))
		{
			framebuffer_setdisplaytext(TRUE);
			framebuffer_clear();

			// Special case for console
			if(setup_current() == setup_root())
			{
				OpenIBootConsole();
				task_stop();
				return;
			}

			setup_boot();
		}

		task_yield();
	}
}

void menu_main()
{
	task_init(&menu_task, "menu");

	nand_setup();
	fs_setup();

	if(script_run_file("(0)/boot/menu.lst"))
	{
		OpenIBootConsole();
		return;
	}

	const char* sTempOS = nvram_getvar("opib-temp-os");
	if(sTempOS && *sTempOS)
	{
		nvram_setvar("opib-temp-os","");
		nvram_save();

		setup_title(sTempOS);
		setup_boot();
	}

	setup_entry(setup_default());
	
	pmu_set_iboot_stage(0);

    framebuffer_setdisplaytext(FALSE);
	framebuffer_clear();

	menu_draw();

	task_start(&menu_task, &menu_run, NULL);
}

static void menu_init_boot()
{
	framebuffer_clear();
	bufferPrintf("Loading openiBoot...\n");

	OpenIBootMain = &menu_main;
}
MODULE_INIT_BOOT(menu_init_boot);

