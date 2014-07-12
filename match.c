#include <ctype.h>

double match(const char *needle, const char *haystack){
	while(*needle){
		if(!*haystack)
			return 0.0;
		while(*haystack && tolower(*needle) == tolower(*haystack++)){
			needle++;
		}
	}
	return 1.0;
}
