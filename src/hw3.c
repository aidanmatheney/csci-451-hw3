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

struct PrintWordCountThreadStartArg {
    char const *word;
    WordSearcher wordSearcher;
    char const *text;
};

struct HW3ThreadInfo {
    WordSearcher wordSearcher;
    struct PrintWordCountThreadStartArg printWordCountThreadStartArg;
    pthread_t printWordCountThreadId;
};

static void *printWordCountThreadStart(void *argAsVoidPtr);

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

    struct HW3ThreadInfo threadInfos[ARRAY_LENGTH(words)];
    for (size_t i = 0; i < ARRAY_LENGTH(words); i += 1) {
        char const * const word = words[i];
        struct HW3ThreadInfo * const threadInfoPtr = &threadInfos[i];

        threadInfoPtr->wordSearcher = WordSearcher_create(word, true);

        threadInfoPtr->printWordCountThreadStartArg.word = word;
        threadInfoPtr->printWordCountThreadStartArg.wordSearcher = threadInfoPtr->wordSearcher;
        threadInfoPtr->printWordCountThreadStartArg.text = text;

        threadInfoPtr->printWordCountThreadId = safePthreadCreate(
            NULL,
            printWordCountThreadStart,
            &threadInfoPtr->printWordCountThreadStartArg,
            "hw3"
        );
    }

    for (size_t i = 0; i < ARRAY_LENGTH(words); i += 1) {
        struct HW3ThreadInfo * const threadInfoPtr = &threadInfos[i];

        safePthreadJoin(threadInfoPtr->printWordCountThreadId, "hw3");
        WordSearcher_destroy(threadInfoPtr->wordSearcher);
    }

    free(text);

    return HW3Result_success();
}

static void *printWordCountThreadStart(void * const argAsVoidPtr) {
    struct PrintWordCountThreadStartArg * const argPtr = argAsVoidPtr;

    unsigned int const wordCount = WordSearcher_countOccurrences(argPtr->wordSearcher, argPtr->text);
    printf("\"%s\" count: %u\n", argPtr->word, wordCount);

    return NULL;
}
