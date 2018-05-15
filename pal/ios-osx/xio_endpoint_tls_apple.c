// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/tlsio_options.h"
#include "azure_c_shared_utility/xio_endpoint.h"
#include "xio_endpoint_config_tls.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFError.h>
#include <CFNetwork/CFSocketStream.h>
#include <Security/SecureTransport.h>

typedef struct APPLE_ENDPOINT_INSTANCE_TAG
{
    CFReadStreamRef sockRead;
    CFWriteStreamRef sockWrite;
} APPLE_ENDPOINT_INSTANCE;

/* Codes_SRS_XIO_ENDPOINT_30_000: [ The xio_endpoint_create shall allocate and initialize all necessary resources and return an instance of the xio_endpoint. ]*/
XIO_ENDPOINT_INSTANCE_HANDLE apple_tls_create()
{
    APPLE_ENDPOINT_INSTANCE* result = malloc(sizeof(APPLE_ENDPOINT_INSTANCE));
    if (result != NULL)
    {
        result->sockRead = NULL;
        result->sockWrite = NULL;
    }
    else
    {
        /* Codes_SRS_XIO_ENDPOINT_30_001: [ If any resource allocation fails, xio_endpoint_create shall log an error and return NULL. ]*/
        LogError("Failed to create endpoint");
    }
    return (XIO_ENDPOINT_INSTANCE_HANDLE)result;
}

/* Codes_SRS_XIO_ENDPOINT_30_010: [ The xio_endpoint parameter is guaranteed to be non-NULL by the calling xio_impl, so concrete implementations shall not add redundant checking. ]*/
void apple_tls_destroy(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance)
{
    APPLE_ENDPOINT_INSTANCE* context = (APPLE_ENDPOINT_INSTANCE*)xio_endpoint_instance;
    /* Codes_SRS_XIO_ENDPOINT_30_011: [ The xio_endpoint_destroy shell release all of the xio_endpoint resources. ]*/
    free(context);
}

/* Codes_SRS_XIO_ENDPOINT_30_020: [ The xio_endpoint parameter is guaranteed to be non-NULL by the calling xio_impl, so concrete implementations shall not add redundant checking. ]*/
static XIO_ASYNC_RESULT apple_tls_open(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config)
{
    XIO_ASYNC_RESULT result;
    APPLE_ENDPOINT_INSTANCE* context = (APPLE_ENDPOINT_INSTANCE*)xio_endpoint_instance;
    TLS_CONFIG_DATA* config = (TLS_CONFIG_DATA*)xio_endpoint_config;
    
    CFStringRef hostname;
    // This will pretty much only fail if we run out of memory
    if (NULL == (hostname = CFStringCreateWithCString(NULL, config->hostname, kCFStringEncodingUTF8)))
    {
        /* Codes_SRS_XIO_ENDPOINT_30_023: [ On failure, xio_endpoint_open shall log an error and return XIO_ASYNC_RESULT_FAILURE. ]*/
        LogError("CFStringCreateWithCString failed");
        result = XIO_ASYNC_RESULT_FAILURE;
    }
    else
    {
        CFStreamCreatePairWithSocketToHost(NULL, hostname, config->port, &context->sockRead, &context->sockWrite);
        if (context->sockRead != NULL && context->sockWrite != NULL)
        {
            if (CFReadStreamSetProperty(context->sockRead, kCFStreamPropertySSLSettings, kCFStreamSocketSecurityLevelNegotiatedSSL))
            {
                if (CFReadStreamOpen(context->sockRead) && CFWriteStreamOpen(context->sockWrite))
                {
                    /* Codes_SRS_XIO_ENDPOINT_30_022: [ On success, xio_endpoint_open shall return XIO_ASYNC_RESULT_SUCCESS. ]*/
                    result = XIO_ASYNC_RESULT_SUCCESS;
                }
                else
                {
                    CFErrorRef readError = CFReadStreamCopyError(context->sockRead);
                    CFErrorRef writeError = CFWriteStreamCopyError(context->sockWrite);

                    /* Codes_SRS_XIO_ENDPOINT_30_023: [ On failure, xio_endpoint_open shall log an error and return XIO_ASYNC_RESULT_FAILURE. ]*/
                    LogInfo("Error opening streams - read error=%d;write error=%d", CFErrorGetCode(readError), CFErrorGetCode(writeError));
                    result = XIO_ASYNC_RESULT_FAILURE;
                }
            }
            else
            {
                /* Codes_SRS_XIO_ENDPOINT_30_023: [ On failure, xio_endpoint_open shall log an error and return XIO_ASYNC_RESULT_FAILURE. ]*/
                LogError("Failed to set socket properties");
                result = XIO_ASYNC_RESULT_FAILURE;
            }
        }
        else
        {
            /* Codes_SRS_XIO_ENDPOINT_30_023: [ On failure, xio_endpoint_open shall log an error and return XIO_ASYNC_RESULT_FAILURE. ]*/
            LogError("Unable to create socket pair");
            result = XIO_ASYNC_RESULT_FAILURE;
        }
    }
    return result;
}


/* Codes_SRS_XIO_ENDPOINT_30_040: [ All parameters are guaranteed to be non-NULL by the calling xio_impl, so concrete implementations shall not add redundant checking. ]*/
/* Codes_SRS_XIO_ENDPOINT_30_041: [ The xio_endpoint_read shall attempt to read buffer_size characters into buffer. ]*/
static int apple_tls_read(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, uint8_t* buffer, uint32_t buffer_size)
{
    int rcv_bytes;
    APPLE_ENDPOINT_INSTANCE* context = (APPLE_ENDPOINT_INSTANCE*)xio_endpoint_instance;
    CFStreamStatus read_status = CFReadStreamGetStatus(context->sockRead);
    if (read_status == kCFStreamStatusAtEnd)
    {
        LogInfo("Communications closed by host");
        rcv_bytes = XIO_ASYNC_RESULT_FAILURE;
    }
    else if (CFReadStreamHasBytesAvailable(context->sockRead))
    {
        // The buffer_size is guaranteed by the calling framweork to be less than INT_MAX
        // in order to ensure that this cast is safe
        /* Codes_SRS_XIO_ENDPOINT_30_042: [ On success, xio_endpoint_read shall return the number of characters copied into buffer. ]*/
        rcv_bytes = (int)CFReadStreamRead(context->sockRead, buffer, (CFIndex)(sizeof(buffer)));
    }
    else
    {
        /* Codes_SRS_XIO_ENDPOINT_30_044: [ If no data is available xio_endpoint_read shall return 0. ]*/
        rcv_bytes = 0;
    }
    return rcv_bytes;
}

/* Codes_SRS_XIO_ENDPOINT_30_050: [ All parameters are guaranteed to be non-NULL by the calling xio_impl, so concrete implementations shall not add redundant checking. ]*/
/* Codes_SRS_XIO_ENDPOINT_30_051: [ The xio_endpoint_write shall attempt to write buffer_size characters from buffer to its underlying data sink. ]*/
static int apple_tls_write(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, const uint8_t* buffer, uint32_t count)
{
    int result;
    APPLE_ENDPOINT_INSTANCE* context = (APPLE_ENDPOINT_INSTANCE*)xio_endpoint_instance;
    // Check to see if the socket will not block
    if (CFWriteStreamCanAcceptBytes(context->sockWrite))
    {
        // The count is guaranteed by the calling framweork to be less than INT_MAX
        // in order to ensure that this cast is safe
        /* Codes_SRS_XIO_ENDPOINT_30_052: [ On success, xio_endpoint_write shall return the number of characters from buffer that are sent. ]*/
        result = (int)CFWriteStreamWrite(context->sockWrite, buffer, count);
        if (result < 0)
        {
            // The write did not succeed. It may be busy, or it may be broken
            CFErrorRef write_error = CFWriteStreamCopyError(context->sockWrite);
            if (CFErrorGetCode(write_error) != errSSLWouldBlock)
            {
                /* Codes_SRS_XIO_ENDPOINT_30_053: [ On failure, xio_endpoint_write shall log an error and return XIO_ASYNC_RW_RESULT_FAILURE. ]*/
                LogInfo("Hard error from CFWriteStreamWrite: %d", CFErrorGetCode(write_error));
                result = XIO_ASYNC_RESULT_FAILURE;
            }
            else
            {
                // The errSSLWouldBlock error is defined as a recoverable error and should just be retried
                /* Codes_SRS_XIO_ENDPOINT_30_054: [ If the underlying data sink is temporarily unable to accept data, xio_endpoint_write shall return 0. ]*/
                LogInfo("errSSLWouldBlock on write");
                result = 0;
            }
        }
    }
    else
    {
        /* Codes_SRS_XIO_ENDPOINT_30_054: [ If the underlying data sink is temporarily unable to accept data, xio_endpoint_write shall return 0. ]*/
        result = 0;
    }

    return result;
}

/* Codes_SRS_XIO_ENDPOINT_30_030: [ All parameters are guaranteed to be non-NULL by the calling xio_impl, so concrete implementations shall not add redundant checking. ]*/
/* Codes_SRS_XIO_ENDPOINT_30_031: [ The xio_endpoint_close shall do what is necessary to close down its operation. ]*/
int apple_tls_close(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance)
{
    APPLE_ENDPOINT_INSTANCE* context = (APPLE_ENDPOINT_INSTANCE*)xio_endpoint_instance;
    if (context->sockRead != NULL)
    {
        CFReadStreamClose(context->sockRead);
        CFRelease(context->sockRead);
        context->sockRead = NULL;
    }

    if (context->sockWrite != NULL)
    {
        CFWriteStreamClose(context->sockWrite);
        CFRelease(context->sockWrite);
        context->sockWrite = NULL;
    }
    /* Codes_SRS_XIO_ENDPOINT_30_032: [ On completion, xio_endpoint_close shall return XIO_ASYNC_RESULT_SUCCESS. ] */
    return XIO_ASYNC_RESULT_SUCCESS;
}

static const XIO_ENDPOINT_INTERFACE apple_tls =
{
    apple_tls_create,
    apple_tls_destroy,
    apple_tls_open,
    apple_tls_close,
    apple_tls_read,
    apple_tls_write
};

const XIO_ENDPOINT_INTERFACE* xio_endpoint_tls_apple_get_interface()
{
    return &apple_tls;
}

const IO_INTERFACE_DESCRIPTION* socketio_get_interface_description(void)
{
    return NULL;
}

