#include "openiboot.h"
#include "util.h"
#include "ftl/yaftl_gc.h"

void gcListPushBack(GCList* _list, uint32_t _block)
{
	uint32_t curr = _list->head;

	// Search if the block already exists.
	while (curr != _list->tail) {
		if (_list->block[curr] == _block)
			return;

		++curr;
		if (curr > GCLIST_MAX)
			curr = 0;
	}

	_list->block[_list->tail] = _block;
	++_list->tail;

	if (_list->tail > GCLIST_MAX)
		_list->tail = 0;

	if (_list->tail == _list->head)
		system_panic("YAFTL: gcListPushBack -- list is full\r\n");
}
