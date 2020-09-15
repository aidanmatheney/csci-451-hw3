#include "../include/hw3.h"

#include "../include/util/result.h"
#include "../include/util/guard.h"
#include "../include/util/wget.h"

#include "../include/WordSearcher.h"

#include <stdlib.h>
#include <stdio.h>

DEFINE_VOID_RESULT(HW3Result, int)

/**
 * Run CSCI 451 HW3. This downloads the text from the given URL, and then, in separate threads, counts the occurrences
 * of the words "easy" and "polar" and prints the counts to stdout.
 *
 * @param textUrl The URL where the text is hosted.
 *                Example: http://undcemcs01.und.edu/~ronald.marsh/CLASS/CS451/hw3-data.txt
 *
 * @returns A result where failure indicates a wget error. The failure error contains the wget exit code. The caller is
 *          is responsible for freeing the result.
 */
HW3Result hw3(char const * const textUrl) {
    WgetGetStringResult const getTextResult = wgetGetString(textUrl);
    if (!WgetGetStringResult_isSuccess(getTextResult)) {
        return HW3Result_failure(WgetGetStringResult_getErrorAndDestroy(getTextResult));
    }
    char * const text = WgetGetStringResult_getValueAndDestroy(getTextResult);

    WordSearcher const easySearcher = WordSearcher_create("easy", true);
    unsigned int const easyCount = WordSearcher_countOccurrences(easySearcher, text);
    WordSearcher_destroy(easySearcher);

    WordSearcher const polarSearcher = WordSearcher_create("polar", true);
    unsigned int const polarCount = WordSearcher_countOccurrences(polarSearcher, text);
    WordSearcher_destroy(polarSearcher);

    free(text);

    printf("\"easy\" occurrence count: %u\n\"polar\" occurrence count: %u\n", easyCount, polarCount);

    return HW3Result_success();
}
