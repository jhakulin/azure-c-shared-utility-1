// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef XIO_ENDPOINT_H
#define XIO_ENDPOINT_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/xio_endpoint_config.h"

typedef struct XIO_ENDPOINT_INTERFACE_TAG* XIO_ENDPOINT_INTERFACE_HANDLE;
typedef struct XIO_ENDPOINT_INTSTANCE_TAG* XIO_ENDPOINT_INSTANCE_HANDLE;


typedef enum XIO_ASYNC_RESULT_TAG
{
    XIO_ASYNC_RESULT_FAILURE = -1,
    XIO_ASYNC_RESULT_WAITING = 0,
    XIO_ASYNC_RESULT_SUCCESS = 1
} XIO_ASYNC_RESULT;

#define XIO_ASYNC_RW_RESULT_FAILURE -1

typedef XIO_ENDPOINT_INSTANCE_HANDLE(*XIO_ENDPOINT_CREATE)(void);
typedef void(*XIO_ENDPOINT_DESTROY)(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance);
typedef XIO_ASYNC_RESULT(*XIO_ENDPOINT_OPEN)(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config);
typedef XIO_ASYNC_RESULT(*XIO_ENDPOINT_CLOSE)(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance);
typedef int(*XIO_ENDPOINT_READ)(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, uint8_t* buffer, uint32_t buffer_size);
typedef int(*XIO_ENDPOINT_WRITE)(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, const uint8_t* buffer, uint32_t count);


// Declarations for mocking only -- actual implementations are exposed only
// within an XIO_ENDPOINT_CONFIG_INTERFACE
MOCKABLE_FUNCTION(, XIO_ENDPOINT_INSTANCE_HANDLE, xio_endpoint_create);
MOCKABLE_FUNCTION(, void, xio_endpoint_destroy, XIO_ENDPOINT_INSTANCE_HANDLE, xio_endpoint);
MOCKABLE_FUNCTION(, XIO_ASYNC_RESULT, xio_endpoint_open, XIO_ENDPOINT_INSTANCE_HANDLE, xio_endpoint_instance, XIO_ENDPOINT_CONFIG_HANDLE, xio_endpoint_config);
MOCKABLE_FUNCTION(, XIO_ASYNC_RESULT, xio_endpoint_close, XIO_ENDPOINT_INSTANCE_HANDLE, xio_endpoint_instance);
// read and write return 0 for waiting, non-zero for data transferred, or XIO_ASYNC_RW_RESULT_FAILURE
MOCKABLE_FUNCTION(, int, xio_endpoint_read, XIO_ENDPOINT_INSTANCE_HANDLE, xio_endpoint_instance, uint8_t*, buffer, uint32_t, buffer_size);
MOCKABLE_FUNCTION(, int, xio_endpoint_write, XIO_ENDPOINT_INSTANCE_HANDLE, xio_endpoint_instance, const uint8_t*, buffer, uint32_t, buffer_size);

// XIO_ENDPOINT_INTERFACE is always acquired through a concrete type, so there's no need
// for a generic get function.
typedef struct XIO_ENDPOINT_INTERFACE_TAG
{
    XIO_ENDPOINT_CREATE create;
    XIO_ENDPOINT_DESTROY destroy;
    XIO_ENDPOINT_OPEN open;
    XIO_ENDPOINT_CLOSE close;
    XIO_ENDPOINT_READ read;
    XIO_ENDPOINT_WRITE write;
} XIO_ENDPOINT_INTERFACE;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* XIO_ENDPOINT_H */
