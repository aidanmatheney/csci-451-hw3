#include "../include/WordSearcher.h"

#include "../include/util/regex.h"
#include "../include/util/memory.h"
#include "../include/util/guard.h"

#include <stdbool.h>
#include <regex.h>

/**
 * Represents an object capable of searching for words in text.
 */
struct WordSearcher {
    regex_t *wordRegex;
};

/**
 * Create a WordSearcher.
 *
 * @param regexPattern The regular expression pattern (regcomp with REG_EXTENDED flag) for the word to search for.
 * @param ignoreCase Whether to ignore case when searching for the word.
 *
 * @returns The newly allocated WordSearcher. The caller is responsible for freeing this memory.
 */
WordSearcher WordSearcher_create(char const * const regexPattern, bool const ignoreCase) {
    guardNotNull(regexPattern, "regexPattern", "WordSearcher_create");

    WordSearcher const wordSearcher = safeMalloc(sizeof *wordSearcher, "WordSearcher_create");

    int regexFlags = REG_EXTENDED;
    if (ignoreCase) {
        regexFlags |= REG_ICASE;
    }
    wordSearcher->wordRegex = safeRegcomp(
        regexPattern,
        regexFlags,
        "WordSearcher_create"
    );

    return wordSearcher;
}

/**
 * Free the memory associated with the WordSearcher.
 *
 * @param wordSearcher The WordSearcher instance.
 */
void WordSearcher_destroy(WordSearcher const wordSearcher) {
    guardNotNull(wordSearcher, "wordSearcher", "WordSearcher_destroy");

    regfree(wordSearcher->wordRegex);
    free(wordSearcher);
}

/**
 * Count the number of occurrences of the word in the given text.
 *
 * @param wordSearcher The WordSearcher instance.
 * @param text The text to search in.
 *
 * @returns The number of occurrences.
 */
unsigned int WordSearcher_countOccurrences(ConstWordSearcher const wordSearcher, char const * const text) {
    guardNotNull(wordSearcher, "wordSearcher", "WordSearcher_countOccurrences");
    guardNotNull(text, "text", "WordSearcher_countOccurrences");

    unsigned int occurrenceCount = 0;
    char const *unparsedText = text;
    regmatch_t regexMatch;
    while (regexec(wordSearcher->wordRegex, unparsedText, 1, &regexMatch, 0) == 0) {
        regoff_t const matchEndOffset = regexMatch.rm_eo;

        occurrenceCount += 1;

        unparsedText += matchEndOffset;
    }

    return occurrenceCount;
}
