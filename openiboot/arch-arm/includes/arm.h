#ifndef ARM_H
#define ARM_H

int arm_setup();
void arm_disable_caches();
void DataCacheOperation(uint8_t mode, uint32_t buffer, uint32_t size);

#endif
