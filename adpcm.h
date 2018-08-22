/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
*/

#include <stdint.h>

struct adpcm_state {
    int32_t      valprev;        /* Previous output value */
    int8_t       index;          /* Index into stepsize table */
}__attribute__((packed));

typedef struct adpcm_state adpcm_state_t;

void adpcm_coder(int32_t [], int8_t [], int32_t, struct adpcm_state *);
void adpcm_decoder(int8_t [], int32_t [], int32_t, struct adpcm_state *);
