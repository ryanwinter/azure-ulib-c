// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstring>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#endif

#include "umock_c/umock_c.h"
#include "testrunnerswitcher.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"
#include "ustream_ctest_aux.h"

#ifdef __cplusplus
extern "C"
{
#endif

static int g_memoryCounter;
void* myMalloc(size_t size)
{
    void* newMemo = malloc(size);
    if(newMemo != NULL)
    {
        g_memoryCounter++;
    }
    return newMemo;
}

void myFree(void* ptr)
{
    if(ptr != NULL)
    {
        g_memoryCounter--;
    }
    free(ptr);
}

#ifdef __cplusplus
}
#endif

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

#define ENABLE_MOCKS

#include "ulib_heap.h"

#undef ENABLE_MOCKS

#include "ustream.h"

static const uint8_t* const USTREAM_LOCAL_EXPECTED_CONTENT_1 = (const uint8_t* const)"0123456789";
static const uint8_t* const USTREAM_LOCAL_EXPECTED_CONTENT_2 = (const uint8_t* const)"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const uint8_t* const USTREAM_LOCAL_EXPECTED_CONTENT_3 = (const uint8_t* const)"abcdefghijklmnopqrstuvwxyz";

/* define constants for the compliance test */
#define USTREAM_EXPECTED_CONTENT        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define USTREAM_EXPECTED_CONTENT_LENGTH 62
static const uint8_t* const USTREAM_LOCAL_EXPECTED_CONTENT = (const uint8_t* const)USTREAM_EXPECTED_CONTENT;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

/**
 * Beginning of the UT for ustream_base.c module.
 */
BEGIN_TEST_SUITE(ustream_base_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(USTREAM, void*);

    REGISTER_GLOBAL_MOCK_HOOK(uLibMalloc, myMalloc);
    REGISTER_GLOBAL_MOCK_HOOK(uLibFree, myFree);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    g_memoryCounter = 0;

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ASSERT_ARE_EQUAL(int, 0, g_memoryCounter, "Memory issue");

    TEST_MUTEX_RELEASE(g_testByTest);
}

/* The Append shall append the provided buffer at the end of the current one. */
/* If current buffer is not a multibuffer, the Append shall convert the current buffer in a multibuffer. */
TEST_FUNCTION(uStreamAppend_startFromEmptyMultibufferSucceed)
{
    ///arrange
    USTREAM* defaultMultibuffer = uStreamMultiCreate();
    ASSERT_IS_NOT_NULL(defaultMultibuffer);

    USTREAM* defaultBuffer1 =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_1,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    ASSERT_IS_NOT_NULL(defaultBuffer1);

    USTREAM* defaultBuffer2 =
        uStreamCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_2,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_2),
            false);
    ASSERT_IS_NOT_NULL(defaultBuffer2);

    USTREAM* defaultBuffer3 =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_3,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_3));
    ASSERT_IS_NOT_NULL(defaultBuffer3);

    ///act
    ULIB_RESULT result1 = uStreamAppend(defaultMultibuffer, defaultBuffer1);
    ULIB_RESULT result2 = uStreamAppend(defaultMultibuffer, defaultBuffer2);
    ULIB_RESULT result3 = uStreamAppend(defaultMultibuffer, defaultBuffer3);

    ///assert
    ASSERT_ARE_EQUAL(int, ULIB_SUCCESS, result1);
    ASSERT_ARE_EQUAL(int, ULIB_SUCCESS, result2);
    ASSERT_ARE_EQUAL(int, ULIB_SUCCESS, result3);
    uStreamDispose(defaultBuffer1);
    uStreamDispose(defaultBuffer2);
    uStreamDispose(defaultBuffer3);
    checkBuffer(
        defaultMultibuffer,
        0, 
        USTREAM_LOCAL_EXPECTED_CONTENT, 
        USTREAM_EXPECTED_CONTENT_LENGTH);

    ///cleanup
    uStreamDispose(defaultMultibuffer);
}

TEST_FUNCTION(uStreamAppend_appendMultipleBuffersSucceed)
{
    ///arrange
    USTREAM* defaultBuffer1 =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_1,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    ASSERT_IS_NOT_NULL(defaultBuffer1);

    USTREAM* defaultBuffer2 =
        uStreamCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_2,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_2),
            false);
    ASSERT_IS_NOT_NULL(defaultBuffer2);

    USTREAM* defaultBuffer3 =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_3,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_3));
    ASSERT_IS_NOT_NULL(defaultBuffer3);

    ///act
    ULIB_RESULT result1 = uStreamAppend(defaultBuffer1, defaultBuffer2);
    ULIB_RESULT result2 = uStreamAppend(defaultBuffer1, defaultBuffer3);

    ///assert
    ASSERT_ARE_EQUAL(int, ULIB_SUCCESS, result1);
    ASSERT_ARE_EQUAL(int, ULIB_SUCCESS, result2);
    uStreamDispose(defaultBuffer2);
    uStreamDispose(defaultBuffer3);
    checkBuffer(
        defaultBuffer1,
        0,
        USTREAM_LOCAL_EXPECTED_CONTENT,
        USTREAM_EXPECTED_CONTENT_LENGTH);

    ///cleanup
    uStreamDispose(defaultBuffer1);
}

/* If the provided interface is NULL, the Append shall return ULIB_ILLEGAL_ARGUMENT_ERROR. */
TEST_FUNCTION(uStreamAppend_nullInterfaceFailed)
{
    ///arrange
    USTREAM* defaultBuffer =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_1,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    ASSERT_IS_NOT_NULL(defaultBuffer);

    ///act
    ULIB_RESULT result = uStreamAppend(NULL, defaultBuffer);

    ///assert
    ASSERT_ARE_EQUAL(int, ULIB_ILLEGAL_ARGUMENT_ERROR, result);

    ///cleanup
    uStreamDispose(defaultBuffer);
}

/* If the provided buffer to add is NULL, the Append shall return ULIB_ILLEGAL_ARGUMENT_ERROR. */
TEST_FUNCTION(uStreamAppend_nullBufferToAddFailed)
{
    ///arrange
    USTREAM* defaultBuffer =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_1,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    ASSERT_IS_NOT_NULL(defaultBuffer);

    ///act
    ULIB_RESULT result = uStreamAppend(defaultBuffer, NULL);

    ///assert
    ASSERT_ARE_EQUAL(int, ULIB_ILLEGAL_ARGUMENT_ERROR, result);

    ///cleanup
    uStreamDispose(defaultBuffer);
}

/* If there is not enough memory to append the buffer, the Append shall return ULIB_OUT_OF_MEMORY_ERROR. */
TEST_FUNCTION(uStreamAppend_startingFromMultibufferWithNotEnoughMemoryFailed)
{
    ///arrange
    USTREAM* defaultMultibuffer = uStreamMultiCreate();
    ASSERT_IS_NOT_NULL(defaultMultibuffer);

    USTREAM* defaultBuffer =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_1,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    ASSERT_IS_NOT_NULL(defaultBuffer);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(uLibMalloc(IGNORED_NUM_ARG)).SetReturn(NULL);

    ///act
    ULIB_RESULT result = uStreamAppend(defaultMultibuffer, defaultBuffer);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, ULIB_OUT_OF_MEMORY_ERROR, result);

    ///cleanup
    uStreamDispose(defaultBuffer);
    uStreamDispose(defaultMultibuffer);
}

TEST_FUNCTION(uStreamAppend_notEnoughMemoryToCreateMultibufferFailed)
{
    ///arrange
    USTREAM* defaultBuffer1 =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_1,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    ASSERT_IS_NOT_NULL(defaultBuffer1);

    USTREAM* defaultBuffer2 =
        uStreamCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_2,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_2),
            false);
    ASSERT_IS_NOT_NULL(defaultBuffer2);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(uLibMalloc(sizeof(USTREAM))).SetReturn(NULL);

    ///act
    ULIB_RESULT result = uStreamAppend(defaultBuffer1, defaultBuffer2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, ULIB_OUT_OF_MEMORY_ERROR, result);
    checkBuffer(
        defaultBuffer1,
        0,
        USTREAM_LOCAL_EXPECTED_CONTENT_1,
        (uint8_t)strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    checkBuffer(
        defaultBuffer2,
        0,
        USTREAM_LOCAL_EXPECTED_CONTENT_2,
        (uint8_t)strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_2));

    ///cleanup
    uStreamDispose(defaultBuffer1);
    uStreamDispose(defaultBuffer2);
}

TEST_FUNCTION(uStreamAppend_notEnoughMemoryToAppendFirstBufferFailed)
{
    ///arrange
    USTREAM* defaultBuffer1 =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_1,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    ASSERT_IS_NOT_NULL(defaultBuffer1);

    USTREAM* defaultBuffer2 =
        uStreamCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_2,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_2),
            false);
    ASSERT_IS_NOT_NULL(defaultBuffer2);

    umock_c_reset_all_calls();
    /* Create multibuffer */
    STRICT_EXPECTED_CALL(uLibMalloc(sizeof(USTREAM)));
    STRICT_EXPECTED_CALL(uLibMalloc(IGNORED_NUM_ARG));
    /* Append first buffer */
    STRICT_EXPECTED_CALL(uLibMalloc(IGNORED_NUM_ARG)).SetReturn(NULL);
    /* Release multibuffer */
    STRICT_EXPECTED_CALL(uLibFree(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uLibFree(IGNORED_PTR_ARG));

    ///act
    ULIB_RESULT result = uStreamAppend(defaultBuffer1, defaultBuffer2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, ULIB_OUT_OF_MEMORY_ERROR, result);
    checkBuffer(
        defaultBuffer1,
        0,
        USTREAM_LOCAL_EXPECTED_CONTENT_1,
        (uint8_t)strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    checkBuffer(
        defaultBuffer2,
        0,
        USTREAM_LOCAL_EXPECTED_CONTENT_2,
        (uint8_t)strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_2));

    ///cleanup
    uStreamDispose(defaultBuffer1);
    uStreamDispose(defaultBuffer2);
}

TEST_FUNCTION(uStreamAppend_notEnoughMemoryToAppendSecondBufferFailed)
{
    ///arrange
    USTREAM* defaultBuffer1 =
        uStreamConstCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_1,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    ASSERT_IS_NOT_NULL(defaultBuffer1);

    USTREAM* defaultBuffer2 =
        uStreamCreate(
            USTREAM_LOCAL_EXPECTED_CONTENT_2,
            strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_2),
            false);
    ASSERT_IS_NOT_NULL(defaultBuffer2);

    umock_c_reset_all_calls();
    /* Create multibuffer */
    STRICT_EXPECTED_CALL(uLibMalloc(sizeof(USTREAM)));
    STRICT_EXPECTED_CALL(uLibMalloc(IGNORED_NUM_ARG));
    /* Append first buffer */
    STRICT_EXPECTED_CALL(uLibMalloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(uLibMalloc(sizeof(USTREAM)));
    STRICT_EXPECTED_CALL(uLibMalloc(IGNORED_NUM_ARG));
    /* Append second buffer */
    STRICT_EXPECTED_CALL(uLibMalloc(IGNORED_NUM_ARG)).SetReturn(NULL);
    /* Release multibuffer */
    STRICT_EXPECTED_CALL(uLibFree(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uLibFree(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uLibFree(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uLibFree(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uLibFree(IGNORED_PTR_ARG));

    ///act
    ULIB_RESULT result = uStreamAppend(defaultBuffer1, defaultBuffer2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, ULIB_OUT_OF_MEMORY_ERROR, result);
    checkBuffer(
        defaultBuffer1,
        0,
        USTREAM_LOCAL_EXPECTED_CONTENT_1,
        (uint8_t)strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_1));
    checkBuffer(
        defaultBuffer2,
        0,
        USTREAM_LOCAL_EXPECTED_CONTENT_2,
        (uint8_t)strlen((const char*)USTREAM_LOCAL_EXPECTED_CONTENT_2));

    ///cleanup
    uStreamDispose(defaultBuffer1);
    uStreamDispose(defaultBuffer2);
}

END_TEST_SUITE(ustream_base_ut)