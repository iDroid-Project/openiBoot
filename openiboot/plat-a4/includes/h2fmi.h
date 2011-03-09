#ifndef  H2FMI_H
#define  H2FMI_H

#include "openiboot.h"
#include "cdma.h"

typedef struct _h2fmi_geometry
{
	uint32_t field_0;
	uint16_t num_ce;
	uint16_t blocks_per_ce;
	uint16_t pages_per_block;
	uint16_t bbt_format;
	uint16_t bytes_per_spare;
	uint16_t banks_per_ce_vfl;
	uint16_t banks_per_ce;
	uint16_t blocks_per_bank;
	uint16_t unk14;
	uint16_t pages_per_block_2;
	uint16_t unk18;
	uint16_t unk1A;
	uint32_t unk1C;
	uint32_t vendorType;
	uint8_t ecc_bits;
	uint8_t ecc_tag;

	uint32_t num_fmi;
	uint32_t blocks_per_bank_32;
	uint32_t banks_per_ce_32;
	uint32_t pages_per_block_32;
	uint32_t pages_per_block_2_32;

	uint32_t page_number_bit_width;
	uint32_t page_number_bit_width_2;
	uint32_t is_ppn;
	uint32_t num_ecc_bytes;
	uint32_t meta_per_logical_page;
	uint32_t field_60;
	uint32_t pages_per_block_per_ce;
} h2fmi_geometry_t;


typedef struct _h2fmi_struct
{
	uint32_t bus_num;
	uint32_t base_address;
	uint32_t num_chips;
	uint32_t bitmap;
	uint32_t clock_gate;
	uint32_t is_ppn;
	uint32_t pages_per_block;
	uint32_t bbt_format;
	uint32_t bytes_per_spare;
	uint32_t blocks_per_ce;
	uint32_t banks_per_ce_vfl;
	uint32_t meta_per_logical_page;
	uint32_t ecc_bytes;
	uint32_t bytes_per_page;
	uint32_t banks_per_ce;
	uint32_t nand_timings;
	uint32_t correctable_bytes_per_page;
	uint32_t unk40;
	uint8_t ecc_bits;
	uint8_t ecc_tag;
	uint32_t fmi_state;
	uint32_t dmaAESInfo_enabled;
	uint32_t dmaAESInfo_AESType;
	uint32_t dmaAESInfo_pKey;
	uint32_t dmaAESInfo_maxSize;
	uint32_t dmaAESInfo_handler;
	uint32_t* dmaAESInfo_dataBuffer;
	dmaAES *aes_info;
	uint32_t unk68;
	uint32_t isNotPendingTask;
	uint32_t unk70;
	uint32_t previous;
	uint32_t next;
	uint32_t currentmode;
	uint32_t state_rd;
	uint32_t numPagesToRead;
	uint32_t current_chip;
	uint32_t input_ANDchip;
	uint32_t input_ANDpageNumber;
	uint32_t input_ANDdatabuffer;
	uint32_t input_ANDwmrdata;
	uint32_t rw_transition_time_ticksLow;
	uint32_t rw_transition_time_ticksHigh;
	uint32_t timer1_ticksLow;
	uint32_t timer1_ticksHigh;
	uint32_t timer2_ticksLow;
	uint32_t timer2_ticksHigh;
	uint32_t rw_status;
	uint32_t rw_attempted;
	uint32_t input_chip;
	uint32_t input_flag1;
	uint32_t input_unk15c_0;
	uint32_t input_unk160_0;
	uint32_t input_unk164_0;
	uint32_t input_flag2;
	uint32_t tasknode;
	uint32_t task_state;
	uint32_t next_task;
	uint32_t previous_task;
	uint32_t unk17c_counter;
	uint32_t fmi_failureDetails;
	uint8_t field_1A0;
	uint8_t field_1A2;

	uint32_t interrupt;
} h2fmi_struct_t;

#endif //H2FMI_H
