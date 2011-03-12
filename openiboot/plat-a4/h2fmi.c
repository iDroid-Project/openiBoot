#include "h2fmi.h"
#include "hardware/h2fmi.h"
#include "timer.h"
#include "tasks.h"
#include "clock.h"
#include "interrupt.h"
#include "util.h"
#include "arm.h"
#include "openiboot-asmhelpers.h"
#include "commands.h"

typedef struct _nand_chipid
{
	uint32_t chipID;
	uint32_t unk1;
} __attribute__((__packed__)) nand_chipid_t;

typedef struct _nand_smth_struct
{
	uint8_t some_array[8];
	uint32_t symmetric_masks[];
} __attribute__((__packed__)) nand_smth_struct_t;

typedef struct _nand_chip_info
{
	nand_chipid_t chipID;
	uint16_t blocks_per_ce;
	uint16_t pages_per_block;
	uint16_t bytes_per_page;
	uint16_t bytes_per_spare;
	uint16_t unk5;
	uint16_t unk6;
	uint32_t unk7;
	uint16_t banks_per_ce;
	uint16_t unk9;
} __attribute__((__packed__)) nand_chip_info_t;

typedef struct _nand_board_id
{
	uint32_t num_busses;
	uint32_t num_symmetric;
	nand_chipid_t chipID;
	uint8_t unk3;
	nand_chipid_t chipID2;
	uint8_t unk4;
} __attribute__((__packed__)) nand_board_id_t;

typedef struct _nand_board_info
{
	nand_board_id_t board_id;
	uint16_t unk1;
	uint16_t unk2;
} __attribute__((__packed__)) nand_board_info_t;

typedef struct _nand_timing_info
{
	nand_board_id_t board_id;
	uint8_t unk1;
	uint8_t unk2;
	uint8_t unk3;
	uint8_t unk4;
	uint8_t unk5;
	uint8_t unk6;
	uint8_t unk7;
	uint8_t unk8;
} __attribute__((__packed__)) nand_timing_info_t;

typedef struct _nand_info
{
	nand_chip_info_t *chip_info;
	nand_board_info_t *board_info;
	nand_timing_info_t *timing_info;

	nand_smth_struct_t *some_mask;
	uint32_t *some_array;
} nand_info_t;

static nand_chip_info_t nand_chip_info[] = {
	{ { 0x7294D7EC, 0x0, }, 0x1038, 0x80, 0x2000, 0x1B4, 0xC, 0, 8, 1, 0 },
	{ { 0x72D5DEEC, 0x0, }, 0x2070, 0x80, 0x2000, 0x1B4, 0xC, 0, 8, 2, 0 },
	{ { 0x29D5D7EC, 0x0, }, 0x2000, 0x80, 0x1000, 0xDA, 0x8, 0, 2, 2, 0 },
	{ { 0x2594D7AD, 0x0, }, 0x2000, 0x80, 0x1000, 0xE0, 0xC, 0, 3, 1, 0 },
	{ { 0x3294E798, 0x0, }, 0x1004, 0x80, 0x2000, 0x1C0, 0x10, 0, 1, 1, 0 },
	{ { 0x3294D798, 0x0, }, 0x1034, 0x80, 0x2000, 0x178, 0x8, 0, 1, 1, 0 },
	{ { 0x3295DE98, 0x0, }, 0x2068, 0x80, 0x2000, 0x178, 0x8, 0, 1, 2, 0 },
	{ { 0x3295EE98, 0x0, }, 0x2008, 0x80, 0x2000, 0x1C0, 0x18, 0, 1, 2, 0 },
	{ { 0x4604682C, 0x0, }, 0x1000, 0x100, 0x1000, 0xE0, 0xC, 0, 7, 1, 0 },
	{ { 0xC605882C, 0x0, }, 0x2000, 0x100, 0x1000, 0xE0, 0xE0, 0, 7, 2, 0 },
	{ { 0x3295DE45, 0x0, }, 0x2000, 0x80, 0x2000, 0x178, 0x8, 0, 9, 2, 0 },
	{ { 0x32944845, 0x0, }, 0x1000, 0x80, 0x2000, 0x1C0, 0x8, 0, 9, 1, 0 },
	{ { 0x32956845, 0x0, }, 0x2000, 0x80, 0x2000, 0x1C0, 0x8, 0, 9, 2, 0 },
};

static nand_board_info_t nand_board_info[] = {
	{ { 2, 1, { 0x4604682C, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x14, 0x12, },
	{ { 2, 1, { 0xC605882C, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x14, 0x12, },
	{ { 2, 1, { 0x3295DE98, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x11, 0x15, },
	{ { 2, 1, { 0x3294D798, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x11, 0x15, },
	{ { 2, 1, { 0x3294E798, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x11, 0x15, },
	{ { 2, 1, { 0x3295EE98, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x11, 0x15, },
	{ { 2, 1, { 0x3295EE98, 0x0 }, 8, { 0x0, 0x0, }, 0, }, 0x11, 0x15, },
	{ { 2, 1, { 0x2594D7AD, 0x0 }, 2, { 0x0, 0x0, }, 0, }, 0x14, 0x10, },
	{ { 2, 1, { 0x2594D7AD, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x14, 0x10, },
	{ { 2, 1, { 0x29D5D7EC, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x14, 0x10, },
	{ { 2, 1, { 0x7294D7EC, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x14, 0x10, },
	{ { 2, 1, { 0x7294D7EC, 0x0 }, 2, { 0x0, 0x0, }, 0, }, 0x14, 0x10, },
	{ { 2, 1, { 0x72D5DEEC, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x14, 0x10, },
	{ { 2, 1, { 0x72D5DEEC, 0x0 }, 8, { 0x0, 0x0, }, 0, }, 0x14, 0x10, },
	{ { 2, 1, { 0x32944845, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x11, 0x15, },
	{ { 2, 1, { 0x32956845, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x11, 0x15, },
	{ { 2, 1, { 0x3295DE45, 0x0 }, 4, { 0x0, 0x0, }, 0, }, 0x11, 0x15, },
};

static nand_timing_info_t nand_timing_info[] = {
	{ { 2, 1, { 0x7294D7EC, 0x0, }, 2, { 0x0, 0x0, }, 0, }, 0x1E, 0xF, 0xA, 0x1E, 0xF, 0xA, 0x19, 0xF, },
	{ { 2, 1, { 0x7294D7EC, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x1E, 0xF, 0xA, 0x1E, 0xF, 0xA, 0x19, 0xF, },
	{ { 2, 1, { 0x72D5DEEC, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x1E, 0xF, 0xA, 0x1E, 0xF, 0xA, 0x14, 0xF, },
	{ { 2, 1, { 0x72D5DEEC, 0x0, }, 8, { 0x0, 0x0, }, 0, }, 0x1E, 0xF, 0xA, 0x1E, 0xF, 0xA, 0x19, 0xF, },
	{ { 2, 1, { 0x29D5D7EC, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x1E, 0xF, 0xA, 0x1E, 0xF, 0xA, 0x14, 0xF, },
	{ { 2, 1, { 0x2594D7AD, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0xF, },
	{ { 2, 1, { 0x2594D7AD, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0xF, },
	{ { 2, 1, { 0x2594D7AD, 0x0, }, 2, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0xF, },
	{ { 2, 1, { 0x3294D798, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0x19, },
	{ { 2, 1, { 0x3295DE98, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0x19, },
	{ { 2, 1, { 0x3294E798, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0x19, },
	{ { 2, 1, { 0x3295EE98, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0x19, },
	{ { 2, 1, { 0x3295EE98, 0x0, }, 8, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0x19, },
	{ { 2, 1, { 0x4604682C, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0xF, },
	{ { 2, 1, { 0xC605882C, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x14, 0xA, 0x7, 0x14, 0xA, 0x7, 0x10, 0xF, },
	{ { 2, 1, { 0x3295DE45, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0x1E, },
	{ { 2, 1, { 0x32944845, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0x19, },
	{ { 2, 1, { 0x32956845, 0x0, }, 4, { 0x0, 0x0, }, 0, }, 0x19, 0xC, 0xA, 0x19, 0xC, 0xA, 0x14, 0x19, },
};

typedef struct _h2fmi_timing_setup
{
	uint64_t freq;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t f;
	uint32_t g;
	uint32_t h;
	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t l;
	uint32_t m;
	uint32_t n;
	uint32_t o;
	uint32_t p;
	uint32_t q;
	uint32_t r;
} h2fmi_timing_setup_t;

typedef struct _h2fmi_dma_state
{
	uint32_t signalled;
	uint32_t b;
	LinkedList list;
} h2fmi_dma_state_t;

typedef struct _h2fmi_dma_task
{
	TaskDescriptor *task;
	LinkedList list;
} h2fmi_dma_task_t;

static h2fmi_dma_state_t h2fmi_dma_state[28];
static int h2fmi_dma_state_initialized = 0;

static h2fmi_struct_t fmi0 = {
	.bus_num = 0,
	.base_address = H2FMI0_BASE,
	.clock_gate = H2FMI0_CLOCK_GATE,
	.interrupt = H2FMI0_INTERRUPT,
	.dma0 = 5,
	.dma1 = 6,
	.currentmode = 0,
};

static h2fmi_struct_t fmi1 = {
	.bus_num = 1,
	.base_address = H2FMI1_BASE,
	.clock_gate = H2FMI1_CLOCK_GATE,
	.interrupt = H2FMI1_INTERRUPT,
	.dma0 = 7,
	.dma1 = 8,
	.currentmode = 0,
};

static h2fmi_struct_t *h2fmi_busses[] = {
	&fmi0,
	&fmi1,
};

#define H2FMI_BUS_COUNT (array_size(h2fmi_busses))

static h2fmi_geometry_t h2fmi_geometry;

typedef struct _h2fmi_map_entry
{
	uint32_t bus;
	uint16_t chip;
} h2fmi_map_entry_t;

static h2fmi_map_entry_t h2fmi_map[H2FMI_CHIP_COUNT];

static uint32_t h2fmi_hash_table[1024];
static uint8_t *h2fmi_wmr_data = NULL;

static uint32_t h2fmi_ftl_count = 0;
static uint32_t h2fmi_ftl_databuf = 0;
static uint32_t h2fmi_ftl_smth[2] = {0, 0};
static uint32_t h2fmi_data_whitening_enabled = 0;

static int compare_board_ids(nand_board_id_t *a, nand_board_id_t *b)
{
	return memcmp(a, b, sizeof(nand_board_id_t)) == 0;
}

static int count_bits(uint32_t _val)
{
	int ret = 0;

	while(_val > 0)
	{
		if(_val & 1)
			ret++;
		
		_val >>= 1;
	}

	return ret;
}

void sub_5FF174A0(h2fmi_state_t *_state) {
	bufferPrintf("Hello.\r\n");
	return;
}

static void h2fmi_hw_reg_int_init(h2fmi_struct_t *_fmi);
void h2fmi_irq_handler_0(h2fmi_struct_t *_fmi) {
	bufferPrintf("h2fmi_irq_handler_0: This is also doing more. But I'm too lazy. Just return.\n");
	return;
	_fmi->fmi_state = GET_REG(H2FMI_UNKREG14(_fmi));
	h2fmi_hw_reg_int_init(_fmi);
	sub_5FF174A0(&_fmi->state);
}

void h2fmi_irq_handler_1(h2fmi_struct_t *_fmi) {
	system_panic("h2fmi_irq_handler_1: This is doing more. Needs to be investigated.\n");
}

void h2fmi_irq_handler_2(h2fmi_struct_t *_fmi) {
	system_panic("h2fmi_irq_handler_2: The panic only function. Weird!\n");
}

typedef void (*H2FMIInterruptServiceRoutine)(h2fmi_struct_t *_fmi);
H2FMIInterruptServiceRoutine h2fmi_irq_handler[3] = {
	h2fmi_irq_handler_0,
	h2fmi_irq_handler_1,
	h2fmi_irq_handler_2,
};

static void h2fmiIRQHandler(uint32_t token) {
	h2fmi_struct_t *_fmi = (h2fmi_struct_t*)token;
	if (_fmi->currentmode > 2)
		system_panic("h2fmiIRQHandler: omgwtfbbq!\n");

	h2fmi_irq_handler[_fmi->currentmode](_fmi);
}

void sub_5FF17160(uint32_t *_struct, int a2, int a3)
{
	*(_struct+3) = (uint32_t)_struct + (sizeof(uint32_t) * 2);
	*(_struct+2) = (uint32_t)_struct + (sizeof(uint32_t) * 2);
	*(_struct) = a3;
	*(_struct+1) = a2;
}

static int h2fmi_wait_for_done(h2fmi_struct_t *_fmi, uint32_t _reg, uint32_t _mask, uint32_t _val)
{
	uint64_t startTime = timer_get_system_microtime();
	while((GET_REG(_reg) & _mask) != _val)
	{
		task_yield();

		if(has_elapsed(startTime, 10000))
		{
			bufferPrintf("h2fmi: timeout on 0x%08x failed.\n", _reg);
			SET_REG(_reg &~ 0x3, _val);
			return -1;
		}
	}

	return 0;
}

static int h2fmi_wait_dma_task_pending(h2fmi_struct_t *_fmi)
{
	uint64_t startTime = timer_get_system_microtime();
	while(!(GET_REG(H2FMI_UNKREG16(_fmi)) & 0x18))
	{
		task_yield();

		if(has_elapsed(startTime, 10000))
		{
			bufferPrintf("h2fmi: timeout on DMA failed.\n");
			return -1;
		}
	}

	return 0;
}

static int h2fmi_pio_read_sector(h2fmi_struct_t *_fmi, void *_dest, int _amt)
{
	if (h2fmi_wait_dma_task_pending(_fmi))
		return -1;

	uint32_t *dest = _dest;
	_amt = (_amt+sizeof(uint32_t)-1)/sizeof(uint32_t);

	int i;
	for (i = 0; i < _amt; i++) {
		dest[i] = GET_REG(H2FMI_UNKREG17(_fmi));
	}

	return 0;
}

static void h2fmi_device_reset(h2fmi_struct_t *_fmi)
{
	clock_gate_reset(_fmi->clock_gate);

	SET_REG(H2FMI_UNK4(_fmi), 6);
	SET_REG(H2FMI_UNKREG2(_fmi), 1);
	SET_REG(H2FMI_UNKREG1(_fmi), _fmi->timing_register_cache_408);
}

static void h2fmi_enable_chip(h2fmi_struct_t *_fmi, uint8_t _chip)
{
	h2fmi_struct_t *chipFMI = (_chip & 0x8) ? &fmi1: &fmi0;
	if(_fmi->bus_num == 0 && (_fmi->bitmap & 0xFF00))
	{
		h2fmi_struct_t *fmi = (_fmi == chipFMI) ? &fmi1: _fmi;
		SET_REG(H2FMI_CHIP_MASK(fmi), 0);
	}

	uint32_t reg = H2FMI_CHIP_MASK(chipFMI);
	SET_REG(reg, GET_REG(reg) | (1 << (_chip & 0x7)));
}

static void h2fmi_disable_chip(uint8_t _chip)
{
	h2fmi_struct_t *fmi = (_chip & 0x8) ? &fmi1: &fmi0;
	uint32_t maskReg = H2FMI_CHIP_MASK(fmi);
	SET_REG(maskReg, GET_REG(maskReg) &~ (1 << (_chip & 0x7)));
}

static void h2fmi_disable_bus(h2fmi_struct_t *_fmi)
{
	SET_REG(H2FMI_CHIP_MASK(_fmi), 0);
}

static int h2fmi_init_bus(h2fmi_struct_t *_fmi)
{
	clock_gate_switch(_fmi->clock_gate, ON);
	clock_gate_switch(_fmi->clock_gate+1, ON);

	_fmi->timing_register_cache_408 = 0xFFFF;
	SET_REG(H2FMI_UNKREG1(_fmi), _fmi->timing_register_cache_408);

	_fmi->field_1A0 = 0xFF;

	h2fmi_device_reset(_fmi);
//	nand_device_set_interrupt(_fmi);

	return 0;
}

static int h2fmi_nand_reset(h2fmi_struct_t *_fmi, uint8_t _chip)
{
	h2fmi_enable_chip(_fmi, _chip);

	SET_REG(H2FMI_UNKREG4(_fmi), 0xFF);
	SET_REG(H2FMI_UNKREG5(_fmi), 1);

	int ret = h2fmi_wait_for_done(_fmi, H2FMI_UNKREG6(_fmi), 1, 1);
	h2fmi_disable_chip(_chip);

	return ret;
}

static int h2fmi_nand_reset_all(h2fmi_struct_t *_fmi)
{
	uint8_t i;
	for(i = 0; i < H2FMI_CHIP_COUNT; i++)
	{
		int ret = h2fmi_nand_reset(_fmi, i);
		if(ret != 0)
			return ret;
	}

	task_sleep(50);

	return 0;
}

static int h2fmi_read_chipid(h2fmi_struct_t *_fmi, uint8_t _chip, void *_buffer, uint8_t _unk)
{
	h2fmi_enable_chip(_fmi, _chip);

	SET_REG(H2FMI_UNKREG4(_fmi), 0x90);
	SET_REG(H2FMI_UNKREG9(_fmi), _unk);
	SET_REG(H2FMI_UNKREG10(_fmi), 0);
	SET_REG(H2FMI_UNKREG5(_fmi), 9);

	int ret = h2fmi_wait_for_done(_fmi, H2FMI_UNKREG6(_fmi), 9, 9);
	if (!ret) {
		SET_REG(H2FMI_UNKREG15(_fmi), 0x801);
		SET_REG(H2FMI_UNK4(_fmi), 3);
		ret = h2fmi_wait_for_done(_fmi, H2FMI_UNKREG14(_fmi), 2, 2);
		if (!ret) {
			h2fmi_pio_read_sector(_fmi, _buffer, H2FMI_CHIPID_LENGTH);
		}
		h2fmi_init_bus(_fmi);
	}

	h2fmi_disable_chip(_chip);
	return ret;
}

static int h2fmi_check_chipid(h2fmi_struct_t *_fmi, char *_id, char *_base, int _unk)
{
	char *ptr = _id;

	_fmi->num_chips = 0;
	_fmi->bitmap = 0;

	uint8_t i;
	for(i = 0; i < H2FMI_CHIP_COUNT; i++)
	{
		if(!memcmp(ptr, _base, 6))
		{
			bufferPrintf("fmi: Found chip ID %2x %2x %2x %2x %2x %2x on fmi%d:ce%d.\n",
					ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5],
					_fmi->bus_num, i);

			_fmi->bitmap |= (1 << i);
			_fmi->num_chips++;
		}
		else if(memcmp(ptr, "\xff\xff\xff\xff\xff\xff", 6) && memcmp(ptr, "\0\0\0\0\0\0", 6))
			bufferPrintf("fmi: Ignoring mismatched chip with ID %2x %2x %2x %2x %2x %2x on fmi%d:ce%d.\n",
					ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5],
					_fmi->bus_num, i);

		ptr += H2FMI_CHIPID_LENGTH;
	}

	return 0;
}

static int h2fmi_reset_and_read_chipids(h2fmi_struct_t *_fmi, void *_buffer, uint8_t _unk)
{
	char *buffer = _buffer;

	_fmi->num_chips = 0;
	_fmi->bitmap = 0;

	int ret = h2fmi_nand_reset_all(_fmi);
	if(ret != 0)
		return ret;

	uint8_t i;
	for(i = 0; i < H2FMI_CHIP_COUNT; i++)
	{
		ret = h2fmi_read_chipid(_fmi, i, buffer, _unk);
		if(ret != 0)
			return ret;

		buffer += H2FMI_CHIPID_LENGTH;
	}

	return ret;
}

static nand_info_t *h2fmi_nand_find_info(char *_id, h2fmi_struct_t **_busses, int _count)
{
	static nand_smth_struct_t nand_smth = { { 0, 0, 0, 3, 4, 3, 4, 0 }, { 0xF0F, 0, 0 } };
	static uint32_t nand_some_array[] = { 0xC, 0xA, 0 };

	uint8_t chipID[H2FMI_CHIPID_LENGTH];
	uint32_t bitmap = 0;
	int found = 0, i;
	nand_board_id_t board_id;
	memset(&board_id, 0, sizeof(board_id));

	board_id.num_busses = _count;

	for(i = 0; i < H2FMI_CHIP_COUNT; i++)
	{
		if(_busses[i] != NULL)
		{
			if(found > 0)
			{
				if(memcmp(chipID, &_id[i*H2FMI_CHIPID_LENGTH], H2FMI_CHIPID_LENGTH) != 0)
				{
					uint8_t *a = (uint8_t*)&board_id.chipID;
					uint8_t *b = (uint8_t*)&_id[i*H2FMI_CHIPID_LENGTH];

					bufferPrintf("fmi: ChipIDs do not match.\n");
					bufferPrintf("fmi: %02x %02x %02x %02x %02x %02x\n", a[0], a[1], a[2], a[3], a[4], a[5]);
					bufferPrintf("fmi: %02x %02x %02x %02x %02x %02x\n", b[0], b[1], b[2], b[3], b[4], b[5]);
					return NULL;
				}
			}
			else
				memcpy(chipID, &_id[i*H2FMI_CHIPID_LENGTH], H2FMI_CHIPID_LENGTH);

			found++;
			bitmap |= (1 << i);
		}
	}

	i = 0;
	uint32_t mask = nand_smth.symmetric_masks[i];
	uint32_t bit_count = count_bits(bitmap & mask);
	while(mask)
	{
		uint32_t bits = bitmap & mask;
		if(bits)
		{
			if(count_bits(bits) != bit_count)
			{
				bufferPrintf("fmi: Chip IDs not symmetric.\n");
				return NULL;
			}

			board_id.num_symmetric++;
		}

		i++;
		mask = nand_smth.symmetric_masks[i];
	}

	nand_info_t *info = malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->some_array = nand_some_array;
	info->some_mask = &nand_smth;

	for(i = 0; i < array_size(nand_chip_info); i++)
	{
		nand_chip_info_t *ci = &nand_chip_info[i];

		if(memcmp(chipID, &ci->chipID, sizeof(uint32_t)) == 0)
			info->chip_info = ci;
	}

	if(!info->chip_info)
	{
		bufferPrintf("fmi: Unsupported chip.\n");
		return NULL;
	}

	nand_chipid_t *chipIDPtr = &board_id.chipID;
	uint8_t *unkPtr = &board_id.unk3;
	for(i = 0; i < board_id.num_symmetric; i++)
	{
		memcpy(chipIDPtr, chipID, sizeof(uint32_t));

		*unkPtr = found / board_id.num_symmetric;

		chipIDPtr = (nand_chipid_t*)(((uint8_t*)chipIDPtr) + sizeof(*chipIDPtr) + sizeof(*unkPtr));
		unkPtr += sizeof(*chipIDPtr) + sizeof(*unkPtr);
	}

	bufferPrintf("fmi: NAND board ID: (%d, %d, 0x%x, 0x%x, %d, 0x%x, 0x%x, %d).\n",
			board_id.num_busses, board_id.num_symmetric, board_id.chipID.chipID, board_id.chipID.unk1,
			board_id.unk3, board_id.chipID2.chipID, board_id.chipID2.unk1, board_id.unk4);

	for(i = 0; i < array_size(nand_board_info); i++)
	{
		nand_board_info_t *bi = &nand_board_info[i];
		if(compare_board_ids(&board_id, &bi->board_id))
			info->board_info = bi;
	}

	if(!info->board_info)
	{
		bufferPrintf("fmi: No support for board.\n");
		return NULL;
	}

	for(i = 0; i < array_size(nand_timing_info); i++)
	{
		nand_timing_info_t *ti = &nand_timing_info[i];
		if(compare_board_ids(&board_id, &ti->board_id))
			info->timing_info = ti;
	}

	if(!info->timing_info)
	{
		bufferPrintf("fmi: Failed to find timing info for board.\n");
		return NULL;
	}

	return info;
}

static void h2fmi_init_dma_event(h2fmi_dma_state_t *_dma, uint32_t _b, uint32_t _signalled)
{
	_dma->signalled = _signalled;
	_dma->b = _b;
	_dma->list.prev = &_dma->list;
	_dma->list.next = &_dma->list;
}

static void h2fmi_handle_dma(h2fmi_dma_state_t *_dma)
{
	EnterCriticalSection();

	_dma->signalled = 1;

	LinkedList *list = _dma->list.next;
	while(list != &_dma->list)
	{
		h2fmi_dma_task_t *dma_task = container_of(h2fmi_dma_task_t, list, list);
		task_wake(dma_task->task);
		free(dma_task);
	}

	_dma->list.next = &_dma->list;
	_dma->list.prev = &_dma->list;

	LeaveCriticalSection();
}

static void h2fmi_dma_handler(int _channel)
{
	h2fmi_handle_dma(&h2fmi_dma_state[_channel]);
}

static void h2fmi_dma_execute_async(uint32_t _dir, uint32_t _channel, uint8_t *_ptr,
		uint32_t _reg, uint32_t _size, uint32_t _wordSize, uint32_t _blockSize, dmaAES *_aes)
{
	if(!h2fmi_dma_state_initialized)
	{
		h2fmi_dma_state_initialized = 1;

		int i;
		for(i = 0; i < array_size(h2fmi_dma_state); i++)
			h2fmi_init_dma_event(&h2fmi_dma_state[i], 1, 0);
	}
	
	if(h2fmi_dma_state[_channel].signalled)
	{
		bufferPrintf("fmi: Tried to start DMA transaction on busy channel %d.\r\n",
					_channel);
		return;
	}

	dma_set_aes(_channel, _aes);
	uint32_t dma_err = dma_init_channel(_dir, _channel, (uint32_t)_ptr, _reg, _size,
			_wordSize, _blockSize, h2fmi_dma_handler);

	if(dma_err != 0)
	{
		bufferPrintf("fmi: Failed to setup DMA transfer, failed with code 0x%08x.\r\n", dma_err);
	}
}

static int h2fmi_dma_wait(uint32_t _channel, uint32_t _timeout)
{
	h2fmi_dma_state_t *dma = &h2fmi_dma_state[_channel];

	uint32_t ret = 0;

	bufferPrintf("fmi: dma_wait %d for %d.\r\n", _channel, _timeout);

	if(dma->signalled)
		return 0;

	EnterCriticalSection();

	h2fmi_dma_task_t *task = malloc(sizeof(task));
	task->task = CurrentRunning;
	task->list.next = &dma->list;
	task->list.prev = dma->list.prev;

	((LinkedList*)dma->list.prev)->next = &task->list;
	dma->list.prev = &task->list;

	if(_timeout)
		ret = (task_wait_timeout(_timeout) == 1)? 0: 1;
	else
		task_wait();

	LeaveCriticalSection();
	return ret;
}

static void h2fmi_dma_cancel(uint32_t _channel)
{
	dma_cancel(_channel);
	h2fmi_init_dma_event(&h2fmi_dma_state[_channel], 1, 0);
}

static void h2fmi_store_810(h2fmi_struct_t *_fmi)
{
	SET_REG(H2FMI_UNK810(_fmi), 0x1A8);
}

static void h2fmi_set_address_inner(h2fmi_struct_t *_fmi, uint32_t _addr)
{
	SET_REG(H2FMI_UNK41C(_fmi), (_addr >> 16) & 0xFF);
	SET_REG(H2FMI_UNKREG9(_fmi), ((_addr & 0xFF) << 16) || ((_addr >> 8) << 24));
	SET_REG(H2FMI_UNKREG10(_fmi), 4);
}

static void h2fmi_set_address(h2fmi_struct_t *_fmi, uint32_t _addr)
{
	bufferPrintf("fmi: Aligning to page %d!\r\n", _addr);

	h2fmi_set_address_inner(_fmi, _addr);

	SET_REG(H2FMI_UNKREG4(_fmi), 0x3000);
	SET_REG(H2FMI_UNKREG5(_fmi), 0xB);

	while((GET_REG(H2FMI_UNKREG6(_fmi)) & 0xB) != 0xB);
	SET_REG(H2FMI_UNKREG6(_fmi), 0xB);
}

static void h2fmi_enable_and_set_address(h2fmi_struct_t *_fmi, uint16_t _bank,
		uint16_t *_chips, uint32_t *_addrs)
{
	h2fmi_enable_chip(_fmi, _chips[_bank]);
	h2fmi_set_address(_fmi, _addrs[_bank]);

	_fmi->field_80 = _chips[_bank];
	_fmi->field_15C++;
}

static void h2fmi_store_80c_810(h2fmi_struct_t *_fmi)
{
	SET_REG(H2FMI_UNK80C(_fmi), 1);
	h2fmi_store_810(_fmi);
}

static void h2fmi_hw_reg_int_init(h2fmi_struct_t *_fmi)
{
	SET_REG(H2FMI_UNK440(_fmi), 0);
	SET_REG(H2FMI_UNK10(_fmi), 0);
	SET_REG(H2FMI_UNKREG6(_fmi), 0x31FFFF);
	SET_REG(H2FMI_UNKREG14(_fmi), 0xF);
}

void nand_device_set_interrupt(h2fmi_struct_t *_fmi)
{
	sub_5FF17160((uint32_t*)_fmi + 0x6C, 1, 0);
	_fmi->fmi_state = 0;
	h2fmi_hw_reg_int_init(_fmi);
	interrupt_set_int_type(_fmi->interrupt, 0);
	interrupt_install(_fmi->interrupt, h2fmiIRQHandler, (uint32_t)_fmi);
	interrupt_enable(_fmi->interrupt);
	sub_5FF17160((uint32_t*)_fmi + 0x16C, 1, 0);
	SET_REG(H2FMI_UNKREG3(_fmi), 0);
}

static uint32_t h2fmi_read_complete_handler(h2fmi_struct_t *_fmi)
{
	h2fmi_hw_reg_int_init(_fmi);
	h2fmi_disable_bus(_fmi);
	return 0;
}

static void h2fmi_set_ecc_bits(h2fmi_struct_t *_fmi, uint32_t _a)
{
	SET_REG(H2FMI_PAGE_SIZE(_fmi), _fmi->page_size);
	SET_REG(H2FMI_UNKREGUNK40(_fmi), _fmi->unk40);
	SET_REG(H2FMI_ECC_BITS(_fmi), ((_a & 0x1F) << 8) | 0x20000);
}

static uint32_t h2fmi_function_1(h2fmi_struct_t *_fmi,
		uint32_t _val, uint8_t *_var, uint8_t *_a, uint32_t _b)
{
	if(_val & 0x40)
	{
		if(_a)
			memset(_a, 0xFE, _b);

		return 2;
	}

	uint32_t ret = (_val & 8)? 0x80000024: 1;
	if(!_var)
		return ret;

	uint32_t counter;
	uint8_t *ptr = _a;
	for(counter = 0; counter != _b; counter++)
	{
		uint32_t reg = GET_REG(H2FMI_UNK80C(_fmi));
		uint8_t c = (reg >> 16) & 0x1F;
		if(c > *_var)
			*_var = c;

		if(_a)
		{
			if(reg & 1)
				*ptr = 0xFF;
			else
				*ptr = c;
		}

		ptr++;
	}

	return ret;
}

static void h2fmi_some_mysterious_function(h2fmi_struct_t *_fmi, uint32_t _val)
{
	uint8_t var_10 = 0;
	uint32_t ret = h2fmi_function_1(_fmi, _val, &var_10,
			_fmi->field_158, _fmi->bbt_format);
	
	if(ret == 0x80000024)
		_fmi->field_154++;
	else if(ret == 0x80000025)
		_fmi->field_150++;
	else if(ret == 2)
		_fmi->field_14C++;
	else
	{
		if(ret != 0x80000024 && ret != 0x80000025)
			goto skipBlock;
	}

	if(_fmi->field_170 == 0xFFFFFFFF)
		_fmi->field_170 = _fmi->chips[_fmi->current_page_index-1];

skipBlock:

	if(_fmi->field_148)
		_fmi->field_148[_fmi->current_page_index-1] = var_10;

	if(_fmi->field_158)
		_fmi->field_158 += _fmi->bbt_format;
}

static uint32_t h2fmi_function_2(h2fmi_struct_t *_fmi)
{
	if(GET_REG(H2FMI_UNKC(_fmi)) & 0x100)
	{
		return 2;
	}
	else
	{
		uint8_t val = 0;
		if(timer_get_system_microtime() - _fmi->field_124 > _fmi->field_12C)
			val = 1;

		return val ^ 1;
	}
}

static void h2fmi_inner_function(h2fmi_struct_t *_fmi, uint8_t _a, uint8_t _b)
{
	SET_REG(H2FMI_UNKREG1(_fmi), GET_REG(H2FMI_UNKREG1(_fmi)) &~ 0x100000);
	SET_REG(H2FMI_UNK44C(_fmi), _a | (_b << 8));

	uint32_t val;
	if(_fmi->state.state == H2FMI_STATE_WRITE)
	{
		if(_fmi->field_100 == 4)
		{
			if(_fmi->current_page_index & 1)
				val = 0xF2;
			else
				val = 0xF1;
		}
		else if(_fmi->field_100 == 5)
		{
			if(_fmi->current_page_index & 2)
				val = 0xF2;
			else
				val = 0xF1;
		}
		else if(_fmi->field_100 == 2)
		{
			val = 0x71;
		}
		else
			val = 0x70;
	}
	else
		val = 0x70;

	SET_REG(H2FMI_UNKREG4(_fmi), val);
	SET_REG(H2FMI_UNKREG5(_fmi), 1);
	while(!(GET_REG(H2FMI_UNKREG6(_fmi)) & 1));
	SET_REG(H2FMI_UNKREG6(_fmi), 1);

	h2fmi_hw_reg_int_init(_fmi);

	SET_REG(H2FMI_UNKREG8(_fmi), 0);
	SET_REG(H2FMI_UNKREG5(_fmi), 0x50);
}

static void h2fmi_another_function(h2fmi_struct_t *_fmi)
{
	h2fmi_inner_function(_fmi, 0x40, 0x40);

	SET_REG(H2FMI_UNK440(_fmi), 0x20);
	SET_REG(H2FMI_UNK10(_fmi), 0x100);
}

static void h2fmi_rw_large_page(h2fmi_struct_t *_fmi)
{
	uint32_t dir = (_fmi->state.state == H2FMI_STATE_READ)? 2: 1;

	bufferPrintf("fmi: rw_large_page.\r\n");

	h2fmi_dma_execute_async(dir, _fmi->dma0, _fmi->data_ptr[_fmi->current_page_index],
			H2FMI_UNK14(_fmi), _fmi->bytes_per_page * _fmi->num_pages_to_read,
			4, 8, _fmi->aes_info);

	h2fmi_dma_execute_async(dir, _fmi->dma1, _fmi->wmr_ptr[_fmi->current_page_index],
			H2FMI_UNK18(_fmi), _fmi->num_pages_to_read * _fmi->ecc_bytes,
			1, 1, NULL);

	bufferPrintf("fmi: rw_large_page done!\r\n");
}

static uint32_t h2fmi_read_state_2_handler(h2fmi_struct_t *_fmi)
{
	bufferPrintf("fmi: read_state_2_handler.\r\n");

	uint32_t val = h2fmi_function_2(_fmi);
	if(val == 0)
	{
		SET_REG(H2FMI_UNKREG5(_fmi), 0);
		h2fmi_disable_bus(_fmi);
		_fmi->field_13C = 0x8000001D;
		_fmi->failure_details.overall_status = 0x8000001D;
		_fmi->state.read_state = H2FMI_READ_DONE;
		h2fmi_read_complete_handler(_fmi);
	}
	else if(val != 1)
	{
		uint32_t reg = GET_REG(H2FMI_UNK810(_fmi));
		SET_REG(H2FMI_UNK810(_fmi), reg);
		h2fmi_hw_reg_int_init(_fmi);

		SET_REG(H2FMI_UNKREG4(_fmi), 0);
		SET_REG(H2FMI_UNKREG5(_fmi), 1);
		while(!(GET_REG(H2FMI_UNKREG6(_fmi)) & 1));
		SET_REG(H2FMI_UNKREG6(_fmi), 1);

		_fmi->state.read_state = H2FMI_READ_3;
		SET_REG(H2FMI_UNK10(_fmi), 2);
		SET_REG(H2FMI_UNK4(_fmi), 3);

		if(_fmi->current_page_index == 0)
		{
			h2fmi_rw_large_page(_fmi);
			_fmi->banks_per_ce_vfl = timer_get_system_microtime();
		}
		else
		{
			_fmi->field_124 = timer_get_system_microtime();
			h2fmi_some_mysterious_function(_fmi, reg);
		}
	}

	bufferPrintf("fmi: read_state_2_handler done!\r\n");
	return 0;
}

static uint32_t h2fmi_read_state_4_handler(h2fmi_struct_t *_fmi)
{
	bufferPrintf("fmi: read_state_4_handler.\r\n");

	if(GET_REG(H2FMI_UNK8(_fmi)) & 4)
	{
		if(timer_get_system_microtime() - _fmi->field_124 > _fmi->field_134)
		{
			_fmi->field_13C = 0;
			_fmi->failure_details.overall_status = 0x8000001F;
		}
		else
			return 0;
	}
	else
	{
		if(_fmi->current_page_index >= _fmi->num_pages_to_read)
		{
			bufferPrintf("ZZZ\r\n");
			uint32_t val = GET_REG(H2FMI_UNK810(_fmi));
			SET_REG(H2FMI_UNK810(_fmi), val);
			h2fmi_some_mysterious_function(_fmi, val);
		}
		else
		{
			bufferPrintf("ZZ2\r\n");
			h2fmi_another_function(_fmi);
			_fmi->field_124 = timer_get_system_microtime();
			_fmi->state.read_state = H2FMI_READ_2;
			return h2fmi_read_state_2_handler(_fmi);
		}
	}

	_fmi->state.read_state = H2FMI_READ_DONE;
	return h2fmi_read_complete_handler(_fmi);
}

static uint32_t h2fmi_read_state_1_handler(h2fmi_struct_t *_fmi)
{
	uint32_t r5;

	bufferPrintf("fmi: read_state_1_handler.\r\n");

	if(_fmi->field_140 == 0)
		r5 = (_fmi->current_page_index >= _fmi->num_pages_to_read)? 0: 1;
	else
	{
		_fmi->field_140 = 0;
		r5 = 0;

		h2fmi_enable_and_set_address(_fmi, _fmi->current_page_index, _fmi->chips, _fmi->pages);
	}	

	if(_fmi->current_page_index < _fmi->num_pages_to_read)
	{
		if(_fmi->chips[_fmi->current_page_index + 1] == _fmi->current_chip)
		{
			_fmi->field_140 = 1;
		}
		else
		{
			_fmi->field_140 = 0;
			h2fmi_enable_and_set_address(_fmi, _fmi->current_page_index, _fmi->chips, _fmi->pages);

			r5 = 1;
		}

		_fmi->current_chip = _fmi->chips[_fmi->current_page_index + 1];
	}

	if(r5)
	{
		h2fmi_enable_chip(_fmi, _fmi->chips[_fmi->current_page_index]);
		_fmi->field_80 = _fmi->chips[_fmi->current_page_index];
	}

	SET_REG(H2FMI_UNK10(_fmi), 0x100000);
	_fmi->state.read_state = H2FMI_READ_4;
	_fmi->field_124 = timer_get_system_microtime();

	return h2fmi_read_state_4_handler(_fmi);
}

static uint32_t h2fmi_read_state_3_handler(h2fmi_struct_t *_fmi)
{
	bufferPrintf("fmi: read_state_3_handler.\r\n");

	if((GET_REG(H2FMI_UNKC(_fmi)) & 2) == 0)
	{
		if(timer_get_system_microtime() - _fmi->field_124 > _fmi->field_12C)
		{
			_fmi->field_13C = 0;
			_fmi->failure_details.overall_status = 0x8000001C;
			_fmi->state.read_state = H2FMI_READ_DONE;
			h2fmi_read_complete_handler(_fmi);
		}
	}
	else
	{
		SET_REG(H2FMI_UNK10(_fmi), 0);
		_fmi->current_page_index++;
		_fmi->state.read_state = H2FMI_READ_1;
	}

	return 0;
}

static uint32_t h2fmi_some_read_timing_thing(h2fmi_struct_t *_fmi)
{
	_fmi->field_15C = 0;
	_fmi->field_170 = 0xFFFFFFFF;
	_fmi->failure_details.overall_status = 0;

	_fmi->field_12C = (clock_get_frequency(FrequencyBaseTimebase) / 1000000) * 2000000;
	_fmi->field_134 = _fmi->field_12C / 4;

	if(_fmi->state.state == H2FMI_STATE_READ)
		h2fmi_set_ecc_bits(_fmi, 0xF);
	else
	{
		h2fmi_set_ecc_bits(_fmi, _fmi->ecc_bits+1);
		h2fmi_store_810(_fmi);
	}

	return 0;
}

static uint32_t h2fmi_read_idle_handler(h2fmi_struct_t *_fmi)
{
	_fmi->current_page_index = 0;
	_fmi->field_13C = 1;
	_fmi->field_140 = 1;

	_fmi->current_chip = *_fmi->chips;

	bufferPrintf("fmi: read_idle_handler.\r\n");

	h2fmi_some_read_timing_thing(_fmi);

	_fmi->state.read_state = H2FMI_READ_1;

	return h2fmi_read_state_1_handler(_fmi);
}

static uint32_t h2fmi_read_state_machine(h2fmi_struct_t *_fmi)
{
	if(_fmi->state.state != H2FMI_STATE_READ)
	{
		bufferPrintf("fmi: read_state_machine called when not reading!\r\n");
		return -1;
	}

	uint32_t state = _fmi->state.read_state;
	bufferPrintf("fmi: read_state_machine %d.\r\n", state);

	static uint32_t (*state_functions[])(h2fmi_struct_t *) =
	{
		h2fmi_read_idle_handler,
		h2fmi_read_state_1_handler,
		h2fmi_read_state_2_handler,
		h2fmi_read_state_3_handler,
		h2fmi_read_state_4_handler,
		h2fmi_read_complete_handler,
	};

	if(_fmi->state.read_state >= array_size(state_functions))
	{
		bufferPrintf("fmi: invalid read state!\r\n");
		_fmi->state.read_state = H2FMI_READ_DONE;
		return -1;
	}

	uint32_t ret = state_functions[_fmi->state.read_state](_fmi);
	bufferPrintf("fmi: state machine %d done!\r\n", state);
	return ret;
}

int h2fmi_read_multi(h2fmi_struct_t *_fmi, uint16_t _num_pages, uint16_t *_chips, uint32_t *_pages,
		uint8_t **_ptr, uint8_t **_wmr_ptr, uint8_t *_6, uint8_t *_7)
{
	EnterCriticalSection();

	_fmi->chips = _chips;
	_fmi->pages = _pages;
	_fmi->data_ptr = _ptr;
	_fmi->num_pages_to_read = _num_pages;
	_fmi->wmr_ptr = _wmr_ptr;
	_fmi->field_148 = _6;
	_fmi->field_158 = _7;

	_fmi->field_14C = 0;
	_fmi->field_154 = 0;
	_fmi->field_150 = 0;

	h2fmi_device_reset(_fmi);

	_fmi->state.state = H2FMI_STATE_READ;
	_fmi->state.read_state = H2FMI_READ_IDLE;

	LeaveCriticalSection();
	
	bufferPrintf("fmi: read_multi.\r\n");

	h2fmi_store_80c_810(_fmi);

	while(_fmi->state.read_state != H2FMI_READ_DONE)
	{
		EnterCriticalSection();
		h2fmi_read_state_machine(_fmi);
		LeaveCriticalSection();

		task_yield();
	}

	bufferPrintf("fmi: state machine done.\r\n");

	if(_fmi->field_13C != 0)
	{
		if(h2fmi_dma_wait(_fmi->dma0, 2000000) != 0
				|| h2fmi_dma_wait(_fmi->dma1, 2000000) != 0)
		{
			bufferPrintf("h2fmi: dma wait failed.\r\n");
			return 1;
		}
	
		_fmi->failure_details.overall_status = 0;
	}
	
	bufferPrintf("fmi: Time to cancel the DMA!\r\n");
	
	h2fmi_dma_cancel(_fmi->dma0);
	h2fmi_dma_cancel(_fmi->dma1);

	_fmi->state.state = H2FMI_STATE_IDLE;

	if(_fmi->failure_details.overall_status != 0)
	{
		bufferPrintf("h2fmi: overall_status is failure.\r\n");
		return _fmi->failure_details.overall_status;
	}
	else
	{
		uint32_t a = _fmi->field_150;
		uint32_t b = _fmi->field_14C;

		bufferPrintf("fmi: Some error thing. 0x%08x 0x%08x.\r\n", a, b);

		if(b != 0)
		{
			_fmi->failure_details.overall_status = b > _num_pages? 2 : 0x80000023;
		}
		else
		{
			if(a != 0)
			{
				_fmi->failure_details.overall_status  = a > _num_pages? 0x80000025: 0x80000024;
			}
			else if(_fmi->field_154 != 0)
			{
				_fmi->failure_details.overall_status = 0x80000024;
			}
		}
	}

	h2fmi_hw_reg_int_init(_fmi);

	bufferPrintf("fmi: read_multi done 0x%08x.\r\n", _fmi->failure_details.overall_status);

	return _fmi->failure_details.overall_status;
}

uint32_t h2fmi_read_single(h2fmi_struct_t *_fmi, uint16_t _chip, uint32_t _page, uint8_t *_data, uint8_t *_wmr, uint8_t *_6, uint8_t *_7)
{
	bufferPrintf("fmi: read_single.\r\n");
	return h2fmi_read_multi(_fmi, 1, &_chip, &_page, &_data, &_wmr, _6, _7);
}

static void h2fmi_aes_handler_1(uint32_t dataBuffer, uint32_t dmaAES_setting2, uint32_t* unknAESSetting1)
{
	//bufferPrintf("fmi: aes_handler_1.\r\n");

	uint32_t val = ((dataBuffer - h2fmi_ftl_databuf) / (h2fmi_geometry.bbt_format << 9)) + h2fmi_ftl_smth[0];
	uint32_t i;
	for(i = 0; i < 4; i++)
	{
		if(val & 1)
			val = (val >> 1) ^ 0x80000061;
		else
			val = (val >> 1);

		unknAESSetting1[i] = val;
	}
}

static uint32_t h2fmi_aes_key_1[] = {
	0x95AE5DF6,
	0x426C900E,
	0x58CC54B2,
	0xCEEE78FC,
};

static void h2fmi_aes_handler_2(uint32_t dataBuffer, uint32_t dmaAES_setting2, uint32_t* unknAESSetting1)
{
	//bufferPrintf("fmi: aes_handler_2.\r\n");
	//bufferPrintf("fmi: aes_handler_2 0x%08x, 0x%08x, 0x%08x.\r\n", dataBuffer, dmaAES_setting2, unknAESSetting1);

	h2fmi_struct_t *fmi = (h2fmi_struct_t*)dataBuffer;
	if(fmi->some_aes_field)
	{
		memcpy(unknAESSetting1, ((char*)&fmi->some_aes_field) + (16*dmaAES_setting2), 16);
	}
	else
	{
		uint32_t val = fmi->pages[dmaAES_setting2];
		uint32_t i;
		for(i = 0; i < 4; i++)
		{
			if(val & 1)
				val = (val >> 1) ^ 0x80000061;
			else
				val = (val >> 1);

			unknAESSetting1[i] = val;
		}
	}
}

static uint32_t h2fmi_aes_key_2[] = {
	0xAB42A792,
	0xBF69C908,
	0x12946C00,
	0xA579CCD3,
};

static uint32_t h2fmi_aes_counter = 1;

static void h2fmi_setup_aes(h2fmi_struct_t *_fmi, uint32_t _enabled, uint32_t a2, uint32_t a3)
{
	if(_enabled)
	{
		uint32_t type;
		if(a3 < (_fmi->bytes_per_page * h2fmi_ftl_count) + h2fmi_ftl_databuf)
			type = 1;
		else
			type = 0;

		if(h2fmi_ftl_databuf > a3)
			type = 0;

		if(type == 0)
		{
			_fmi->aes_struct.dataBuffer = a3;
			_fmi->aes_struct.dataSize = _fmi->bytes_per_page;
			_fmi->aes_struct.handler = h2fmi_aes_handler_1;
			_fmi->aes_struct.key = h2fmi_aes_key_1;
			_fmi->aes_struct.unkn0 = (a2 == 0)? 0: 1;
			_fmi->aes_struct.AESType = 0;
		}
		else
		{
			_fmi->aes_struct.dataBuffer = (uint32_t)_fmi;
			_fmi->aes_struct.dataSize = _fmi->bytes_per_page;
			_fmi->aes_struct.handler = h2fmi_aes_handler_2;
			_fmi->aes_struct.key = h2fmi_aes_key_2;
			_fmi->aes_struct.unkn0 = (1 - a2 > 1)? 0: (1 - a2);
			_fmi->aes_struct.AESType = 0;
		}

		_fmi->aes_info = &_fmi->aes_struct;
	}
	else
		_fmi->aes_info = NULL;

}

uint32_t h2fmi_read_single_page(uint32_t _ce, uint32_t _page, uint8_t *_ptr, uint8_t *_meta_ptr, uint8_t *_6, uint8_t *_7, uint32_t _8)
{
	uint32_t bus = h2fmi_map[_ce].bus;
	uint32_t chip = h2fmi_map[_ce].chip;

	h2fmi_struct_t *fmi = h2fmi_busses[bus];

	uint32_t var_28 = fmi->meta_per_logical_page - fmi->ecc_bytes;

	if(_meta_ptr)
		_meta_ptr[0] = 0;

	DataCacheOperation(3, (uint32_t)_ptr, round_up(fmi->bytes_per_page, 64));
	DataCacheOperation(3, (uint32_t)h2fmi_wmr_data, round_up(fmi->meta_per_logical_page, 64));

	uint32_t flag = (1 - _8);
	if(flag > 1)
		flag = 0;

	if(h2fmi_aes_counter == 0)
		flag = 0;
//	else
//		flag++;

	h2fmi_setup_aes(fmi, flag, 0, (uint32_t)_ptr);

	uint32_t read_ret = h2fmi_read_single(fmi, chip, _page, _ptr, h2fmi_wmr_data, _6, _7);

	if(_meta_ptr)
		memcpy(_meta_ptr, h2fmi_wmr_data, fmi->meta_per_logical_page);

	if(h2fmi_data_whitening_enabled)
	{
		uint32_t i;
		for(i = 0; i < 4; i++)
			((uint32_t*)_ptr)[i] ^= h2fmi_hash_table[i + _page];
	}

	uint32_t ret = 0;
	if(read_ret == 0x80000023)
		ret = 0x80000002;
	else if(read_ret == 0x80000024)
	{
		bufferPrintf("fmi: UECC ce %d page 0x%08x.\r\n", _ce, _page);
		ret = 0x80000002;
	}
	else if(read_ret == 0)
	{
		if(_meta_ptr)
		{
			uint32_t i;
			for(i = 1; i < var_28; i++)
				_meta_ptr[fmi->ecc_bytes + i] = 0xFF;
		}

		ret = 0;
	}
	else if(read_ret == 2)
		ret = 1;
	else
	{
		bufferPrintf("fmi: read_single_page hardware error 0x%08x.\r\n", read_ret);
	}

	DataCacheOperation(3, (uint32_t)_ptr, round_up(fmi->bytes_per_page, 64));
	DataCacheOperation(3, (uint32_t)h2fmi_wmr_data, round_up(fmi->meta_per_logical_page, 64));

	return ret;
}

uint8_t h2fmi_calculate_ecc_bits(h2fmi_struct_t *_fmi)
{
	uint32_t val = (_fmi->bytes_per_spare - _fmi->ecc_bytes) / (_fmi->bytes_per_page >> 10);
	static uint8_t some_array[] = { 0x35, 0x1E, 0x33, 0x1D, 0x2C, 0x19, 0x1C, 0x10, 0x1B, 0xF };

	int i;
	for(i = 0; i < sizeof(some_array)/sizeof(uint8_t); i += 2)
	{
		if(val >= some_array[i])
			return some_array[i+1];
	}

	bufferPrintf("h2fmi: calculating ecc bits failed (0x%08x, 0x%08x, 0x%08x) -> 0x%08x.\r\n",
			_fmi->bytes_per_spare, _fmi->ecc_bytes, _fmi->bytes_per_page, val);
	return 0;
}

static int64_t s64_rem(int64_t _a, int64_t _b)
{
	return _a - ((_a/_b)*_b);
}

static int64_t some_math_fn(uint8_t _a, uint8_t _b)
{
	uint32_t b = ((s64_rem(_b, _a) & 0xFF)? 1 : 0) + (_b/_a);

	if(b == 0)
		return 0;

	return b - 1;
}

static uint32_t h2fmi_setup_timing(h2fmi_timing_setup_t *_timing, uint8_t *_buffer)
{
	int64_t smth = (1000000000L / ((int64_t)_timing->freq / 1000)) / 1000;
	uint8_t r1 = (uint8_t)(_timing->q + _timing->g);

	uint32_t var_28 = some_math_fn(smth, _timing->q + _timing->g);

	uint32_t var_2C;
	if(_timing->p <= var_28 * smth)
		var_2C = 0;
	else
		var_2C = _timing->p - (var_28 * smth);

	_buffer[0] = some_math_fn(smth, _timing->k + _timing->g);

	uint32_t some_time = (_buffer[0] + 1) * smth;

	uint32_t r3 = _timing->m + _timing->h + _timing->g;
	r1 = MAX(_timing->j, r3);
	
	uint32_t lr = (some_time < r1)? r1 - some_time: 0;
	uint32_t r4 = (some_time < r3)? r1 - some_time: 0;

	_buffer[1] = some_math_fn(smth, MAX(_timing->l + _timing->f, lr));
	_buffer[2] = (((smth + r4 - 1) / smth) * smth) / smth;
	_buffer[3] = var_28;
	_buffer[4] = some_math_fn(smth, MAX(_timing->f + _timing->r, var_2C));

	return 0;
}

static void h2fmi_init_virtual_physical_map()
{
	h2fmi_wmr_data = malloc(0x400);
	memset(h2fmi_map, 0xFF, sizeof(h2fmi_map));

	uint32_t count[H2FMI_BUS_COUNT];
	memset(count, 0, sizeof(count));

	uint16_t total = 0;
	uint32_t bus;
	for(bus = 0; bus < H2FMI_BUS_COUNT; bus++)
		total += h2fmi_busses[bus]->num_chips;

	uint32_t chip = 0;
	for(chip = 0; chip < total;)
	{
		for(bus = 0; bus < H2FMI_BUS_COUNT; bus++)
		{
			h2fmi_struct_t *fmi = h2fmi_busses[bus];
			if(fmi->bitmap & (1 << count[bus]))
			{
				h2fmi_map_entry_t *e = &h2fmi_map[chip];
				e->bus = bus;
				e->chip = count[bus];

				fmi->field_1A2 = bus;

				chip++;
			}

			count[bus]++;
		}
	}
}

int isPPN = 0;
void h2fmi_init()
{
	memset(h2fmi_dma_state, 0, sizeof(h2fmi_dma_state));
	h2fmi_init_bus(&fmi0);
	h2fmi_init_bus(&fmi1);

	char *buff1 = malloc(H2FMI_CHIPID_LENGTH*H2FMI_CHIP_COUNT*2);
	char *buff2 = buff1 + (H2FMI_CHIPID_LENGTH*H2FMI_CHIP_COUNT);
	h2fmi_reset_and_read_chipids(&fmi0, buff1, 0);
	h2fmi_reset_and_read_chipids(&fmi1, buff2, 0);

	if (!strcmp(buff1, "PPN\x01")) {
		bufferPrintf("fmi: PPN Device Detected\n");
		isPPN = 1;
		return; // We don't know no PPN.
	}

	h2fmi_check_chipid(&fmi0, buff1, buff1, 0);
	h2fmi_check_chipid(&fmi1, buff2, buff1, 0);

	h2fmi_struct_t *busses[H2FMI_CHIP_COUNT];
	memset(busses, 0, sizeof(busses));

	uint32_t i;
	for(i = 0; i < H2FMI_CHIP_COUNT; i++)
	{
		if(fmi1.bitmap & (1 << i))
		{
			memcpy(buff1 + (i*H2FMI_CHIPID_LENGTH), 
					buff2 + (i*H2FMI_CHIPID_LENGTH),
					H2FMI_CHIPID_LENGTH);
			busses[i] = &fmi1;
		}
		else if(fmi0.bitmap & (1 << i))
			busses[i] = &fmi0;
	}

	int bus_count = (fmi0.num_chips > 0) + (fmi1.num_chips > 0);
	uint32_t chip_count = fmi0.num_chips + fmi1.num_chips;
	nand_info_t *info = h2fmi_nand_find_info(buff1, busses, bus_count);
	if (!info)
		return;

	for(i = 0; i < sizeof(h2fmi_busses)/sizeof(h2fmi_struct_t*); i++)
	{
		h2fmi_struct_t *fmi = h2fmi_busses[i];
		if(fmi)
		{
			fmi->is_ppn = 0;
			fmi->blocks_per_ce = info->chip_info->blocks_per_ce;
			fmi->banks_per_ce_vfl = 1;
			fmi->bbt_format = info->chip_info->bytes_per_page >> 10;
			fmi->pages_per_block = info->chip_info->pages_per_block;
			fmi->bytes_per_spare = info->chip_info->bytes_per_spare;
			fmi->ecc_bytes = info->some_array[1];			// valid-meta-per-logical-page
			fmi->meta_per_logical_page = info->some_array[0];	// meta-per-logical-page
										// Third some array entry: logical-page-size
			fmi->bytes_per_page = info->chip_info->bytes_per_page;
			fmi->banks_per_ce = info->chip_info->banks_per_ce;

			uint8_t ecc_bits = h2fmi_calculate_ecc_bits(fmi);
			fmi->ecc_bits = ecc_bits;

			if(ecc_bits > 8)
			{
				int8_t z = (((uint64_t)ecc_bits << 3) * 0x66666667) >> 34;
				fmi->ecc_tag = z;
			}
			else
				fmi->ecc_tag = 8;

			fmi->page_size = ((fmi->ecc_bits & 0x1F) << 3) | 5;
			fmi->unk40 = fmi->bbt_format | 0x40000 | ((fmi->ecc_bytes & 0x3F) << 25) | ((fmi->ecc_bytes & 0x3F) << 19);
		}
	}

	h2fmi_timing_setup_t timing_setup;
	memset(&timing_setup, 0, sizeof(timing_setup));

	uint32_t nand_freq = clock_get_frequency(FrequencyNand);

	timing_setup.freq = nand_freq;
	timing_setup.f = info->some_mask->some_array[3];
	timing_setup.g = info->some_mask->some_array[4];
	timing_setup.h = info->some_mask->some_array[5];
	timing_setup.i = info->some_mask->some_array[6];
	timing_setup.j = info->timing_info->unk4;
	timing_setup.k = info->timing_info->unk5;
	timing_setup.l = info->timing_info->unk6;
	timing_setup.m = info->timing_info->unk7;
	timing_setup.n = info->timing_info->unk8;

	timing_setup.o = 0;

	timing_setup.p = info->timing_info->unk1;
	timing_setup.q = info->timing_info->unk2;
	timing_setup.r = info->timing_info->unk3;

	uint8_t timing_info[5];
	if(h2fmi_setup_timing(&timing_setup, timing_info))
	{
		bufferPrintf("h2fmi: Failed to setup timing array.\r\n");
	}

	// Initialize NAND Geometry
	{
		memset(&h2fmi_geometry, 0, sizeof(h2fmi_geometry));
		h2fmi_geometry.num_fmi = bus_count;
		h2fmi_geometry.num_ce = chip_count;
		h2fmi_geometry.blocks_per_ce = fmi0.blocks_per_ce;
		h2fmi_geometry.pages_per_block = fmi0.pages_per_block;
		h2fmi_geometry.bbt_format = fmi0.bbt_format;
		h2fmi_geometry.bytes_per_spare = fmi0.bytes_per_spare;
		h2fmi_geometry.banks_per_ce_vfl = fmi0.banks_per_ce_vfl;
		h2fmi_geometry.banks_per_ce = fmi0.banks_per_ce;
		h2fmi_geometry.blocks_per_bank = h2fmi_geometry.blocks_per_ce / h2fmi_geometry.banks_per_ce;
		
		// Check that blocks per CE is a POT.
		if((h2fmi_geometry.blocks_per_ce & (h2fmi_geometry.blocks_per_ce-1)) == 0)
		{
			h2fmi_geometry.unk14 = h2fmi_geometry.blocks_per_bank;
			h2fmi_geometry.unk18 = h2fmi_geometry.blocks_per_ce;
			h2fmi_geometry.unk1A = h2fmi_geometry.blocks_per_ce;
		}
		else
		{
			// Find next biggest power of two.
			uint32_t nextPOT = 1;
			if((h2fmi_geometry.blocks_per_bank & 0x80000000) == 0)
				while(nextPOT < h2fmi_geometry.blocks_per_bank)
					nextPOT <<= 1;

			uint32_t unk14;
			if(nextPOT - h2fmi_geometry.blocks_per_bank != 0)
				unk14 = nextPOT << 1;
			else
				unk14 = nextPOT;

			h2fmi_geometry.unk14 = unk14;
			h2fmi_geometry.unk18 = ((h2fmi_geometry.banks_per_ce-1)*unk14)
				+ h2fmi_geometry.blocks_per_bank;
			h2fmi_geometry.unk1A = unk14;
		}

		// Find next biggest power of two.
		uint32_t nextPOT = 1;
		if((h2fmi_geometry.pages_per_block & 0x80000000) == 0)
			while(nextPOT < h2fmi_geometry.pages_per_block)
				nextPOT <<= 1;

		if(nextPOT - h2fmi_geometry.pages_per_block != 0)
			h2fmi_geometry.pages_per_block_2 = nextPOT << 1;
		else
			h2fmi_geometry.pages_per_block_2 = nextPOT;
		
		h2fmi_geometry.is_ppn = fmi0.is_ppn;
		if(fmi0.is_ppn)
		{
			// TODO: Write this code, currently is_ppn
			// is always 0... -- Ricky26
		}
		else
		{
			h2fmi_geometry.blocks_per_bank_32 = h2fmi_geometry.blocks_per_bank;
			h2fmi_geometry.pages_per_block_32 = h2fmi_geometry.pages_per_block;
			h2fmi_geometry.pages_per_block_2_32 = h2fmi_geometry.pages_per_block;
			h2fmi_geometry.banks_per_ce_32 = h2fmi_geometry.banks_per_ce;

			nextPOT = 1;
			if(((h2fmi_geometry.pages_per_block - 1) & 0x80000000) == 0)
				while(nextPOT < h2fmi_geometry.pages_per_block - 1)
					nextPOT <<= 1;

			h2fmi_geometry.page_number_bit_width = nextPOT;
			h2fmi_geometry.page_number_bit_width_2 = nextPOT;
			h2fmi_geometry.pages_per_block_per_ce
				= h2fmi_geometry.banks_per_ce_vfl * h2fmi_geometry.pages_per_block;
			h2fmi_geometry.unk1C = info->chip_info->unk7;
			h2fmi_geometry.vendorType = info->board_info->unk1;

		}

		h2fmi_geometry.num_ecc_bytes = info->some_array[1];
		h2fmi_geometry.meta_per_logical_page = info->some_array[0];
		h2fmi_geometry.field_60 = info->some_array[2];
		h2fmi_geometry.ecc_bits = fmi0.ecc_bits;
		h2fmi_geometry.ecc_tag = fmi0.ecc_tag;
	}

	for(i = 0; i < sizeof(h2fmi_busses)/sizeof(h2fmi_struct_t*); i++)
	{
		h2fmi_struct_t *fmi = h2fmi_busses[i];
		if(fmi)
		{
			uint32_t tVal = (timing_info[4] & 0xF)
							| ((timing_info[3] & 0xF) << 4)
							| ((timing_info[1] & 0xF) << 8)
							| ((timing_info[0] & 0xF) << 12)
							| ((timing_info[2] & 0xF) << 16);
			bufferPrintf("fmi: bus %d, new tval = 0x%08x\r\n", fmi->bus_num, tVal);
			fmi->timing_register_cache_408 = tVal;
			SET_REG(H2FMI_UNKREG1(fmi), tVal);
		}
	}

	h2fmi_init_virtual_physical_map();

	for(i = 0; i < array_size(h2fmi_map); i++)
	{
		h2fmi_map_entry_t *e = &h2fmi_map[i];
		bufferPrintf("fmi: Map %d: %d, %d.\r\n", i, e->bus, e->chip);
	}

	// This is a very simple PRNG with
	// a preset seed. What are you
	// up to Apple? -- Ricky26
	// Same as in 3GS NAND -- Bluerise
	uint32_t val = 0x50F4546A;
	for(i = 0; i < 1024; i++)
	{
		val = (0x19660D * val) + 0x3C6EF35F;

		int j;
		for(j = 1; j < 763; j++)
		{
			val = (0x19660D * val) + 0x3C6EF35F;
		} while(j < 763);

		h2fmi_hash_table[i] = val;
	} while(i < 1024);

	bufferPrintf("fmi: Initialized NAND memory! %d bytes per page, %d pages per block, %d blocks per CE.\r\n",
		fmi0.bytes_per_page, h2fmi_geometry.pages_per_block, h2fmi_geometry.blocks_per_ce);

	if(info)
		free(info);

	free(buff1);
}
MODULE_INIT(h2fmi_init);

void cmd_nand_test(int argc, char** argv)
{
	if(argc < 8)
	{
		bufferPrintf("Usage: %s [ce] [page] [data] [metadata] [buf1] [buf2] [flag]\r\n", argv[0]);
		return;
	}
	
	uint32_t ce = parseNumber(argv[1]);
	uint32_t page = parseNumber(argv[2]);
	uint32_t data = parseNumber(argv[3]);
	uint32_t meta = parseNumber(argv[4]);
	uint32_t buf1 = parseNumber(argv[5]);
	uint32_t buf2 = parseNumber(argv[6]);
	uint32_t flag = parseNumber(argv[7]);

	uint32_t ret = h2fmi_read_single_page(ce, page,
			(uint8_t*)data, (uint8_t*)meta, (uint8_t*)buf1, (uint8_t*)buf2,
			flag);

	bufferPrintf("fmi: Command completed with result 0x%08x.\r\n", ret);
}
COMMAND("nand_test", "H2FMI NAND test", cmd_nand_test);
