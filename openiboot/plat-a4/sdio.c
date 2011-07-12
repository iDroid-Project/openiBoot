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

typedef struct _sdio_regs
{
	reg32_t	sdma_addr;
	reg16_t	blk_size;
	reg16_t	blk_cnt;
	reg32_t	argument;
	reg16_t	tx_mode;
	reg16_t	command;
	reg32_t	response[4];
	reg32_t	bdata;
	reg32_t	prn_status;
	reg8_t	host_control;
	reg8_t	pwr_control;
	reg8_t	blk_gap;
	reg8_t	wake_control;
	reg16_t clock_control;
	reg8_t	timeout_control;
	reg8_t	sw_reset;
	reg16_t int_sts;
	reg16_t err_int_sts;
	reg16_t int_en;
	reg16_t err_int_en;
	reg16_t int_sig_en;
	reg16_t err_int_sig_en;
	reg32_t acmd12_err_status;
	reg32_t capabilities;
	reg32_t unused_44;
	reg32_t max_curr_cap;
	reg32_t unused_4C;
	reg16_t feaer;
	reg16_t feerr;
	reg32_t adma_err;
	reg32_t adma_addr;
	reg32_t unused_5c[9];
	reg32_t control2;
	reg32_t control3;
	reg32_t unused_88;
	reg32_t control4;
	reg32_t unused_90[27];
	reg16_t unused_fc;
	reg16_t version;
} __attribute__((packed)) __attribute__((aligned(4))) sdio_regs_t;

typedef struct SDIOFunction
{
	int blocksize;
	int maxBlockSize;
	int enableTimeout;
	InterruptServiceRoutine irqHandler;
	uint32_t irqHandlerToken;
} SDIOFunction;

typedef struct _sdio_controller
{
	sdio_regs_t *regs;
} sdio_controller_t;

//uint32_t RCA;
static int NumberOfFunctions = 0;

SDIOFunction* SDIOFunctions;

uint8_t* fn0cisBuf;

sdio_controller_t sdio0 = {
	.regs = (sdio_regs_t*)SDIO,
};

void set_card_interrupt(sdio_controller_t *_sdio, OnOff on_off)
{
	if(on_off == ON)
	{
		SET_REG16(&_sdio->regs->int_en, GET_REG16(&_sdio->regs->int_en) | 1);
		SET_REG16(&_sdio->regs->err_int_en, GET_REG16(&_sdio->regs->err_int_en) | 0xF);
		SET_REG16(&_sdio->regs->int_en, GET_REG16(&_sdio->regs->int_en) | 2);
		SET_REG16(&_sdio->regs->err_int_en, GET_REG16(&_sdio->regs->err_int_en) | 0x70);
	}
	else
	{   
		SET_REG16(&_sdio->regs->int_en, GET_REG16(&_sdio->regs->int_en) &~ 1);
		SET_REG16(&_sdio->regs->err_int_en, GET_REG16(&_sdio->regs->err_int_en) &~ 0xF);
		SET_REG16(&_sdio->regs->int_en, GET_REG16(&_sdio->regs->int_en) &~ 2);
		SET_REG16(&_sdio->regs->err_int_en, GET_REG16(&_sdio->regs->err_int_en) &~ 0x70);
	}
}

void enable_card_interrupt(sdio_controller_t *_sdio, OnOff on_off)
{
	if(on_off == ON)
	{
		SET_REG16(&_sdio->regs->int_en, GET_REG16(&_sdio->regs->int_en) | 0x100);
		SET_REG16(&_sdio->regs->int_sig_en, GET_REG16(&_sdio->regs->int_sig_en) | 0x100);
	}
	else
	{   
		SET_REG16(&_sdio->regs->int_en, GET_REG16(&_sdio->regs->int_en) &~ 0x100);
		SET_REG16(&_sdio->regs->int_sig_en, GET_REG16(&_sdio->regs->int_sig_en) &~ 0x100);
	}
}

error_t sdio_block_reset(sdio_controller_t *_sdio)
{
	int i = 0;

	SET_REG8(&_sdio->regs->sw_reset, SDIO_SWRESET_ALL);
	while(((GET_REG8(&_sdio->regs->sw_reset) & SDIO_SWRESET_ALL) != 0) && i < 100)
	{
		udelay(5000);
		i++;
	}

	if(i == 100)
		return ETIMEDOUT;

	if(SDIO_SWRESET_ALL & 1)
	{
		SET_REG16(&_sdio->regs->clock_control,
				GET_REG16(&_sdio->regs->clock_control) | 1);

		i = 0;
		while(((GET_REG16(&_sdio->regs->clock_control) & 0x2) == 0) && i < 10)
		{
			udelay(5000);
			i++;
		}

		if(i == 10)
			return ETIMEDOUT;

		set_card_interrupt(_sdio, ON);

		return 0;
	}

	udelay(400000);

	return SUCCESS;
}

error_t sdio_reset()
{
	// function-device_reset
	gpio_pin_output(SDIO_GPIO_DEVICE_RESET, 0);
	udelay(12000);
	gpio_pin_output(SDIO_GPIO_DEVICE_RESET, 1);
	udelay(150000);

	return SUCCESS;
}

error_t sdio_set_speed_mode(sdio_controller_t *_sdio, uint32_t _speed_mode)
{
	if(!_speed_mode || _speed_mode > 2)
		return EINVAL;

	switch(_speed_mode)
	{
	case 1: // Normal Speed
		SET_REG8(&_sdio->regs->host_control, GET_REG8(&_sdio->regs->host_control) &~ 4);
		break;
	
	case 2: // High Speed
		SET_REG8(&_sdio->regs->host_control, GET_REG8(&_sdio->regs->host_control) | 4);
		break;
	}

	return SUCCESS;
}

error_t sdio_set_width(sdio_controller_t *_sdio, uint32_t _width)
{
	error_t ret = SUCCESS;
	int int_en = (GET_REG16(&_sdio->regs->int_en) & 0x100)
		&& (GET_REG16(&_sdio->regs->int_sig_en) & 0x100);

	enable_card_interrupt(_sdio, OFF);

	switch(_width)
	{
	case 1:
		SET_REG8(&_sdio->regs->host_control, GET_REG8(&_sdio->regs->host_control) &~ 2);
		break;

	case 4:
		SET_REG8(&_sdio->regs->host_control, GET_REG8(&_sdio->regs->host_control) | 2);
		break;

	default:
		ret = EINVAL;
		break;
	}

	enable_card_interrupt(_sdio, int_en);

	return ret;
}

error_t sdio_set_clock_rate(sdio_controller_t *_sdio, uint32_t _clkrate)
{
	if (_clkrate > SDIO_Max_Frequency)
	{
		bufferPrintf("sdio: clock frequency out of range!\r\n");
		return EINVAL;
	}

	uint32_t sd_en = GET_REG16(&_sdio->regs->clock_control) & 4;
	SET_REG16(&_sdio->regs->clock_control, GET_REG16(&_sdio->regs->clock_control) &~ 4);

	int i = 0;
	while ((SDIO_Base_Frequency / (1 << i)) > _clkrate)
		i++;
    
	int div = 1 << i;
	if (div >= 256)
		div = 256;
	
	SET_REG16(&_sdio->regs->clock_control, GET_REG16(&_sdio->regs->clock_control) & 0xff);
	SET_REG16(&_sdio->regs->clock_control, GET_REG16(&_sdio->regs->clock_control) | 1 | ((div >> 1) << 8));

	if(sd_en)
	{
		i = 0;
		while((GET_REG16(&_sdio->regs->clock_control) & 0x2) == 0 && i < 10)
		{
			udelay(5000);
			i++;
		}

		if(i == 10)
		{ 
			bufferPrintf("sdio: Failed to set clock rate!\r\n");
			return EIO;
		}
	}

	i = 7;
	int timeout = GET_REG16(&_sdio->regs->clock_control) & 0xFF00;
	while(!(timeout & 1))
	{
		i--;
		timeout >>= 1;		

		if(!(i & 0xFF))
			break;
	}

	int state = GET_REG16(&_sdio->regs->err_int_en);

	SET_REG16(&_sdio->regs->err_int_en, GET_REG16(&_sdio->regs->err_int_en) &~ 0x10);

	SET_REG16(&_sdio->regs->timeout_control, timeout & 0xF);

	if (state & 0x10)
		SET_REG16(&_sdio->regs->err_int_en, GET_REG16(&_sdio->regs->err_int_en) | 0x10);

	if (!sd_en)
		SET_REG16(&_sdio->regs->clock_control, GET_REG16(&_sdio->regs->clock_control) &~ 0x4);
	else
		SET_REG16(&_sdio->regs->clock_control, GET_REG16(&_sdio->regs->clock_control) | 0x4);

	return SUCCESS;
}

error_t sdio_set_clock_mode(sdio_controller_t *_sdio, int _clkmode)
{
	if(_clkmode != 1 && _clkmode != -1)
	{
		bufferPrintf("sdio: invalid clock mode setting! \r\n");
		return EINVAL;
	}

	if(_clkmode == -1)
		SET_REG16(&_sdio->regs->clock_control, GET_REG16(&_sdio->regs->clock_control) &~ 0x4);
	else if(_clkmode == 1)
	{
		SET_REG16(&_sdio->regs->clock_control, GET_REG16(&_sdio->regs->clock_control) | 1);

		int i = 0;
		while(((GET_REG16(&_sdio->regs->clock_control) & 0x2) == 0) && i < 10)
		{
			udelay(5000);
			i++;
		}

		if(i == 10)
		{ 
			bufferPrintf("sdio: Clock failed to stabilise!\r\n");
			return EIO;
		}

		SET_REG16(&_sdio->regs->clock_control, GET_REG16(&_sdio->regs->clock_control) | 4);
	}

	return SUCCESS;
}

error_t sdio_set_controller(sdio_controller_t *_sdio, int _clkmode, int _clkrate, uint8_t _buswidth, int _speedmode)
{
	CHAIN_FAIL(sdio_block_reset(_sdio));
	CHAIN_FAIL(sdio_set_speed_mode(_sdio, _speedmode));
	CHAIN_FAIL(sdio_set_width(_sdio, _buswidth));
	CHAIN_FAIL(sdio_set_clock_rate(_sdio, _clkrate));
	CHAIN_FAIL(sdio_set_clock_mode(_sdio, _clkmode));
	return SUCCESS;
}

error_t sdio_wait_for_ready(sdio_controller_t *_sdio)
{
	int i = 0;

	bufferPrintf("%s\n", __func__);

	// Wait for CMDINHIBIT to clear
	while(((GET_REG(&_sdio->regs->prn_status) & 1) != 0) && (i < 20))
	{
		task_sleep(5);
		i++;
	}
	
	if (i == 20)
	{
		bufferPrintf("sdio: %s: Never ready!\n", __func__);
		return ETIMEDOUT;
	}

	bufferPrintf("sdio: %s done.\n", __func__);
	return SUCCESS;
}

error_t sdio_wait_for_cmd_ready(sdio_controller_t *_sdio)
{
	// wait for the command to be ready to transmit
	int i = 0;

	bufferPrintf("%s.\n", __func__);

	while((GET_REG16(&_sdio->regs->int_sts) & 1) == 0
			&& (GET_REG16(&_sdio->regs->err_int_sts) & 0xf)== 0
			&& i < 1000)
	{
		task_sleep(5);
		i++;
	}

	if (i == 1000)
	{
		bufferPrintf("sdio: %s: Never ready!\n", __func__);
		return ETIMEDOUT;
	}

	bufferPrintf("sdio: %s done.\n", __func__);
	return SUCCESS;
}

error_t sdio_command_check_interr(sdio_controller_t *_sdio)
{
	if((GET_REG16(&_sdio->regs->err_int_sts) & 0xF) != 0)
	{
		bufferPrintf("sdio: %s: Failed (0x%x)!\n", __func__,
				GET_REG16(&_sdio->regs->err_int_sts));
		return EIO;
	}
	
	bufferPrintf("sdio: %s OK.\n", __func__);
	return SUCCESS;
}

error_t sdio_send_io(sdio_controller_t *_sdio, uint8_t command, uint32_t ocr, uint32_t* rocr)
{
	uint16_t cmd;

	// clear the upper bits that would indicate card status
	ocr &= 0x1FFFFFF;

	CHAIN_FAIL(sdio_wait_for_ready(_sdio));

	bufferPrintf("%s: 0x%08x, 0x%08x.\n", __func__,
			GET_REG16(&_sdio->regs->int_sts),
			GET_REG16(&_sdio->regs->err_int_sts));

	SET_REG32(&_sdio->regs->argument, ocr);

	//set CMD
	cmd = (command << 8) & 0x3F00;
	if((command & 0xFFFF) == 0x35)
		cmd |= 0x20;

	if(command == 5)
		cmd |= 2;
	else
		cmd |= 0x1A;

	SET_REG16(&_sdio->regs->command, cmd);

	CHAIN_FAIL(sdio_wait_for_cmd_ready(_sdio));
	CHAIN_FAIL(sdio_command_check_interr(_sdio));

	//clear interrupt	
	SET_REG16(&_sdio->regs->int_sts, 1);
	SET_REG16(&_sdio->regs->err_int_sts, 0xF);

	//get response
	rocr[0] = GET_REG(&_sdio->regs->response[0]);
	rocr[1] = GET_REG(&_sdio->regs->response[1]);
	rocr[2] = GET_REG(&_sdio->regs->response[2]);
	rocr[3] = GET_REG(&_sdio->regs->response[3]);
	return SUCCESS;
}


error_t sdio_io_rw_direct(sdio_controller_t *_sdio, int isWrite, int function, uint32_t address, uint8_t in, uint8_t* out)
{
	uint32_t arg = 0;
	uint32_t response[4];

	arg |= isWrite ? 0x80000000 : 0; // write bit
	arg |= isWrite ? in : 0; // write value
	arg |= function << 28;
	arg |= address << 9;

	error_t ret = sdio_send_io(_sdio, 52, arg, response);
	if(FAILED(ret))
	{
		bufferPrintf("sdio: error doing cmd52 read/write!\r\n");
		return ret;
	}

	*out = (uint8_t)response[0];
	return SUCCESS;
}


error_t sdio_read_cis(sdio_controller_t *_sdio, int function)
{
	int i;
	uint8_t data;

	uint32_t ptr = 0;
	for(i = 0; i < 3; ++i)
	{
		CHAIN_FAIL(sdio_io_rw_direct(_sdio, FALSE, 0, (function * 0x100) + 0x9 + i, 0, &data));
		ptr += data << (8 * i);
	}

	for(i = 0; i < 200; ++i)
	{
		CHAIN_FAIL(sdio_io_rw_direct(_sdio, FALSE, 0, ptr + i, 0, &fn0cisBuf[i]));
	}

	return SUCCESS;
}


error_t sdio_parse_fn0_cis(sdio_controller_t *_sdio)
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
			return EIO;

		// end of info
		if(tuple == 0xFF)
			return SUCCESS;

		if(bufferSize < (i + 2))
			return EIO;

		size = fn0cisBuf[i+1];
		if(bufferSize < (size + i + 2))
			return EIO;

		// looking for version or manufacturer info
		if((tuple != 21) && (tuple != 32))
			continue;

		if(tuple == 21)
		{
			if (size <= 6)
			{
				bufferPrintf("sdio: invalid version tuple!! \r\n");
				return EIO;
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
				return EIO;
			}

			bufferPrintf("sdio: device manufacturer ID 0x%4x, product ID 0x%4x \r\n", fn0cisBuf[i+2] | fn0cisBuf[i+3] << 8, fn0cisBuf[i+4] | fn0cisBuf[i+5] << 8);
		}

		i += (size + 2);
	}

	return SUCCESS;
}

error_t sdio_enable_func_interrupt(sdio_controller_t *_sdio, uint8_t function, OnOff on_off)
{
	uint8_t reg;

	if(function > 7)
		return EINVAL;

	CHAIN_FAIL(sdio_io_rw_direct(_sdio, FALSE, 0, 0x4, 0, &reg));

	if(on_off == ON)
		reg |= 1 << function; //enable
	else
		reg &= ~(1 << function); //disable

	CHAIN_FAIL(sdio_io_rw_direct(_sdio, TRUE, 0, 0x4, reg, NULL));

	return SUCCESS;
}

int sdio_enable_function(sdio_controller_t *_sdio, uint8_t function, OnOff on_off)
{
	uint8_t reg;

	if(function <= 0 || function > 7)
		return EINVAL;

	CHAIN_FAIL(sdio_io_rw_direct(_sdio, FALSE, 0, 0x2, 0, &reg));

	if(on_off == ON)
		reg |= 1 << function;
	else
		reg &= ~(1 << function);

	CHAIN_FAIL(sdio_io_rw_direct(_sdio, TRUE, 0, 0x2, reg, NULL));

	if(on_off == ON)
	{
		uint64_t startTime = timer_get_system_microtime();
		while (1) {
			CHAIN_FAIL(sdio_io_rw_direct(_sdio, FALSE, 0, 0x3, 0, &reg));

			if (reg & (1 << function))
				break;

			if(has_elapsed(startTime, 1000))
			{
				bufferPrintf("sdio: timeout enabling functon!!\r\n");
				return ETIMEDOUT;
			
			}
		}
	}

	return SUCCESS;
}

static void sdio_handle_interrupt(uint32_t _tnk)
{
	bufferPrintf("%s\n", __func__);
}

error_t sdio_setup()
{
	int i;
	uint8_t data;
	sdio_controller_t *sdio = &sdio0;
	fn0cisBuf = (uint8_t*)malloc(0x200); 

	SET_REG16(&sdio->regs->int_en, 0);
	SET_REG16(&sdio->regs->int_sig_en, 0);
	SET_REG16(&sdio->regs->int_sts, 0xFFFF);
	SET_REG16(&sdio->regs->err_int_en, 0);
	SET_REG16(&sdio->regs->err_int_sig_en, 0);
	SET_REG16(&sdio->regs->err_int_sts, 0xFFFF);

	bufferPrintf("%s: caps 0x%08x, max_caps 0x%08x.\n",
			__func__, GET_REG(&sdio->regs->capabilities),
			GET_REG(&sdio->regs->max_curr_cap));

	interrupt_install(0x26, sdio_handle_interrupt, 0);
	interrupt_enable(0x26);
	
	clock_gate_switch(SDIO_CLOCKGATE, ON);

	SET_REG8(&sdio->regs->pwr_control, 0xF);
	
	// clockmode=1, clockrate=25MHz, buswidth=4, speedmode=1
	error_t ret = sdio_set_controller(sdio, 1, 20000000, 1, 1);
	if(FAILED(ret))
	{
		bufferPrintf("sdio: failed to configure SDIO block! \r\n");
		return ret;
	}
	
	ret = sdio_reset();
	if(FAILED(ret))
	{
		bufferPrintf("sdio: error resetting card\r\n");
		return ret;
	}
	
	uint8_t abort;
	ret = sdio_io_rw_direct(sdio, 0, 0, 0x6, 0, &abort);
	if(FAILED(ret))
		abort = 0x08;
	else
		abort |= 0x08;

	ret = sdio_io_rw_direct(sdio, 1, 0, 0x6, abort, NULL);

	// Enable SDIO IRQs - may not be here but well, who knows....
	enable_card_interrupt(sdio, ON);
    
	//**Enumerate card slot**//
	uint32_t ocr[4];
	i = 0;
	
	// get initial card operating conditions
	do
	{
		ret = sdio_send_io(sdio, 5, 0, ocr); //io_send_op_cond()
		if(FAILED(ret))
		{
			bufferPrintf("sdio: error querying card operating conditions\r\n");
			return ret;
		}
		
		task_sleep(1);
		i++;
	}
	while((ocr[0] >> 0x1F == 0) && (i < 100));
	
	if (i == 100)
	{
		bufferPrintf("sdio: timeout waiting for CMD5 I/O ready! \r\n");
		return ETIMEDOUT;
	}
	
	bufferPrintf("CMD5 response: OCR: 0x%8x\r\n", ocr[0] & 0xFFFFF);
	bufferPrintf("CMD5 response: Device has memory: %d\r\n", (ocr[0] >> 0x1B) & 0x1);

	if((ocr[0] & 0x70000000) == 0)
		bufferPrintf("CMD5 response: No I/O functions");
	else
		NumberOfFunctions = 1;
	
	ret = sdio_send_io(sdio, 5, 0xFFFF00, ocr);
	if(FAILED(ret)) //io_send_op_cond()
	{
		bufferPrintf("sdio: error querying card operating conditions\r\n");
		return ret;
	}

	bufferPrintf("sdio: Ready! CMD5 response: 0x%8x\r\n", ocr[0] >> 0x1F);
	
	// probe for relative card address
	ret = sdio_send_io(sdio, 3, 0, ocr);
	if(FAILED(ret)) //send_relative_addr()
	{
		bufferPrintf("sdio: probing card relative address failed!\r\n");
		return ret;
	}
	
	// select/deselect card with relative address
	ret = sdio_send_io(sdio, 7, ocr[0] & 0xFFFF0000, ocr);
	if(FAILED(ret)) //select_deselect_card()
	{
		bufferPrintf("sdio: select card with relative address 0x%8x failed!\r\n", ocr[0] & 0xFFFF0000);
		return ret;
	}
	
	bufferPrintf("sdio: CMD7 response [0x%8x][0x%8x][0x%8x][0x%8x]", ocr[0], ocr[1], ocr[2], ocr[3]);
	
	// read card information structure
	ret = sdio_read_cis(sdio, 0);
	if(FAILED(ret))
	{
		bufferPrintf("sdio: fail to read CIS data!! \r\n");
		return ret;
	}
	
	ret = sdio_parse_fn0_cis(sdio);
	if(FAILED(ret))
	{
		bufferPrintf("sdio: error parsing function 0 CIS data!! \r\n");
		return ret;
	}
	
	CHAIN_FAIL(sdio_io_rw_direct(sdio, FALSE, 0, 8, 0, &data));

	bufferPrintf("sdio: card capability 0x%4x\r\n", data);
	
	//** Done enumerate card slot**//
	
	bufferPrintf("sdio: Ready!!\r\n");
	
	return SUCCESS;
}


void sdio_init()
{
	sdio_setup();
}

static void cmd_sdio_setup(int argc, char** argv)
{
	sdio_setup();
	bufferPrintf("sdio setup done\r\n");
}
COMMAND("sdio_setup", "SDIO setup", cmd_sdio_setup);
