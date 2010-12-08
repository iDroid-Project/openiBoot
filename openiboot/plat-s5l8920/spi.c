#include "openiboot.h"
#include "openiboot-asmhelpers.h"
#include "spi.h"
#include "hardware/spi.h"
#include "util.h"
#include "clock.h"
#include "chipid.h"
#include "timer.h"
#include "interrupt.h"

typedef struct _SPIRegisters
{
	uint32_t reg00;
	uint32_t reg04;
	uint32_t reg08;
	uint32_t reg0C;
	uint32_t reg10;
	uint32_t reg14;
	uint32_t reg18;
	uint32_t reg1C;
	uint32_t reg20;
	uint32_t reg24;
	uint32_t reg28;
	uint32_t reg2C;
	uint32_t reg30;
	uint32_t reg34;
	uint32_t gate;
	uint32_t irq;
} SPIRegisters;

typedef struct _SPIStruct
{
	SPIRegisters *registers;
} SPIStruct;

SPIRegisters SPIRegs[] = {
	{ 0x82000000, 0x82000004, 0x82000008, 0x8200000C, 0x82000010, 0x82000020, 0x82000030, 0x82000034, 0x82000038, 0x8200003C, 0x82000040, 0x82000044, 0x82000048, 0x8200004C, 0x9, 0x1D, },
	{ 0x82100000, 0x82100004, 0x82100008, 0x8210000C, 0x82100010, 0x82100020, 0x82100030, 0x82100034, 0x82100038, 0x8210003C, 0x82100040, 0x82100044, 0x82100048, 0x8210004C, 0xA, 0x1C, },
	{ 0x82200000, 0x82200004, 0x82200008, 0x8220000C, 0x82200010, 0x82200020, 0x82200030, 0x82200034, 0x82200038, 0x8220003C, 0x82200040, 0x82200044, 0x82200048, 0x8220004C, 0xB, 0x1B, },
	{ 0x82300000, 0x82300004, 0x82300008, 0x8230000C, 0x82300010, 0x82300020, 0x82300030, 0x82300034, 0x82300038, 0x8230003C, 0x82300040, 0x82300044, 0x82300048, 0x8230004C, 0xC, 0x1A, },
	{ 0x82400000, 0x82400004, 0x82400008, 0x8240000C, 0x82400010, 0x82400020, 0x82400030, 0x82400034, 0x82400038, 0x8240003C, 0x82400040, 0x82400044, 0x82400048, 0x8240004C, 0xD, 0x19, },
};
#define SPICount (sizeof(SPIRegs)/sizeof(SPIRegs[0]))

SPIStruct SPIData[SPICount];

static void spi_enable(SPIStruct *_spi, int _val)
{
	SET_REG(_spi->registers->reg00, _val != 0);
}

static void spi_irq_handler(SPIStruct *_spi)
{
}

void spi_init()
{
	int i;
	for(i = 0; i < SPICount; i++)
	{
		SPIRegisters *regs = &SPIRegs[i];
		SPIStruct *data = &SPIData[i];
		memset(data, 0, sizeof(*data));

		clock_gate_switch(regs->gate, ON);

		data->registers = regs;
		spi_enable(data, 1);

		interrupt_install(regs->irq, (void (*)(uint32_t))spi_irq_handler, (uint32_t)data);
		interrupt_enable(regs->irq);
	}
}

/*
static void spiIRQHandler(uint32_t port);

int spi_setup() {
	clock_gate_switch(SPI0_CLOCKGATE, ON);
	clock_gate_switch(SPI1_CLOCKGATE, ON);
	clock_gate_switch(SPI2_CLOCKGATE, ON);

	memset(spi_info, 0, sizeof(SPIInfo) * NUM_SPIPORTS);

	int i;
	for(i = 0; i < NUM_SPIPORTS; i++) {
		spi_info[i].clockSource = NCLK;
		SET_REG(SPIRegs[i].control, 0);
	}

	interrupt_install(SPI0_IRQ, spiIRQHandler, 0);
	interrupt_install(SPI1_IRQ, spiIRQHandler, 1);
	interrupt_install(SPI2_IRQ, spiIRQHandler, 2);
	interrupt_enable(SPI0_IRQ);
	interrupt_enable(SPI1_IRQ);
	interrupt_enable(SPI2_IRQ);

	return 0;
}

static void spi_txdata(int port, const volatile void *buff, int from, int to)
{
	int i=from>>spi_info[port].wordSize;
	int j=to>>spi_info[port].wordSize;
	switch(spi_info[port].wordSize)
	{
		case 0: // 8-bytes
			{
				uint8_t *buf = (uint8_t*)buff;
				for(;i<j;i++)
				{
					SET_REG(SPIRegs[port].txData, buf[i]);
				}
			}
			break;

		case 1: // 16-bytes
			{
				uint16_t *buf = (uint16_t*)buff;
				for(;i<j;i++)
				{
					SET_REG(SPIRegs[port].txData, buf[i]);
				}
			}
			break;

		case 2: // 32-bytes
			{
				uint32_t *buf = (uint32_t*)buff;
				for(;i<j;i++)
				{
					SET_REG(SPIRegs[port].txData, buf[i]);
				}
			}
			break;
	}
}

static void spi_rxdata(int port, const volatile void *buff, int from, int to)
{
	int i=from>>spi_info[port].wordSize;
	int j=to>>spi_info[port].wordSize;
	switch(spi_info[port].wordSize)
	{
		case 0: // 8-bytes
			{
				uint8_t *buf = (uint8_t*)buff;
				for(;i<j;i++)
				{
					buf[i] = GET_REG(SPIRegs[port].rxData);
				}
			}
			break;

		case 1: // 16-bytes
			{
				uint16_t *buf = (uint16_t*)buff;
				for(;i<j;i++)
				{
					buf[i] = GET_REG(SPIRegs[port].rxData);
				}
			}
			break;

		case 2: // 32-bytes
			{
				uint32_t *buf = (uint32_t*)buff;
				for(;i<j;i++)
				{
					buf[i] = GET_REG(SPIRegs[port].rxData);
				}
			}
			break;
	}
}

int spi_tx(int port, const uint8_t* buffer, int len, int block, int unknown) {
	if(port > (NUM_SPIPORTS - 1)) {
		return -1;
	}

	SET_REG(SPIRegs[port].control, GET_REG(SPIRegs[port].control) | (1 << 2));
	SET_REG(SPIRegs[port].control, GET_REG(SPIRegs[port].control) | (1 << 3));

	spi_info[port].txBuffer = buffer;

	if(len > MAX_TX_BUFFER)
		spi_info[port].txCurrentLen = MAX_TX_BUFFER;
	else
		spi_info[port].txCurrentLen = len;

	spi_info[port].txTotalLen = len;
	spi_info[port].txDone = FALSE;

	if(unknown == 0) {
		SET_REG(SPIRegs[port].cnt, 0);
	}

	spi_txdata(port, buffer, 0, spi_info[port].txCurrentLen);

	SET_REG(SPIRegs[port].control, 1);

	if(block) {
		while(!spi_info[port].txDone || GET_BITS(GET_REG(SPIRegs[port].status), 4, 4) != 0) {
			// yield
		}
		return len;
	} else {
		return 0;
	}
}

int spi_rx(int port, uint8_t* buffer, int len, int block, int noTransmitJunk) {
	if(port > (NUM_SPIPORTS - 1)) {
		return -1;
	}

	SET_REG(SPIRegs[port].control, GET_REG(SPIRegs[port].control) | (1 << 2));
	SET_REG(SPIRegs[port].control, GET_REG(SPIRegs[port].control) | (1 << 3));

	spi_info[port].rxBuffer = buffer;
	spi_info[port].rxDone = FALSE;
	spi_info[port].rxCurrentLen = 0;
	spi_info[port].rxTotalLen = len;
	spi_info[port].counter = 0;

	if(noTransmitJunk == 0) {
		SET_REG(SPIRegs[port].setup, GET_REG(SPIRegs[port].setup) | 1);
	}

	SET_REG(SPIRegs[port].cnt, (len + ((1<<spi_info[port].wordSize)-1)) >> spi_info[port].wordSize);
	SET_REG(SPIRegs[port].control, 1);

	if(block) {
		uint64_t startTime = timer_get_system_microtime();
		while(!spi_info[port].rxDone) {
			// yield
			if(has_elapsed(startTime, 1000)) {
				EnterCriticalSection();
				spi_info[port].rxDone = TRUE;
				spi_info[port].rxBuffer = NULL;
				LeaveCriticalSection();
				if(noTransmitJunk == 0) {
					SET_REG(SPIRegs[port].setup, GET_REG(SPIRegs[port].setup) & ~1);
				}
				return -1;
			}
		}
		if(noTransmitJunk == 0) {
			SET_REG(SPIRegs[port].setup, GET_REG(SPIRegs[port].setup) & ~1);
		}
		return len;
	} else {
		return 0;
	}
}

int spi_txrx(int port, const uint8_t* outBuffer, int outLen, uint8_t* inBuffer, int inLen, int block)
{
	if(port > (NUM_SPIPORTS - 1)) {
		return -1;
	}

	SET_REG(SPIRegs[port].control, GET_REG(SPIRegs[port].control) | (1 << 2));
	SET_REG(SPIRegs[port].control, GET_REG(SPIRegs[port].control) | (1 << 3));

	spi_info[port].txBuffer = outBuffer;

	if(outLen > MAX_TX_BUFFER)
		spi_info[port].txCurrentLen = MAX_TX_BUFFER;
	else
		spi_info[port].txCurrentLen = outLen;

	spi_info[port].txTotalLen = outLen;
	spi_info[port].txDone = FALSE;

	spi_info[port].rxBuffer = inBuffer;
	spi_info[port].rxDone = FALSE;
	spi_info[port].rxCurrentLen = 0;
	spi_info[port].rxTotalLen = inLen;
	spi_info[port].counter = 0;

	spi_txdata(port, outBuffer, 0, spi_info[port].txCurrentLen);

	SET_REG(SPIRegs[port].cnt, (inLen + ((1<<spi_info[port].wordSize)-1)) >> spi_info[port].wordSize);
	SET_REG(SPIRegs[port].control, 1);

	if(block) {
		while(!spi_info[port].txDone || !spi_info[port].rxDone || GET_BITS(GET_REG(SPIRegs[port].status), 4, 4) != 0) {
			// yield
		}
		return inLen;
	} else {
		return 0;
	}
}

void spi_set_baud(int port, int baud, SPIWordSize wordSize, int isMaster, int isActiveLow, int lastClockEdgeMissing) {
	if(port > (NUM_SPIPORTS - 1)) {
		return;
	}

	SET_REG(SPIRegs[port].control, 0);

	switch(wordSize) {
		case SPIWordSize8:
			spi_info[port].wordSize = 0;
			break;

		case SPIWordSize16:
			spi_info[port].wordSize = 1;
			break;

		case SPIWordSize32:
			spi_info[port].wordSize = 2;
			break;
	}

	spi_info[port].isActiveLow = isActiveLow;
	spi_info[port].lastClockEdgeMissing = lastClockEdgeMissing;

	uint32_t clockFrequency;

	if(spi_info[port].clockSource == PCLK) {
		clockFrequency = clock_get_frequency(FrequencyBasePeripheral);
	} else {
		clockFrequency = clock_get_frequency(FrequencyBaseFixed);
	}

	uint32_t divider;

	if(chipid_spi_clocktype() != 0) {
		divider = clockFrequency / baud;
		if(divider < 2)
			divider = 2;
	} else {
		divider = clockFrequency / (baud * 2 - 1);
	}

	if(divider > MAX_DIVIDER) {
		return;
	}
	
	SET_REG(SPIRegs[port].clkDivider, divider);
	spi_info[port].baud = baud;
	spi_info[port].isMaster = isMaster;

	uint32_t options = (lastClockEdgeMissing << 1)
			| (isActiveLow << 2)
			| ((isMaster ? 0x3 : 0) << 3)
			| ((spi_info[port].option5 ? 0x2 : 0x3D) << 5)
			| (spi_info[port].clockSource << CLOCK_SHIFT)
			| spi_info[port].wordSize << 13;

	SET_REG(SPIRegs[port].setup, options);
	SET_REG(SPIRegs[port].pin, 0);
	SET_REG(SPIRegs[port].control, 1);

}

static void spiIRQHandler(uint32_t port) {
	if(port > (NUM_SPIPORTS - 1)) {
		return;
	}

	uint32_t status = GET_REG(SPIRegs[port].status);
	if(status & (1 << 3)) {
		spi_info[port].counter++;
	}

	if(status & (1 << 1)) {
		while(TRUE) {
			// take care of tx
			if(spi_info[port].txBuffer != NULL) {
				if(spi_info[port].txCurrentLen < spi_info[port].txTotalLen)
				{
					int toTX = spi_info[port].txTotalLen - spi_info[port].txCurrentLen;
					int canTX = (MAX_TX_BUFFER - TX_BUFFER_LEFT(status)) << spi_info[port].wordSize;

					if(toTX > canTX)
						toTX = canTX;

					spi_txdata(port, spi_info[port].txBuffer, spi_info[port].txCurrentLen, spi_info[port].txCurrentLen+toTX);
					spi_info[port].txCurrentLen += toTX;

				} else {
					spi_info[port].txDone = TRUE;
					spi_info[port].txBuffer = NULL;
				}
			}

dorx:
			// take care of rx
			if(spi_info[port].rxBuffer == NULL)
				break;

			int toRX = spi_info[port].rxTotalLen - spi_info[port].rxCurrentLen;
			int canRX = GET_BITS(status, 8, 4) << spi_info[port].wordSize;

			if(toRX > canRX)
				toRX = canRX;

			spi_rxdata(port, spi_info[port].rxBuffer, spi_info[port].rxCurrentLen, spi_info[port].rxCurrentLen+toRX);
			spi_info[port].rxCurrentLen += toRX;

			if(spi_info[port].rxCurrentLen < spi_info[port].rxTotalLen)
				break;

			spi_info[port].rxDone = TRUE;
			spi_info[port].rxBuffer = NULL;
		}


	} else  if(status & (1 << 0)) {
		// jump into middle of the loop to handle rx only, stupidly
		goto dorx;
	}

	// acknowledge interrupt handling complete
	SET_REG(SPIRegs[port].status, status);
}

void spi_on_off(uint8_t spi, OnOff on_off) {
        if (on_off == ON) {
                SET_REG(SPIRegs[spi].pin, GET_REG(SPIRegs[spi].pin) | 0x2);
        } else {
                SET_REG(SPIRegs[spi].pin, GET_REG(SPIRegs[spi].pin) & (~0x2));
        }
}

int spi_status(uint8_t spi) {
	return ((GET_REG(SPIRegs[spi].pin) >> 1) & 1);
}
*/
