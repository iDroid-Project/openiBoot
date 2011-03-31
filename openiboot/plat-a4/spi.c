#include "openiboot.h"
#include "arm/arm.h"
#include "spi.h"
#include "hardware/spi.h"
#include "util.h"
#include "clock.h"
#include "chipid.h"
#include "timer.h"
#include "interrupt.h"
#include "tasks.h"

typedef struct _SPIRegisters
{
	uint32_t control;
	uint32_t config;
	uint32_t status;
	uint32_t pin;
	uint32_t txData;
	uint32_t rxData;
	uint32_t clkDiv;
	uint32_t rxCount;
	uint32_t reg20;
	uint32_t reg24;
	uint32_t reg28;
	uint32_t reg2C;
	uint32_t reg30;
	uint32_t txCount;
	uint32_t gate;
	uint32_t irq;
} SPIRegisters;

typedef struct _SPIStruct
{
	SPIRegisters *registers;
	SPIClockSource clockSource;

	uint32_t config;

	int wordSize;

	void *txBuffer;
	uint32_t txLength;
	uint32_t txDone;

	void *rxBuffer;
	uint32_t rxLength;
	uint32_t rxDone;
} SPIStruct;

SPIRegisters SPIRegs[] = {
	{ 0x82000000, 0x82000004, 0x82000008, 0x8200000C, 0x82000010, 0x82000020, 0x82000030, 0x82000034, 0x82000038, 0x8200003C, 0x82000040, 0x82000044, 0x82000048, 0x8200004C, 0x2B, 0x1D, },
	{ 0x82100000, 0x82100004, 0x82100008, 0x8210000C, 0x82100010, 0x82100020, 0x82100030, 0x82100034, 0x82100038, 0x8210003C, 0x82100040, 0x82100044, 0x82100048, 0x8210004C, 0x2C, 0x1E, },
	{ 0x82200000, 0x82200004, 0x82200008, 0x8220000C, 0x82200010, 0x82200020, 0x82200030, 0x82200034, 0x82200038, 0x8220003C, 0x82200040, 0x82200044, 0x82200048, 0x8220004C, 0x2D, 0x1F, },
	{ 0x82300000, 0x82300004, 0x82300008, 0x8230000C, 0x82300010, 0x82300020, 0x82300030, 0x82300034, 0x82300038, 0x8230003C, 0x82300040, 0x82300044, 0x82300048, 0x8230004C, 0x2E, 0x20, },
	{ 0x82400000, 0x82400004, 0x82400008, 0x8240000C, 0x82400010, 0x82400020, 0x82400030, 0x82400034, 0x82400038, 0x8240003C, 0x82400040, 0x82400044, 0x82400048, 0x8240004C, 0x2F, 0x21, },
};
#define SPICount (sizeof(SPIRegs)/sizeof(SPIRegs[0]))

SPIStruct SPIData[SPICount];

void spi_set_baud(int _bus, int _baud, SPIWordSize _wordSize, int _isMaster, int cpol, int cpha)
{
	if(_bus < 0 || _bus >= SPICount)
		return;

	uint32_t wordSize;
	switch(_wordSize)
	{
	case 8:
		wordSize = 0;
		break;

	case 16:
		wordSize = 1;
		break;

	case 32:
		wordSize = 2;
		break;

	default:
		return;
	}

	SPIStruct *spi = &SPIData[_bus];
	spi->wordSize = wordSize;
	SET_REG(spi->registers->control, 0);

	spi->config = (_isMaster)? 0x18 : 0;
	uint32_t freq = clock_get_frequency(spi->clockSource == NCLK? FrequencyBaseFixed: FrequencyBasePeripheral);
	uint32_t div = 1;

	if(_baud > 0)
		div = freq/_baud;

	spi->config |= (cpha << 1)
			| (cpol << 2)
			| (wordSize << SPISETUP_WORDSIZE_SHIFT)
			| (0x20); // or 0x40, (but this never arises in iBoot).

	if(spi->clockSource == NCLK)
		spi->config |= SPISETUP_CLOCKSOURCE;

	//bufferPrintf("spi: set_baud(%d) isMaster=%d, config=0x%08x, div=%d.\n", _bus, _isMaster, spi->config, div);

	SET_REG(spi->registers->clkDiv, div);
	SET_REG(spi->registers->config, spi->config);
	SET_REG(spi->registers->pin, 2);
	SET_REG(spi->registers->control, 1);
}

static void spi_txdata(SPIStruct *_spi, const volatile void *buff, int from, int to)
{
	int i = from >> _spi->wordSize;
	int j = to >> _spi->wordSize;
	switch(_spi->wordSize)
	{
		case 0: // 8-bytes
			{
				uint8_t *buf = (uint8_t*)buff;
				for(;i<j;i++)
				{
					SET_REG(_spi->registers->txData, buf[i]);
				}
			}
			break;

		case 1: // 16-bytes
			{
				uint16_t *buf = (uint16_t*)buff;
				for(;i<j;i++)
				{
					SET_REG(_spi->registers->txData, buf[i]);
				}
			}
			break;

		case 2: // 32-bytes
			{
				uint32_t *buf = (uint32_t*)buff;
				for(;i<j;i++)
				{
					SET_REG(_spi->registers->txData, buf[i]);
				}
			}
			break;
	}
}

static void spi_rxdata(SPIStruct *_spi, const volatile void *buff, int from, int to)
{
	int i= from >> _spi->wordSize;
	int j= to >> _spi->wordSize;
	switch(_spi->wordSize)
	{
		case 0: // 8-bytes
			{
				uint8_t *buf = (uint8_t*)buff;
				for(;i<j;i++)
				{
					buf[i] = GET_REG(_spi->registers->rxData);
				}
			}
			break;

		case 1: // 16-bytes
			{
				uint16_t *buf = (uint16_t*)buff;
				for(;i<j;i++)
				{
					buf[i] = GET_REG(_spi->registers->rxData);
				}
			}
			break;

		case 2: // 32-bytes
			{
				uint32_t *buf = (uint32_t*)buff;
				for(;i<j;i++)
				{
					buf[i] = GET_REG(_spi->registers->rxData);
				}
			}
			break;
	}
}

int spi_rx(int port, uint8_t* buffer, int len, int block, int noTransmitJunk)
{
	if(port >= SPICount || port < 0)
		return -1;

	SPIStruct *spi = &SPIData[port];

	SET_REG(spi->registers->control, GET_REG(spi->registers->control) | (1 << 2));
	SET_REG(spi->registers->control, GET_REG(spi->registers->control) | (1 << 3));

	// Set len to number of words.
	len = (len + ((1<<spi->wordSize)-1)) >> spi->wordSize;

	spi->txBuffer = NULL;
	spi->rxBuffer = buffer;
	spi->rxDone = 0;
	spi->rxLength = len;

	if(noTransmitJunk == 0)
		spi->config |= 1;
	else
	{
		spi->config |= 0x200000;
		SET_REG(spi->registers->txCount, len);
	}

	//bufferPrintf("spi_rx(%d, %p, %d, %d, %d)\n", port, buffer, len, block, noTransmitJunk);

	SET_REG(spi->registers->rxCount, len);

	spi->config |= 0x180;
	SET_REG(spi->registers->config, spi->config);
	
	SET_REG(spi->registers->control, 1);

	if(block)
	{
		while(spi->rxBuffer != NULL || spi->txBuffer != NULL)
			task_yield();

		return len;
	}
	else
		return 0;
}

int spi_tx(int port, const uint8_t* buffer, int len, int block, int unknown)
{
	if(port >= SPICount || port < 0)
		return -1;

	SPIStruct *spi = &SPIData[port];

	SET_REG(spi->registers->control, GET_REG(spi->registers->control) | (1 << 2));
	SET_REG(spi->registers->control, GET_REG(spi->registers->control) | (1 << 3));

	// Set len to number of words.
	len = (len + ((1<<spi->wordSize)-1)) >> spi->wordSize;

	spi->txBuffer = (void*)buffer;
	spi->rxBuffer = NULL;
	spi->txLength = len;

	if(len > MAX_TX_BUFFER)
		spi->txDone = MAX_TX_BUFFER;
	else
		spi->txDone = len;

	if(unknown == 0) {
		SET_REG(spi->registers->rxCount, 0);
	}
	else
	{
		spi->config |= 0x80;
	}

	//bufferPrintf("spi_tx(%d, %p, %d, %d, %d)\n", port, buffer, len, block, unknown);
	
	spi_txdata(spi, buffer, 0, spi->txDone);
	
	spi->config |= 0x200100;
	SET_REG(spi->registers->txCount, len);
	SET_REG(spi->registers->config, spi->config);
	SET_REG(spi->registers->control, 1);

	if(block)
	{
		while(spi->txBuffer != NULL || TX_BUFFER_LEFT(GET_REG(spi->registers->status)) > 0)
			task_yield();

		return len;
	}
	else
		return 0;
}

static void spi_irq_handler(uint32_t i)
{
	SPIStruct *spi = &SPIData[i];
	uint32_t status = GET_REG(spi->registers->status);

	if((status & 1) && !(status & 0x400002))
		goto do_rx;
	else if(!(status & 0x400002))
		goto complete;

	while(TRUE)
	{
		if(spi->txBuffer != NULL)
		{
			uint32_t avail = MAX_TX_BUFFER - TX_BUFFER_LEFT(status);
			uint32_t left = spi->txLength - spi->txDone;
			if(avail > left)
				avail = left;

			spi_txdata(spi, spi->txBuffer, spi->txDone, spi->txDone + avail);

			spi->txDone += avail;
			if(spi->txDone >= spi->txLength)
			{
				spi->txBuffer = NULL;
				spi->txLength = 0;
				spi->txDone = 0;

				spi->config &=~ 0x200000;
				SET_REG(spi->registers->config, spi->config);
			}
		}

	do_rx:
		if(spi->rxBuffer != NULL)
		{
			uint32_t avail = RX_BUFFER_LEFT(status);
			uint32_t left = spi->rxLength - spi->rxDone;
			if(avail > left)
				avail = left;

			if(avail > 0)
			{
				spi_rxdata(spi, spi->rxBuffer, spi->rxDone, spi->rxDone + avail);
				spi->rxDone += avail;
			}

			spi->rxDone += avail;

			if(spi->rxDone >= spi->rxLength)
			{
				spi->rxBuffer = NULL;
				spi->rxLength = 0;
				spi->rxDone = 0;

				spi->config &=~ 0x81;
				SET_REG(spi->registers->config, spi->config);
			}
		}

	complete:
		if(spi->txBuffer == NULL && spi->rxBuffer == NULL)
		{
			spi->config &=~ 0x200181;
			SET_REG(spi->registers->config, spi->config);
			break;
		}
		else
			break;
	}

	SET_REG(spi->registers->status, status);
}

int spi_setup()
{
	int i;
	for(i = 0; i < SPICount; i++)
	{
		// Only enable SPIs 1-3.
		if(((0x7 >> i) & 1) == 0)
			continue;

		SPIRegisters *regs = &SPIRegs[i];
		SPIStruct *data = &SPIData[i];
		memset(data, 0, sizeof(*data));

		data->registers = regs;
		data->clockSource = NCLK;
		
		clock_gate_switch(regs->gate, ON);
		SET_REG(data->registers->control, 0);

		interrupt_install(regs->irq, spi_irq_handler, i);
		interrupt_enable(regs->irq);
	}

	return 0;
}

