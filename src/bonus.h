#ifndef BONUS_H
#define BONUS_H BONUS_H

#include "../config.h"

const score_t bonus_states[3][256] = {
	{ 0 },
	{
		['/'] = SCORE_MATCH_SLASH,
		['-'] = SCORE_MATCH_WORD,
		['_'] = SCORE_MATCH_WORD,
		[' '] = SCORE_MATCH_WORD,
		['.'] = SCORE_MATCH_DOT,
	},
	{
		['/'] = SCORE_MATCH_SLASH,
		['-'] = SCORE_MATCH_WORD,
		['_'] = SCORE_MATCH_WORD,
		[' '] = SCORE_MATCH_WORD,
		['.'] = SCORE_MATCH_DOT,

		/* ['a' ... 'z'] = SCORE_MATCH_CAPITAL, */
		['a'] = SCORE_MATCH_CAPITAL,
		['b'] = SCORE_MATCH_CAPITAL,
		['c'] = SCORE_MATCH_CAPITAL,
		['d'] = SCORE_MATCH_CAPITAL,
		['e'] = SCORE_MATCH_CAPITAL,
		['f'] = SCORE_MATCH_CAPITAL,
		['g'] = SCORE_MATCH_CAPITAL,
		['h'] = SCORE_MATCH_CAPITAL,
		['i'] = SCORE_MATCH_CAPITAL,
		['j'] = SCORE_MATCH_CAPITAL,
		['k'] = SCORE_MATCH_CAPITAL,
		['l'] = SCORE_MATCH_CAPITAL,
		['m'] = SCORE_MATCH_CAPITAL,
		['n'] = SCORE_MATCH_CAPITAL,
		['o'] = SCORE_MATCH_CAPITAL,
		['p'] = SCORE_MATCH_CAPITAL,
		['q'] = SCORE_MATCH_CAPITAL,
		['r'] = SCORE_MATCH_CAPITAL,
		['s'] = SCORE_MATCH_CAPITAL,
		['t'] = SCORE_MATCH_CAPITAL,
		['u'] = SCORE_MATCH_CAPITAL,
		['v'] = SCORE_MATCH_CAPITAL,
		['w'] = SCORE_MATCH_CAPITAL,
		['x'] = SCORE_MATCH_CAPITAL,
		['y'] = SCORE_MATCH_CAPITAL,
		['z'] = SCORE_MATCH_CAPITAL,
	},
};

const size_t bonus_index[256] = {
	/* ['A' ... 'Z'] = 2 */
	['A'] = 2,
	['B'] = 2,
	['C'] = 2,
	['D'] = 2,
	['E'] = 2,
	['F'] = 2,
	['G'] = 2,
	['H'] = 2,
	['I'] = 2,
	['J'] = 2,
	['K'] = 2,
	['L'] = 2,
	['M'] = 2,
	['N'] = 2,
	['O'] = 2,
	['P'] = 2,
	['Q'] = 2,
	['R'] = 2,
	['S'] = 2,
	['T'] = 2,
	['U'] = 2,
	['V'] = 2,
	['W'] = 2,
	['X'] = 2,
	['Y'] = 2,
	['Z'] = 2,

	/* ['a' ... 'z'] = 1 */
	['a'] = 1,
	['b'] = 1,
	['c'] = 1,
	['d'] = 1,
	['e'] = 1,
	['f'] = 1,
	['g'] = 1,
	['h'] = 1,
	['i'] = 1,
	['j'] = 1,
	['k'] = 1,
	['l'] = 1,
	['m'] = 1,
	['n'] = 1,
	['o'] = 1,
	['p'] = 1,
	['q'] = 1,
	['r'] = 1,
	['s'] = 1,
	['t'] = 1,
	['u'] = 1,
	['v'] = 1,
	['w'] = 1,
	['x'] = 1,
	['y'] = 1,
	['z'] = 1,

	/* ['0' ... '9'] = 1 */
	['0'] = 1,
	['1'] = 1,
	['2'] = 1,
	['3'] = 1,
	['4'] = 1,
	['5'] = 1,
	['6'] = 1,
	['7'] = 1,
	['8'] = 1,
	['9'] = 1
};

#define COMPUTE_BONUS(last_ch, ch) (bonus_states[bonus_index[(size_t)(ch)]][(size_t)(last_ch)])

#endif
