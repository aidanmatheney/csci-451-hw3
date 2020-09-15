/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW3
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void abortWithError(char const *errorMessage);
void abortWithErrorFmt(char const *errorMessageFormat, ...);
void abortWithErrorFmtVA(char const *errorMessageFormat, va_list errorMessageFormatArgs);

/**
 * Turn the given macro token into a string literal.
 *
 * @param macroToken The macro token.
 *
 * @returns The string literal.
 */
#define STRINGIFY(macroToken) #macroToken

/**
 * Get the length of the given compile-time array.
 *
 * @param array The array.
 *
 * @returns The length number literal.
 */
#define ARRAY_LENGTH(array) (sizeof (array) / sizeof (array)[0])

/**
 * Get a stack-allocated mutable string from the given string literal.
 *
 * @param stringLiteral The string literal.
 *
 * @returns The `char [length + 1]`-typed stack-allocated string.
*/
#define MUTABLE_STRING(stringLiteral) ((char [ARRAY_LENGTH(stringLiteral)]){ stringLiteral })

void *safeMalloc(size_t size, char const *callerDescription);
void *safeRealloc(void *memory, size_t newSize, char const *callerDescription);

/**
 * Declare (.h file) a generic Result class which holds no success value but can hold a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DECLARE_VOID_RESULT(TResult, TError) \
    struct TResult; \
    typedef struct TResult * TResult; \
    typedef struct TResult const * Const##TResult; \
    \
    TResult TResult##_success(void); \
    TResult TResult##_failure(TError error); \
    void TResult##_destroy(TResult result); \
    \
    bool TResult##_isSuccess(Const##TResult result); \
    TError TResult##_getError(Const##TResult result); \
    TError TResult##_getErrorAndDestroy(TResult result);

/**
 * Define (.c file) a generic Result class which holds no success value but can hold a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DEFINE_VOID_RESULT(TResult, TError) \
    DECLARE_VOID_RESULT(TResult, TError) \
    \
    struct TResult { \
        bool success; \
        TError error; \
    }; \
    \
    TResult TResult##_success(void) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = true; \
        return result; \
    } \
    \
    TResult TResult##_failure(TError const error) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = false; \
        result->error = error; \
        return result; \
    } \
    \
    void TResult##_destroy(TResult const result) { \
        free(result); \
    } \
    \
    bool TResult##_isSuccess(Const##TResult const result) { \
        return result->success; \
    } \
    \
    TError TResult##_getError(Const##TResult const result) { \
        if (result->success) { \
            abortWithErrorFmt("%s: Cannot get result error. Result is success", STRINGIFY(TResult##_getError)); \
        } \
        \
        return result->error; \
    } \
    \
    TError TResult##_getErrorAndDestroy(TResult const result) { \
        TError const error = TResult##_getError(result); \
        TResult##_destroy(result); \
        return error; \
    }

/**
 * Declare (.h file) a generic Result class which can hold either a sucess value or a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DECLARE_RESULT(TResult, TValue, TError) \
    struct TResult; \
    typedef struct TResult * TResult; \
    typedef struct TResult const * Const##TResult; \
    \
    TResult TResult##_success(TValue value); \
    TResult TResult##_failure(TError error); \
    void TResult##_destroy(TResult result); \
    \
    bool TResult##_isSuccess(Const##TResult result); \
    TValue TResult##_getValue(Const##TResult result); \
    TValue TResult##_getValueAndDestroy(TResult result); \
    TError TResult##_getError(Const##TResult result); \
    TError TResult##_getErrorAndDestroy(TResult result);

/**
 * Define (.c file) a generic Result class which can hold either a sucess value or a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DEFINE_RESULT(TResult, TValue, TError) \
    DECLARE_RESULT(TResult, TValue, TError) \
    \
    struct TResult { \
        bool success; \
        TValue value; \
        TError error; \
    }; \
    \
    TResult TResult##_success(TValue const value) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = true; \
        result->value = value; \
        return result; \
    } \
    \
    TResult TResult##_failure(TError const error) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = false; \
        result->error = error; \
        return result; \
    } \
    \
    void TResult##_destroy(TResult const result) { \
        free(result); \
    } \
    \
    bool TResult##_isSuccess(Const##TResult const result) { \
        return result->success; \
    } \
    \
    TValue TResult##_getValue(Const##TResult const result) { \
        if (!result->success) { \
            abortWithErrorFmt("%s: Cannot get result value. Result is failure", STRINGIFY(TResult##_getValue)); \
        } \
        \
        return result->value; \
    } \
    \
    TValue TResult##_getValueAndDestroy(TResult const result) { \
        TValue const value = TResult##_getValue(result); \
        TResult##_destroy(result); \
        return value; \
    } \
    \
    TError TResult##_getError(Const##TResult const result) { \
        if (result->success) { \
            abortWithErrorFmt("%s: Cannot get result error. Result is success", STRINGIFY(TResult##_getError)); \
        } \
        \
        return result->error; \
    } \
    \
    TError TResult##_getErrorAndDestroy(TResult const result) { \
        TError const error = TResult##_getError(result); \
        TResult##_destroy(result); \
        return error; \
    }

void guard(bool expression, char const *errorMessage);
void guardFmt(bool expression, char const *errorMessageFormat, ...);
void guardFmtVA(bool expression, char const *errorMessageFormat, va_list errorMessageFormatArgs);

void guardNotNull(void const *object, char const *paramName, char const *callerName);

#define DECLARE_ENUMERATOR(TEnumerator, TItem) \
    struct TEnumerator; \
    typedef struct TEnumerator * TEnumerator; \
    typedef struct TEnumerator const * Const##TEnumerator; \
    \
    void TEnumerator##_destroy(TEnumerator enumerator); \
    \
    bool TEnumerator##_moveNext(TEnumerator enumerator); \
    TItem TEnumerator##_current(Const##TEnumerator enumerator); \
    void TEnumerator##_reset(TEnumerator enumerator);

/**
 * Declare (.h file) a generic ListEnumerator class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DECLARE_LIST_ENUMERATOR(TList, TItem) \
    DECLARE_ENUMERATOR(TList##Enumerator, TItem) \
    \
    TList##Enumerator TList##Enumerator##_create(Const##TList list, int direction);

/**
 * Define (.c file) a generic ListEnumerator class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DEFINE_LIST_ENUMERATOR(TList, TItem) \
    DECLARE_LIST_ENUMERATOR(TList, TItem) \
    \
    struct TList##Enumerator { \
        Const##TList list; \
        int direction; \
        int currentIndex; \
    }; \
    \
    TList##Enumerator TList##Enumerator##_create(Const##TList const list, int const direction) { \
        guardNotNull(list, "list", STRINGIFY(TList##Enumerator##_create)); \
        \
        TList##Enumerator const enumerator = safeMalloc(sizeof *enumerator, STRINGIFY(TList##Enumerator##_create)); \
        enumerator->list = list; \
        enumerator->direction = direction; \
        enumerator->currentIndex = direction == 1 ? -1 : (int)TList##_count(list); \
        return enumerator; \
    } \
    \
    void TList##Enumerator##_destroy(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_destroy)); \
        free(enumerator); \
    } \
    \
    bool TList##Enumerator##_moveNext(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_moveNext)); \
        \
        enumerator->currentIndex += enumerator->direction; \
        return enumerator->currentIndex >= 0 && enumerator->currentIndex < (int)TList##_count(enumerator->list); \
    } \
    \
    TItem TList##Enumerator##_current(Const##TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_current)); \
        \
        if (enumerator->currentIndex < 0 || enumerator->currentIndex >= (int)TList##_count(enumerator->list)) { \
            abortWithErrorFmt( \
                "%s: Cannot get current item when current index (%d) is out of range (list count: %zu)", \
                STRINGIFY(TList##Enumerator##_current), \
                enumerator->currentIndex, \
                TList##_count(enumerator->list) \
            ); \
        } \
        \
        return TList##_get(enumerator->list, (size_t)enumerator->currentIndex); \
    } \
    \
    void TList##Enumerator##_reset(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_reset)); \
        enumerator->currentIndex = enumerator->direction == 1 ? -1 : (int)TList##_count(enumerator->list); \
    }

/**
 * Declare (.h file) a typedef to a function pointer for a function with the specified args that returns void.
 *
 * @param TAction The name of the new type.
 * @param ... The function's parameter types.
 */
#define DECLARE_ACTION(TAction, ...) typedef void (*TAction)(__VA_ARGS__);

/**
 * Declare (.h file) a typedef to a function pointer for a function with the specified args and return type.
 *
 * @param TFunc The name of the new type.
 * @param TResult The function's return type.
 * @param ... The function's parameter types.
 */
#define DECLARE_FUNC(TFunc, TResult, ...) typedef TResult (*TFunc)(__VA_ARGS__);

/**
 * Declare (.h file) a generic List class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DECLARE_LIST(TList, TItem) \
    struct TList; \
    typedef struct TList * TList; \
    typedef struct TList const * Const##TList; \
    \
    DECLARE_ACTION(TList##ForEachCallback, void *, size_t, TItem) \
    DECLARE_FUNC(TList##FindCallback, bool, void *, size_t, TItem) \
    DECLARE_RESULT(TList##FindItemResult, TItem, void *) \
    \
    DECLARE_LIST_ENUMERATOR(TList, TItem) \
    \
    TList TList##_create(void); \
    TList TList##_fromItems(TItem const *items, size_t count); \
    TList TList##_fromList(Const##TList list); \
    void TList##_destroy(TList list); \
    \
    TItem const *TList##_items(Const##TList list); \
    size_t TList##_count(Const##TList list); \
    \
    TItem TList##_get(Const##TList list, size_t index); \
    \
    void TList##_add(TList list, TItem item); \
    void TList##_addMany(TList list, TItem const *items, size_t count); \
    void TList##_insert(TList list, size_t index, TItem item); \
    void TList##_insertMany(TList list, size_t index, TItem const *items, size_t count); \
    void TList##_set(TList list, size_t index, TItem item); \
    \
    void TList##_removeAt(TList list, size_t index); \
    void TList##_removeManyAt(TList list, size_t startIndex, size_t count); \
    void TList##_clear(TList list); \
    \
    void TList##_forEach(Const##TList list, void *state, TList##ForEachCallback callback); \
    void TList##_forEachReverse(Const##TList list, void *state, TList##ForEachCallback callback); \
    bool TList##_has(Const##TList list, TItem item); \
    size_t TList##_indexOf(Const##TList list, TItem item); \
    size_t TList##_lastIndexOf(Const##TList list, TItem item); \
    bool TList##_findHas(Const##TList list, void *state, TList##FindCallback callback); \
    TList##FindItemResult TList##_find(Const##TList list, void *state, TList##FindCallback callback); \
    size_t TList##_findIndex(Const##TList list, void *state, TList##FindCallback callback); \
    TList##FindItemResult TList##_findLast(Const##TList list, void *state, TList##FindCallback callback); \
    size_t TList##_findLastIndex(Const##TList list, void *state, TList##FindCallback callback); \
    \
    TList##Enumerator TList##_enumerate(Const##TList list); \
    TList##Enumerator TList##_enumerateReverse(Const##TList list); \
    \
    void TList##_fillArray(Const##TList list, TItem *array, size_t startIndex, size_t count);

/**
 * Define (.c file) a generic List class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DEFINE_LIST(TList, TItem) \
    DECLARE_LIST(TList, TItem) \
    \
    static void TList##_ensureCapacity(TList list, size_t targetCapacity); \
    static void TList##_guardIndexInRange(Const##TList list, size_t index, char const *callerName); \
    static void TList##_guardIndexInInsertRange(Const##TList list, size_t index, char const *callerName); \
    static void TList##_guardStartIndexAndCountInRange(Const##TList list, size_t startIndex, size_t count, char const *callerName); \
    \
    DEFINE_RESULT(TList##FindItemResult, TItem, void *) \
    \
    DEFINE_LIST_ENUMERATOR(TList, TItem) \
    \
    struct TList { \
        TItem *items; \
        size_t count; \
        size_t capacity; \
    }; \
    \
    TList TList##_create(void) { \
        TList const list = safeMalloc(sizeof *list, STRINGIFY(TList##_create)); \
        list->items = NULL; \
        list->count = 0; \
        list->capacity = 0; \
        return list; \
    } \
    \
    TList TList##_fromItems(TItem const * const items, size_t const count) { \
        guardNotNull(items, "items", STRINGIFY(TList##_fromItems)); \
        \
        TList const list = TList##_create(); \
        TList##_addMany(list, items, count); \
        return list; \
    } \
    \
    TList TList##_fromList(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_fromList)); \
        return TList##_fromItems(list->items, list->count); \
    } \
    \
    void TList##_destroy(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_destroy)); \
        \
        free(list->items); \
        free(list); \
    } \
    \
    TItem const *TList##_items(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_items)); \
        return list->items; \
    } \
    \
    size_t TList##_count(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_count)); \
        return list->count; \
    } \
    \
    TItem TList##_get(Const##TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_get)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_get)); \
        \
        TItem const item = list->items[index]; \
        return item; \
    } \
    \
    void TList##_add(TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_add)); \
        \
        TList##_ensureCapacity(list, list->count + 1); \
        list->items[list->count] = item; \
        list->count += 1; \
    } \
    \
    void TList##_addMany(TList const list, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_addMany)); \
        guardNotNull(items, "items", STRINGIFY(TList##_addMany)); \
        \
        TList##_ensureCapacity(list, list->count + count); \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            list->items[list->count + i] = item; \
        } \
        list->count += count; \
    } \
    \
    void TList##_insert(TList const list, size_t const index, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insert)); \
        TList##_guardIndexInInsertRange(list, index, STRINGIFY(TList##_insert)); \
        \
        TList##_ensureCapacity(list, list->count + 1); \
        for (int i = (int)list->count - 1; i >= (int)index; i -= 1) { \
            /* Shift each item at an index >= the target index one to the right */ \
            list->items[i + 1] = list->items[i]; \
        } \
        list->items[index] = item; \
        list->count += 1; \
    } \
    \
    void TList##_insertMany(TList const list, size_t const index, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insertMany)); \
        TList##_guardIndexInInsertRange(list, index, STRINGIFY(TList##_insertMany)); \
        guardNotNull(items, "items", STRINGIFY(TList##_insertMany)); \
        \
        TList##_ensureCapacity(list, list->count + count); \
        for (int i = (int)list->count - 1; i >= (int)index; i -= 1) { \
            /* Shift each item at an index >= the target index count to the right */ \
            list->items[(size_t)i + count] = list->items[i]; \
        } \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            list->items[index + i] = item; \
        } \
        list->count += count; \
    } \
    \
    void TList##_set(TList const list, size_t const index, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_set)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_set)); \
        \
        list->items[index] = item; \
    } \
    \
    void TList##_removeAt(TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_removeAt)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_removeAt)); \
        \
        for (size_t i = index; i < list->count; i += 1) { \
            /* Shift each item at an index > the target index one to the left */ \
            list->items[i] = list->items[i + 1]; \
        } \
        list->count -= 1; \
    } \
    \
    void TList##_removeManyAt(TList const list, size_t const startIndex, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_removeManyAt)); \
        TList##_guardStartIndexAndCountInRange(list, startIndex, count, STRINGIFY(TList##_removeManyAt)); \
        \
        for (size_t i = startIndex; i < list->count; i += 1) { \
            /* Shift each item at an index > the start index count to the left */ \
            list->items[i] = list->items[i + count]; \
        } \
        list->count -= count; \
    } \
    \
    void TList##_clear(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_clear)); \
        list->count = 0; \
    } \
    \
    void TList##_forEach(Const##TList const list, void * const state, TList##ForEachCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_forEach)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            callback(state, i, item); \
        } \
    } \
    \
    void TList##_forEachReverse(Const##TList const list, void * const state, TList##ForEachCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_forEachReverse)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            callback(state, (size_t)i, item); \
        } \
    } \
    \
    bool TList##_has(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_has)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return true; \
            } \
        } \
        \
        return false; \
    } \
    \
    size_t TList##_indexOf(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_indexOf)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    size_t TList##_lastIndexOf(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_lastIndexOf)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return (size_t)i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    bool TList##_findHas(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findHas)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return true; \
            } \
        } \
        \
        return false; \
    } \
    \
    TList##FindItemResult TList##_find(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_find)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return TList##FindItemResult_success(item); \
            } \
        } \
        \
        return TList##FindItemResult_failure(NULL); \
    } \
    \
    size_t TList##_findIndex(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findIndex)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    TList##FindItemResult TList##_findLast(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findLast)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, (size_t)i, item); \
            if (found) { \
                return TList##FindItemResult_success(item); \
            } \
        } \
        \
        return TList##FindItemResult_failure(NULL); \
    } \
    \
    size_t TList##_findLastIndex(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findLastIndex)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, (size_t)i, item); \
            if (found) { \
                return (size_t)i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    TList##Enumerator TList##_enumerate(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_enumerate)); \
        return TList##Enumerator_create(list, 1); \
    } \
    \
    TList##Enumerator TList##_enumerateReverse(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_enumerateReverse)); \
        return TList##Enumerator_create(list, -1); \
    } \
    \
    void TList##_fillArray(Const##TList const list, TItem * const array, size_t const startIndex, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_fillArray)); \
        guardNotNull(array, "array", STRINGIFY(TList##_fillArray)); \
        TList##_guardStartIndexAndCountInRange(list, startIndex, count, STRINGIFY(TList##_fillArray)); \
        \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = list->items[i + startIndex]; \
            array[i] = item; \
        } \
    } \
    \
    static void TList##_ensureCapacity(TList const list, size_t const requiredCapacity) { \
        assert(list != NULL); \
        \
        if (requiredCapacity <= list->capacity) { \
            return; \
        } \
        \
        size_t newCapacity = list->capacity == 0 ? 4 : (list->capacity * 2); \
        while (newCapacity < requiredCapacity) { \
            newCapacity *= 2; \
        } \
        \
        list->items = safeRealloc(list->items, sizeof *list->items * newCapacity, STRINGIFY(TList##_ensureCapacity)); \
        list->capacity = newCapacity; \
    } \
    \
    static void TList##_guardIndexInRange(Const##TList const list, size_t const index, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        guardFmt( \
            index < list->count, \
            "%s: Index (%zu) must be in range (count: %zu)", \
            callerName, \
            index, \
            list->count \
        ); \
    } \
    \
    static void TList##_guardIndexInInsertRange(Const##TList const list, size_t const index, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        guardFmt( \
            index <= list->count, \
            "%s: Index (%zu) must be in range (count: %zu) or the next available index", \
            callerName, \
            index, \
            list->count \
        ); \
    } \
    \
    static void TList##_guardStartIndexAndCountInRange(Const##TList const list, size_t const startIndex, size_t const count, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        TList##_guardIndexInRange(list, startIndex, callerName); \
        \
        size_t const endIndex = startIndex + count - 1; \
        guardFmt( \
            endIndex < list->count, \
            "%s: End index (%zu) must be within range (count: %zu)", \
            callerName, \
            endIndex, \
            list->count \
        ); \
    }

DECLARE_VOID_RESULT(WgetResult, int)
WgetResult wget(char const *sourceFileUrl, char const *destinationFilePath);

DECLARE_RESULT(WgetGetStringResult, char *, int)
WgetGetStringResult wgetGetString(char const *sourceFileUrl);

DECLARE_FUNC(PthreadCreateStartRoutine, void *, void *)

pthread_t safePthreadCreate(
    pthread_attr_t const *attributes,
    PthreadCreateStartRoutine startRoutine,
    void *startRoutineArg,
    char const *callerDescription
);

void *safePthreadJoin(pthread_t threadId, char const *callerDescription);

struct StringBuilder;
typedef struct StringBuilder * StringBuilder;
typedef struct StringBuilder const * ConstStringBuilder;

StringBuilder StringBuilder_create(void);
StringBuilder StringBuilder_fromChars(char const *value, size_t count);
StringBuilder StringBuilder_fromString(char const *value);
void StringBuilder_destroy(StringBuilder builder);

char const *StringBuilder_chars(ConstStringBuilder builder);
size_t StringBuilder_length(ConstStringBuilder builder);

void StringBuilder_appendChar(StringBuilder builder, char value);
void StringBuilder_appendChars(StringBuilder builder, char const *value, size_t count);
void StringBuilder_append(StringBuilder builder, char const *value);
void StringBuilder_appendFmt(StringBuilder builder, char const *valueFormat, ...);
void StringBuilder_appendFmtVA(StringBuilder builder, char const *valueFormat, va_list valueFormatArgs);
void StringBuilder_appendLine(StringBuilder builder, char const *value);
void StringBuilder_appendLineFmt(StringBuilder builder, char const *valueFormat, ...);
void StringBuilder_appendLineFmtVA(StringBuilder builder, char const *valueFormat, va_list valueFormatArgs);

void StringBuilder_insertChar(StringBuilder builder, size_t index, char value);
void StringBuilder_insertChars(StringBuilder builder, size_t index, char const *value, size_t count);
void StringBuilder_insert(StringBuilder builder, size_t index, char const *value);
void StringBuilder_insertFmt(StringBuilder builder, size_t index, char const *valueFormat, ...);
void StringBuilder_insertFmtVA(StringBuilder builder, size_t index, char const *valueFormat, va_list valueFormatArgs);

void StringBuilder_removeAt(StringBuilder builder, size_t index);
void StringBuilder_removeManyAt(StringBuilder builder, size_t startIndex, size_t count);

char *StringBuilder_toString(ConstStringBuilder builder);
char *StringBuilder_toStringAndDestroy(StringBuilder builder);

size_t safeSnprintf(
    char *buffer,
    size_t bufferLength,
    char const *format,
    char const *callerDescription,
    ...
);
size_t safeVsnprintf(
    char *buffer,
    size_t bufferLength,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);
size_t safeSprintf(
    char *buffer,
    char const *format,
    char const *callerDescription,
    ...
);
size_t safeVsprintf(
    char *buffer,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);

char *formatString(char const *format, ...);
char *formatStringVA(char const *format, va_list formatArgs);

regex_t *safeRegcomp(char const *pattern, int flags, char const *callerDescription);

char *safeTmpnam(char *filePath, char const *callerDescription);
char *generateTempFilePath(void);

DECLARE_LIST(CharList, char)
DECLARE_LIST(StringList, char *)

FILE *safeFopen(char const *filePath, char const *modes, char const *callerDescription);
bool safeFgets(char *buffer, size_t bufferLength, FILE *file, char const *callerDescription);
char *readAllFileText(char const *filePath);

struct WordSearcher;
typedef struct WordSearcher *WordSearcher;
typedef struct WordSearcher * const ConstWordSearcher;

WordSearcher WordSearcher_create(char const *regexPattern, bool ignoreCase);
void WordSearcher_destroy(WordSearcher wordSearcher);

unsigned int WordSearcher_countOccurrences(ConstWordSearcher wordSearcher, char const *text);

DECLARE_VOID_RESULT(HW3Result, int)

HW3Result hw3(char const *textUrl);

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

/**
 * Abort program execution after printing the specified error message to stderr.
 *
 * @param errorMessage The error message, not terminated by a newline.
 */
void abortWithError(char const * const errorMessage) {
    guardNotNull(errorMessage, "errorMessage", "abortWithError");

    fputs(errorMessage, stderr);
    fputc('\n', stderr);
    abort();
}

/**
 * Abort program execution after formatting and printing the specified error message to stderr.
 *
 * @param errorMessage The error message format (printf), not terminated by a newline.
 * @param ... The error message format arguments (printf).
 */
void abortWithErrorFmt(char const * const errorMessageFormat, ...) {
    va_list errorMessageFormatArgs;
    va_start(errorMessageFormatArgs, errorMessageFormat);
    abortWithErrorFmtVA(errorMessageFormat, errorMessageFormatArgs);
    va_end(errorMessageFormatArgs);
}

/**
 * Abort program execution after formatting and printing the specified error message to stderr.
 *
 * @param errorMessage The error message format (printf), not terminated by a newline.
 * @param errorMessageFormatArgs The error message format arguments (printf).
 */
void abortWithErrorFmtVA(char const * const errorMessageFormat, va_list errorMessageFormatArgs) {
    guardNotNull(errorMessageFormat, "errorMessageFormat", "abortWithErrorFmtVA");

    char * const errorMessage = formatStringVA(errorMessageFormat, errorMessageFormatArgs);
    abortWithError(errorMessage);
    free(errorMessage);
}

/**
 * Open the file using fopen. If the operation fails, abort the program with an error message.
 *
 * @param filePath The file path.
 * @param modes The fopen modes string.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The opened file.
 */
FILE *safeFopen(char const * const filePath, char const * const modes, char const * const callerDescription) {
    guardNotNull(filePath, "filePath", "safeFopen");
    guardNotNull(modes, "modes", "safeFopen");
    guardNotNull(callerDescription, "callerDescription", "safeFopen");

    FILE * const file = fopen(filePath, modes);
    if (file == NULL) {
        int const fopenErrorCode = errno;
        char const * const fopenErrorMessage = strerror(fopenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open file \"%s\" with modes \"%s\" using fopen (error code: %d; error message: \"%s\")",
            callerDescription,
            filePath,
            modes,
            fopenErrorCode,
            fopenErrorMessage
        );
        return NULL;
    }

    return file;
}

/**
 * Read characters from the given file into the given buffer. Stop as soon as one of the following conditions has been
 * met: (A) `bufferLength - 1` characters have been read, (B) a newline is encountered, or (C) the end of the file is
 * reached. The string read into the buffer will end with a terminating character. If the operation fails, abort the
 * program with an error message.
 *
 * @param buffer The buffer into which to read the string.
 * @param bufferLength The length of the buffer.
 * @param file The file to read from.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns Whether unread characters remain.
 */
bool safeFgets(
    char * const buffer,
    size_t const bufferLength,
    FILE * const file,
    char const * const callerDescription
) {
    guardNotNull(buffer, "buffer", "safeFgets");
    guardNotNull(file, "file", "safeFgets");
    guardNotNull(callerDescription, "callerDescription", "safeFgets");

    char * const fgetsResult = fgets(buffer, (int)bufferLength, file);
    bool const fgetsError = ferror(file);
    if (fgetsError) {
        int const fgetsErrorCode = errno;
        char const * const fgetsErrorMessage = strerror(fgetsErrorCode);

        abortWithErrorFmt(
            "%s: Failed to read %zu chars from file using fgets (error code: %d; error message: \"%s\")",
            callerDescription,
            bufferLength,
            fgetsErrorCode,
            fgetsErrorMessage
        );
        return false;
    }

    if (fgetsResult == NULL || feof(file)) {
        return false;
    }

    return true;
}

/**
 * Open a text file, read all the text in the file into a string, and then close the file.
 *
 * @param filePath The path to the file.
 *
 * @returns A string containing all text in the file. The caller is responsible for freeing this memory.
 */
char *readAllFileText(char const * const filePath) {
    guardNotNull(filePath, "filePath", "readAllFileText");

    StringBuilder const fileTextBuilder = StringBuilder_create();

    FILE * const file = safeFopen(filePath, "r", "readAllFileText");
    char fgetsBuffer[100];
    while (safeFgets(fgetsBuffer, 100, file, "readAllFileText")) {
        StringBuilder_append(fileTextBuilder, fgetsBuffer);
    }
    fclose(file);

    char * const fileText = StringBuilder_toStringAndDestroy(fileTextBuilder);

    return fileText;
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessage The error message.
 */
void guard(bool const expression, char const * const errorMessage) {
    if (errorMessage == NULL) {
        abortWithError("guard: errorMessage must not be null");
        return;
    }

    if (expression) {
        return;
    }

    abortWithError(errorMessage);
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessageFormat The error message format (printf).
 * @param ... The error message format arguments (printf).
 */
void guardFmt(bool const expression, char const * const errorMessageFormat, ...) {
    va_list errorMessageFormatArgs;
    va_start(errorMessageFormatArgs, errorMessageFormat);
    guardFmtVA(expression, errorMessageFormat, errorMessageFormatArgs);
    va_end(errorMessageFormatArgs);
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessageFormat The error message format (printf).
 * @param errorMessageFormatArgs The error message format arguments (printf).
 */
void guardFmtVA(bool const expression, char const * const errorMessageFormat, va_list errorMessageFormatArgs) {
    if (errorMessageFormat == NULL) {
        abortWithError("guardFmtVA: errorMessageFormat must not be null");
        return;
    }

    if (expression) {
        return;
    }

    abortWithErrorFmtVA(errorMessageFormat, errorMessageFormatArgs);
}

/**
 * Ensure that the given object supplied by a parameter is not null. If it is null, abort the program with an error
 * message.
 *
 * @param object The object to verify is not null.
 * @param paramName The name of the parameter supplying the object.
 * @param callerName The name of the calling function.
 */
void guardNotNull(void const * const object, char const * const paramName, char const * const callerName) {
    guard(paramName != NULL, "guardNotNull: paramName must not be null");
    guard(callerName != NULL, "guardNotNull: callerName must not be null");

    guardFmt(object != NULL, "%s: %s must not be null", callerName, paramName);
}

DEFINE_LIST(CharList, char)
DEFINE_LIST(StringList, char *)

/**
 * Allocate memory of the given size using malloc. If the allocation fails, abort the program with an error message.
 *
 * @param size The size of the memory, in bytes.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The allocated memory.
 */
void *safeMalloc(size_t const size, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeMalloc");

    void * const memory = malloc(size);
    if (memory == NULL) {
        int const mallocErrorCode = errno;
        char const * const mallocErrorMessage = strerror(mallocErrorCode);

        abortWithErrorFmt(
            "%s: Failed to allocate %zu bytes of memory using malloc (error code: %d; error message: \"%s\")",
            callerDescription,
            size,
            mallocErrorCode,
            mallocErrorMessage
        );
        return NULL;
    }

    return memory;
}

/**
 * Resize the given memory using realloc. If the reallocation fails, abort the program with an error message.
 *
 * @param memory The existing memory, or null.
 * @param newSize The new size of the memory, in bytes.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The reallocated memory.
 */
void *safeRealloc(void * const memory, size_t const newSize, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeRealloc");

    void * const newMemory = realloc(memory, newSize);
    if (newMemory == NULL) {
        int const reallocErrorCode = errno;
        char const * const reallocErrorMessage = strerror(reallocErrorCode);

        abortWithErrorFmt(
            "%s: Failed to reallocate memory to %zu bytes using realloc (error code: %d; error message: \"%s\")",
            callerDescription,
            newSize,
            reallocErrorCode,
            reallocErrorMessage
        );
        return NULL;
    }

    return newMemory;
}

/**
 * Generate a unique file path that does not name a currently existing file.
 *
 * @param filePath Pointer to a character array capable of holding at least L_tmpnam characters, to be used as a result
 *                 buffer. If a null pointer is passed, a pointer to an internal static buffer is returned.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns filePath if filePath was not a null pointer. Otherwise, a pointer to an internal static buffer is returned.
 */
char *safeTmpnam(char * const filePath, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeTmpnam");

    char * const tmpnamResult = tmpnam(filePath);
    if (tmpnamResult == NULL) {
        int const tmpnamErrorCode = errno;
        char const * const tmpnamErrorMessage = strerror(tmpnamErrorCode);

        abortWithErrorFmt(
            "%s: Failed to generate a temporary file path using tmpnam (error code: %d; error message: \"%s\")",
            callerDescription,
            tmpnamErrorCode,
            tmpnamErrorMessage
        );
        return NULL;
    }

    return tmpnamResult;
}

/**
 * Generate a unique file path that does not name a currently existing file.
 *
 * @returns The generated file path. The caller is responsible for freeing this memory.
 */
char *generateTempFilePath(void) {
    char * const filePath = safeMalloc(sizeof *filePath * L_tmpnam, "generateTempFilePath");
    safeTmpnam(filePath, "generateTempFilePath");
    return filePath;
}

/**
 * Compile the given regular expression pattern and flags into a regex object using regcomp. If the compilation fails,
 * abort the program with an error message.
 *
 * @param pattern The regular expression pattern.
 * @param flags The regular expression flags.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 * the calling function, plus extra information if useful.
 *
 * @returns The compiled regex object. The caller is responsible for freeing this memory.
 */
regex_t *safeRegcomp(char const * const pattern, int const flags, char const * const callerDescription) {
    guardNotNull(pattern, "pattern", "safeRegcomp");
    guardNotNull(callerDescription, "callerDescription", "safeRegcomp");

    regex_t * const regex = safeMalloc(sizeof *regex, callerDescription);
    int const regcompErrorCode = regcomp(regex, pattern, flags);
    if (regcompErrorCode != 0) {
        char regcompErrorMessage[100];
        regerror(regcompErrorCode, regex, regcompErrorMessage, 100);

        free(regex);

        abortWithErrorFmt(
            "%s: Failed to compile regular expression /%s/%d using regcomp (error code: %d; error message: \"%s\")",
            callerDescription,
            pattern,
            flags,
            regcompErrorCode,
            regcompErrorMessage
        );
        return NULL;
    }

    return regex;
}

/**
 * If the given buffer is non-null, format the string into the buffer. If the buffer is null, simply calculate the
 * number of characters that would have been written if the buffer had been sufficiently large. If the operation fails,
 * abort the program with an error message.
 *
 * @param buffer The buffer into which to write, or null if only the formatted string length is desired.
 * @param bufferLength The length of the buffer, or 0 if the buffer is null.
 * @param format The string format (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param ... The string format arguments (printf).
 *
 * @returns The number of characters that would have been written if the buffer had been sufficiently large, not
 *          counting the terminating null character.
 */
size_t safeSnprintf(
    char * const buffer,
    size_t const bufferLength,
    char const * const format,
    char const * const callerDescription,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, callerDescription);
    size_t const length = safeVsnprintf(buffer, bufferLength, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return length;
}

/**
 * If the given buffer is non-null, format the string into the buffer. If the buffer is null, simply calculate the
 * number of characters that would have been written if the buffer had been sufficiently large. If the operation fails,
 * abort the program with an error message.
 *
 * @param buffer The buffer into which to write, or null if only the formatted string length is desired.
 * @param bufferLength The length of the buffer, or 0 if the buffer is null.
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of characters that would have been written if the buffer had been sufficiently large, not
 *          counting the terminating null character.
 */
size_t safeVsnprintf(
    char * const buffer,
    size_t const bufferLength,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(format, "format", "safeVsnprintf");
    guardNotNull(callerDescription, "callerDescription", "safeVsnprintf");

    int const vsnprintfResult = vsnprintf(buffer, bufferLength, format, formatArgs);
    if (vsnprintfResult < 0) {
        abortWithErrorFmt(
            "%s: Failed to format string using vsnprintf (format: \"%s\"; result: %d)",
            callerDescription,
            format,
            vsnprintfResult
        );
        return (size_t)-1;
    }

    return (size_t)vsnprintfResult;
}

/**
 * Format the string into the buffer. If the operation fails, abort the program with an error message.
 *
 * @param buffer The buffer into which to write.
 * @param format The string format (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param ... The string format arguments (printf).
 *
 * @returns The number of characters written.
 */
size_t safeSprintf(
    char * const buffer,
    char const * const format,
    char const * const callerDescription,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, callerDescription);
    size_t const length = safeVsprintf(buffer, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return length;
}

/**
 * Format the string into the buffer. If the operation fails, abort the program with an error message.
 *
 * @param buffer The buffer into which to write.
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of characters written.
 */
size_t safeVsprintf(
    char * const buffer,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(buffer, "buffer", "safeVsprintf");
    guardNotNull(format, "format", "safeVsprintf");
    guardNotNull(callerDescription, "callerDescription", "safeVsprintf");

    int const vsprintfResult = vsprintf(buffer, format, formatArgs);
    if (vsprintfResult < 0) {
        abortWithErrorFmt(
            "%s: Failed to format string using vsprintf (format: \"%s\"; result: %d)",
            callerDescription,
            format,
            vsprintfResult
        );
        return (size_t)-1;
    }

    return (size_t)vsprintfResult;
}

/**
 * Create a string using the specified format and format args.
 *
 * @param format The string format (printf).
 * @param ... The string format arguments (printf).
 *
 * @returns The formatted string. The caller is responsible for freeing the memory.
 */
char *formatString(char const * const format, ...) {
    va_list formatArgs;
    va_start(formatArgs, format);
    char * const formattedString = formatStringVA(format, formatArgs);
    va_end(formatArgs);
    return formattedString;
}

/**
 * Create a string using the specified format and format args.
 *
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 *
 * @returns The formatted string. The caller is responsible for freeing the memory.
 */
char *formatStringVA(char const * const format, va_list formatArgs) {
    guardNotNull(format, "format", "formatStringVA");

    va_list formatArgsForVsprintf;
    va_copy(formatArgsForVsprintf, formatArgs);

    size_t const formattedStringLength = safeVsnprintf(NULL, 0, format, formatArgs, "formatStringVA");
    char * const formattedString = safeMalloc(sizeof *formattedString * (formattedStringLength + 1), "formatStringVA");

    safeVsprintf(formattedString, format, formatArgsForVsprintf, "formatStringVA");
    va_end(formatArgsForVsprintf);

    return formattedString;
}

/**
 * Represents a mutable string of characters with convenience methods for string manipulation.
 */
struct StringBuilder {
    CharList chars;
};

/**
 * Create an empty StringBuilder.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_create(void) {
    StringBuilder const builder = safeMalloc(sizeof *builder, "StringBuilder_create");
    builder->chars = CharList_create();
    return builder;
}

/**
 * Create a StringBuilder initialized with the given characters.
 *
 * @param value The characters.
 * @param count The number of characters.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_fromChars(char const * const value, size_t const count) {
    guardNotNull(value, "value", "StringBuilder_fromChars");

    StringBuilder const builder = safeMalloc(sizeof *builder, "StringBuilder_fromChars");
    builder->chars = CharList_fromItems(value, count);
    return builder;
}

/**
 * Create a StringBuilder initialized with the given string.
 *
 * @param value The string.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_fromString(char const * const value) {
    guardNotNull(value, "value", "StringBuilder_fromString");
    return StringBuilder_fromChars(value, strlen(value));
}

/**
 * Free the memory associated with the StringBuilder.
 *
 * @param builder The StringBuilder instance.
 */
void StringBuilder_destroy(StringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_destroy");

    CharList_destroy(builder->chars);
    free(builder);
}

/**
 * Get the characters that compose the current value.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns The current value as a character array. This array is not null-terminated.
 */
char const *StringBuilder_chars(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_chars");
    return CharList_items(builder->chars);
}

/**
 * Get the length of the current value.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns The string length of the current value.
 */
size_t StringBuilder_length(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_length");
    return CharList_count(builder->chars);
}

/**
 * Append the given character to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The character.
 */
void StringBuilder_appendChar(StringBuilder const builder, char const value) {
    guardNotNull(builder, "builder", "StringBuilder_appendChar");
    CharList_add(builder->chars, value);
}

/**
 * Append the given characters to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The characters.
 * @param count The number of characters.
 */
void StringBuilder_appendChars(StringBuilder const builder, char const * const value, size_t const count) {
    guardNotNull(builder, "builder", "StringBuilder_appendChars");
    guardNotNull(value, "value", "StringBuilder_appendChars");

    CharList_addMany(builder->chars, value, count);
}

/**
 * Append the given string to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The string.
 */
void StringBuilder_append(StringBuilder const builder, char const * const value) {
    guardNotNull(value, "value", "StringBuilder_append");
    StringBuilder_appendChars(builder, value, strlen(value));
}

/**
 * Append the string specified by the given format and format args to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_appendFmt(StringBuilder const builder, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_appendFmtVA(builder, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Append the string specified by the given format and format args to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_appendFmtVA(StringBuilder const builder, char const * const valueFormat, va_list valueFormatArgs) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_appendFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_append(builder, value);
    free(value);
}

/**
 * Append the given string, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The string.
 */
void StringBuilder_appendLine(StringBuilder const builder, char const * const value) {
    guardNotNull(builder, "builder", "StringBuilder_appendLine");
    guardNotNull(value, "value", "StringBuilder_appendLine");

    CharList_addMany(builder->chars, value, strlen(value));
    CharList_add(builder->chars, '\n');
}

/**
 * Append the string specified by the given format and format args, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_appendLineFmt(StringBuilder const builder, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_appendLineFmtVA(builder, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Append the string specified by the given format and format args, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_appendLineFmtVA(
    StringBuilder const builder,
    char const * const valueFormat,
    va_list valueFormatArgs
) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_appendLineFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_appendLine(builder, value);
    free(value);
}

/**
 * Insert the given character into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The character.
 */
void StringBuilder_insertChar(StringBuilder const builder, size_t const index, char const value) {
    guardNotNull(builder, "builder", "StringBuilder_insertChar");
    CharList_insert(builder->chars, index, value);
}

/**
 * Insert the given characters into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The characters.
 * @param count The number of characters.
 */
void StringBuilder_insertChars(
    StringBuilder const builder,
    size_t const index,
    char const * const value,
    size_t const count
) {
    guardNotNull(builder, "builder", "StringBuilder_insertChars");
    guardNotNull(value, "value", "StringBuilder_insertChars");

    CharList_insertMany(builder->chars, index, value, count);
}

/**
 * Insert the given string into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The string.
 */
void StringBuilder_insert(StringBuilder const builder, size_t const index, char const * const value) {
    guardNotNull(value, "value", "StringBuilder_insert");
    StringBuilder_insertChars(builder, index, value, strlen(value));
}

/**
 * Insert the string specified by the given format and format args into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_insertFmt(StringBuilder const builder, size_t const index, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_insertFmtVA(builder, index, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Insert the string specified by the given format and format args into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_insertFmtVA(
    StringBuilder const builder,
    size_t const index,
    char const * const valueFormat,
    va_list valueFormatArgs
) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_insertFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_insert(builder, index, value);
    free(value);
}

/**
 * Remove the character at the given index from the current value.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 */
void StringBuilder_removeAt(StringBuilder const builder, size_t const index) {
    guardNotNull(builder, "builder", "StringBuilder_removeAt");
    CharList_removeAt(builder->chars, index);
}

/**
 * Remove a series of characters starting at the given index from the current value.
 *
 * @param builder The StringBuilder instance.
 * @param startIndex The index at which to begin removal.
 * @param count The number of characters to remove.
 */
void StringBuilder_removeManyAt(StringBuilder const builder, size_t const startIndex, size_t const count) {
    guardNotNull(builder, "builder", "StringBuilder_removeManyAt");
    CharList_removeManyAt(builder->chars, startIndex, count);
}

/**
 * Convert the current value to a string.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns A newly allocated string containing the value. The caller is responsible for freeing this memory.
 */
char *StringBuilder_toString(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_toString");

    size_t const length = CharList_count(builder->chars);
    char * const value = safeMalloc(sizeof *value * (length + 1), "StringBuilder_toString");
    CharList_fillArray(builder->chars, value, 0, length);
    value[length] = '\0';
    return value;
}

/**
 * Convert the current value to a string, then destroy the StringBuilder.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns A newly allocated string containing the value. The caller is responsible for freeing this memory.
 */
char *StringBuilder_toStringAndDestroy(StringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_toStringAndDestroy");

    char * const valueString = StringBuilder_toString(builder);
    StringBuilder_destroy(builder);
    return valueString;
}

/**
 * Create a new thread. If the operation fails, abort the program with an error message.
 *
 * @param attributes The attributes with which to create the thread, or null to use the default attributes.
 * @param startRoutine The function to run in the new thread. This function will be called with startRoutineArg as its
 *                     sole argument. If this function returns, the effect is as if there was an implicit call to
 *                     pthread_exit() using the return value of startRoutine as the exit status.
 * @param startRoutineArg The argument to pass to startRoutine.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The ID of the newly created thread.
 */
pthread_t safePthreadCreate(
    pthread_attr_t const * const attributes,
    PthreadCreateStartRoutine const startRoutine,
    void * const startRoutineArg,
    char const * const callerDescription
) {
    guardNotNull(callerDescription, "callerDescription", "safePthreadCreate");

    pthread_t threadId;
    int const pthreadCreateErrorCode = pthread_create(&threadId, attributes, startRoutine, startRoutineArg);
    if (pthreadCreateErrorCode != 0) {
        char const * const pthreadCreateErrorMessage = strerror(pthreadCreateErrorCode);

        abortWithErrorFmt(
            "%s: Failed to create new thread using pthread_create (error code: %d; error message: \"%s\")",
            callerDescription,
            pthreadCreateErrorCode,
            pthreadCreateErrorMessage
        );
    }

    return threadId;
}

/**
 * Wait for the given thread to terminate. If the operation fails, abort the program with an error message.
 *
 * @param threadId The thread ID.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The thread's return value.
 */
void *safePthreadJoin(pthread_t const threadId, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safePthreadJoin");

    void *threadReturnValue;
    int const pthreadJoinErrorCode = pthread_join(threadId, &threadReturnValue);
    if (pthreadJoinErrorCode != 0) {
        char const * const pthreadJoinErrorMessage = strerror(pthreadJoinErrorCode);

        abortWithErrorFmt(
            "%s: Failed to join threads using pthread_join (error code: %d; error message: \"%s\")",
            callerDescription,
            pthreadJoinErrorCode,
            pthreadJoinErrorMessage
        );
    }

    return threadReturnValue;
}

DEFINE_VOID_RESULT(WgetResult, int)

/**
 * Execute wget in quiet mode to download the file at the specified URL to the specified file path.
 *
 * @param sourceFileUrl The URL of the file to download.
 * @param destinationFilePath The file path to which to download.
 *
 * @returns A result where failure indicates a wget error. The failure error contains the wget exit code. The caller is
 *          is responsible for freeing the result.
 */
WgetResult wget(char const * const sourceFileUrl, char const * const destinationFilePath) {
    guardNotNull(sourceFileUrl, "sourceFileUrl", "wget");
    guardNotNull(destinationFilePath, "destinationFilePath", "wget");

    char * const wgetCommand = formatString(
        "wget --quiet --output-document=\"%s\" \"%s\"",
        destinationFilePath,
        sourceFileUrl
    );
    int const wgetExitCode = system(wgetCommand);
    free(wgetCommand);

    return wgetExitCode == 0 ? WgetResult_success() : WgetResult_failure(wgetExitCode);
}

DEFINE_RESULT(WgetGetStringResult, char *, int)

/**
 * Use wget to download the file at the specified URL and read its contents as text.
 *
 * @param sourceFileUrl The URL of the file whose text to be downloaded.
 *
 * @returns A result where failure indicates a wget error. The success value contains the downloaded string (the caller
 *          is responsible for freeing this memory). The failure error contains the wget exit code. The caller is
 *          responsible for freeing the result.
 */
WgetGetStringResult wgetGetString(char const * const sourceFileUrl) {
    guardNotNull(sourceFileUrl, "sourceFileUrl", "wgetGetString");

    char * const destinationFilePath = generateTempFilePath();
    WgetResult const wgetResult = wget(sourceFileUrl, destinationFilePath);
    if (!WgetResult_isSuccess(wgetResult)) {
        unlink(destinationFilePath);
        free(destinationFilePath);
        return WgetGetStringResult_failure(WgetResult_getErrorAndDestroy(wgetResult));
    }
    WgetResult_destroy(wgetResult);

    char * const destinationFileText = readAllFileText(destinationFilePath);
    unlink(destinationFilePath);
    free(destinationFilePath);

    return WgetGetStringResult_success(destinationFileText);
}

/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW3
 */

int main(int const argc, char ** const argv) {
    HW3Result const hw3Result = hw3("http://undcemcs01.und.edu/~ronald.marsh/CLASS/CS451/hw3-data.txt");
    if (!HW3Result_isSuccess(hw3Result)) {
        fprintf(
            stderr,
            "ERROR: Failed to download HW3 data (wget exit code: %d)\n",
            HW3Result_getErrorAndDestroy(hw3Result)
        );
        return EXIT_FAILURE;
    }
    HW3Result_destroy(hw3Result);

    return EXIT_SUCCESS;
}

