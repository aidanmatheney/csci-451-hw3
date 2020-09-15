#pragma once

#include <stdbool.h>

struct WordSearcher;
typedef struct WordSearcher *WordSearcher;
typedef struct WordSearcher * const ConstWordSearcher;

WordSearcher WordSearcher_create(char const *regexPattern, bool ignoreCase);
void WordSearcher_destroy(WordSearcher wordSearcher);

unsigned int WordSearcher_countOccurrences(ConstWordSearcher wordSearcher, char const *text);
