#include "ftl/l2v.h"
#include "ftl/yaftl_mem.h"
#include "openiboot.h"
#include "util.h"

static void L2V_SetFirstNode(L2VNode* node);

static L2VDesc L2V;

error_t L2V_Init(uint32_t totalPages, uint32_t numBlocks, uint32_t pagesPerSublk)
{
	int32_t i;

	if (numBlocks * pagesPerSublk > 0x1FF0001)
		system_panic("L2V: Tree bitspace not sufficient for geometry %dx%d\r\n", numBlocks, pagesPerSublk);

	L2V.numBlocks = numBlocks;
	L2V.pagesPerSublk = pagesPerSublk;
	L2V.numRoots = (totalPages >> 15) + 1;
	L2V.nextNode = NULL;

	L2V.Tree = (uint32_t*)yaftl_alloc(L2V.numRoots * sizeof(uint32_t));
	if (!L2V.Tree)
		system_panic("L2V: tree allocation has failed\r\n");

	L2V.TreeNodes = (uint32_t*)yaftl_alloc(L2V.numRoots * sizeof(uint32_t));
	if (!L2V.TreeNodes)
		system_panic("L2V: tree nodes allocation has failed\r\n");

	L2V.UpdatesSinceRepack = (uint32_t*)yaftl_alloc(L2V.numRoots * sizeof(uint32_t));
	if (!L2V.UpdatesSinceRepack)
		system_panic("L2V: updates since repack allocation has failed\r\n");

	for (i = 0; i < L2V.numRoots; ++i)
		L2V.Tree[i] = 0x7FC000Cu;

	L2V.previousNode = NULL;
	L2V.Pool = (L2VNode*)yaftl_alloc(0x10000 * sizeof(L2VNode));
	if (!L2V.Pool)
		return ENOMEM;

	L2V.firstNode = NULL;
	L2V.nodeCount = 0;
	for (i = 0xFFF0; i >= 0; i -= 0x10)
		L2V_SetFirstNode(&L2V.Pool[i]);

	return SUCCESS;
}

static void L2V_SetFirstNode(L2VNode* node)
{
	if (node < &L2V.Pool[0] || node > &L2V.Pool[0x10000])
		system_panic("L2V: new first node must be in pool range\r\n");

	node->next = L2V.firstNode;
	L2V.firstNode = node;
	++L2V.nodeCount;
}
