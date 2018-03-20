// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/tlsio_options.h"
#include "azure_c_shared_utility/xio_impl.h"
#include "azure_c_shared_utility/xio_endpoint.h"
#include "azure_c_shared_utility/xio_endpoint_config.h"

typedef struct
{
    unsigned char* bytes;
    size_t size;
    size_t unsent_size;
    ON_SEND_COMPLETE on_send_complete;
    void* callback_context;
} PENDING_TRANSMISSION;

#ifdef __APPLE__
#define USE_NO_CERT_PARAM_HEADER
#endif

// The use of USE_NO_CERT_PARAM_HEADER is not unit tested because the only meaningful
// test of this is to ensure that AMQP and MQTT over websockets work when 
// used with Mac and iOS.
#ifdef USE_NO_CERT_PARAM_HEADER
const char WEBSOCKET_HEADER_START[] = "GET /$iothub/websocket";
const char WEBSOCKET_HEADER_NO_CERT_PARAM[] = "?iothub-no-client-cert=true";
const size_t WEBSOCKET_HEADER_START_SIZE = sizeof(WEBSOCKET_HEADER_START) - 1;
const size_t WEBSOCKET_HEADER_NO_CERT_PARAM_SIZE = sizeof(WEBSOCKET_HEADER_NO_CERT_PARAM) - 1;
#endif // USE_NO_CERT_PARAM_HEADER

#define MAX_VALID_PORT 0xffff

// The XIO_RECEIVE_BUFFER_SIZE has very little effect on performance, and is kept small
// to minimize memory consumption.
#define XIO_RECEIVE_BUFFER_SIZE 64


typedef enum XIO_IMPL_STATE_TAG
{
    XIO_IMPL_STATE_CLOSED,
    XIO_IMPL_STATE_CLOSING,
    XIO_IMPL_STATE_OPENING,
    XIO_IMPL_STATE_OPEN,
    XIO_IMPL_STATE_ERROR,
} XIO_IMPL_STATE;

typedef struct XIO_IMPL_TAG
{
    ON_BYTES_RECEIVED on_bytes_received;
    ON_IO_ERROR on_io_error;
    ON_IO_OPEN_COMPLETE on_open_complete;
    void* on_bytes_received_context;
    void* on_io_error_context;
    void* on_open_complete_context;
    XIO_IMPL_STATE xio_impl_state;
#ifdef USE_NO_CERT_PARAM_HEADER
    bool no_messages_yet_sent;
#endif // USE_NO_CERT_PARAM_HEADER
    SINGLYLINKEDLIST_HANDLE pending_transmission_list;
    const XIO_ENDPOINT_INTERFACE* xio_endpoint_interface;
    XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance;
    XIO_ENDPOINT_CONFIG_INTERFACE* xio_endpoint_config_interface;
    XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config_instance;

} XIO_IMPL;

/* Codes_SRS_XIO_IMPL_30_005: [ The phrase "enter XIO_IMPL_STATE_EXT_ERROR" means the adapter shall call the on_io_error function and pass the on_io_error_context that was supplied in xio_impl_open_async. ]*/
/* Codes_SRS_XIO_IMPL_30_082: [ If the xio_endpoint returns XIO_ASYNC_RESULT_FAILURE, xio_impl_dowork shall log an error, call on_open_complete with the on_open_complete_context parameter provided in xio_impl_open_async and IO_OPEN_ERROR, and enter XIO_IMPL_STATE_EXT_CLOSED. ] */
static void enter_xio_impl_error_state(XIO_IMPL* xio_impl)
{
    if (xio_impl->xio_impl_state != XIO_IMPL_STATE_ERROR)
    {
        xio_impl->xio_impl_state = XIO_IMPL_STATE_ERROR;
        xio_impl->on_io_error(xio_impl->on_io_error_context);
    }
}

/* Codes_SRS_XIO_IMPL_30_005: [ When the adapter enters XIO_IMPL_STATE_EXT_ERROR it shall call the  on_io_error function and pass the on_io_error_context that were supplied in  xio_impl_open . ]*/
static void enter_open_error_state(XIO_IMPL* xio_impl)
{
    // save instance variables in case the framework destroys this object before we exit
    ON_IO_OPEN_COMPLETE on_open_complete = xio_impl->on_open_complete;
    void* on_open_complete_context = xio_impl->on_open_complete_context;
    enter_xio_impl_error_state(xio_impl);
    on_open_complete(on_open_complete_context, IO_OPEN_ERROR);
}

// Return true if a message was available to remove
static bool process_and_destroy_head_message(XIO_IMPL* xio_impl, IO_SEND_RESULT send_result)
{
    bool result;
    LIST_ITEM_HANDLE head_pending_io;
    if (send_result == IO_SEND_ERROR)
    {
        /* Codes_SRS_XIO_IMPL_30_095: [ If the send process fails before sending all of the bytes in an enqueued message, the xio_impl_dowork shall call the message's on_send_complete along with its associated callback_context and IO_SEND_ERROR. ]*/
        enter_xio_impl_error_state(xio_impl);
    }
    head_pending_io = singlylinkedlist_get_head_item(xio_impl->pending_transmission_list);
    if (head_pending_io != NULL)
    {
        PENDING_TRANSMISSION* head_message = (PENDING_TRANSMISSION*)singlylinkedlist_item_get_value(head_pending_io);

        if (singlylinkedlist_remove(xio_impl->pending_transmission_list, head_pending_io) != 0)
        {
            // This particular situation is a bizarre and unrecoverable internal error
            /* Codes_SRS_XIO_IMPL_30_094: [ If the send process encounters an internal error or calls on_send_complete with IO_SEND_ERROR due to either failure or timeout, it shall also call on_io_error and pass in the associated on_io_error_context. ]*/
            enter_xio_impl_error_state(xio_impl);
            LogError("Failed to remove message from list");
        }

        // on_send_complete is checked for NULL during PENDING_TRANSMISSION creation
        /* Codes_SRS_XIO_IMPL_30_095: [ If the send process fails before sending all of the bytes in an enqueued message, the xio_impl_dowork shall call the message's on_send_complete along with its associated callback_context and IO_SEND_ERROR. ]*/
        head_message->on_send_complete(head_message->callback_context, send_result);

        free(head_message->bytes);
        free(head_message);
        result = true;
    }
    else
    {
        result = false;
    }
    return result;
}

static void internal_close(XIO_IMPL* xio_impl)
{
    /* Codes_SRS_XIO_IMPL_30_009: [ The phrase "enter XIO_IMPL_STATE_EXT_CLOSING" means the adapter shall iterate through any unsent messages in the queue and shall delete each message after calling its on_send_complete with the associated callback_context and IO_SEND_CANCELLED. ]*/
    /* Codes_SRS_XIO_IMPL_30_006: [ The phrase "enter XIO_IMPL_STATE_EXT_CLOSED" means the adapter shall forcibly close any existing connections then call the on_io_close_complete function and pass the on_io_close_complete_context that was supplied in xio_impl_close_async. ]*/
    XIO_ASYNC_RESULT close_result = xio_impl->xio_endpoint_interface->close(xio_impl->xio_endpoint_instance);

    while (process_and_destroy_head_message(xio_impl, IO_SEND_CANCELLED));
    // singlylinkedlist_destroy gets called in the main destroy

    xio_impl->on_bytes_received = NULL;
    xio_impl->on_io_error = NULL;
    xio_impl->on_bytes_received_context = NULL;
    xio_impl->on_io_error_context = NULL;
    /* Codes_SRS_XIO_IMPL_30_105: [ The xio_impl_dowork shall delegate close behavior to the xio_endpoint provided during xio_impl_open_async. ]*/
    /* Codes_SRS_XIO_IMPL_30_106: [ If the xio_endpoint returns XIO_ASYNC_RESULT_FAILURE, xio_impl_dowork shall log an error and enter XIO_IMPL_STATE_EXT_CLOSED. ]*/
    /* Codes_SRS_XIO_IMPL_30_107: [ If the xio_endpoint returns XIO_ASYNC_RESULT_SUCCESS, xio_impl_dowork shall enter XIO_IMPL_STATE_EXT_CLOSED. ]*/
    /* Codes_SRS_XIO_IMPL_30_108: [ If the xio_endpoint returns XIO_ASYNC_RESULT_WAITING, xio_impl_dowork shall remain in the XIO_IMPL_STATE_EXT_CLOSING state. ]*/
    xio_impl->xio_impl_state = close_result == XIO_ASYNC_RESULT_WAITING ? XIO_IMPL_STATE_CLOSING : XIO_IMPL_STATE_CLOSED;
    xio_impl->on_open_complete = NULL;
    xio_impl->on_open_complete_context = NULL;
}

void xio_impl_destroy(CONCRETE_IO_HANDLE xio_impl_in)
{
    if (xio_impl_in == NULL)
    {
        /* Codes_SRS_XIO_IMPL_30_020: [ If xio_impl_in is NULL, xio_impl_destroy shall do nothing. ]*/
        LogError("NULL xio_impl_in");
    }
    else
    {
        XIO_IMPL* xio_impl = (XIO_IMPL*)xio_impl_in;
        if (xio_impl->xio_impl_state != XIO_IMPL_STATE_CLOSED)
        {
            /* Codes_SRS_XIO_IMPL_30_022: [ If the adapter is in any state other than XIO_IMPL_STATE_EX_CLOSED when xio_impl_destroy is called, the adapter shall enter XIO_IMPL_STATE_EX_CLOSING and then enter XIO_IMPL_STATE_EX_CLOSED before completing the destroy process. ]*/
            LogError("xio_impl_destroy called while not in XIO_IMPL_STATE_CLOSED.");
            internal_close(xio_impl);
        }
        /* Codes_SRS_XIO_IMPL_30_021: [ The xio_impl_destroy shall release all allocated resources and then release xio_impl_handle. ]*/
        xio_impl->xio_endpoint_interface->destroy(xio_impl->xio_endpoint_instance);
        xio_impl->xio_endpoint_config_interface->destroy(xio_impl->xio_endpoint_config_instance);
        singlylinkedlist_destroy(xio_impl->pending_transmission_list);

        free(xio_impl);
    }
}

/* Codes_SRS_XIO_IMPL_30_010: [ The xio_impl_create shall allocate and initialize all necessary resources and return an instance of the xio_impl_compact. ]*/
CONCRETE_IO_HANDLE xio_impl_create(const XIO_ENDPOINT_INTERFACE* endpoint_interface,
                                   XIO_ENDPOINT_CONFIG_INTERFACE* xio_endpoint_config_interface,
                                   XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config_instance)
{
    XIO_IMPL* result;

    if (endpoint_interface == NULL || xio_endpoint_config_interface == NULL || xio_endpoint_config_instance == NULL)
    {
        /* Codes_SRS_XIO_IMPL_30_013: [ If any of the input parameters is NULL, xio_impl_create shall log an error and return NULL. ]*/
        LogError("NULL parameter to xio_impl_create");
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(XIO_IMPL));
        if (result == NULL)
        {
            /* Codes_SRS_XIO_IMPL_30_011: [ If any resource allocation fails, xio_impl_create shall return NULL. ]*/
            LogError("malloc failed");
        }
        else
        {
            memset(result, 0, sizeof(XIO_IMPL));
            result->xio_impl_state = XIO_IMPL_STATE_CLOSED;
            result->xio_endpoint_interface = endpoint_interface;
            result->xio_endpoint_config_instance = NULL;
            result->xio_endpoint_config_interface = xio_endpoint_config_interface;
            result->xio_endpoint_config_instance = xio_endpoint_config_instance;
            result->pending_transmission_list = NULL;
            // Create the message queue
            result->pending_transmission_list = singlylinkedlist_create();
            if (result->pending_transmission_list == NULL)
            {
                /* Codes_SRS_XIO_IMPL_30_011: [ If any resource allocation fails, xio_impl_create shall return NULL. ]*/
                LogError("Failed singlylinkedlist_create");
                xio_impl_destroy(result);
                result = NULL;
            }
            else if ((result->xio_endpoint_instance = result->xio_endpoint_interface->create()) == NULL)
            {
                /* Codes_SRS_XIO_IMPL_30_011: [ If any resource allocation fails, xio_impl_create shall return NULL. ]*/
                LogError("Failed xio_endpoint create");
                xio_impl_destroy(result);
                result = NULL;
            }
        }
    }

    return (CONCRETE_IO_HANDLE)result;
}


int xio_impl_open_async(CONCRETE_IO_HANDLE xio_impl_in,
    ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_context,
    ON_BYTES_RECEIVED on_bytes_received, void* on_bytes_received_context,
    ON_IO_ERROR on_io_error, void* on_io_error_context)
{
    int result;
    /* Codes_SRS_XIO_IMPL_30_030: [ If any of the xio_impl_handle, on_open_complete, on_bytes_received, or on_io_error parameters is NULL, xio_impl_open_async shall log an error and return _FAILURE_. ]*/
    if (xio_impl_in == NULL || on_io_open_complete == NULL || on_bytes_received == NULL || on_io_error == NULL)
    {
        LogError("Required parameter is NULL");
        result = __FAILURE__;
    }
    else
    {
        XIO_IMPL* xio_impl = (XIO_IMPL*)xio_impl_in;

        if (xio_impl->xio_impl_state != XIO_IMPL_STATE_CLOSED)
        {
            /* Codes_SRS_XIO_IMPL_30_037: [ If the adapter is in any state other than XIO_IMPL_STATE_EXT_CLOSED when xio_impl_open  is called, it shall log an error, and return FAILURE. ]*/
            LogError("Invalid xio_impl_state. Expected state is XIO_IMPL_STATE_CLOSED.");
            result = __FAILURE__;
        }
        else
        {
#ifdef USE_NO_CERT_PARAM_HEADER
            xio_impl->no_messages_yet_sent = true;
#endif // USE_NO_CERT_PARAM_HEADER
            /* Codes_SRS_XIO_IMPL_30_034: [ The xio_impl_open shall store the provided on_bytes_received, on_bytes_received_context, on_io_error, on_io_error_context, on_io_open_complete, and on_io_open_complete_context parameters for later use as specified and tested per other line entries in this document. ]*/
            xio_impl->on_bytes_received = on_bytes_received;
            xio_impl->on_bytes_received_context = on_bytes_received_context;

            xio_impl->on_io_error = on_io_error;
            xio_impl->on_io_error_context = on_io_error_context;

            xio_impl->on_open_complete = on_io_open_complete;
            xio_impl->on_open_complete_context = on_io_open_complete_context;

            /* Codes_SRS_XIO_IMPL_30_035: [ On xio_impl_open success the adapter shall enter XIO_IMPL_STATE_EX_OPENING and return 0. ]*/
            // All the real work happens in dowork
            xio_impl->xio_impl_state = XIO_IMPL_STATE_OPENING;
            result = 0;
        }
    }
    /* Codes_SRS_XIO_IMPL_30_039: [ On failure, xio_impl_open_async shall not call on_io_open_complete. ]*/

    return result;
}

int xio_impl_close_async(CONCRETE_IO_HANDLE xio_impl_in, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context)
{
    int result;

    if (xio_impl_in == NULL)
    {
        /* Codes_SRS_XIO_IMPL_30_050: [ If the xio_impl_in parameter is NULL, xio_impl_close_async shall log an error and return FAILURE. ]*/
        LogError("NULL xio_impl_in");
        result = __FAILURE__;
    }
    else
    {
        if (on_io_close_complete == NULL)
        {
            /* Codes_SRS_XIO_IMPL_30_055: [ If the on_io_close_complete parameter is NULL, xio_impl_close_async shall log an error and return FAILURE. ]*/
            LogError("NULL on_io_close_complete");
            result = __FAILURE__;
        }
        else
        {
            XIO_IMPL* xio_impl = (XIO_IMPL*)xio_impl_in;

            if (xio_impl->xio_impl_state != XIO_IMPL_STATE_OPEN &&
                xio_impl->xio_impl_state != XIO_IMPL_STATE_ERROR)
            {
                /* Codes_SRS_XIO_IMPL_30_053: [ If the adapter is in any state other than XIO_IMPL_STATE_EXT_OPEN or XIO_IMPL_STATE_EXT_ERROR then xio_impl_close_async shall log that xio_impl_close_async has been called and then continue normally. ]*/
                // LogInfo rather than LogError because this is an unusual but not erroneous situation
                LogInfo("xio_impl_close has been called when in neither XIO_IMPL_STATE_OPEN nor XIO_IMPL_STATE_ERROR.");
            }

            if (xio_impl->xio_impl_state == XIO_IMPL_STATE_OPENING)
            {
                /* Codes_SRS_XIO_IMPL_30_057: [ On success, if the adapter is in XIO_IMPL_STATE_EXT_OPENING, it shall call on_io_open_complete with the on_io_open_complete_context supplied in xio_impl_open_async and IO_OPEN_CANCELLED. This callback shall be made before changing the internal state of the adapter. ]*/
                xio_impl->on_open_complete(xio_impl->on_open_complete_context, IO_OPEN_CANCELLED);
            }
            // This adapter does not support asynchronous closing
            /* Codes_SRS_XIO_IMPL_30_056: [ On success the adapter shall enter XIO_IMPL_STATE_EX_CLOSING. ]*/
            /* Codes_SRS_XIO_IMPL_30_051: [ On success,  xio_impl_close_async  shall invoke the XIO_IMPL_STATE_EXT_CLOSING behavior of  xio_impl_dowork . ]*/
            /* Codes_SRS_XIO_IMPL_30_052: [ On success xio_impl_close shall return 0. ]*/
            internal_close(xio_impl);
            on_io_close_complete(callback_context);
            result = 0;
        }
    }
    /* Codes_SRS_XIO_IMPL_30_054: [ On failure, the adapter shall not call on_io_close_complete. ]*/

    return result;
}

static void dowork_read(XIO_IMPL* xio_impl)
{
    // TRANSFER_BUFFER_SIZE is not very important because if the message is bigger
    // then the framework just calls dowork repeatedly until it gets everything. So
    // a bigger buffer would just use memory without buying anything.
    // Putting this buffer in a small function also allows it to exist on the stack
    // rather than adding to heap fragmentation.
    uint8_t buffer[XIO_RECEIVE_BUFFER_SIZE];
    int rcv_bytes;

    if (xio_impl->xio_impl_state == XIO_IMPL_STATE_OPEN)
    {
        rcv_bytes = 1;
        while (rcv_bytes > 0)
        {
            // We are guaranteeing that buffer is smaller than MAX_INT
            /* Codes_SRS_XIO_IMPL_30_101: [ The xio_impl_dowork shall delegate recieve behavior to the xio_endpoint provided during xio_impl_open_async. ]*/
            rcv_bytes = xio_impl->xio_endpoint_interface->read(xio_impl->xio_endpoint_instance, buffer, (uint32_t)sizeof(buffer));
            
            if (rcv_bytes > 0)
            {
                // xio_impl->on_bytes_received was already checked for NULL
                // in the call to xio_impl_open_async
                /* Codes_SRS_XIO_IMPL_30_100: [ If the xio_endpoint returns a positive number of bytes received, xio_impl_dowork shall repeatedly read this data and call on_bytes_received with the pointer to the buffer containing the data, the number of bytes received, and the on_bytes_received_context. ]*/
                xio_impl->on_bytes_received(xio_impl->on_bytes_received_context, buffer, rcv_bytes);
            }
            else if (rcv_bytes < 0)
            {
                /* Codes_SRS_XIO_IMPL_30_101: [ If the xio_endpoint returns XIO_ASYNC_RW_RESULT_FAILURE then xio_impl_dowork shall enter XIO_IMPL_STATE_EXT_ERROR. ]*/
                LogInfo("Communications error while reading");
                enter_xio_impl_error_state(xio_impl);
            }
        }
        /* Codes_SRS_XIO_IMPL_30_102: [ If the xio_endpoint returns 0 bytes received then xio_impl_dowork shall not call the on_bytes_received callback. ]*/
    }
}

static void dowork_send(XIO_IMPL* xio_impl)
{
    LIST_ITEM_HANDLE first_pending_io = singlylinkedlist_get_head_item(xio_impl->pending_transmission_list);
    if (first_pending_io != NULL)
    {
        PENDING_TRANSMISSION* pending_message = (PENDING_TRANSMISSION*)singlylinkedlist_item_get_value(first_pending_io);
        uint8_t* buffer = ((uint8_t*)pending_message->bytes) + pending_message->size - pending_message->unsent_size;

        /* Codes_SRS_XIO_IMPL_30_090: [ The  xio_impl_dowork  shall delegate send behavior to the  xio_endpoint  provided during  xio_impl_open_async . ]*/
        int write_result =xio_impl->xio_endpoint_interface->write(xio_impl->xio_endpoint_instance, buffer, (uint32_t)pending_message->unsent_size);
        if (write_result > 0)
        {
            pending_message->unsent_size -= write_result;
            if (pending_message->unsent_size == 0)
            {
                /* Codes_SRS_XIO_IMPL_30_091: [ If xio_impl_dowork is able to send all the bytes in an enqueued message, it shall call the messages's on_send_complete along with its associated callback_context and IO_SEND_OK. ]*/
                // The whole message has been sent successfully
                process_and_destroy_head_message(xio_impl, IO_SEND_OK);
            }
            else
            {
                /* Codes_SRS_XIO_IMPL_30_093: [ If the TLS connection was not able to send an entire enqueued message at once, subsequent calls to xio_impl_dowork shall continue to send the remaining bytes. ]*/
                // Repeat the send on the next pass with the rest of the message
                // This empty else compiles to nothing but helps readability
            }
        }
        else if (write_result < 0)
        {
            // This is an unexpected error, and we need to bail out. Probably lost internet connection.
            LogInfo("Unrecoverable error from xio_endpoint_write");
            process_and_destroy_head_message(xio_impl, IO_SEND_ERROR);
        }
    }
    else
    {
        /* Codes_SRS_XIO_IMPL_30_096: [ If there are no enqueued messages available, xio_impl_dowork shall do nothing. ]*/
    }
}

static void dowork_poll_open(XIO_IMPL* xio_impl)
{
    /* Codes_SRS_XIO_IMPL_30_080: [ The xio_impl_dowork shall delegate open behavior to the xio_endpoint provided during xio_impl_open_async. ]*/
    XIO_ASYNC_RESULT result = xio_impl->xio_endpoint_interface->open(xio_impl->xio_endpoint_instance,  xio_impl->xio_endpoint_config_instance);
    if (result == XIO_ASYNC_RESULT_SUCCESS)
    {   
        // Connect succeeded
        xio_impl->xio_impl_state = XIO_IMPL_STATE_OPEN;
        /* Codes_SRS_XIO_IMPL_30_007: [ The phrase "enter XIO_IMPL_STATE_EXT_OPEN" means the adapter shall call the on_io_open_complete function and pass IO_OPEN_OK and the on_io_open_complete_context that was supplied in xio_impl_open . ]*/
        /* Codes_SRS_XIO_IMPL_30_083: [ If xio_endpoint returns XIO_ASYNC_RESULT_SUCCESS, xio_impl_dowork shall enter XIO_IMPL_STATE_EXT_OPEN. ]*/
        xio_impl->on_open_complete(xio_impl->on_open_complete_context, IO_OPEN_OK);
    }
    else if (result == XIO_ASYNC_RESULT_FAILURE)
    {
        /* Codes_SRS_XIO_IMPL_30_082: [ If the xio_endpoint returns XIO_ASYNC_RESULT_FAILURE, xio_impl_dowork shall log an error, call on_open_complete with the on_open_complete_context parameter provided in xio_impl_open_async and IO_OPEN_ERROR, and enter XIO_IMPL_STATE_EXT_CLOSED. ] */
        enter_open_error_state(xio_impl);
    }
    /* Codes_SRS_XIO_IMPL_30_081: [ If the  xio_endpoint  returns  XIO_ASYNC_RESULT_WAITING ,  xio_impl_dowork  shall remain in the XIO_IMPL_STATE_EXT_OPENING state. ] */
}

void xio_impl_dowork(CONCRETE_IO_HANDLE xio_impl_in)
{
    if (xio_impl_in == NULL)
    {
        /* Codes_SRS_XIO_IMPL_30_070: [ If the xio_impl_in parameter is NULL, xio_impl_dowork shall do nothing except log an error. ]*/
        LogError("NULL xio_impl_in");
    }
    else
    {
        XIO_IMPL* xio_impl = (XIO_IMPL*)xio_impl_in;

        // This switch statement handles all of the state transitions during the opening process
        switch (xio_impl->xio_impl_state)
        {
        case XIO_IMPL_STATE_CLOSED:
            /* Codes_SRS_XIO_IMPL_30_075: [ If the adapter is in XIO_IMPL_STATE_EXT_CLOSED then  xio_impl_dowork  shall do nothing. ]*/
            // Waiting to be opened, nothing to do
            break;
        case XIO_IMPL_STATE_CLOSING:
            internal_close(xio_impl);
            break;
        case XIO_IMPL_STATE_OPENING:
            dowork_poll_open(xio_impl);
            break;
        case XIO_IMPL_STATE_OPEN:
            dowork_read(xio_impl);
            dowork_send(xio_impl);
            break;
        case XIO_IMPL_STATE_ERROR:
            /* Codes_SRS_XIO_IMPL_30_071: [ If the adapter is in XIO_IMPL_STATE_EXT_ERROR then xio_impl_dowork shall do nothing. ]*/
            // There's nothing valid to do here but wait to be retried
            break;
        default:
            LogError("Unexpected internal state");
            break;
        }
    }
}

int xio_impl_setoption(CONCRETE_IO_HANDLE xio_impl_in, const char* optionName, const void* value)
{
    XIO_IMPL* xio_impl = (XIO_IMPL*)xio_impl_in;
    /* Codes_SRS_XIO_IMPL_30_120: [ If any of the the xio_impl_handle, optionName, or value parameters is NULL, xio_impl_setoption shall do nothing except log an error and return _FAILURE_. ]*/
    int result;
    if (xio_impl == NULL || optionName == NULL || value == NULL)
    {
        LogError("NULL required parameter");
        result = __FAILURE__;
    }
    else
    {
        /* Codes_SRS_XIO_IMPL_30_121 [ xio_impl shall delegate the behavior of xio_impl_setoption to the xio_endpoint_config supplied in xio_impl_create. ]*/
        TLSIO_OPTIONS_RESULT options_result = xio_impl->xio_endpoint_config_interface->setoption(xio_impl->xio_endpoint_config_instance, optionName, value);
        if (options_result != TLSIO_OPTIONS_RESULT_SUCCESS)
        {
            LogError("Failed xio_impl_setoption");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

int xio_impl_send_async(CONCRETE_IO_HANDLE xio_impl_in, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context)
{
    int result;
    if (on_send_complete == NULL || xio_impl_in == NULL || buffer == NULL || size == 0 || size >= INT_MAX || on_send_complete == NULL)
    {
        /* Codes_SRS_XIO_IMPL_30_062: [ If the on_send_complete is NULL, xio_impl_send_async shall log the error and return FAILURE. ]*/
        result = __FAILURE__;
        LogError("Invalid parameter specified: xio_impl_in: %p, buffer: %p, size: %zu, on_send_complete: %p", xio_impl_in, buffer, size, on_send_complete);
    }
    else
    {
        XIO_IMPL* xio_impl = (XIO_IMPL*)xio_impl_in;
        if (xio_impl->xio_impl_state != XIO_IMPL_STATE_OPEN)
        {
            /* Codes_SRS_XIO_IMPL_30_060: [ If the xio_impl_in parameter is NULL, xio_impl_send_async shall log an error and return FAILURE. ]*/
            /* Codes_SRS_XIO_IMPL_30_065: [ If xio_impl_open has not been called or the opening process has not been completed, xio_impl_send_async shall log an error and return FAILURE. ]*/
            result = __FAILURE__;
            LogError("xio_impl_send_async without a prior successful open");
        }
        else
        {
            PENDING_TRANSMISSION* pending_transmission = (PENDING_TRANSMISSION*)malloc(sizeof(PENDING_TRANSMISSION));
            if (pending_transmission == NULL)
            {
                /* Codes_SRS_XIO_IMPL_30_064: [ If the supplied message cannot be enqueued for transmission, xio_impl_send_async shall log an error and return FAILURE. ]*/
                result = __FAILURE__;
                LogError("malloc failed");
            }
            else
            {
#ifdef USE_NO_CERT_PARAM_HEADER
                // For AMQP and MQTT over websockets, the interaction of the IoT Hub and the
                // Apple TLS requires hacking the websocket upgrade header with a
                // "iothub-no-client-cert=true" parameter to avoid a TLS hang.
                bool add_no_cert_url_parameter = false;
                if (xio_impl->no_messages_yet_sent)
                {
                    xio_impl->no_messages_yet_sent = false;
                    if (strncmp((const char*)buffer, WEBSOCKET_HEADER_START, WEBSOCKET_HEADER_START_SIZE) == 0)
                    {
                        add_no_cert_url_parameter = true;
                        size += WEBSOCKET_HEADER_NO_CERT_PARAM_SIZE;
                    }
                }
#endif // USE_NO_CERT_PARAM_HEADER

                /* Codes_SRS_XIO_IMPL_30_063: [ The xio_impl_send_async shall enqueue for transmission the on_send_complete, the callback_context, the size, and the contents of buffer. ]*/
                if ((pending_transmission->bytes = (unsigned char*)malloc(size)) == NULL)
                {
                    /* Codes_SRS_XIO_IMPL_30_064: [ If the supplied message cannot be enqueued for transmission, xio_impl_send_async shall log an error and return FAILURE. ]*/
                    LogError("malloc failed");
                    free(pending_transmission);
                    result = __FAILURE__;
                }
                else
                {
                    pending_transmission->size = size;
                    pending_transmission->unsent_size = size;
                    pending_transmission->on_send_complete = on_send_complete;
                    pending_transmission->callback_context = callback_context;
#ifdef USE_NO_CERT_PARAM_HEADER
                    if (add_no_cert_url_parameter)
                    {
                        // Insert the WEBSOCKET_HEADER_NO_CERT_PARAM after the url
                        (void)memcpy(pending_transmission->bytes, WEBSOCKET_HEADER_START, WEBSOCKET_HEADER_START_SIZE);
                        (void)memcpy(pending_transmission->bytes + WEBSOCKET_HEADER_START_SIZE, WEBSOCKET_HEADER_NO_CERT_PARAM, WEBSOCKET_HEADER_NO_CERT_PARAM_SIZE);
                        (void)memcpy(pending_transmission->bytes + WEBSOCKET_HEADER_START_SIZE + WEBSOCKET_HEADER_NO_CERT_PARAM_SIZE, buffer + WEBSOCKET_HEADER_START_SIZE, size - WEBSOCKET_HEADER_START_SIZE - WEBSOCKET_HEADER_NO_CERT_PARAM_SIZE);
                    }
                    else
                    {
#endif //USE_NO_CERT_PARAM_HEADER
                        (void)memcpy(pending_transmission->bytes, buffer, size);
#ifdef USE_NO_CERT_PARAM_HEADER
                    }
#endif // USE_NO_CERT_PARAM_HEADER

                    if (singlylinkedlist_add(xio_impl->pending_transmission_list, pending_transmission) == NULL)
                    {
                        /* Codes_SRS_XIO_IMPL_30_064: [ If the supplied message cannot be enqueued for transmission, xio_impl_send_async shall log an error and return FAILURE. ]*/
                        LogError("Unable to add socket to pending list.");
                        free(pending_transmission->bytes);
                        free(pending_transmission);
                        result = __FAILURE__;
                    }
                    else
                    {
                        /* Codes_SRS_XIO_IMPL_30_063: [ On success,  xio_impl_send_async  shall enqueue for transmission the  on_send_complete , the  callback_context , the  size , and the contents of  buffer  and then return 0. ]*/
                        result = 0;
                        if (xio_impl->xio_impl_state == XIO_IMPL_STATE_OPEN)
                        {
                            dowork_send(xio_impl);
                        }
                    }
                }
            }
        }
        /* Codes_SRS_XIO_IMPL_30_066: [ On failure, on_send_complete shall not be called. ]*/
    }
    return result;
}

/* Codes_SRS_XIO_IMPL_30_161: [ xio_impl shall delegate the behavior of xio_impl_retrieveoptions to the xio_endpoint_config supplied in xio_impl_create. ]*/
OPTIONHANDLER_HANDLE xio_impl_retrieveoptions(CONCRETE_IO_HANDLE xio_impl_in)
{
    XIO_IMPL* xio_impl = (XIO_IMPL*)xio_impl_in;
    /* Codes_SRS_XIO_IMPL_30_160: [ If the xio_impl_in parameter is NULL, xio_impl_retrieveoptions shall do nothing except log an error and return FAILURE. ]*/
    OPTIONHANDLER_HANDLE result;
    if (xio_impl == NULL)
    {
        LogError("NULL xio_impl_in");
        result = NULL;
    }
    else
    {
        result = xio_impl->xio_endpoint_config_interface->retrieveoptions(xio_impl->xio_endpoint_config_instance);
    }
    return result;
}

