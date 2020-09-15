#include "../include/hw3.h"

#include "../include/util/macro.h"
#include "../include/util/result.h"
#include "../include/util/guard.h"
#include "../include/util/thread.h"
#include "../include/util/wget.h"

#include "../include/WordSearcher.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

struct CountWordOccurrencesThreadStartArg {
    WordSearcher wordSearcher;
    char const *text;

    unsigned int *returnWordCountPtr;
};

struct HW3WordData {
    char const *word;
    WordSearcher wordSearcher;
    unsigned int wordCount;
    struct CountWordOccurrencesThreadStartArg countWordOccurrencesThreadStartArg;
    pthread_t countWordOccurrencesThreadId;
};

static void *countWordOccurrencesThreadStart(void *argAsVoidPtr);

DEFINE_VOID_RESULT(HW3Result, int)

/**
 * Run CSCI 451 HW3. This downloads the text from the given URL, counts the occurrences of the words "easy" and "polar"
 * in the text in separate threads, and prints the counts to stdout.
 *
 * @param textUrl The URL where the text is hosted.
 *                Example: http://undcemcs01.und.edu/~ronald.marsh/CLASS/CS451/hw3-data.txt
 *
 * @returns A result where failure indicates a wget error. The failure error contains the wget exit code. The caller is
 *          is responsible for freeing the result.
 */
HW3Result hw3(char const * const textUrl) {
    guardNotNull(textUrl, "textUrl", "hw3");

    WgetGetStringResult const getTextResult = wgetGetString(textUrl);
    if (!WgetGetStringResult_isSuccess(getTextResult)) {
        return HW3Result_failure(WgetGetStringResult_getErrorAndDestroy(getTextResult));
    }
    char * const text = WgetGetStringResult_getValueAndDestroy(getTextResult);

    static char const * const words[] = {
        "easy",
        "polar"
    };

    struct HW3WordData wordDatas[ARRAY_LENGTH(words)];
    for (size_t i = 0; i < ARRAY_LENGTH(words); i += 1) {
        char const * const word = words[i];
        struct HW3WordData * const wordDataPtr = &wordDatas[i];

        wordDataPtr->word = word;

        wordDataPtr->wordSearcher = WordSearcher_create(word, true);

        wordDataPtr->countWordOccurrencesThreadStartArg.wordSearcher = wordDataPtr->wordSearcher;
        wordDataPtr->countWordOccurrencesThreadStartArg.text = text;
        wordDataPtr->countWordOccurrencesThreadStartArg.returnWordCountPtr = &wordDataPtr->wordCount;

        wordDataPtr->countWordOccurrencesThreadId = safePthreadCreate(
            NULL,
            countWordOccurrencesThreadStart,
            &wordDataPtr->countWordOccurrencesThreadStartArg,
            "hw3"
        );
    }

    for (size_t i = 0; i < ARRAY_LENGTH(words); i += 1) {
        struct HW3WordData * const wordDataPtr = &wordDatas[i];

        safePthreadJoin(wordDataPtr->countWordOccurrencesThreadId, "hw3");
        WordSearcher_destroy(wordDataPtr->wordSearcher);
    }

    free(text);

    for (size_t i = 0; i < ARRAY_LENGTH(words); i += 1) {
        struct HW3WordData * const wordDataPtr = &wordDatas[i];

        printf("\"%s\" occurrence count: %u\n", wordDataPtr->word, wordDataPtr->wordCount);
    }

    return HW3Result_success();
}

static void *countWordOccurrencesThreadStart(void * const argAsVoidPtr) {
    struct CountWordOccurrencesThreadStartArg * const argPtr = (
        (struct CountWordOccurrencesThreadStartArg *)argAsVoidPtr
    );

    unsigned int const wordCount = WordSearcher_countOccurrences(argPtr->wordSearcher, argPtr->text);

    *argPtr->returnWordCountPtr = wordCount;
    return NULL;
}
