#ifndef FTL_L2V_H
#define FTL_L2V_H

#include "openiboot.h"
#include "ftl/yaftl_common.h"

#define L2V_VPN_SPECIAL	(0x1FF0002)
#define L2V_VPN_MISS	(0x1FF0003)

#define L2V_VPN_ISNORMAL(v)	((v) < L2V_VPN_SPECIAL)

/* Types */

typedef struct {
	uint32_t numRoots;
	uint32_t* Tree;
	uint32_t* TreeNodes;
	uint32_t* UpdatesSinceRepack;
	L2VNode* previousNode;
	L2VNode* nextNode;
	uint32_t numBlocks;
	uint32_t pagesPerSublk;
	L2VNode* Pool;
	L2VNode* firstNode;
	uint32_t nodeCount;
} L2VDesc;

error_t L2V_Init(uint32_t totalPages, uint32_t numBlocks, uint32_t pagesPerSublk);

void L2V_Update(uint32_t _start, uint32_t _count, uint32_t _vpn);

void L2V_Search(GCReadC* _c);

#endif // FTL_L2V_H
