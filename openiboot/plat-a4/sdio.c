#include "openiboot.h"
#include "arm/arm.h"
#include "commands.h"
#include "util.h"
#include "sdio.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "interrupt.h"
#include "hardware/sdio.h"
#include "tasks.h"

typedef struct SDIOFunction
{
	int blocksize;
	int maxBlockSize;
	int enableTimeout;
	InterruptServiceRoutine irqHandler;
	uint32_t irqHandlerToken;
} SDIOFunction;

//uint32_t RCA;
static int NumberOfFunctions = 0;

SDIOFunction* SDIOFunctions;

uint8_t* fn0cisBuf;

int sdio_block_reset();
int sdio_set_controller(int _clkmode, int _clkrate, uint8_t _buswidth, int _speedmode);
int sdio_reset();
int sdio_send_io(uint8_t command, uint32_t ocr, uint32_t* rocr);
void enable_card_interrupt(int on_off);
int sdio_read_cis (int function);
int sdio_parse_fn0_cis();
int sdio_io_rw_direct(int isWrite, int function, uint32_t address, uint8_t in, uint8_t* out);
void set_card_interrupt (OnOff on_off);

int sdio_setup()
{
	int i;
	uint8_t data;
	fn0cisBuf = (uint8_t*)malloc(0x200); // check if this will work -- has to be done there

	//interrupt_install(0x26, sdio_handle_interrupt, 0);
	//interrupt_enable(0x26);

	clock_gate_switch(SDIO_CLOCKGATE, ON);
	
	if (sdio_block_reset() != 0)
	{
		bufferPrintf("sdio: Failed to complete soft reset!\r\n");
		return -1;
	}
	
	// clockmode=1, clockrate=25MHz, buswidth=4, speedmode=1
	if (sdio_set_controller(1, 25000000, 4, 1) !=0)
	{
		bufferPrintf("sdio: failed to configure SDIO block! \r\n");
		return -1;
	}
	
	if(sdio_reset() != 0)
	{
		bufferPrintf("sdio: error resetting card\r\n");
		return -1;
	}
	

	// Enable SDIO IRQs - may not be here but well, who knows....
	enable_card_interrupt(ON);
    
	//**Enumerate card slot**//
	uint32_t ocr[4];
	i = 0;
	
		// get initial card operating conditions
	do
	{	if(sdio_send_io(5, 0, ocr) != 0) //io_send_op_cond()
		{
			bufferPrintf("sdio: error querying card operating conditions\r\n");
			return -1;
		}
		
		task_sleep(1);
		i++;
	}
	while((ocr[0] >> 0x1F == 0) && (i < 100));
	
	if (i == 100)
	{
		bufferPrintf("sdio: timeout waiting for CMD5 I/O ready! \r\n");
		return -1;
	}
	
	bufferPrintf("CMD5 response: OCR: 0x%8x\r\n", ocr[0] & 0xFFFFF);
	bufferPrintf("CMD5 response: Device has memory: %d\r\n", (ocr[0] >> 0x1B) & 0x1);

	if((ocr[0] & 0x70000000) == 0)
		bufferPrintf("CMD5 response: No I/O functions");
	else
		NumberOfFunctions = 1;
	
		
	if(sdio_send_io(5, 0xFFFF00, ocr) != 0) //io_send_op_cond()
		{
			bufferPrintf("sdio: error querying card operating conditions\r\n");
			return -1;
		}
	bufferPrintf("sdio: Ready! CMD5 response: 0x%8x\r\n", ocr[0] >> 0x1F);
	
		// probe for relative card address
	if(sdio_send_io(3, 0, ocr) != 0) //send_relative_addr()
	{
			bufferPrintf("sdio: probing card relative address failed!\r\n");
			return -1;
	}
	
		// select/deselect card with relative address
	if(sdio_send_io(7, ocr[0] & 0xFFFF0000, ocr) != 0) //select_deselect_card()
	{
			bufferPrintf("sdio: select card with relative address 0x%8x failed!\r\n", ocr[0] & 0xFFFF0000);
			return -1;
	}
	
	bufferPrintf("sdio: CMD7 response [0x%8x][0x%8x][0x%8x][0x%8x]", ocr[0], ocr[1], ocr[2], ocr[3]);
	
		// read card information structure
	if(sdio_read_cis(0) != 0)
	{
		bufferPrintf("sdio: fail to read CIS data!! \r\n");
		return -1;
	}
	
	if(sdio_parse_fn0_cis() != 0)
	{
		bufferPrintf("sdio: error parsing function 0 CIS data!! \r\n");
		return -1;
	}
	
	if(sdio_io_rw_direct(FALSE, 0, 8, 0, &data) != 0)
			return -1;
	bufferPrintf("sdio: card capability 0x%4x\r\n", data);
	
	//** Done enumerate card slot**//
	
	bufferPrintf("sdio: Ready!!\r\n");
	
	return 0;
}


void sdio_init()
{
	sdio_setup();
}


int sdio_block_reset()
{
	SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFFFF) | ((SDIO_CTRL2_SOFTRESET(GET_REG(SDIO + SDIO_CTRL2)) | (SDIOBLOCK_RESET_MASK & 0x7)) << 24));
	int i = 0;
	while(((SDIO_CTRL2_SOFTRESET(GET_REG(SDIO + SDIO_CTRL2)) & 0x7) != 0) && i < 100)
	{
		udelay(5000);
		i++;
	}

	if(i == 100)
		return -1;

	if(SDIOBLOCK_RESET_MASK & 1)
	{
		SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFF0000) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) | 1 ));
		i = 0;
		while(((SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & 0x2) == 0) && i < 10)
		{
			udelay(5000);
			i++;
		}

		if(i == 10)
			return -1;

		set_card_interrupt(ON);

		return 0;
	}

	udelay(400000);

	return 0;
}


int sdio_set_controller (int _clkmode, int _clkrate, uint8_t _buswidth, int _speedmode)
{
	int i = 0;
	int status = 0;
	//reset sdio block
	if (sdio_block_reset() !=0)
		return -1;

	//set bus speed mode
	if (_speedmode != 1 && _speedmode != 2)
	{
		bufferPrintf("sdio: invalid bus speed mode setting! \r\n");
		return -1;
	}
	if (_speedmode == 1)
		SET_REG(SDIO + SDIO_CTRL1, (GET_REG(SDIO + SDIO_CTRL1) & 0xFFFFFF) | ((SDIO_CTRL1_WAKEUP(GET_REG(SDIO + SDIO_CTRL1)) & ~0x4) << 24));
		
	if (_speedmode == 2)
		SET_REG(SDIO + SDIO_CTRL1, (GET_REG(SDIO + SDIO_CTRL1) & 0xFFFFFF) | ((SDIO_CTRL1_WAKEUP(GET_REG(SDIO + SDIO_CTRL1)) | 0x4) << 24));

	//set bus width
	if (_buswidth != 1 && _buswidth != 4)
	{
		bufferPrintf("sdio: invalid bus width setting! \r\n");
		return -1;	
	}

	status = 0;
	if ((SDIO_IRQEN_STATUS(GET_REG(SDIO + SDIO_IRQEN)) & 0x100) != 0)
		status = (SDIO_ISREN_STATUS(GET_REG(SDIO + SDIO_ISREN)) >> 8) & 1;

	enable_card_interrupt(OFF);

	if (_buswidth == 4)
		SET_REG(SDIO + SDIO_CTRL1, (GET_REG(SDIO + SDIO_CTRL1) & 0xFFFFFF00) | (SDIO_CTRL1_HOST(GET_REG(SDIO + SDIO_CTRL1)) | 0x2));
	else
		SET_REG(SDIO + SDIO_CTRL1, (GET_REG(SDIO + SDIO_CTRL1) & 0xFFFFFF00) | (SDIO_CTRL1_HOST(GET_REG(SDIO + SDIO_CTRL1)) & ~0x2));

	enable_card_interrupt(status);

	//set clock rate
	if (_clkrate > SDIO_Max_Frequency)
	{
		bufferPrintf("sdio: clock frequency out of range!\r\n");
		return -1;
	}

	status = SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & 0x4;
	SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFF0000) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & ~0x4 ));	

	i = 0;
	while ((SDIO_Base_Frequency / (1 << i)) > _clkrate)
		i++;
    
	int div = 1 << i;
	if (div >= 256)
		div = 256;
	
	SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFF0000) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & 0xFF ));	
	SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFF0000) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) | 0x1 | ((div >> 0x1) << 0x8)));	
	
	if (status != 0)
	{
		i = 0;
		while(((SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & 0x2) == 0) && i < 10)
		{
			udelay(5000);
			i++;
		}

		if(i == 10)
		{ 
			bufferPrintf("sdio: Failed to set clock rate!\r\n");
			return -1;
		}
	}

	i = 7;
	int clock_state = SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & 0xFF00;
	while (((clock_state & 1) == 0) && (i != 0))
	{
		i--;
		clock_state <<= 1;		
	}

	int state = SDIO_IRQEN_ERRSTATUS(GET_REG(SDIO + SDIO_IRQEN));

	SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF) | ((SDIO_IRQEN_ERRSTATUS(GET_REG(SDIO + SDIO_IRQEN)) & ~0x10) << 16));

	SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFFFF) | ((SDIO_CTRL2_SOFTRESET(GET_REG(SDIO + SDIO_CTRL2)) | (clock_state & 0xF)) << 24));

	if (state & 0x10)
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF) | ((SDIO_IRQEN_ERRSTATUS(GET_REG(SDIO + SDIO_IRQEN)) | 0x10) << 16));

	if (status == 0)
		SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFFFF00) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & ~0x4 ));	
	else
		SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFFFF00) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) | 0x4 ));	

	//set clock mode
	if (_clkmode != 1 && _clkmode != -1)
	{
		bufferPrintf("sdio: invalid clock mode setting! \r\n");
		return -1;
	}

	if (_clkmode == -1)
		SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFFFF00) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & ~0x4 ));	

	if (_clkmode == 1)
	{
		SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFFFF00) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) | 1 ));
		i = 0;
		while(((SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) & 0x2) == 0) && i < 10)
		{
			udelay(5000);
			i++;
		}

		if(i == 10)
		{ 
			bufferPrintf("sdio: Failed to complete soft reset!\r\n");
			return -1;
		}

		SET_REG(SDIO + SDIO_CTRL2, (GET_REG(SDIO + SDIO_CTRL2) & 0xFFFFFF00) | (SDIO_CTRL2_CLOCK(GET_REG(SDIO + SDIO_CTRL2)) | 0x4 ));
	}

	return 0;
}

int sdio_reset()
{
	// function-device_reset
	gpio_pin_output(SDIO_GPIO_DEVICE_RESET, 0);
	udelay(12000);
	gpio_pin_output(SDIO_GPIO_DEVICE_RESET, 0);
	udelay(150000);
	return 0;
}

void set_card_interrupt (OnOff on_off)
{
	if(on_off == ON)
	{
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF0000) | (SDIO_IRQEN_STATUS(GET_REG(SDIO + SDIO_IRQEN)) | 0x1));
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF) | ((SDIO_IRQEN_ERRSTATUS(GET_REG(SDIO + SDIO_IRQEN)) | 0xF) << 16));
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF0000) | (SDIO_IRQEN_STATUS(GET_REG(SDIO + SDIO_IRQEN)) | 0x2));
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF) | ((SDIO_IRQEN_ERRSTATUS(GET_REG(SDIO + SDIO_IRQEN)) | 0x70) << 16));
	}
	else
	{   
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF0000) | (SDIO_IRQEN_STATUS(GET_REG(SDIO + SDIO_IRQEN)) & ~0x1));
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF) | ((SDIO_IRQEN_ERRSTATUS(GET_REG(SDIO + SDIO_IRQEN)) & ~0xF) << 16));
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF0000) | (SDIO_IRQEN_STATUS(GET_REG(SDIO + SDIO_IRQEN)) & ~0x2));
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF) | ((SDIO_IRQEN_ERRSTATUS(GET_REG(SDIO + SDIO_IRQEN)) & ~0x70) << 16));
	}
}

void enable_card_interrupt (int on_off)
{
	if(on_off == ON)
	{
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF0000) | (SDIO_IRQEN_STATUS(GET_REG(SDIO + SDIO_IRQEN)) | 0x100));
		SET_REG(SDIO + SDIO_ISREN, (GET_REG(SDIO + SDIO_ISREN) & 0xFFFF) | (SDIO_ISREN_STATUS(GET_REG(SDIO + SDIO_ISREN)) | 0x100));
	}
	else
	{   
		SET_REG(SDIO + SDIO_IRQEN, (GET_REG(SDIO + SDIO_IRQEN) & 0xFFFF0000) | (SDIO_IRQEN_STATUS(GET_REG(SDIO + SDIO_IRQEN)) & ~0x100));
		SET_REG(SDIO + SDIO_ISREN, (GET_REG(SDIO + SDIO_ISREN) & 0xFFFF) | (SDIO_ISREN_STATUS(GET_REG(SDIO + SDIO_ISREN)) & ~0x100));
	}
}

int sdio_wait_for_ready()
{
	// wait for CMD_STATE to be CMD_IDLE
	int i = 0;
	while(((GET_REG(SDIO + SDIO_STATE) & 1) != 0) && (i < 20))
	{
		task_sleep(5);
		i++;
	}
	
	if (i == 20)
		return -1;

	return 0;
}



int sdio_wait_for_cmd_ready()
{
	// wait for the command to be ready to transmit
	int i = 0;
	while(((SDIO_IRQ_STATUS(GET_REG(SDIO + SDIO_IRQ)) & 1) == 0) && ((SDIO_IRQ_ERRSTAT(GET_REG(SDIO + SDIO_IRQ)) & 0xF) == 0) && i < 1000)
	{
		i++;
	}

	if (i == 1000)
		return -1;

	return 0;
}

int sdio_command_check_interr()
{
	if ((SDIO_IRQ_ERRSTAT(GET_REG(SDIO + SDIO_IRQ)) & 0xF) != 0)
		return -1;
	return 0;
}

int sdio_send_io(uint8_t command, uint32_t ocr, uint32_t* rocr)
{
	int ret;
	uint16_t cmd;

	// clear the upper bits that would indicate card status
	ocr &= 0x1FFFFFF;

	if((GET_REG(SDIO + SDIO_STATE) & 1) != 0)
	{
		ret = sdio_wait_for_ready();
		if(ret)
			return ret;
	}

	SET_REG(SDIO + SDIO_ARGU, ocr);

	//set CMD
	cmd = (command << 8) & 0x3F00;
	if((command << 16) == 0x350000)
		cmd |= 0x20;

	if(command == 5)
		cmd |= 2;
	else
		cmd |= 0x1A;

	SET_REG(SDIO + SDIO_CMD, (SDIO_CMD_COMMAND(GET_REG(SDIO + SDIO_CMD)) & 0xFFFF) | (cmd << 16));

	if(((SDIO_IRQ_STATUS(GET_REG(SDIO + SDIO_IRQ)) & 1) == 0) && ((SDIO_IRQ_ERRSTAT(GET_REG(SDIO + SDIO_IRQ)) & 0xF) == 0))
	{
		ret = sdio_wait_for_cmd_ready();
		if(ret)
			return ret;
	}

	if(sdio_command_check_interr())
		return -1;

	//clear interrupt	
	SET_REG(SDIO + SDIO_IRQ, 1 | (0xF << 16));

	//get response
	rocr[0] = GET_REG(SDIO + SDIO_RESP0);
	rocr[1] = GET_REG(SDIO + SDIO_RESP1);
	rocr[2] = GET_REG(SDIO + SDIO_RESP2);
	rocr[3] = GET_REG(SDIO + SDIO_RESP3);

	return 0;
}


int sdio_io_rw_direct(int isWrite, int function, uint32_t address, uint8_t in, uint8_t* out)
{
	uint32_t arg = 0;
	uint32_t response[4];

	arg |= isWrite ? 0x80000000 : 0; // write bit
	arg |= isWrite ? in : 0; // write value
	arg |= function << 28;
	arg |= address << 9;


	if(sdio_send_io(52, arg, &response[0]) != 0)
	{
		bufferPrintf("sdio: error doing cmd52 read/write!\r\n");
		return -1;
	}

	*out = (uint8_t)response[0];

	return 0;
}


int sdio_read_cis (int function)
{
	int i;
	uint8_t data;

	uint32_t ptr = 0;
	for(i = 0; i < 3; ++i)
	{
		if(sdio_io_rw_direct(FALSE, 0, (function * 0x100) + 0x9 + i, 0, &data) != 0)
			return -1;

		ptr += data << (8 * i);
	}

	for(i = 0; i < 200; ++i)
	{
		if(sdio_io_rw_direct(FALSE, 0, ptr + i, 0, &fn0cisBuf[i]) != 0)
			return -1;
	}

	return 0;
}


int sdio_parse_fn0_cis()
{
	uint32_t bufferSize = 0x200;
	uint32_t size;
	uint32_t tuple;
	int n;

	int i = 0;
	while((i + 1) < bufferSize)
	{
		tuple = fn0cisBuf[i];
		if(tuple == 0)
			return -1;

		// end of info
		if(tuple == 0xFF)
			return 0;

		if(bufferSize < (i + 2))
			return -1;

		size = fn0cisBuf[i+1];
		if(bufferSize < (size + i + 2))
			return -1;

		// looking for version or manufacturer info
		if((tuple != 21) && (tuple != 32))
			continue;

		if(tuple == 21) 
		{
			if (size <= 6)
			{
				bufferPrintf("sdio: invalid version tuple!! \r\n");
				return -1;
			}

			bufferPrintf("sdio: product info:");
			for(n = 0; n < (size - 3); n++)
				bufferPrintf(" 0x%4x", fn0cisBuf[4+n]);
			bufferPrintf("\r\n");
		}

		if(tuple == 32)
		{
			if(size != 4)
			{
				bufferPrintf("sdio: invalid manufacturer tuple!! \r\n");
				return -1;
			}

			bufferPrintf("sdio: device manufacturer ID 0x%4x, product ID 0x%4x \r\n", fn0cisBuf[i+2] | fn0cisBuf[i+3] << 8, fn0cisBuf[i+4] | fn0cisBuf[i+5] << 8);
		}

		i += (size + 2);
	}

	return 0;
}

int sdio_enable_func_interrupt(uint8_t function, OnOff on_off)
{
	uint8_t reg;

	if(function > 7)
		return -1;

	if(sdio_io_rw_direct(FALSE, 0, 0x4, 0, &reg) != 0)
		return -1;

	if(on_off == ON)
		reg |= 1 << function; //enable
	else
		reg &= ~(1 << function); //disable

	if(sdio_io_rw_direct(TRUE, 0, 0x4, reg, NULL) != 0)
		return -1;

	return 0;
}

int sdio_enable_function(uint8_t function, OnOff on_off)
{
	uint8_t reg;

	if(function <= 0 || function > 7)
		return -1;

	if(sdio_io_rw_direct(FALSE, 0, 0x2, 0, &reg) != 0)
		return -1;

	if(on_off == ON)
		reg |= 1 << function;
	else
		reg &= ~(1 << function);

	if(sdio_io_rw_direct(TRUE, 0, 0x2, reg, NULL) != 0)
		return -1;

	if(on_off == ON)
	{
		uint64_t startTime = timer_get_system_microtime();
		while (1) {
			if(sdio_io_rw_direct(FALSE, 0, 0x3, 0, &reg) != 0)
				return -1;

			if (reg & (1 << function))
				break;

			if(has_elapsed(startTime, 1000))
			{
				bufferPrintf("sdio: timeout enabling functon!!\r\n");
				return -1;
			
			}
		}
	}

	return 0;
}

static void cmd_sdio_setup(int argc, char** argv)
{
	sdio_setup();
	bufferPrintf("sdio setup done\r\n");
}
COMMAND("sdio_setup", "SDIO setup", cmd_sdio_setup);
