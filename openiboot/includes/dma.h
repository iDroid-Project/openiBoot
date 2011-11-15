/**
 * dma.h
 *
 * Copyright 2011 iDroid Project
 *
 * This file is part of iDroid. An android distribution for Apple products.
 * For more information, please visit http://www.idroidproject.org/.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DMA_H
#define DMA_H

#define ERROR_DMA 0x13
#define ERROR_BUSY 0x15
#define ERROR_ALIGN 0x9

typedef void (*DMAHandler)(int status, int controller, int channel);

typedef struct DMARequest {
	int started;
	int done;
	DMAHandler handler;
	// TODO: fill this thing out
} DMARequest;

typedef struct DMALinkedList {
    uint32_t source;	
    uint32_t destination;
    struct DMALinkedList* next;
    uint32_t control;
} DMALinkedList;

#define DMA_I2S0_RX 19
#define DMA_I2S0_TX 20
#define DMA_I2S1_RX 14
#define DMA_I2S1_TX 15
#define DMA_MEMORY 25
#define DMA_NAND 8

int dma_setup();
int dma_shutdown();
int dma_request(int Source, int SourceTransferWidth, int SourceBurstSize, int Destination, int DestinationTransferWidth, int DestinationBurstSize, int* controller, int* channel, DMAHandler handler);
int dma_perform(uint32_t Source, uint32_t Destination, int size, int continueList, int* controller, int* channel);
int dma_finish(int controller, int channel, int timeout);
uint32_t dma_dstpos(int controller, int channel);
uint32_t dma_srcpos(int controller, int channel);
void dma_pause(int controller, int channel);
void dma_resume(int controller, int channel);

#endif

