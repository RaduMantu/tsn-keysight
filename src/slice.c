#include "slice.h"
#include "util.h"

#define SLICE_FREQ (BASE_FREQ * 100)    /* slice interval in Hz */
#define NO_GATES   8                    /* number of gates */

static tscval_t t_start;    /* moment when first recv occurred */


/* mark_start - called once when transmission starts
 */
void
mark_start(void)
{
    rdtsc(t_start.low, t_start.high);
}

uint32_t
get_current_gate(void)
{
    /* get current timestamp */
    tscval_t ts;
    rdtsc(ts.low, ts.high);

    /* translate elapsed time into gate */
    return ((ts.raw - t_start.raw) / SLICE_FREQ) % NO_GATES;
}

