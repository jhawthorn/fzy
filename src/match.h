#ifndef MATCH_H
#define MATCH_H MATCH_H

typedef double score_t;
#define SCORE_MAX INFINITY
#define SCORE_MIN -INFINITY

int has_match(const char *needle, const char *haystack);
score_t match_positions(const char *needle, const char *haystack, size_t *positions);
score_t match(const char *needle, const char *haystack);

#endif
