#ifndef A4_PMU_H
#define A4_PMU_H

/**
 * @brief A4 Specific gpio setup
 *
 * @param _idx
 * @param _dir
 * @param _pol
 *
 * @return error_t
 */
error_t pmu_setup_gpio(int _idx, int _dir, int _pol);

#endif

