#include <ctype.h>

static int is_subset(const char *needle, const char *haystack){
	while(*needle){
		if(!*haystack)
			return 0;
		while(*haystack && tolower(*needle) == tolower(*haystack++))
			needle++;
	}
	return 1;
}

double match(const char *needle, const char *haystack){
	if(!is_subset(needle, haystack)){
		return 0.0;
	}
	return 1.0;
}
