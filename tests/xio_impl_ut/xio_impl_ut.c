// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>

#ifdef __cplusplus
#else
#endif

/**
 * Include the C standards here.
 */
#ifdef __cplusplus
#include <cstddef>
#include <cstdlib>
#include <ctime>
#else
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#endif

/**
 * The gballoc.h will replace the malloc, free, and realloc by the my_gballoc functions, in this case,
 *    if you define these mock functions after include the gballoc.h, you will create an infinity recursion,
 *    so, places the my_gballoc functions before the #include "azure_c_shared_utility/gballoc.h"
 */
#include "gballoc_ut_impl_1.h"

 /**
 * Include the mockable headers here.
 * These are the headers that contains the functions that you will replace to execute the test.
 */
#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xio_endpoint.h"
#include "azure_c_shared_utility/xio_endpoint_config.h"
#undef ENABLE_MOCKS

/**
 * Include the test tools.
 */
#include "azure_c_shared_utility/xio_impl.h"
#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/tlsio.h"

// These "headers" are actually source files that are broken out of this file for readability
#include "fake_endpoint.h"
#include "callbacks.h"

#define SETOPTION_PV_COUNT 3
#define OPEN_PV_COUNT 4
#define SEND_PV_COUNT 4
#define CLOSE_PV_COUNT 2

static bool bool_true = true;
static bool bool_false = false;
static size_t sizeof_bool = sizeof(bool);



#include "gballoc_ut_impl_2.h"

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

typedef struct create_params
{
    const XIO_ENDPOINT_INTERFACE* endpoint_interface;
    XIO_ENDPOINT_CONFIG_INTERFACE* xio_endpoint_config_interface;
    XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config_instance;
} create_params;

OPTIONHANDLER_HANDLE fake_option_handler = (OPTIONHANDLER_HANDLE)0x0004;

/**
 * This is necessary for the test suite, just keep as is.
 */
static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;
static bool negative_mocks_used = false;


BEGIN_TEST_SUITE(xio_impl_unittests)

    TEST_SUITE_INITIALIZE(a)
    {
        int result;
        size_t type_size;
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);

        result = umocktypes_charptr_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);
        result = umocktypes_bool_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);
        umocktypes_stdint_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);

        REGISTER_UMOCK_ALIAS_TYPE(XIO_ENDPOINT_INSTANCE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(XIO_ENDPOINT_CONFIG_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(XIO_ASYNC_RESULT, int);

        type_size = sizeof(time_t);
        if (type_size == sizeof(uint64_t))
        {
            REGISTER_UMOCK_ALIAS_TYPE(time_t, uint64_t);
        }
        else if (type_size == sizeof(uint32_t))
        {
            REGISTER_UMOCK_ALIAS_TYPE(time_t, uint32_t);
        }
        else
        {
            ASSERT_FAIL("Bad size_t size");
        }

        REGISTER_GLOBAL_MOCK_RETURNS(xio_endpoint_create, &endpoint_instance, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(xio_endpoint_close, XIO_ASYNC_RESULT_SUCCESS, XIO_ASYNC_RESULT_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURNS(xio_endpoint_open, XIO_ASYNC_RESULT_SUCCESS, XIO_ASYNC_RESULT_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURNS(xio_endpoint_config_retrieveoptions, fake_option_handler, NULL);
        REGISTER_GLOBAL_MOCK_RETURNS(xio_endpoint_config_setoption, 0, 1);
        
        REGISTER_GLOBAL_MOCK_HOOK(xio_endpoint_read, my_xio_endpoint_read);
        REGISTER_GLOBAL_MOCK_HOOK(xio_endpoint_write, my_xio_endpoint_write);

        /**
         * Or you can combine, for example, in the success case malloc will call my_gballoc_malloc, and for
         *    the failed cases, it will return NULL.
         */
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    }

    static void use_negative_mocks()
    {
        int negativeTestsInitResult = umock_c_negative_tests_init();
        negative_mocks_used = true;
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);
    }

    /**
     * The test suite will call this function to cleanup your machine.
     * It is called only once, after all tests is done.
     */
    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    /**
     * The test suite will call this function to prepare the machine for the new test.
     * It is called before execute each test.
     */
    TEST_FUNCTION_INITIALIZE(initialize)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("Could not acquire test serialization mutex.");
        }

        umock_c_reset_all_calls();
        reset_callback_context_records();
		init_gballoc_checks();
    }

    /**
     * The test suite will call this function to cleanup your machine for the next test.
     * It is called after execute each test.
     */
    TEST_FUNCTION_CLEANUP(cleans)
    {
		if (negative_mocks_used)
        {
            negative_mocks_used = false;
            umock_c_negative_tests_deinit();
        }
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    static CONCRETE_IO_HANDLE open_helper()
    {
        CONCRETE_IO_HANDLE result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        int open_result = xio_impl_open_async(result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        reset_callback_context_records();

        // Pump dowork until it opens
        xio_impl_dowork(result);
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_OK);
        reset_callback_context_records();
        return result;
    }

    /* Tests_SRS_XIO_IMPL_30_201: [ The "high-level retry sequence" shall succeed after an injected fault which causes on_io_error to be called. ]*/
    TEST_FUNCTION(xio_impl__retry_open_after_io_failure__succeeds)
    {
        int send_result;
        int open_result;
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();
        reset_callback_context_records();

        // Queue up the message to eventually fail on
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_FAIL_ME_SENT_MESSAGE_SIZE)).SetReturn(0);
        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_FAIL_ME_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, 0, send_result);

        umock_c_reset_all_calls();
        reset_callback_context_records();
        // Make sure the io fails
        xio_impl_dowork(xio_impl);
        ASSERT_IO_ERROR_CALLBACK(true);

        // Close the error'd xio_impl
        xio_impl_close_async(xio_impl, on_io_close_complete, IO_CLOSE_COMPLETE_CONTEXT);
        ASSERT_IO_CLOSE_CALLBACK(true);

        // Retry the open
        open_result = xio_impl_open_async(xio_impl, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);

        reset_callback_context_records();

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        // Check that we got the on_open callback for our retry
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_OK);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
		xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_200: [ The "high-level retry sequence" shall succeed after an injected fault which causes on_io_open_complete to return with IO_OPEN_ERROR. ]*/
    TEST_FUNCTION(xio_impl__retry_open_after_open_failure__succeeds)
    {
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);

        int open_result = xio_impl_open_async(xio_impl, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);
        umock_c_reset_all_calls();

        // dowork_poll_open_ssl (done) Fail the SSL_connect call
        STRICT_EXPECTED_CALL(xio_endpoint_open(IGNORED_PTR_ARG, &config_instance)).SetReturn(XIO_ASYNC_RW_RESULT_FAILURE);


        xio_impl_dowork(xio_impl);
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_ERROR);

        // Close the error'd xio_impl
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);

        // Retry the open
        umock_c_reset_all_calls();
        open_result = xio_impl_open_async(xio_impl, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        reset_callback_context_records();

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        // Check that we got the on_open callback for our retry
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_OK);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_050: [ If the xio_impl_handle or on_close_complete parameter is NULL, xio_impl_close_async shall log an error and return _FAILURE_. ]*/
    /* Tests_SRS_XIO_IMPL_30_054: [ On failure, the adapter shall not call on_io_close_complete. ]*/
    TEST_FUNCTION(xio_impl__close_parameter_validation__fails)
    {
        int k;
        bool p0[CLOSE_PV_COUNT];
        ON_IO_CLOSE_COMPLETE p1[CLOSE_PV_COUNT];
        const char* fm[CLOSE_PV_COUNT];
        int i;

        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        umock_c_reset_all_calls();

        k = 0;
        p0[k] = false; p1[k] = on_io_close_complete; fm[k] = "Unexpected close success when tlsio_handle is NULL"; /* */  k++;
        p0[k] = true; p1[k] = NULL; /*           */ fm[k] = "Unexpected close success when on_io_close_complete is NULL"; k++;

        // Cycle through each failing combo of parameters
        for (i = 0; i < CLOSE_PV_COUNT; i++)
        {
            int close_result;
            reset_callback_context_records();
            ///arrange

            ///act
            close_result = xio_impl_close_async(p0[i] ? xio_impl : NULL, p1[i], IO_CLOSE_COMPLETE_CONTEXT);

            ///assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, close_result, fm[i]);
            ASSERT_IO_CLOSE_CALLBACK(false);
        }

        ///cleanup
        xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

	/* Tests_SRS_XIO_IMPL_30_009: [ The phrase "enter TLSIO_STATE_EXT_CLOSING" means the adapter shall iterate through any unsent messages in the queue and shall delete each message after calling its on_send_complete with the associated callback_context and IO_SEND_CANCELLED . ]*/
	/* Tests_SRS_XIO_IMPL_30_056: [ On success the adapter shall enter TLSIO_STATE_EX_CLOSING. ]*/
	TEST_FUNCTION(xio_impl__close_with_unsent_messages__succeeds)
    {
        ///arrange
        int send_result;
        int close_result;
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        // Make sure the arrangement is correct
        reset_callback_context_records();

        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(0);
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(0);
        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_SHORT_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);
        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_SHORT_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);


        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(xio_endpoint_close(IGNORED_PTR_ARG));
        // Message 1 delete
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        // Message 2 delete
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        reset_callback_context_records();
        // End of arrange

        ///act
		close_result = xio_impl_close_async(xio_impl, on_io_close_complete, IO_CLOSE_COMPLETE_CONTEXT);

        ///assert
		ASSERT_ARE_EQUAL(int, 0, close_result);
        ASSERT_IO_CLOSE_CALLBACK(true);
        ASSERT_IO_SEND_ABANDONED(2); // 2 messages in this test
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());


        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_053: [ If the adapter is in any state other than XIO_IMPL_STATE_EXT_OPEN or XIO_IMPL_STATE_EXT_ERROR then xio_impl_close_async shall log that xio_impl_close_async has been called and then continue normally. ]*/
    TEST_FUNCTION(xio_impl__close_while_closed__succeeds)
    {
        int close_result;
        CONCRETE_IO_HANDLE xio_impl;
        ///arrange
        reset_callback_context_records();
        xio_impl = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_OK);
        umock_c_reset_all_calls();
        reset_callback_context_records();
        STRICT_EXPECTED_CALL(xio_endpoint_close(IGNORED_NUM_ARG));

        ///act
		close_result = xio_impl_close_async(xio_impl, on_io_close_complete, IO_CLOSE_COMPLETE_CONTEXT);

        ///assert
		ASSERT_ARE_EQUAL(int, 0, close_result);
		ASSERT_IO_CLOSE_CALLBACK(true);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}
    
	/* Tests_SRS_XIO_IMPL_30_063: [ On success, xio_impl_send_async shall enqueue for transmission the on_send_complete, the callback_context, the size, and the contents of buffer and then return 0. ]*/
	TEST_FUNCTION(xio_impl__send__succeeds)
	{
        int send_result;
		///arrange
		CONCRETE_IO_HANDLE xio_impl = open_helper();

        reset_callback_context_records();
		umock_c_reset_all_calls();

		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // PENDING_SOCKET_IO
		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // message bytes
		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // singlylinkedlist_add

		///act
		send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
			SSL_send_message_size, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);

		///assert
		ASSERT_ARE_EQUAL_WITH_MSG(int, send_result, 0, "Unexpected send failure");

		///cleanup
		xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
		xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

	/* Tests_SRS_XIO_IMPL_30_064: [ If the supplied message cannot be enqueued for transmission, xio_impl_send_async shall log an error and return _FAILURE_. ]*/
	/* Tests_SRS_XIO_IMPL_30_066: [ On failure, on_send_complete shall not be called. ]*/
	TEST_FUNCTION(xio_impl__send_unhappy_paths__fails)
	{
		///arrange
        size_t i;
		use_negative_mocks();

		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // PENDING_TRANSMISSION
		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // message bytes
		STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  // singlylinkedlist_add
		umock_c_negative_tests_snapshot();

		for (i = 0; i < umock_c_negative_tests_call_count(); i++)
		{
            int send_result;
			///arrange
			CONCRETE_IO_HANDLE xio_impl = open_helper();
			reset_callback_context_records();

			umock_c_reset_all_calls();

			umock_c_negative_tests_reset();
			umock_c_negative_tests_fail_call(i);

			///act
			send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
				SSL_send_message_size, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);

			///assert
			ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, send_result, 0, "Unexpected send success on unhappy path");
            ASSERT_IO_SEND_CALLBACK(false, IO_SEND_ERROR);

			///cleanup
			xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
			xio_impl_destroy(xio_impl);
		}

		///cleanup
		assert_gballoc_checks();
	}

	/* Tests_SRS_XIO_IMPL_30_060: [ If any of the xio_impl_handle, buffer, or on_send_complete parameters is NULL, xio_impl_send_async shall log an error and return _FAILURE_. ]*/
	/* Tests_SRS_XIO_IMPL_30_067: [ If the  size  is 0,  tlsio_send  shall log the error and return FAILURE. ]*/
    /* Tests_SRS_XIO_IMPL_30_066: [ On failure, on_send_complete shall not be called. ]*/
    TEST_FUNCTION(xio_impl__send_parameter_validation__fails)
	{
		// Parameters arrays
		bool p0[SEND_PV_COUNT];
		const void* p1[SEND_PV_COUNT];
		size_t p2[SEND_PV_COUNT];
		ON_SEND_COMPLETE p3[SEND_PV_COUNT];
		const char* fm[SEND_PV_COUNT];
        int k;
        int i;

		///arrange
		CONCRETE_IO_HANDLE xio_impl = open_helper();

		k = 0;
		p0[k] = false; p1[k] = SSL_send_buffer; p2[k] = SSL_send_message_size; p3[k] = on_io_send_complete; fm[k] = "Unexpected send success when tlsio_handle is NULL"; k++;
		p0[k] = true; p1[k] = NULL; /*       */ p2[k] = SSL_send_message_size; p3[k] = on_io_send_complete; fm[k] = "Unexpected send success when send buffer is NULL"; k++;
		p0[k] = true; p1[k] = SSL_send_buffer; p2[k] = 0; /*                */ p3[k] = on_io_send_complete; fm[k] = "Unexpected send success when size is 0"; k++;
		p0[k] = true; p1[k] = SSL_send_buffer; p2[k] = SSL_send_message_size; p3[k] = NULL; /*           */ fm[k] = "Unexpected send success when on_send_complete is NULL"; k++;

		// Cycle through each failing combo of parameters
		for (i = 0; i < SEND_PV_COUNT; i++)
		{
            int send_result;
			///arrange
			reset_callback_context_records();

			///act
			send_result = xio_impl_send_async(p0[i] ? xio_impl : NULL, p1[i], p2[i], p3[i], IO_SEND_COMPLETE_CONTEXT);

			///assert
			ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, send_result, 0, fm[i]);
			ASSERT_IO_SEND_CALLBACK(false, IO_SEND_ERROR);

			///cleanup
		}

		///cleanup
		xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
		xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

	/* Tests_SRS_XIO_IMPL_30_065: [ If the adapter state is not XIO_IMPL_STATE_EXT_OPEN, xio_impl_send_async shall log an error and return _FAILURE_. ]*/
    /* Tests_SRS_XIO_IMPL_30_066: [ On failure, on_send_complete shall not be called. ]*/
    TEST_FUNCTION(xio_impl__send_not_open__fails)
	{
        int send_result;
		///arrange
        CONCRETE_IO_HANDLE xio_impl = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        reset_callback_context_records();

		///act
		send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
			SSL_send_message_size, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);

		///assert
		ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, send_result, 0, "Unexpected success in sending from wrong state");
		ASSERT_IO_SEND_CALLBACK(false, IO_SEND_ERROR);

		///cleanup
		xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_071: [ If the adapter is in XIO_IMPL_STATE_EXT_ERROR then xio_impl_dowork shall do nothing. ]*/
    TEST_FUNCTION(xio_impl__dowork_send_post_error_do_nothing__succeeds)
    {
        int send_result;
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        // Queue up two messages, one to fail and one that should be ignored by the dowork
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(0);
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(0);
        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_SHORT_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);
        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_SHORT_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);

        reset_callback_context_records();
        init_fake_read(0);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(XIO_ASYNC_RW_RESULT_FAILURE);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));

        xio_impl_dowork(xio_impl);

        ASSERT_IO_SEND_CALLBACK(true, IO_SEND_ERROR);
        ASSERT_IO_ERROR_CALLBACK(true);

        reset_callback_context_records();
        umock_c_reset_all_calls();

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
		ASSERT_NO_CALLBACKS();
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

	/* Tests_SRS_XIO_IMPL_30_002: [ The phrase "destroy the failed message" means that the adapter shall remove the message from the queue and destroy it after calling the message's on_send_complete along with its associated callback_context and IO_SEND_ERROR. ]*/
	/* Tests_SRS_XIO_IMPL_30_005: [ When the adapter enters TLSIO_STATE_EXT_ERROR it shall call the  on_io_error function and pass the on_io_error_context that were supplied in  tlsio_open . ]*/
	/* Tests_SRS_XIO_IMPL_30_095: [ If the send process fails before sending all of the bytes in an enqueued message, xio_impl_dowork shall destroy the failed message and enter XIO_IMPL_STATE_EXT_ERROR. ]*/
    TEST_FUNCTION(xio_impl__dowork_send_unhappy_path__fails)
    {
        int send_result;
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(0);
        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_SHORT_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);

        init_fake_read(0);
        reset_callback_context_records();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(XIO_ASYNC_RW_RESULT_FAILURE);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        ASSERT_IO_SEND_CALLBACK(true, IO_SEND_ERROR);
        ASSERT_IO_ERROR_CALLBACK(true);
		ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_093: [ If the xio_endpoint was not able to send an entire enqueued message at once, subsequent calls to xio_impl_dowork shall continue to send the remaining bytes. ]*/
    TEST_FUNCTION(xio_impl__dowork_send_big_message__succeeds)
    {
        int send_result;
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        // Don't let the async_write's call to dowork_send consume the message
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_TEST_MESSAGE_SIZE)).SetReturn(0);

        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_TEST_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);
        ASSERT_IO_ERROR_CALLBACK(false);

        init_fake_read(0);
        reset_callback_context_records();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_TEST_MESSAGE_SIZE));

        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_TEST_MESSAGE_SIZE - SSL_WRITE_MAX_TEST_SIZE));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        xio_impl_dowork(xio_impl);
        ASSERT_IO_ERROR_CALLBACK(false);

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        ASSERT_IO_SEND_CALLBACK(true, IO_SEND_OK);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IO_ERROR_CALLBACK(false);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_091: [ If xio_impl_dowork is able to send all the bytes in an enqueued message, it shall first dequeue the message then call the messages's on_send_complete along with its associated callback_context and IO_SEND_OK. ]*/
    TEST_FUNCTION(xio_impl__dowork_send__succeeds)
    {
        int send_result;
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        // Don't let the async_write's call to dowork_send consume the message
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(0);

        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_SHORT_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);
        ASSERT_IO_ERROR_CALLBACK(false);

        init_fake_read(0);
        reset_callback_context_records();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        ASSERT_IO_SEND_CALLBACK(true, IO_SEND_OK);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IO_ERROR_CALLBACK(false);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_096: [ If there are no enqueued messages available, xio_impl_dowork shall do nothing. ]*/
    TEST_FUNCTION(xio_impl__dowork_send_empty_queue__succeeds)
    {
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        init_fake_read(0);
        reset_callback_context_records();
        umock_c_reset_all_calls();

        // We do expect an empty read when we call dowork
        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        // Verify we got no callback for 0 messages
        ASSERT_IO_SEND_CALLBACK(false, IO_SEND_OK);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IO_ERROR_CALLBACK(false);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
        assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_101: [ If the xio_endpoint returns XIO_ASYNC_RW_RESULT_FAILURE then xio_impl_dowork shall enter XIO_IMPL_STATE_EXT_ERROR. ] */
    TEST_FUNCTION(xio_impl__dowork_receive_unhappy_path__fails)
    {
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        init_fake_read(0);
        reset_callback_context_records();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(XIO_ASYNC_RW_RESULT_FAILURE);

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        // Verify we got no callback for 0 bytes
        ASSERT_BYTES_RECEIVED_CALLBACK(false, 0);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IO_ERROR_CALLBACK(true);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_102: [ If the TLS connection receives no data then tlsio_dowork shall not call the on_bytes_received callback. ]*/
    TEST_FUNCTION(xio_impl__dowork_receive_no_data__succeeds)
    {
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        init_fake_read(0);
        reset_callback_context_records();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        // Verify we got no callback for 0 bytes
        ASSERT_BYTES_RECEIVED_CALLBACK(false, 0);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IO_ERROR_CALLBACK(false);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_100: [ If the xio_endpoint returns a positive number of bytes received, xio_impl_dowork shall repeatedly read this data and call on_bytes_received with the pointer to the buffer containing the data, the number of bytes received, and the on_bytes_received_context. ]*/
    TEST_FUNCTION(xio_impl__dowork_receive_short_message__succeeds)
    {
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        init_fake_read(SSL_SHORT_RECEIVED_MESSAGE_SIZE);
        reset_callback_context_records();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_read(&endpoint_instance, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        // Verify we got the bytes and their callback context
        ASSERT_BYTES_RECEIVED_CALLBACK(true, SSL_SHORT_RECEIVED_MESSAGE_SIZE);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IO_ERROR_CALLBACK(false);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
        assert_gballoc_checks();
    }


    /* Tests_SRS_XIO_IMPL_30_100: [ If the xio_endpoint returns a positive number of bytes received, xio_impl_dowork shall repeatedly read this data and call on_bytes_received with the pointer to the buffer containing the data, the number of bytes received, and the on_bytes_received_context. ]*/
    TEST_FUNCTION(xio_impl__dowork_receive_long_message__succeeds)
    {
        ///arrange
        CONCRETE_IO_HANDLE xio_impl = open_helper();

        init_fake_read(SSL_LONG_RECEIVED_MESSAGE_SIZE);
        reset_callback_context_records();
        umock_c_reset_all_calls();

        ///act
        xio_impl_dowork(xio_impl);

        ///assert
        // Verify we got the bytes and their callback context
        ASSERT_BYTES_RECEIVED_CALLBACK(true, SSL_LONG_RECEIVED_MESSAGE_SIZE);
        ASSERT_IO_ERROR_CALLBACK(false);

        ///cleanup
        xio_impl_close_async(xio_impl, on_io_close_complete, NULL);
        xio_impl_destroy(xio_impl);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_075: [ If the adapter is in XIO_IMPL_STATE_EXT_CLOSED then xio_impl_dowork shall do nothing. ]*/
    TEST_FUNCTION(xio_impl__dowork_post_close__succeeds)
    {
        CONCRETE_IO_HANDLE create_result;
        int open_result;
        int close_result;
        ///arrange
        reset_callback_context_records();
        create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        open_result = xio_impl_open_async(create_result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_open(IGNORED_PTR_ARG, &config_instance));
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_OK);
        reset_callback_context_records();
        xio_impl_dowork(create_result);
        // Check that we got the on_open callback
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_OK);
        close_result = xio_impl_close_async(create_result, on_io_close_complete, IO_CLOSE_COMPLETE_CONTEXT);
        ASSERT_IO_CLOSE_CALLBACK(true);
        reset_callback_context_records();
        umock_c_reset_all_calls();
        // End of arrange

        ///act
        xio_impl_dowork(create_result);

        ///assert
        // Verify that the dowork did nothing
        ASSERT_NO_CALLBACKS();
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_009: [ The phrase "enter TLSIO_STATE_EXT_CLOSING" means the adapter shall iterate through any unsent messages in the queue and shall delete each message after calling its on_send_complete with the associated callback_context and IO_SEND_CANCELLED. ]*/
    /* Tests_SRS_XIO_IMPL_30_006: [ The phrase "enter TLSIO_STATE_EXT_CLOSED" means the adapter shall forcibly close any existing connections then call the on_io_close_complete function and pass the on_io_close_complete_context that was supplied in tlsio_close_async. ]*/
    /* Tests_SRS_XIO_IMPL_30_056: [ On success the adapter shall enter TLSIO_STATE_EX_CLOSING. ]*/
    /* Tests_SRS_XIO_IMPL_30_051: [ On success,  xio_impl_close_async  shall invoke the XIO_IMPL_STATE_EXT_CLOSING behavior of  xio_impl_dowork . ]*/
    /* Tests_SRS_XIO_IMPL_30_052: [ On success xio_impl_close_async shall return 0. ]*/
    TEST_FUNCTION(xio_impl__close_after_open__succeeds)
    {
        CONCRETE_IO_HANDLE create_result;
        int open_result;
        int close_result;
        ///arrange
        reset_callback_context_records();
        create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        open_result = xio_impl_open_async(create_result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_open(IGNORED_PTR_ARG, &config_instance));
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_OK);
        reset_callback_context_records();
        xio_impl_dowork(create_result);
        // Check that we got the on_open callback
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_OK);
        // End of arrange

        ///act
        close_result = xio_impl_close_async(create_result, on_io_close_complete, IO_CLOSE_COMPLETE_CONTEXT);

        ///assert
        ASSERT_ARE_EQUAL(int, 0, close_result);
        ASSERT_IO_CLOSE_CALLBACK(true);

        ///cleanup
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_053: [ If the adapter is in any state other than XIO_IMPL_STATE_EXT_OPEN or XIO_IMPL_STATE_EXT_ERROR then xio_impl_close_async shall log that xio_impl_close_async has been called and then continue normally. ]*/
    /* Tests_SRS_XIO_IMPL_30_057: [ On success, if the adapter is in TLSIO_STATE_EXT_OPENING, it shall call on_io_open_complete with the on_io_open_complete_context supplied in tlsio_open_async and IO_OPEN_CANCELLED. This callback shall be made before changing the internal state of the adapter. ]*/
    TEST_FUNCTION(xio_impl__close_while_opening__succeeds)
    {
        CONCRETE_IO_HANDLE create_result;
        int open_result;
        int close_result;
        ///arrange
        reset_callback_context_records();
        create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        open_result = xio_impl_open_async(create_result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);
        umock_c_reset_all_calls();

        //STRICT_EXPECTED_CALL(xio_endpoint_open(IGNORED_PTR_ARG, &config_instance)).SetReturn(XIO_ASYNC_RESULT_FAILURE);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_OK);
        reset_callback_context_records();
        // End of arrange

        ///act
        close_result = xio_impl_close_async(create_result, on_io_close_complete, IO_CLOSE_COMPLETE_CONTEXT);

        ///assert
        ASSERT_ARE_EQUAL(int, 0, close_result);
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_CANCELLED);
        ASSERT_IO_CLOSE_CALLBACK(true);

        ///cleanup
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_082: [ If the xio_endpoint returns XIO_ASYNC_RESULT_FAILURE, xio_impl_dowork shall log an error, call on_open_complete with the on_open_complete_context parameter provided in xio_impl_open_async and IO_OPEN_ERROR, and enter XIO_IMPL_STATE_EXT_CLOSED. ]*/
    TEST_FUNCTION(xio_impl__dowork_open__fails)
    {
        CONCRETE_IO_HANDLE create_result;
        int open_result;
        ///arrange
        reset_callback_context_records();
        create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        open_result = xio_impl_open_async(create_result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_open(IGNORED_PTR_ARG, &config_instance)).SetReturn(XIO_ASYNC_RESULT_FAILURE);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_OK);

        ///act
        xio_impl_dowork(create_result);

        ///assert
        // Check that we got the on_open callback
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_ERROR);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_close_async(create_result, on_io_close_complete, NULL);
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_081: [ If the xio_endpoint returns XIO_ASYNC_RESULT_WAITING, xio_impl_dowork shall remain in the XIO_IMPL_STATE_EXT_OPENING state. ]*/
    TEST_FUNCTION(xio_impl__dowork_open__waiting)
    {
        CONCRETE_IO_HANDLE create_result;
        int open_result;
        ///arrange
        reset_callback_context_records();
        create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        open_result = xio_impl_open_async(create_result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_open(IGNORED_PTR_ARG, &config_instance)).SetReturn(XIO_ASYNC_RESULT_WAITING);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_OK);

        ///act
        xio_impl_dowork(create_result);

        ///assert
        // Check that we got the on_open callback
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_OK);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_close_async(create_result, on_io_close_complete, NULL);
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_080: [ The xio_impl_dowork shall delegate open behavior to the xio_endpoint provided during xio_impl_open_async. ]*/
    /* Tests_SRS_XIO_IMPL_30_007: [ The phrase "enter TLSIO_STATE_EXT_OPEN" means the adapter shall call the on_io_open_complete function and pass IO_OPEN_OK and the on_io_open_complete_context that was supplied in tlsio_open . ]*/
    /* Tests_SRS_XIO_IMPL_30_083: [ If xio_endpoint returns XIO_ASYNC_RESULT_SUCCESS, xio_impl_dowork shall enter XIO_IMPL_STATE_EXT_OPEN. ]*/
    TEST_FUNCTION(xio_impl__dowork_open__succeeds)
    {
        CONCRETE_IO_HANDLE create_result;
        int open_result;
        ///arrange
        reset_callback_context_records();
        create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        open_result = xio_impl_open_async(create_result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_open(IGNORED_PTR_ARG, &config_instance));
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_OK);

        ///act
        xio_impl_dowork(create_result);

        ///assert
        // Check that we got the on_open callback
        ASSERT_IO_OPEN_CALLBACK(true, IO_OPEN_OK);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_close_async(create_result, on_io_close_complete, NULL);
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_075: [ If the adapter is in XIO_IMPL_STATE_EXT_CLOSED then xio_impl_dowork shall do nothing. ]*/
    TEST_FUNCTION(xio_impl__dowork_pre_open__succeeds)
    {
        ///arrange
        CONCRETE_IO_HANDLE create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        umock_c_reset_all_calls();
        reset_callback_context_records();

        ///act
        xio_impl_dowork(create_result);

        ///assert
        ASSERT_NO_CALLBACKS();
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_070: [ If the xio_impl_handle parameter is NULL, xio_impl_dowork shall do nothing except log an error. ]*/
    TEST_FUNCTION(xio_impl__dowork_parameter_validation__fails)
    {
        ///arrange
        reset_callback_context_records();

        ///act
        xio_impl_dowork(NULL);

        ///assert
        ASSERT_NO_CALLBACKS();

        ///cleanup
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_120: [ If any of the the xio_impl_handle, optionName, or value parameters is NULL, xio_impl_setoption shall do nothing except log an error and return _FAILURE_. ]*/
    TEST_FUNCTION(xio_impl__setoption_parameter_validation__fails)
    {
        int k;
        // Parameters arrays
        bool p0[SETOPTION_PV_COUNT];
        const char* p1[SETOPTION_PV_COUNT];
        const char*  p2[SETOPTION_PV_COUNT];
        const char* fm[SETOPTION_PV_COUNT]; 
        int i;

        ///arrange
        umock_c_reset_all_calls();

        k = 0;
        p0[k] = false; p1[k] = "fake name"; p2[k] = "fake value"; fm[k] = "Unexpected setoption success when tlsio_handle is NULL"; /* */  k++;
        p0[k] = true; p1[k] = NULL; /*   */ p2[k] = "fake value"; fm[k] = "Unexpected setoption success when option_name is NULL"; /*  */  k++;
        p0[k] = true; p1[k] = "fake name"; p2[k] = NULL; /*    */ fm[k] = "Unexpected setoption success when option_value is NULL"; /* */  k++;


        // Cycle through each failing combo of parameters
        for (i = 0; i < SETOPTION_PV_COUNT; i++)
        {
            int result;
            ///arrange
            CONCRETE_IO_HANDLE create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
            ASSERT_IS_NOT_NULL(create_result);

            ///act
            result = xio_impl_setoption(p0[i] ? create_result : NULL, p1[i], p2[i]);

            ///assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, fm[i]);

            ///cleanup
            xio_impl_destroy(create_result);
        }
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_121 [ xio_impl shall delegate the behavior of xio_impl_setoption to the xio_endpoint_config supplied in xio_impl_create. ]*/
    TEST_FUNCTION(xio_impl__setoption__succeeds)
    {
        int result;
        ///arrange
        CONCRETE_IO_HANDLE create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(xio_endpoint_config_setoption(&config_instance, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        ///act
        result = xio_impl_setoption(create_result, "fake name", "fake value");

        ///assert
        ASSERT_ARE_EQUAL(int, 0, result);

        ///cleanup
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_160: [ If the xio_impl_handle parameter is NULL, xio_impl_retrieveoptions shall do nothing except log an error and return _FAILURE_. ]*/
    TEST_FUNCTION(xio_impl__retrieveoptions_parameter_validation__fails)
    {
        ///arrange

        ///act
        OPTIONHANDLER_HANDLE result = xio_impl_retrieveoptions(NULL);

        ///assert
        ASSERT_IS_NULL((void*)result);

        ///cleanup
		assert_gballoc_checks();
	}

    /* Tests_SRS_XIO_IMPL_30_161: [ xio_impl shall delegate the behavior of xio_impl_retrieveoptions to the xio_endpoint_config supplied in xio_impl_create. ]*/
    TEST_FUNCTION(xio_impl__retrieveoptions__succeeds)
    {
        OPTIONHANDLER_HANDLE result;
        ///arrange
        CONCRETE_IO_HANDLE create_result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(create_result);
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(xio_endpoint_config_retrieveoptions(&config_instance));

        ///act
        result = xio_impl_retrieveoptions(create_result);

        ///assert
        ASSERT_ARE_EQUAL(int, ((int)result), ((int)fake_option_handler));
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_destroy(create_result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_037: [ If the adapter is in any state other than XIO_IMPL_STATE_EXT_CLOSED when xio_impl_open_async is called, it shall log an error, and return _FAILURE_. ]*/
    /* Tests_SRS_XIO_IMPL_30_039: [ On failure, tlsio_open_async shall not call on_io_open_complete. ]*/
    TEST_FUNCTION(xio_impl__open_wrong_state__fails)
    {
        int open_result_2;
        ///arrange
        CONCRETE_IO_HANDLE result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        int open_result = xio_impl_open_async(result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);
        ASSERT_ARE_EQUAL(int, open_result, 0);
        reset_callback_context_records();

        ///act
        open_result_2 = xio_impl_open_async(result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);

        ///assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, open_result_2, 0, "Unexpected 2nd open success");
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);

        ///cleanup
        xio_impl_destroy(result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_030: [ If any of the xio_impl_handle, on_open_complete, on_bytes_received, or on_io_error parameters is NULL, xio_impl_open_async shall log an error and return _FAILURE_. ]*/
    TEST_FUNCTION(xio_impl__open_parameter_validation_fails__fails)
    {
        ///arrange

        // Parameters arrays
        bool p0[OPEN_PV_COUNT];
        ON_IO_OPEN_COMPLETE p1[OPEN_PV_COUNT];
        ON_BYTES_RECEIVED p2[OPEN_PV_COUNT];
        ON_IO_ERROR p3[OPEN_PV_COUNT];
        const char* fm[OPEN_PV_COUNT];
        int i;

        int k = 0;
        p0[k] = false; p1[k] = on_io_open_complete; p2[k] = on_bytes_received; p3[k] = on_io_error; fm[k] = "Unexpected open success when tlsio_handle is NULL"; /* */  k++;
        p0[k] = true; p1[k] = NULL; /*           */ p2[k] = on_bytes_received; p3[k] = on_io_error; fm[k] = "Unexpected open success when on_io_open_complete is NULL"; k++;
        p0[k] = true; p1[k] = on_io_open_complete; p2[k] = NULL; /*         */ p3[k] = on_io_error; fm[k] = "Unexpected open success when on_bytes_received is NULL"; k++;
        p0[k] = true; p1[k] = on_io_open_complete; p2[k] = on_bytes_received;  p3[k] = NULL; /*  */ fm[k] = "Unexpected open success when on_io_error is NULL"; /*   */ k++;

        // Cycle through each failing combo of parameters
        for (i = 0; i < OPEN_PV_COUNT; i++)
        {
            CONCRETE_IO_HANDLE result;
            int open_result;
            ///arrange
            reset_callback_context_records();
            result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);

            ///act
            open_result = xio_impl_open_async(p0[i] ? result : NULL, p1[i], IO_OPEN_COMPLETE_CONTEXT, p2[i],
                IO_BYTES_RECEIVED_CONTEXT, p3[i], IO_ERROR_CONTEXT);

            ///assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, open_result, 0, fm[i]);
            ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);

            ///cleanup
            xio_impl_destroy(result);
        }
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_035: [ On success, xio_impl_open_async shall cause the adapter to enter XIO_IMPL_STATE_EXT_OPENING and return 0. ]*/
    /* Tests_SRS_XIO_IMPL_30_034: [ On success, xio_impl_open_async shall store the provided on_bytes_received, on_bytes_received_context, on_io_error, on_io_error_context, on_open_complete, and on_open_complete_context parameters for later use as specified and tested per other line entries in this document. ]*/
    TEST_FUNCTION(xio_impl__open_async__succeeds)
    {
        int open_result;
        ///arrange
        CONCRETE_IO_HANDLE result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        umock_c_reset_all_calls();
        reset_callback_context_records();

        ///act
        open_result = xio_impl_open_async(result, on_io_open_complete, IO_OPEN_COMPLETE_CONTEXT, on_bytes_received,
            IO_BYTES_RECEIVED_CONTEXT, on_io_error, IO_ERROR_CONTEXT);

        ///assert
        ASSERT_ARE_EQUAL(int, open_result, 0);
        // Should not have made any callbacks yet
        ASSERT_IO_OPEN_CALLBACK(false, IO_OPEN_ERROR);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        // Ensure that the callbacks were stored properly in the internal instance

        ///cleanup
        xio_impl_destroy(result);
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_022: [ If the adapter is in any state other than TLSIO_STATE_EX_CLOSED when tlsio_destroy is called, the adapter shall enter TLSIO_STATE_EX_CLOSING and then enter TLSIO_STATE_EX_CLOSED before completing the destroy process. ]*/
    TEST_FUNCTION(xio_impl__destroy_with_unsent_messages__succeeds)
    {
        CONCRETE_IO_HANDLE xio_impl;
        int send_result;
        ///arrange
        xio_impl = open_helper();


        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(0);
        STRICT_EXPECTED_CALL(xio_endpoint_write(&endpoint_instance, IGNORED_PTR_ARG, SSL_SHORT_SENT_MESSAGE_SIZE)).SetReturn(0);
        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_SHORT_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);
        send_result = xio_impl_send_async(xio_impl, SSL_send_buffer,
            SSL_SHORT_SENT_MESSAGE_SIZE, on_io_send_complete, IO_SEND_COMPLETE_CONTEXT);
        ASSERT_ARE_EQUAL(int, send_result, 0);

        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(xio_endpoint_close(IGNORED_PTR_ARG));
        // message 1
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        // message 2
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(xio_endpoint_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_config_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        // End of arrange

        ///act
        xio_impl_destroy(xio_impl);

        ///assert
        ASSERT_IO_SEND_ABANDONED(2); // 2 messages in this test
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_020: [ If tlsio_handle is NULL, tlsio_destroy shall do nothing. ]*/
    TEST_FUNCTION(xio_impl__destroy_NULL__succeeds)
    {
        ///arrange

        ///act
        xio_impl_destroy(NULL);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_021: [ The xio_impl_destroy shall release all allocated resources and then release xio_impl_handle. ]*/
    TEST_FUNCTION(xio_impl__destroy_unopened__succeeds)
    {
        ///arrange
        CONCRETE_IO_HANDLE result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);
        ASSERT_IS_NOT_NULL(result);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(xio_endpoint_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_config_destroy(&config_instance));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG)); 
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_NUM_ARG));

        ///act
        xio_impl_destroy(result);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_011: [ If any resource allocation fails, xio_impl_create shall return NULL. ]*/
    TEST_FUNCTION(xio_impl__create_unhappy_paths__fails)
    {
        size_t i;
        ///arrange
        use_negative_mocks();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(xio_endpoint_create());
        umock_c_negative_tests_snapshot();

        for (i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            CONCRETE_IO_HANDLE result;
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);

            ///assert
            ASSERT_IS_NULL(result);
        }

        ///cleanup
        assert_gballoc_checks();
    }

    /* Tests_SRS_XIO_IMPL_30_013: [ If any of the input parameters is NULL, xio_impl_create shall log an error and return NULL. ]*/
    TEST_FUNCTION(xio_impl__create_parameter_validation_fails__fails)
    {
        ///arrange
        int i;
        create_params params[3] = {
            { NULL, &endpoint_config_interface, &config_instance },
            { &endpoint_interface, NULL, &config_instance },
            { &endpoint_interface, &endpoint_config_interface, NULL }
        };

        // Cycle through each failing combo of parameters
        for (i = 0; i < 3; i++)
        {
            ///act
            CONCRETE_IO_HANDLE result = xio_impl_create(params[i].endpoint_interface, 
                params[i].xio_endpoint_config_interface, params[i].xio_endpoint_config_instance);

            ///assert
            ASSERT_IS_NULL_WITH_MSG(result, "Unexpected success in xio_impl_create");
        }
        assert_gballoc_checks();
    }


    /* Tests_SRS_XIO_IMPL_30_010: [ The xio_impl_create shall allocate and initialize all necessary resources and return an instance of the xio_impl. ]*/
    TEST_FUNCTION(xio_impl__create__succeeds)
    {
        CONCRETE_IO_HANDLE result;
        ///arrange

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)); 
        STRICT_EXPECTED_CALL(xio_endpoint_create()); 

        ///act
        result = xio_impl_create(&endpoint_interface, &endpoint_config_interface, &config_instance);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        xio_impl_destroy(result);
        assert_gballoc_checks();
    }

END_TEST_SUITE(xio_impl_unittests)
