#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <float.h>

#include "fzy.h"

int has_match(const char *needle, const char *haystack){
	while(*needle){
		if(!*haystack)
			return 0;
		while(*haystack && tolower(*needle) == tolower(*haystack++))
			needle++;
	}
	return 1;
}

#define max(a, b) (((a) > (b)) ? (a) : (b))
typedef double score_t;
#define SCORE_MAX DBL_MAX
#define SCORE_MIN -DBL_MAX

/* print one of the internal matrices */
void mat_print(score_t *mat, int n, int m){
	int i, j;
	for(i = 0; i < n; i++){
		for(j = 0; j < m; j++){
			score_t val = mat[i*m + j];
			if(val == SCORE_MIN){
				fprintf(stderr, " -inf");
			}else{
				fprintf(stderr, " %.2f", val);
			}
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n\n");
}

double calculate_score(const char *needle, const char *haystack, size_t *positions){
	if(!*haystack || !*needle)
		return SCORE_MIN;

	int n = strlen(needle);
	int m = strlen(haystack);

	if(m > 1024){
		/*
		 * Unreasonably large candidate: return no score
		 * If it is a valid match it will still be returned, it will
		 * just be ranked below any reasonably sized candidates
		 */
		return 0;
	}

	score_t match_bonus[m];
	score_t D[n][m], M[n][m];
	bzero(D, sizeof(D));
	bzero(M, sizeof(M));

	/*
	 * D[][] Stores the best score for this position ending with a match.
	 * M[][] Stores the best possible score at this position.
	 */

	/* Which positions are beginning of words */
	int at_bow = 1;
	char last_ch = '\0';
	for(int i = 0; i < m; i++){
		char ch = haystack[i];
		/* TODO: What about allcaps (ex. README) */
		int bow = (at_bow && isalnum(ch)) || (isupper(ch) && !isupper(last_ch));
		at_bow = !isalnum(ch);
		last_ch = ch;

		match_bonus[i] = bow ? 1.5 : 0;
	}

	for(int i = 0; i < n; i++){
		for(int j = 0; j < m; j++){
			D[i][j] = SCORE_MIN;
			int match = tolower(needle[i]) == tolower(haystack[j]);
			if(match){
				score_t score = 0;
				if(i && j){
					score = M[i-1][j-1] + match_bonus[j];

					/* consecutive match, doesn't stack with match_bonus */
					score = max(score, 1 + D[i-1][j-1]);
				}
				M[i][j] = D[i][j] = score;
			}
			if(j)
				M[i][j] = max(M[i][j], M[i][j-1] - 0.05);
		}
	}

#if 0
	mat_print(&D[0][0], n, m);
	mat_print(&M[0][0], n, m);
#endif

	/* backtrace to find the positions of optimal matching */
	if(positions){
		for(int i = n-1, j = m-1; i >= 0; i--){
			for(; j >= 0; j--){
				/*
				 * There may be multiple paths which result in
				 * the optimal weight.
				 *
				 * For simplicity, we will pick the first one
				 * we encounter, the latest in the candidate
				 * string.
				 */
				if(D[i][j] == M[i][j]){
					positions[i] = j;
					break;
				}
			}
		}
	}

	return (float)(M[n-1][m-1]) / (float)(n * 2 + 1);
}

double match_positions(const char *needle, const char *haystack, size_t *positions){
	if(!*needle){
		return SCORE_MAX;
	}else if(!has_match(needle, haystack)){
		return SCORE_MIN;
	}else if(!strcasecmp(needle, haystack)){
		if(positions){
			int n = strlen(needle);
			for(int i = 0; i < n; i++)
				positions[i] = i;
		}
		return SCORE_MAX;
	}else{
		return calculate_score(needle, haystack, positions);
	}
}

double match(const char *needle, const char *haystack){
	return match_positions(needle, haystack, NULL);
}
