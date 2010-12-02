#include "openiboot.h"
#include "commands.h"
#include "framebuffer.h"
#include "util.h"
#include "images/installerBarEmptyPNG.h"
#include "images/installerBarFullPNG.h"
#include "images/installerLogoPNG.h"

static uint32_t *cmd_progress_empty = NULL;
static int cmd_progress_empty_width, cmd_progress_empty_height;
static uint32_t *cmd_progress_full = NULL;
static int cmd_progress_full_width, cmd_progress_full_height;

static void installer_init()
{
	framebuffer_setdisplaytext(FALSE);
	framebuffer_clear();

	{
		int w, h;
		uint32_t *bgImg = framebuffer_load_image(datainstallerLogoPNG, sizeof(datainstallerLogoPNG), &w, &h, TRUE);
		if(bgImg)
		{
			int x = (framebuffer_width() - w)/2;
			int y = (framebuffer_height() - h)/3;

			framebuffer_draw_image(bgImg, x, y, w, h);
		}
		else
		{
			framebuffer_setdisplaytext(TRUE);
			bufferPrintf("Failed to load image...\n");
		}
	}
}
MODULE_INIT_BOOT(installer_init);

void cmd_progress(int argc, char **argv)
{
	if(argc != 2)
	{
		bufferPrintf("Usage: %s <percentage>\n", argv[0]);
		return;		
	}

	int x,y,w;
	int percent = parseNumber(argv[1]);

	if(cmd_progress_empty == NULL)
		cmd_progress_empty = framebuffer_load_image(
				datainstallerBarEmptyPNG, sizeof(datainstallerBarEmptyPNG),
				&cmd_progress_empty_width, &cmd_progress_empty_height, 0);
		
	if(cmd_progress_full == NULL)
		cmd_progress_full = framebuffer_load_image(
				datainstallerBarFullPNG, sizeof(datainstallerBarFullPNG),
				&cmd_progress_full_width, &cmd_progress_full_height, 0);

	x = (framebuffer_width()-cmd_progress_empty_width)/2;
	y = 2*((framebuffer_height()-cmd_progress_empty_height)/3);
	w = (percent*cmd_progress_full_width)/100;

	if(percent < 0)
	{
		framebuffer_fill_rect(0, x, y, cmd_progress_empty_width, cmd_progress_empty_height);
	}
	else if(percent > 100)
	{
		int left = ((percent-100)%120)-20;
		int right = left + 20;
		if(left < 0)
			left = 0;
		if(right > 100)
			right = 100;

		left = (left*cmd_progress_full_width)/100;
		right = (right*cmd_progress_full_width)/100;

		if(right < 100)
			framebuffer_draw_image(cmd_progress_empty, x, y,
					cmd_progress_empty_width, cmd_progress_empty_height);	
		
		framebuffer_draw_image_clip(cmd_progress_full, x, y,
				cmd_progress_full_width, cmd_progress_full_height,
				right, cmd_progress_full_height);

		if(left > 0)
			framebuffer_draw_image_clip(cmd_progress_empty, x, y,
					cmd_progress_empty_width, cmd_progress_empty_height,
					left, cmd_progress_empty_height);
	}
	else
	{	
		framebuffer_draw_image(cmd_progress_empty, x, y,
				cmd_progress_empty_width, cmd_progress_empty_height);	

		framebuffer_draw_image_clip(cmd_progress_full, x, y,
				cmd_progress_full_width, cmd_progress_full_height,
				w, cmd_progress_full_height);
	}
}
COMMAND("progress", "Set the install progress.", cmd_progress);
