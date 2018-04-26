/**
 * @file flip_sse_bitscan.c
 *
 * This module deals with flipping discs.
 *
 * A function is provided for each square of the board. These functions are
 * gathered into an array of functions, so that a fast access to each function
 * is allowed. The generic form of the function take as input the player and
 * the opponent bitboards and return the flipped squares into a bitboard.
 *
 * Given the following notation:
 *  - x = square where we play,
 *  - P = player's disc pattern,
 *  - O = opponent's disc pattern,
 * the basic principle is to read into an array the result of a move. Doing
 * this is easier for a single line ; so we can use arrays of the form:
 *  - ARRAY[x][8-bits disc pattern].
 * The problem is thus to convert any line of a 64-bits disc pattern into an
 * 8-bits disc pattern. A fast way to do this is to select the right line,
 * with a bit-mask, to gather the masked-bits into a continuous set by a simple
 * multiplication and to right-shift the result to scale it into a number
 * between 0 and 255.
 * Once we get our 8-bits disc patterns,a first array (OUTFLANK) is used to
 * get the player's discs that surround the opponent discs:
 *  - outflank = OUTFLANK[x][O] & P
 * (Only inner 6-bits of the P are in interest here.)
 * The result is then used as an index to access a second array giving the
 * flipped discs according to the surrounding player's discs:
 *  - flipped = FLIPPED[x][outflank].
 * (Flipped discs fall into inner 6-bits.)
 * Finally, a precomputed array transform the inner 6-bits disc pattern back into a
 * 64-bits disc pattern, and the flipped squares for each line are gathered and
 * returned to generate moves.
 *
 * If the OUTFLANK search is in LSB to MSB direction, carry propagation 
 * can be used to determine contiguous opponent discs.
 * If the OUTFLANK search is in MSB to LSB direction, CONTIG_X tables
 * are used to determine coutiguous opponent discs.
 *
 * @date 1998 - 2018
 * @author Richard Delorme
 * @author Toshihiko Okuhara
 * @version 4.3
 */

#include "bit.h"

static const V2DI maskl[64][2] = {
	{ {{ ~0x00000000000000fe, ~0x0000000000000000 }}, {{ ~0x0101010101010100, ~0x8040201008040200 }} },
	{ {{ ~0x00000000000000fc, ~0x0000000000000100 }}, {{ ~0x0202020202020200, ~0x0080402010080400 }} },
	{ {{ ~0x00000000000000f8, ~0x0000000000010200 }}, {{ ~0x0404040404040400, ~0x0000804020100800 }} },
	{ {{ ~0x00000000000000f0, ~0x0000000001020400 }}, {{ ~0x0808080808080800, ~0x0000008040201000 }} },
	{ {{ ~0x00000000000000e0, ~0x0000000102040800 }}, {{ ~0x1010101010101000, ~0x0000000080402000 }} },
	{ {{ ~0x00000000000000c0, ~0x0000010204081000 }}, {{ ~0x2020202020202000, ~0x0000000000804000 }} },
	{ {{ ~0x0000000000000080, ~0x0001020408102000 }}, {{ ~0x4040404040404000, ~0x0000000000008000 }} },
	{ {{ ~0x0000000000000000, ~0x0102040810204000 }}, {{ ~0x8080808080808000, ~0x0000000000000000 }} },
	{ {{ ~0x000000000000fe00, ~0x0000000000000000 }}, {{ ~0x0101010101010000, ~0x4020100804020000 }} },
	{ {{ ~0x000000000000fc00, ~0x0000000000010000 }}, {{ ~0x0202020202020000, ~0x8040201008040000 }} },
	{ {{ ~0x000000000000f800, ~0x0000000001020000 }}, {{ ~0x0404040404040000, ~0x0080402010080000 }} },
	{ {{ ~0x000000000000f000, ~0x0000000102040000 }}, {{ ~0x0808080808080000, ~0x0000804020100000 }} },
	{ {{ ~0x000000000000e000, ~0x0000010204080000 }}, {{ ~0x1010101010100000, ~0x0000008040200000 }} },
	{ {{ ~0x000000000000c000, ~0x0001020408100000 }}, {{ ~0x2020202020200000, ~0x0000000080400000 }} },
	{ {{ ~0x0000000000008000, ~0x0102040810200000 }}, {{ ~0x4040404040400000, ~0x0000000000800000 }} },
	{ {{ ~0x0000000000000000, ~0x0204081020400000 }}, {{ ~0x8080808080800000, ~0x0000000000000000 }} },
	{ {{ ~0x0000000000fe0000, ~0x0000000000000000 }}, {{ ~0x0101010101000000, ~0x2010080402000000 }} },
	{ {{ ~0x0000000000fc0000, ~0x0000000001000000 }}, {{ ~0x0202020202000000, ~0x4020100804000000 }} },
	{ {{ ~0x0000000000f80000, ~0x0000000102000000 }}, {{ ~0x0404040404000000, ~0x8040201008000000 }} },
	{ {{ ~0x0000000000f00000, ~0x0000010204000000 }}, {{ ~0x0808080808000000, ~0x0080402010000000 }} },
	{ {{ ~0x0000000000e00000, ~0x0001020408000000 }}, {{ ~0x1010101010000000, ~0x0000804020000000 }} },
	{ {{ ~0x0000000000c00000, ~0x0102040810000000 }}, {{ ~0x2020202020000000, ~0x0000008040000000 }} },
	{ {{ ~0x0000000000800000, ~0x0204081020000000 }}, {{ ~0x4040404040000000, ~0x0000000080000000 }} },
	{ {{ ~0x0000000000000000, ~0x0408102040000000 }}, {{ ~0x8080808080000000, ~0x0000000000000000 }} },
	{ {{ ~0x00000000fe000000, ~0x0000000000000000 }}, {{ ~0x0101010100000000, ~0x1008040200000000 }} },
	{ {{ ~0x00000000fc000000, ~0x0000000100000000 }}, {{ ~0x0202020200000000, ~0x2010080400000000 }} },
	{ {{ ~0x00000000f8000000, ~0x0000010200000000 }}, {{ ~0x0404040400000000, ~0x4020100800000000 }} },
	{ {{ ~0x00000000f0000000, ~0x0001020400000000 }}, {{ ~0x0808080800000000, ~0x8040201000000000 }} },
	{ {{ ~0x00000000e0000000, ~0x0102040800000000 }}, {{ ~0x1010101000000000, ~0x0080402000000000 }} },
	{ {{ ~0x00000000c0000000, ~0x0204081000000000 }}, {{ ~0x2020202000000000, ~0x0000804000000000 }} },
	{ {{ ~0x0000000080000000, ~0x0408102000000000 }}, {{ ~0x4040404000000000, ~0x0000008000000000 }} },
	{ {{ ~0x0000000000000000, ~0x0810204000000000 }}, {{ ~0x8080808000000000, ~0x0000000000000000 }} },
	{ {{ ~0x000000fe00000000, ~0x0000000000000000 }}, {{ ~0x0101010000000000, ~0x0804020000000000 }} },
	{ {{ ~0x000000fc00000000, ~0x0000010000000000 }}, {{ ~0x0202020000000000, ~0x1008040000000000 }} },
	{ {{ ~0x000000f800000000, ~0x0001020000000000 }}, {{ ~0x0404040000000000, ~0x2010080000000000 }} },
	{ {{ ~0x000000f000000000, ~0x0102040000000000 }}, {{ ~0x0808080000000000, ~0x4020100000000000 }} },
	{ {{ ~0x000000e000000000, ~0x0204080000000000 }}, {{ ~0x1010100000000000, ~0x8040200000000000 }} },
	{ {{ ~0x000000c000000000, ~0x0408100000000000 }}, {{ ~0x2020200000000000, ~0x0080400000000000 }} },
	{ {{ ~0x0000008000000000, ~0x0810200000000000 }}, {{ ~0x4040400000000000, ~0x0000800000000000 }} },
	{ {{ ~0x0000000000000000, ~0x1020400000000000 }}, {{ ~0x8080800000000000, ~0x0000000000000000 }} },
	{ {{ ~0x0000fe0000000000, ~0x0000000000000000 }}, {{ ~0x0101000000000000, ~0x0402000000000000 }} },
	{ {{ ~0x0000fc0000000000, ~0x0001000000000000 }}, {{ ~0x0202000000000000, ~0x0804000000000000 }} },
	{ {{ ~0x0000f80000000000, ~0x0102000000000000 }}, {{ ~0x0404000000000000, ~0x1008000000000000 }} },
	{ {{ ~0x0000f00000000000, ~0x0204000000000000 }}, {{ ~0x0808000000000000, ~0x2010000000000000 }} },
	{ {{ ~0x0000e00000000000, ~0x0408000000000000 }}, {{ ~0x1010000000000000, ~0x4020000000000000 }} },
	{ {{ ~0x0000c00000000000, ~0x0810000000000000 }}, {{ ~0x2020000000000000, ~0x8040000000000000 }} },
	{ {{ ~0x0000800000000000, ~0x1020000000000000 }}, {{ ~0x4040000000000000, ~0x0080000000000000 }} },
	{ {{ ~0x0000000000000000, ~0x2040000000000000 }}, {{ ~0x8080000000000000, ~0x0000000000000000 }} },
	{ {{ ~0x00fe000000000000, ~0x0000000000000000 }}, {{ ~0x0100000000000000, ~0x0200000000000000 }} },
	{ {{ ~0x00fc000000000000, ~0x0100000000000000 }}, {{ ~0x0200000000000000, ~0x0400000000000000 }} },
	{ {{ ~0x00f8000000000000, ~0x0200000000000000 }}, {{ ~0x0400000000000000, ~0x0800000000000000 }} },
	{ {{ ~0x00f0000000000000, ~0x0400000000000000 }}, {{ ~0x0800000000000000, ~0x1000000000000000 }} },
	{ {{ ~0x00e0000000000000, ~0x0800000000000000 }}, {{ ~0x1000000000000000, ~0x2000000000000000 }} },
	{ {{ ~0x00c0000000000000, ~0x1000000000000000 }}, {{ ~0x2000000000000000, ~0x4000000000000000 }} },
	{ {{ ~0x0080000000000000, ~0x2000000000000000 }}, {{ ~0x4000000000000000, ~0x8000000000000000 }} },
	{ {{ ~0x0000000000000000, ~0x4000000000000000 }}, {{ ~0x8000000000000000, ~0x0000000000000000 }} },
	{ {{ ~0xfe00000000000000, ~0x0000000000000000 }}, {{ ~0x0000000000000000, ~0x0000000000000000 }} },
	{ {{ ~0xfc00000000000000, ~0x0000000000000000 }}, {{ ~0x0000000000000000, ~0x0000000000000000 }} },
	{ {{ ~0xf800000000000000, ~0x0000000000000000 }}, {{ ~0x0000000000000000, ~0x0000000000000000 }} },
	{ {{ ~0xf000000000000000, ~0x0000000000000000 }}, {{ ~0x0000000000000000, ~0x0000000000000000 }} },
	{ {{ ~0xe000000000000000, ~0x0000000000000000 }}, {{ ~0x0000000000000000, ~0x0000000000000000 }} },
	{ {{ ~0xc000000000000000, ~0x0000000000000000 }}, {{ ~0x0000000000000000, ~0x0000000000000000 }} },
	{ {{ ~0x8000000000000000, ~0x0000000000000000 }}, {{ ~0x0000000000000000, ~0x0000000000000000 }} },
	{ {{ ~0x0000000000000000, ~0x0000000000000000 }}, {{ ~0x0000000000000000, ~0x0000000000000000 }} }
};

#if !defined(__LZCNT__) || !defined(__x86_64__)

static const unsigned long long masko[64][4] = {
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000002, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000006, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000000e, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000001e, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000003e, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000007e, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000200, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000600, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000e00, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000001e00, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000003e00, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000007e00, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000200, 0x0000000000000100, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000400, 0x0000000000000200, 0x0000000000000000,
	0x0000000000020000, 0x0000000000000800, 0x0000000000000400, 0x0000000000000200,
	0x0000000000060000, 0x0000000000001000, 0x0000000000000800, 0x0000000000000400,
	0x00000000000e0000, 0x0000000000002000, 0x0000000000001000, 0x0000000000000800,
	0x00000000001e0000, 0x0000000000004000, 0x0000000000002000, 0x0000000000001000,
	0x00000000003e0000, 0x0000000000000000, 0x0000000000004000, 0x0000000000002000,
	0x00000000007e0000, 0x0000000000000000, 0x0000000000008000, 0x0000000000004000,
	0x0000000000000000, 0x0000000000020400, 0x0000000000010100, 0x0000000000000000,
	0x0000000000000000, 0x0000000000040800, 0x0000000000020200, 0x0000000000000000,
	0x0000000002000000, 0x0000000000081000, 0x0000000000040400, 0x0000000000020000,
	0x0000000006000000, 0x0000000000102000, 0x0000000000080800, 0x0000000000040200,
	0x000000000e000000, 0x0000000000204000, 0x0000000000101000, 0x0000000000080400,
	0x000000001e000000, 0x0000000000400000, 0x0000000000202000, 0x0000000000100800,
	0x000000003e000000, 0x0000000000000000, 0x0000000000404000, 0x0000000000201000,
	0x000000007e000000, 0x0000000000000000, 0x0000000000808000, 0x0000000000402000,
	0x0000000000000000, 0x0000000002040800, 0x0000000001010100, 0x0000000000000000,
	0x0000000000000000, 0x0000000004081000, 0x0000000002020200, 0x0000000000000000,
	0x0000000200000000, 0x0000000008102000, 0x0000000004040400, 0x0000000002000000,
	0x0000000600000000, 0x0000000010204000, 0x0000000008080800, 0x0000000004020000,
	0x0000000e00000000, 0x0000000020400000, 0x0000000010101000, 0x0000000008040200,
	0x0000001e00000000, 0x0000000040000000, 0x0000000020202000, 0x0000000010080400,
	0x0000003e00000000, 0x0000000000000000, 0x0000000040404000, 0x0000000020100800,
	0x0000007e00000000, 0x0000000000000000, 0x0000000080808000, 0x0000000040201000,
	0x0000000000000000, 0x0000000204081000, 0x0000000101010100, 0x0000000000000000,
	0x0000000000000000, 0x0000000408102000, 0x0000000202020200, 0x0000000000000000,
	0x0000020000000000, 0x0000000810204000, 0x0000000404040400, 0x0000000200000000,
	0x0000060000000000, 0x0000001020400000, 0x0000000808080800, 0x0000000402000000,
	0x00000e0000000000, 0x0000002040000000, 0x0000001010101000, 0x0000000804020000,
	0x00001e0000000000, 0x0000004000000000, 0x0000002020202000, 0x0000001008040200,
	0x00003e0000000000, 0x0000000000000000, 0x0000004040404000, 0x0000002010080400,
	0x00007e0000000000, 0x0000000000000000, 0x0000008080808000, 0x0000004020100800,
	0x0000000000000000, 0x0000020408102000, 0x0000010101010100, 0x0000000000000000,
	0x0000000000000000, 0x0000040810204000, 0x0000020202020200, 0x0000000000000000,
	0x0002000000000000, 0x0000081020400000, 0x0000040404040400, 0x0000020000000000,
	0x0006000000000000, 0x0000102040000000, 0x0000080808080800, 0x0000040200000000,
	0x000e000000000000, 0x0000204000000000, 0x0000101010101000, 0x0000080402000000,
	0x001e000000000000, 0x0000400000000000, 0x0000202020202000, 0x0000100804020000,
	0x003e000000000000, 0x0000000000000000, 0x0000404040404000, 0x0000201008040200,
	0x007e000000000000, 0x0000000000000000, 0x0000808080808000, 0x0000402010080400,
	0x0000000000000000, 0x0002040810204000, 0x0001010101010100, 0x0000000000000000,
	0x0000000000000000, 0x0004081020400000, 0x0002020202020200, 0x0000000000000000,
	0x0200000000000000, 0x0008102040000000, 0x0004040404040400, 0x0002000000000000,
	0x0600000000000000, 0x0010204000000000, 0x0008080808080800, 0x0004020000000000,
	0x0e00000000000000, 0x0020400000000000, 0x0010101010101000, 0x0008040200000000,
	0x1e00000000000000, 0x0040000000000000, 0x0020202020202000, 0x0010080402000000,
	0x3e00000000000000, 0x0000000000000000, 0x0040404040404000, 0x0020100804020000,
	0x7e00000000000000, 0x0000000000000000, 0x0080808080808000, 0x0040201008040200
};
#endif

static const unsigned long long maskr[64][4] = {
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000001, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000003, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000007, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000000f, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000001f, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000003f, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x000000000000007f, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
	0x0000000000000000, 0x0000000000000002, 0x0000000000000001, 0x0000000000000000,
	0x0000000000000100, 0x0000000000000004, 0x0000000000000002, 0x0000000000000001,
	0x0000000000000300, 0x0000000000000008, 0x0000000000000004, 0x0000000000000002,
	0x0000000000000700, 0x0000000000000010, 0x0000000000000008, 0x0000000000000004,
	0x0000000000000f00, 0x0000000000000020, 0x0000000000000010, 0x0000000000000008,
	0x0000000000001f00, 0x0000000000000040, 0x0000000000000020, 0x0000000000000010,
	0x0000000000003f00, 0x0000000000000080, 0x0000000000000040, 0x0000000000000020,
	0x0000000000007f00, 0x0000000000000000, 0x0000000000000080, 0x0000000000000040,
	0x0000000000000000, 0x0000000000000204, 0x0000000000000101, 0x0000000000000000,
	0x0000000000010000, 0x0000000000000408, 0x0000000000000202, 0x0000000000000100,
	0x0000000000030000, 0x0000000000000810, 0x0000000000000404, 0x0000000000000201,
	0x0000000000070000, 0x0000000000001020, 0x0000000000000808, 0x0000000000000402,
	0x00000000000f0000, 0x0000000000002040, 0x0000000000001010, 0x0000000000000804,
	0x00000000001f0000, 0x0000000000004080, 0x0000000000002020, 0x0000000000001008,
	0x00000000003f0000, 0x0000000000008000, 0x0000000000004040, 0x0000000000002010,
	0x00000000007f0000, 0x0000000000000000, 0x0000000000008080, 0x0000000000004020,
	0x0000000000000000, 0x0000000000020408, 0x0000000000010101, 0x0000000000000000,
	0x0000000001000000, 0x0000000000040810, 0x0000000000020202, 0x0000000000010000,
	0x0000000003000000, 0x0000000000081020, 0x0000000000040404, 0x0000000000020100,
	0x0000000007000000, 0x0000000000102040, 0x0000000000080808, 0x0000000000040201,
	0x000000000f000000, 0x0000000000204080, 0x0000000000101010, 0x0000000000080402,
	0x000000001f000000, 0x0000000000408000, 0x0000000000202020, 0x0000000000100804,
	0x000000003f000000, 0x0000000000800000, 0x0000000000404040, 0x0000000000201008,
	0x000000007f000000, 0x0000000000000000, 0x0000000000808080, 0x0000000000402010,
	0x0000000000000000, 0x0000000002040810, 0x0000000001010101, 0x0000000000000000,
	0x0000000100000000, 0x0000000004081020, 0x0000000002020202, 0x0000000001000000,
	0x0000000300000000, 0x0000000008102040, 0x0000000004040404, 0x0000000002010000,
	0x0000000700000000, 0x0000000010204080, 0x0000000008080808, 0x0000000004020100,
	0x0000000f00000000, 0x0000000020408000, 0x0000000010101010, 0x0000000008040201,
	0x0000001f00000000, 0x0000000040800000, 0x0000000020202020, 0x0000000010080402,
	0x0000003f00000000, 0x0000000080000000, 0x0000000040404040, 0x0000000020100804,
	0x0000007f00000000, 0x0000000000000000, 0x0000000080808080, 0x0000000040201008,
	0x0000000000000000, 0x0000000204081020, 0x0000000101010101, 0x0000000000000000,
	0x0000010000000000, 0x0000000408102040, 0x0000000202020202, 0x0000000100000000,
	0x0000030000000000, 0x0000000810204080, 0x0000000404040404, 0x0000000201000000,
	0x0000070000000000, 0x0000001020408000, 0x0000000808080808, 0x0000000402010000,
	0x00000f0000000000, 0x0000002040800000, 0x0000001010101010, 0x0000000804020100,
	0x00001f0000000000, 0x0000004080000000, 0x0000002020202020, 0x0000001008040201,
	0x00003f0000000000, 0x0000008000000000, 0x0000004040404040, 0x0000002010080402,
	0x00007f0000000000, 0x0000000000000000, 0x0000008080808080, 0x0000004020100804,
	0x0000000000000000, 0x0000020408102040, 0x0000010101010101, 0x0000000000000000,
	0x0001000000000000, 0x0000040810204080, 0x0000020202020202, 0x0000010000000000,
	0x0003000000000000, 0x0000081020408000, 0x0000040404040404, 0x0000020100000000,
	0x0007000000000000, 0x0000102040800000, 0x0000080808080808, 0x0000040201000000,
	0x000f000000000000, 0x0000204080000000, 0x0000101010101010, 0x0000080402010000,
	0x001f000000000000, 0x0000408000000000, 0x0000202020202020, 0x0000100804020100,
	0x003f000000000000, 0x0000800000000000, 0x0000404040404040, 0x0000201008040201,
	0x007f000000000000, 0x0000000000000000, 0x0000808080808080, 0x0000402010080402,
	0x0000000000000000, 0x0002040810204080, 0x0001010101010101, 0x0000000000000000,
	0x0100000000000000, 0x0004081020408000, 0x0002020202020202, 0x0001000000000000,
	0x0300000000000000, 0x0008102040800000, 0x0004040404040404, 0x0002010000000000,
	0x0700000000000000, 0x0010204080000000, 0x0008080808080808, 0x0004020100000000,
	0x0f00000000000000, 0x0020408000000000, 0x0010101010101010, 0x0008040201000000,
	0x1f00000000000000, 0x0040800000000000, 0x0020202020202020, 0x0010080402010000,
	0x3f00000000000000, 0x0080000000000000, 0x0040404040404040, 0x0020100804020100,
	0x7f00000000000000, 0x0000000000000000, 0x0080808080808080, 0x0040201008040201
};

#if defined(_MSC_VER) && defined(_M_X64) && !defined(__AVX2__)
static inline int lzcnt64(unsigned long long n) {
	unsigned long i;
	if (_BitScanReverse64(&i, n))
		return i ^ 63;
	else
		return 64;
}
#else
#define	lzcnt64(x)	_lzcnt_u64(x)
#endif

#if (defined(__LZCNT__) && defined(__x86_64__)) || (defined(_MSC_VER) && defined(_M_X64))
#define	count_opp_reverse(O,masko,maskr)	lzcnt64(~(O) & (maskr))
#else
// with guardian bit to avoid __builtin_clz(0)
#define	count_opp_reverse(O,masko,maskr)	__builtin_clzll(((O) & (masko)) ^ (maskr))
#endif

#define	SWAP64	0x4e	// for _mm_shuffle_epi32
#define	SWAP32	0xb1

/**
 * Make inverted flip mask if opponent's disc are surrounded by player's.
 *
 * 0xffffffffffffffffULL (-1) if outflank is 0
 * 0x0000000000000000ULL ( 0) if a 1 is in 64 bit
 */
static inline __v2di flipmask (__v2di outflank) {
	return _mm_cmpeq_epi32(_mm_shuffle_epi32(outflank, SWAP32), outflank);
}

/**
 * Compute flipped discs when playing on square pos.
 *
 * @param pos player's move.
 * @param P player's disc pattern.
 * @param O opponent's disc pattern.
 * @return flipped disc pattern.
 */
unsigned long long flip(int pos, const unsigned long long P, const unsigned long long O)
{
	__v2di	outflank17, outflank89, PP, OO;
	unsigned long long flipped, outflankr1, outflankr7, outflankr8, outflankr9;
	static const V2DI minusone = { -1LL, -1LL };

	outflankr1 = (0x8000000000000000ULL >> count_opp_reverse(O, masko[pos][0], maskr[pos][0])) & P;
	flipped  = (-outflankr1 * 2) & maskr[pos][0];
	outflankr7 = (0x8000000000000000ULL >> count_opp_reverse(O, masko[pos][1], maskr[pos][1])) & P;
	flipped |= (-outflankr7 * 2) & maskr[pos][1];
	outflankr8 = (0x8000000000000000ULL >> count_opp_reverse(O, masko[pos][2], maskr[pos][2])) & P;
	flipped |= (-outflankr8 * 2) & maskr[pos][2];
	outflankr9 = (0x8000000000000000ULL >> count_opp_reverse(O, masko[pos][3], maskr[pos][3])) & P;
	flipped |= (-outflankr9 * 2) & maskr[pos][3];

	PP = _mm_set1_epi64x(P);
	OO = _mm_set1_epi64x(O);
	outflank17 = ~maskl[pos][0].v2 & ((OO | maskl[pos][0].v2) - minusone.v2) & PP;
	outflank89 = ~maskl[pos][1].v2 & ((OO | maskl[pos][1].v2) - minusone.v2) & PP;
	outflank17 = ~maskl[pos][0].v2 & (outflank17 - (flipmask(outflank17) - minusone.v2));
	outflank89 = ~maskl[pos][1].v2 & (outflank89 - (flipmask(outflank89) - minusone.v2));
	outflank17 |= outflank89;
	outflank17 |= _mm_shuffle_epi32(outflank17, SWAP64);

	return flipped | outflank17[0];
}
