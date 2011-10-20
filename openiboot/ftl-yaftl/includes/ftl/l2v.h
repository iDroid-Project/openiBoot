#ifndef FTL_L2V_H
#define FTL_L2V_H

#include "openiboot.h"

typedef struct _L2VNode {
	struct _L2VNode* next;
} L2VNode;

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

// TODO: documentation
error_t L2V_Init(uint32_t totalPages, uint32_t numBlocks, uint32_t pagesPerSublk);

#endif // FTL_L2V_H
