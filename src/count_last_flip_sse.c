/**
 * @file count_last_flip_sse.c
 *
 *
 * A function is provided to count the number of fipped disc of the last move.
 *
 * The basic principle is to read into an array a precomputed result. Doing
 * this is easy for a single line ; as we can use arrays of the form:
 *  - COUNT_FLIP[square where we play][8-bits disc pattern].
 * The problem is thus to convert any line of a 64-bits disc pattern into an
 * 8-bits disc pattern. A fast way to do this is to select the right line,
 * with a bit-mask, to gather the masked-bits into a continuous set by the 
 * SSE PMOVMSKB instruction.
 * Once we get our 8-bits disc patterns, we directly get the number of
 * flipped discs from the precomputed array, and add them from each flipping
 * lines.
 * For optimization purpose, the value returned is twice the number of flipped
 * disc, to facilitate the computation of disc difference.
 *
 * @date 1998 - 2018
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.4
 * 
 */

#include "bit.h"

/** precomputed count flip array */
static const unsigned char COUNT_FLIP[8][256] = {
	{
		 0,  0,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		 8,  8,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		10, 10,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		 8,  8,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		12, 12,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		 8,  8,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		10, 10,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
		 8,  8,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,  6,  6,  0,  0,  2,  2,  0,  0,  4,  4,  0,  0,  2,  2,  0,  0,
	},
	{
		 0,  0,  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 6,  6,  6,  6,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 8,  8,  8,  8,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 6,  6,  6,  6,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		10, 10, 10, 10,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 6,  6,  6,  6,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 8,  8,  8,  8,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
		 6,  6,  6,  6,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,  4,  4,  4,  4,  0,  0,  0,  0,  2,  2,  2,  2,  0,  0,  0,  0,
	},
	{
		 0,  2,  0,  0,  0,  2,  0,  0,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 6,  8,  6,  6,  6,  8,  6,  6,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 8, 10,  8,  8,  8, 10,  8,  8,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 6,  8,  6,  6,  6,  8,  6,  6,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
		 4,  6,  4,  4,  4,  6,  4,  4,  0,  2,  0,  0,  0,  2,  0,  0,  2,  4,  2,  2,  2,  4,  2,  2,  0,  2,  0,  0,  0,  2,  0,  0,
	},
	{
		 0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 4,  8,  6,  6,  4,  4,  4,  4,  4,  8,  6,  6,  4,  4,  4,  4,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 6, 10,  8,  8,  6,  6,  6,  6,  6, 10,  8,  8,  6,  6,  6,  6,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 4,  8,  6,  6,  4,  4,  4,  4,  4,  8,  6,  6,  4,  4,  4,  4,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
		 2,  6,  4,  4,  2,  2,  2,  2,  2,  6,  4,  4,  2,  2,  2,  2,  0,  4,  2,  2,  0,  0,  0,  0,  0,  4,  2,  2,  0,  0,  0,  0,
	},
	{
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
		 2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
		 4, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
		 2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  4,  4,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,
	},
	{
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 2, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 2, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  8,  6,  6,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	},
	{
		 0, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0, 10,  8,  8,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	},
	{
		 0, 12, 10, 10,  8,  8,  8,  8,  6,  6,  6,  6,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
		 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0, 12, 10, 10,  8,  8,  8,  8,  6,  6,  6,  6,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
		 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	},
};

/* bit masks for diagonal lines */
#ifdef __AVX2__
static const V4DI mask_d_v[64] = {
	{{ 0x0000000000000001ULL, 0x8040201008040201ULL, 0x0101010101010101ULL, 0 }},
	{{ 0x0000000000000102ULL, 0x0080402010080402ULL, 0x0202020202020202ULL, 0 }},
	{{ 0x0000000000010204ULL, 0x0000804020100804ULL, 0x0404040404040404ULL, 0 }},
	{{ 0x0000000001020408ULL, 0x0000008040201008ULL, 0x0808080808080808ULL, 0 }},
	{{ 0x0000000102040810ULL, 0x0000000080402010ULL, 0x1010101010101010ULL, 0 }},
	{{ 0x0000010204081020ULL, 0x0000000000804020ULL, 0x2020202020202020ULL, 0 }},
	{{ 0x0001020408102040ULL, 0x0000000000008040ULL, 0x4040404040404040ULL, 0 }},
	{{ 0x0102040810204080ULL, 0x0000000000000080ULL, 0x8080808080808080ULL, 0 }},
	{{ 0x0000000000000102ULL, 0x4020100804020100ULL, 0x0101010101010101ULL, 0 }},
	{{ 0x0000000000010204ULL, 0x8040201008040201ULL, 0x0202020202020202ULL, 0 }},
	{{ 0x0000000001020408ULL, 0x0080402010080402ULL, 0x0404040404040404ULL, 0 }},
	{{ 0x0000000102040810ULL, 0x0000804020100804ULL, 0x0808080808080808ULL, 0 }},
	{{ 0x0000010204081020ULL, 0x0000008040201008ULL, 0x1010101010101010ULL, 0 }},
	{{ 0x0001020408102040ULL, 0x0000000080402010ULL, 0x2020202020202020ULL, 0 }},
	{{ 0x0102040810204080ULL, 0x0000000000804020ULL, 0x4040404040404040ULL, 0 }},
	{{ 0x0204081020408000ULL, 0x0000000000008040ULL, 0x8080808080808080ULL, 0 }},
	{{ 0x0000000000010204ULL, 0x2010080402010000ULL, 0x0101010101010101ULL, 0 }},
	{{ 0x0000000001020408ULL, 0x4020100804020100ULL, 0x0202020202020202ULL, 0 }},
	{{ 0x0000000102040810ULL, 0x8040201008040201ULL, 0x0404040404040404ULL, 0 }},
	{{ 0x0000010204081020ULL, 0x0080402010080402ULL, 0x0808080808080808ULL, 0 }},
	{{ 0x0001020408102040ULL, 0x0000804020100804ULL, 0x1010101010101010ULL, 0 }},
	{{ 0x0102040810204080ULL, 0x0000008040201008ULL, 0x2020202020202020ULL, 0 }},
	{{ 0x0204081020408000ULL, 0x0000000080402010ULL, 0x4040404040404040ULL, 0 }},
	{{ 0x0408102040800000ULL, 0x0000000000804020ULL, 0x8080808080808080ULL, 0 }},
	{{ 0x0000000001020408ULL, 0x1008040201000000ULL, 0x0101010101010101ULL, 0 }},
	{{ 0x0000000102040810ULL, 0x2010080402010000ULL, 0x0202020202020202ULL, 0 }},
	{{ 0x0000010204081020ULL, 0x4020100804020100ULL, 0x0404040404040404ULL, 0 }},
	{{ 0x0001020408102040ULL, 0x8040201008040201ULL, 0x0808080808080808ULL, 0 }},
	{{ 0x0102040810204080ULL, 0x0080402010080402ULL, 0x1010101010101010ULL, 0 }},
	{{ 0x0204081020408000ULL, 0x0000804020100804ULL, 0x2020202020202020ULL, 0 }},
	{{ 0x0408102040800000ULL, 0x0000008040201008ULL, 0x4040404040404040ULL, 0 }},
	{{ 0x0810204080000000ULL, 0x0000000080402010ULL, 0x8080808080808080ULL, 0 }},
	{{ 0x0000000102040810ULL, 0x0804020100000000ULL, 0x0101010101010101ULL, 0 }},
	{{ 0x0000010204081020ULL, 0x1008040201000000ULL, 0x0202020202020202ULL, 0 }},
	{{ 0x0001020408102040ULL, 0x2010080402010000ULL, 0x0404040404040404ULL, 0 }},
	{{ 0x0102040810204080ULL, 0x4020100804020100ULL, 0x0808080808080808ULL, 0 }},
	{{ 0x0204081020408000ULL, 0x8040201008040201ULL, 0x1010101010101010ULL, 0 }},
	{{ 0x0408102040800000ULL, 0x0080402010080402ULL, 0x2020202020202020ULL, 0 }},
	{{ 0x0810204080000000ULL, 0x0000804020100804ULL, 0x4040404040404040ULL, 0 }},
	{{ 0x1020408000000000ULL, 0x0000008040201008ULL, 0x8080808080808080ULL, 0 }},
	{{ 0x0000010204081020ULL, 0x0402010000000000ULL, 0x0101010101010101ULL, 0 }},
	{{ 0x0001020408102040ULL, 0x0804020100000000ULL, 0x0202020202020202ULL, 0 }},
	{{ 0x0102040810204080ULL, 0x1008040201000000ULL, 0x0404040404040404ULL, 0 }},
	{{ 0x0204081020408000ULL, 0x2010080402010000ULL, 0x0808080808080808ULL, 0 }},
	{{ 0x0408102040800000ULL, 0x4020100804020100ULL, 0x1010101010101010ULL, 0 }},
	{{ 0x0810204080000000ULL, 0x8040201008040201ULL, 0x2020202020202020ULL, 0 }},
	{{ 0x1020408000000000ULL, 0x0080402010080402ULL, 0x4040404040404040ULL, 0 }},
	{{ 0x2040800000000000ULL, 0x0000804020100804ULL, 0x8080808080808080ULL, 0 }},
	{{ 0x0001020408102040ULL, 0x0201000000000000ULL, 0x0101010101010101ULL, 0 }},
	{{ 0x0102040810204080ULL, 0x0402010000000000ULL, 0x0202020202020202ULL, 0 }},
	{{ 0x0204081020408000ULL, 0x0804020100000000ULL, 0x0404040404040404ULL, 0 }},
	{{ 0x0408102040800000ULL, 0x1008040201000000ULL, 0x0808080808080808ULL, 0 }},
	{{ 0x0810204080000000ULL, 0x2010080402010000ULL, 0x1010101010101010ULL, 0 }},
	{{ 0x1020408000000000ULL, 0x4020100804020100ULL, 0x2020202020202020ULL, 0 }},
	{{ 0x2040800000000000ULL, 0x8040201008040201ULL, 0x4040404040404040ULL, 0 }},
	{{ 0x4080000000000000ULL, 0x0080402010080402ULL, 0x8080808080808080ULL, 0 }},
	{{ 0x0102040810204080ULL, 0x0100000000000000ULL, 0x0101010101010101ULL, 0 }},
	{{ 0x0204081020408000ULL, 0x0201000000000000ULL, 0x0202020202020202ULL, 0 }},
	{{ 0x0408102040800000ULL, 0x0402010000000000ULL, 0x0404040404040404ULL, 0 }},
	{{ 0x0810204080000000ULL, 0x0804020100000000ULL, 0x0808080808080808ULL, 0 }},
	{{ 0x1020408000000000ULL, 0x1008040201000000ULL, 0x1010101010101010ULL, 0 }},
	{{ 0x2040800000000000ULL, 0x2010080402010000ULL, 0x2020202020202020ULL, 0 }},
	{{ 0x4080000000000000ULL, 0x4020100804020100ULL, 0x4040404040404040ULL, 0 }},
	{{ 0x8000000000000000ULL, 0x8040201008040201ULL, 0x8080808080808080ULL, 0 }}
};
#else
static const V2DI mask_d[64] = {
	{{ 0x0000000000000001ULL, 0x8040201008040201ULL }}, {{ 0x0000000000000102ULL, 0x0080402010080402ULL }},
	{{ 0x0000000000010204ULL, 0x0000804020100804ULL }}, {{ 0x0000000001020408ULL, 0x0000008040201008ULL }},
	{{ 0x0000000102040810ULL, 0x0000000080402010ULL }}, {{ 0x0000010204081020ULL, 0x0000000000804020ULL }},
	{{ 0x0001020408102040ULL, 0x0000000000008040ULL }}, {{ 0x0102040810204080ULL, 0x0000000000000080ULL }},
	{{ 0x0000000000000102ULL, 0x4020100804020100ULL }}, {{ 0x0000000000010204ULL, 0x8040201008040201ULL }},
	{{ 0x0000000001020408ULL, 0x0080402010080402ULL }}, {{ 0x0000000102040810ULL, 0x0000804020100804ULL }},
	{{ 0x0000010204081020ULL, 0x0000008040201008ULL }}, {{ 0x0001020408102040ULL, 0x0000000080402010ULL }},
	{{ 0x0102040810204080ULL, 0x0000000000804020ULL }}, {{ 0x0204081020408000ULL, 0x0000000000008040ULL }},
y	{{ 0x0000000000010204ULL, 0x2010080402010000ULL }}, {{ 0x0000000001020408ULL, 0x4020100804020100ULL }},
	{{ 0x0000000102040810ULL, 0x8040201008040201ULL }}, {{ 0x0000010204081020ULL, 0x0080402010080402ULL }},
	{{ 0x0001020408102040ULL, 0x0000804020100804ULL }}, {{ 0x0102040810204080ULL, 0x0000008040201008ULL }},
	{{ 0x0204081020408000ULL, 0x0000000080402010ULL }}, {{ 0x0408102040800000ULL, 0x0000000000804020ULL }},
	{{ 0x0000000001020408ULL, 0x1008040201000000ULL }}, {{ 0x0000000102040810ULL, 0x2010080402010000ULL }},
	{{ 0x0000010204081020ULL, 0x4020100804020100ULL }}, {{ 0x0001020408102040ULL, 0x8040201008040201ULL }},
	{{ 0x0102040810204080ULL, 0x0080402010080402ULL }}, {{ 0x0204081020408000ULL, 0x0000804020100804ULL }},
	{{ 0x0408102040800000ULL, 0x0000008040201008ULL }}, {{ 0x0810204080000000ULL, 0x0000000080402010ULL }},
	{{ 0x0000000102040810ULL, 0x0804020100000000ULL }}, {{ 0x0000010204081020ULL, 0x1008040201000000ULL }},
	{{ 0x0001020408102040ULL, 0x2010080402010000ULL }}, {{ 0x0102040810204080ULL, 0x4020100804020100ULL }},
	{{ 0x0204081020408000ULL, 0x8040201008040201ULL }}, {{ 0x0408102040800000ULL, 0x0080402010080402ULL }},
	{{ 0x0810204080000000ULL, 0x0000804020100804ULL }}, {{ 0x1020408000000000ULL, 0x0000008040201008ULL }},
	{{ 0x0000010204081020ULL, 0x0402010000000000ULL }}, {{ 0x0001020408102040ULL, 0x0804020100000000ULL }},
	{{ 0x0102040810204080ULL, 0x1008040201000000ULL }}, {{ 0x0204081020408000ULL, 0x2010080402010000ULL }},
	{{ 0x0408102040800000ULL, 0x4020100804020100ULL }}, {{ 0x0810204080000000ULL, 0x8040201008040201ULL }},
	{{ 0x1020408000000000ULL, 0x0080402010080402ULL }}, {{ 0x2040800000000000ULL, 0x0000804020100804ULL }},
	{{ 0x0001020408102040ULL, 0x0201000000000000ULL }}, {{ 0x0102040810204080ULL, 0x0402010000000000ULL }},
	{{ 0x0204081020408000ULL, 0x0804020100000000ULL }}, {{ 0x0408102040800000ULL, 0x1008040201000000ULL }},
	{{ 0x0810204080000000ULL, 0x2010080402010000ULL }}, {{ 0x1020408000000000ULL, 0x4020100804020100ULL }},
	{{ 0x2040800000000000ULL, 0x8040201008040201ULL }}, {{ 0x4080000000000000ULL, 0x0080402010080402ULL }},
	{{ 0x0102040810204080ULL, 0x0100000000000000ULL }}, {{ 0x0204081020408000ULL, 0x0201000000000000ULL }},
	{{ 0x0408102040800000ULL, 0x0402010000000000ULL }}, {{ 0x0810204080000000ULL, 0x0804020100000000ULL }},
	{{ 0x1020408000000000ULL, 0x1008040201000000ULL }}, {{ 0x2040800000000000ULL, 0x2010080402010000ULL }},
	{{ 0x4080000000000000ULL, 0x4020100804020100ULL }}, {{ 0x8000000000000000ULL, 0x8040201008040201ULL }}
};
#endif

#if !defined(__x86_64__) && !defined(_M_X64)
// to utilize store to load forwarding
static inline __v2di _mm_cvtsi64_si128(const unsigned long long x) {
	return _mm_unpacklo_epi32(_mm_cvtsi32_si128(x), _mm_cvtsi32_si128(x >> 32));
}
#endif

// Patch for GCC Bug 43514
#if defined(__GNUC__) && (__GNUC__ == 4) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#ifdef __AVX__

static inline __v2di _mm_sll_epi64(__v2di A, __v2di B) {
	__v2di C;
	__asm__ (
		"vpsllq	%2, %1, %0"
	: "=x" (C) : "x" (A), "xm" (B));
	return C;
}

static inline __v2di _mm_srl_epi64(__v2di A, __v2di B) {
	__v2di C;
	__asm__ (
		"vpsrlq	%2, %1, %0"
	: "=x" (C) : "x" (A), "xm" (B));
	return C;
}

#else

static inline __v2di _mm_sll_epi64(__v2di A, __v2di B) {
	__asm__ (
		"psllq	%1, %0"
	: "+x" (A) : "xm" (B));
	return A;
}

static inline __v2di _mm_srl_epi64(__v2di A, __v2di B) {
	__asm__ (
		"psrlq	%1, %0"
	: "+x" (A) : "xm" (B));
	return A;
}

#endif // __AVX__
#endif // __GNUC__

/**
 * Count last flipped discs when playing on the last empty.
 *
 * @param pos the last empty square.
 * @param P player's disc pattern.
 * @return flipped disc count.
 */

int last_flip(int pos, unsigned long long P)
{
	unsigned char	n_flipped;
	unsigned int	t;
	int	x = pos & 7;
	int	y = pos >> 3;
	__m128i	P0 = _mm_cvtsi64_si128(P);

#ifdef __AVX2__
	t = ~_mm256_movemask_epi8(_mm256_cmpeq_epi8(
		_mm256_and_si256(_mm256_broadcastq_epi64(P0), mask_d_v[pos].v4), _mm256_setzero_si256()));
	n_flipped  = COUNT_FLIP[y][t >> 16];
#else
	n_flipped  = COUNT_FLIP[y][_mm_movemask_epi8(_mm_sll_epi64(P0, _mm_cvtsi32_si128(x ^ 7)))];
	t = ~_mm_movemask_epi8(_mm_cmpeq_epi8(
		_mm_and_si128(_mm_unpacklo_epi64(P0, P0), mask_d[pos].v2), _mm_setzero_si128()));
#endif
	n_flipped += COUNT_FLIP[y][(unsigned char) (t >> 8)];
	n_flipped += COUNT_FLIP[y][(unsigned char) t];
	n_flipped += COUNT_FLIP[x][(unsigned char) (P >> (y * 8))];

	return n_flipped;
}
