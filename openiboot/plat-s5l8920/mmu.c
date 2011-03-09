#include "openiboot.h"
#include "mmu.h"
#include "hardware/arm.h"
#include "hardware/platform.h"
#include "openiboot-asmhelpers.h"

uint32_t* CurrentPageTable;

static void initialize_pagetable();

static PhysicalAddressMap PhysicalAddressMapping[] = {
    {0x0,        0xFFFFFFFF, RAMStart, RAMSize},
    {0x40000000, 0xFFFFFFFF, RAMStart, RAMSize},
    {AMC0Map,        0xFFFFFFFF, AMC0Start, AMC0Size},

	{0x84000000, 0xFFFFFFFF, 0x84000000, 0x400000},
	{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0},
};

int mmu_setup() {
	CurrentPageTable = (uint32_t*) PageTable;

	// Initialize the page table

	initialize_pagetable();

	// Implement the page table

	// Disable checking TLB permissions for domain 0, which is the only domain we're using so anyone can access
	// anywhere in memory, since we trust ourselves.
	WriteDomainAccessControlRegister(ARM11_DomainAccessControl_D0_ALL);
						
	WriteTranslationTableBaseRegister0(CurrentPageTable);
	InvalidateUnifiedTLBUnlockedEntries();
	mmu_enable();
	InvalidateUnifiedTLBUnlockedEntries();

	return 0;
}

void mmu_enable() {
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() | 0x1);
}

void mmu_disable() {
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~0x4);
	WriteControlRegisterConfigData(ReadControlRegisterConfigData() & ~0x1);
}

void initialize_pagetable() {
	// I have absolutely no clue what this does, but
	// Apple does it, so hey, it must work, right?
	SetC9C012(GetC9C012() | 0x2000000);

	// Initialize the page table with a default identity mapping
	mmu_map_section_range(MemoryStart, MemoryEnd, MemoryStart, FALSE, FALSE);
	mmu_map_section_range(RAMStart, RAMEnd, RAMStart, TRUE, TRUE);
	mmu_map_section_range(HiMemStart, HiMemEnd, RAMStart, FALSE, FALSE);
	mmu_map_section_range(AMC0Start, AMC0End, AMC0Map, FALSE, FALSE);
}

void mmu_map_section(uint32_t section, uint32_t target, Boolean cacheable, Boolean bufferable) {
	uint32_t* sectionEntry = &CurrentPageTable[section >> 20];

	*sectionEntry =
		(target & MMU_SECTION_MASK)
		| (cacheable ? MMU_CACHEABLE : 0x0)
		| (bufferable ? MMU_BUFFERABLE : 0x0)
		| MMU_AP_BOTHWRITE		// set AP to 11 (APX is always 0, so this means R/W for everyone)
		| MMU_EXECUTENEVER
		| MMU_SECTION;			// this is a section

	//bufferPrintf("map section (%x): %x -> %x, %d %d (%x)\r\n", sectionEntry, section, target, cacheable, bufferable, *sectionEntry);

	CleanDataCacheLineMVA(sectionEntry);
	InvalidateUnifiedTLBUnlockedEntries();
}

void mmu_map_section_range(uint32_t rangeStart, uint32_t rangeEnd, uint32_t target, Boolean cacheable, Boolean bufferable) {
	uint32_t currentSection;
	uint32_t curTargetSection = target;
	Boolean started = FALSE;

	for(currentSection = rangeStart; currentSection < rangeEnd; currentSection += MMU_SECTION_SIZE) {
		if(started && currentSection == 0) {
			// We need this check because if rangeEnd is 0xFFFFFFFF, currentSection
			// will always be < rangeEnd, since we overflow the uint32.
			break;
		}
		started = TRUE;
		mmu_map_section(currentSection, curTargetSection, cacheable, bufferable);
		curTargetSection += MMU_SECTION_SIZE;
	}
}

PhysicalAddressMap* get_address_map(uint32_t address, uint32_t* address_map_base) {
	int i = 0;

	while (1) {
		if (!PhysicalAddressMapping[i].size)
			break;
		if (PhysicalAddressMapping[i].logicalStart != 0xFFFFFFFF &&
			PhysicalAddressMapping[i].logicalStart <= address &&
			address < PhysicalAddressMapping[i].logicalStart + PhysicalAddressMapping[i].size) {
				      *address_map_base = address - PhysicalAddressMapping[i].logicalStart;
				      return &PhysicalAddressMapping[i];
		}
		if (PhysicalAddressMapping[i].logicalEnd != 0xFFFFFFFF &&
			address >= PhysicalAddressMapping[i].logicalEnd &&
			address < PhysicalAddressMapping[i].logicalEnd + PhysicalAddressMapping[i].size) {
				      *address_map_base = address - PhysicalAddressMapping[i].logicalEnd;
				      return &PhysicalAddressMapping[i];
		}
		i++;
	}

	return 0;
}

uint32_t get_physical_address(uint32_t address) {
	uint32_t address_map_base;
	uint32_t result;

	PhysicalAddressMap* physicalAddressMap = get_address_map(address, &address_map_base);
	if (physicalAddressMap)
		result = physicalAddressMap->physicalStart + address_map_base;
	else
		result = -1;
	return result;
}
