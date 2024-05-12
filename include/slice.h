#pragma once

#include <stdint.h> /* [u]int32_t */

#define NO_GATES   8    /* number of gates */

void mark_start(void);
uint32_t get_current_gate(void);
