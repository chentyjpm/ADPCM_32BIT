/***********************************************************
Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version 1.2, 18-Dec-92.
**
** Change log:
** - Fixed a stupid bug, where the delta was computed as
**   stepsize*code/4 in stead of stepsize*(code+0.5)/4.
** - There was an off-by-one error causing it to pick
**   an incorrect delta once in a blue moon.
** - The NODIVMUL define has been removed. Computations are now always done
**   using shifts, adds and subtracts. It turned out that, because the standard
**   is defined using shift/add/subtract, you needed bits of fixup code
**   (because the div/mul simulation using shift/add/sub made some rounding
**   errors that real div/mul don't make) and all together the resultant code
**   ran slower than just using the shifts all the time.
** - Changed some of the variable names to be more meaningful.
*/

/* modefied by juguofeng<jgfntu@163.com> 2012-05-20 */

#include "adpcm.h"
#include <stdio.h>

/* Intel ADPCM step variation table */
static int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static int stepsizeTable[89] =
{
    60816,	74672,	89820,	106260,	123992,	143016,	163332,	184940,	221696,	246534,	287812,	331674,	378120,	427150,	496496,	569718,	646816,	727790,	832956,	943290,	1080400,	1223970,	1374000,	1554036,	1766016,	1987040,	2242592,	2534610,	2865032,	3235796,	3648840,	4106102,	4609520,	5191038,	5823880,	6541282,	7347120,	8245270,	9272844,	10401774,	11635936,	13049554,	14614560,	16373234,	18333328,	20540352,	22965592,	25694900,	28739904,	32112232,	35905488,	40093542,	44816800,	50051716,	55859304,	62391598,	69628928,	77685570,	86680968,	96692932,	107801856,	120186906,	133986260,	149343264,	166456464,	185481480,	206630424,	230123160,	256292536,	285377796,	317733752,	353728136,	393686400,	438111870,	487525960,	542411034,	603380528,	671185410,	746425260,	830076078,	922966352,	1026133452,	1140710000,	1267927100,	1409180520,	1565973554,	1740112984,	1933332660,	2147418112,
};
/*
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
*/
void adpcm_coder(int32_t indata[], int8_t outdata[], int32_t len, adpcm_state_t *state)
{
    int32_t *inp;			/* Input buffer pointer */
    int8_t *outp;	/* output buffer pointer */
    int32_t val;			/* Current input sample value */
    int32_t sign;			/* Current adpcm sign bit */
    int64_t delta;			/* Current adpcm output value */
    int64_t diff;			/* Difference between val and valprev */
    int32_t step;			/* Stepsize */
    int64_t valpred;		/* Predicted output value */
    int64_t vpdiff;			/* Current change to valpred */
    int32_t index;			/* Current step change index */
    int32_t outputbuffer;	/* place to keep previous 4-bit value */
    int32_t bufferstep;		/* toggle between outputbuffer/output */

    outp = (int8_t *)outdata;
    inp = indata;

    valpred = state->valprev;
    index = state->index;
    step = stepsizeTable[index];

    bufferstep = 1;
	//len /= 4;

    for ( ; len > 0 ; len-- ) {
	val = *inp++;

	/* Step 1 - compute difference with previous value */
	diff = val - valpred;
	sign = (diff < 0) ? 8 : 0;
	if ( sign ) diff = (-diff);

	/* Step 2 - Divide and clamp */
	/* Note:
	** This code *approximately* computes:
	**    delta = diff*4/step;
	**    vpdiff = (delta+0.5)*step/4;
	** but in shift step bits are dropped. The net result of this is
	** that even if you have fast mul/div hardware you cannot put it to
	** good use since the fixup would be too expensive.
	*/
	delta = 0;
	vpdiff = (step >> 3);

	if ( diff >= step ) {
	    delta = 4;
	    diff -= step;
	    vpdiff += step;
	}
	step >>= 1;
	if ( diff >= step  ) {
	    delta |= 2;
	    diff -= step;
	    vpdiff += step;
	}
	step >>= 1;
	if ( diff >= step ) {
	    delta |= 1;
	    vpdiff += step;
	}

	/* Step 3 - Update previous value */
	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;

	/* Step 4 - Clamp previous value to 16 bits */
	if ( valpred > 0x7FFFFFFF )
	  valpred = 0x7FFFFFFF;
	else if ( valpred < -0x7FFFFFFF )
	  valpred = -0x7FFFFFFF;

	/* Step 5 - Assemble value, update index and step values */
	delta |= sign;

	index += indexTable[delta];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;
	step = stepsizeTable[index];

	/* Step 6 - Output value */
	if ( bufferstep ) {
	    outputbuffer = (delta << 4) & 0xf0;
	} else {
	    *outp++ = (delta & 0x0f) | outputbuffer;
	}
	bufferstep = !bufferstep;
    }

    /* Output last step, if needed */
    if ( !bufferstep )
      *outp++ = outputbuffer;

    state->valprev = valpred;
    state->index = index;
}

void adpcm_decoder(int8_t indata[], int32_t outdata[], int32_t len, adpcm_state_t *state)
{
    int8_t *inp;	/* Input buffer pointer */
    int32_t *outp;		    /* output buffer pointer */
    int32_t sign;			/* Current adpcm sign bit */
    int32_t delta;			/* Current adpcm output value */
    int32_t step;			/* Stepsize */
    int64_t valpred;		/* Predicted value */
    int32_t vpdiff;			/* Current change to valpred */
    int32_t index;			/* Current step change index */
    int32_t inputbuffer;	/* place to keep next 4-bit value */
    int32_t bufferstep;		/* toggle between inputbuffer/input */

    outp = outdata;
    inp = (int8_t *)indata;

    valpred = state->valprev;
    index = state->index;
    step = stepsizeTable[index];

    bufferstep = 0;
    //len *= 4;			/* !!!! verify important TODO (FIX ME) JGF*/

    for ( ; len > 0 ; len-- ) {

	/* Step 1 - get the delta value */
	if ( bufferstep ) {
	    delta = inputbuffer & 0xf;
	} else {
	    inputbuffer = *inp++;
	    delta = (inputbuffer >> 4) & 0xf;
	}
	bufferstep = !bufferstep;

	/* Step 2 - Find new index value (for later) */
	index += indexTable[delta];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;

	/* Step 3 - Separate sign and magnitude */
	sign = delta & 8;
	delta = delta & 7;

	/* Step 4 - Compute difference and new predicted value */
	/*
	** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
	** in adpcm_coder.
	*/
	vpdiff = step >> 3;
	if ( delta & 4 ) vpdiff += step;
	if ( delta & 2 ) vpdiff += step>>1;
	if ( delta & 1 ) vpdiff += step>>2;

	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;

	/* Step 5 - clamp output value */
	if ( valpred > 0x7FFFFFFF )
	  valpred = 0x7FFFFFFF;
	else if ( valpred < -0x7FFFFFFF )
	  valpred = -0x7FFFFFFF;

	/* Step 6 - Update step value */
	step = stepsizeTable[index];

	/* Step 7 - Output value */
	*outp++ = valpred;
    }

    state->valprev = valpred;
    state->index = index;
}
