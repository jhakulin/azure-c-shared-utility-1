// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef XIO_IMPL_H
#define XIO_IMPL_H

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/xio_endpoint_config.h"
#include "azure_c_shared_utility/xio_endpoint.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

CONCRETE_IO_HANDLE xio_impl_create(const XIO_ENDPOINT_INTERFACE* endpoint_interface, XIO_ENDPOINT_CONFIG_INTERFACE* xio_endpoint_config_interface, XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config_instance);
    
// These functions replace the corresponding functions in your specific xio_get_interface_description
MOCKABLE_FUNCTION(, void, xio_impl_destroy, CONCRETE_IO_HANDLE, xio);
MOCKABLE_FUNCTION(, int, xio_impl_open_async, CONCRETE_IO_HANDLE, xio, ON_IO_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context, ON_BYTES_RECEIVED, on_bytes_received, void*, on_bytes_received_context, ON_IO_ERROR, on_io_error, void*, on_io_error_context);
MOCKABLE_FUNCTION(, int, xio_impl_close_async, CONCRETE_IO_HANDLE, xio, ON_IO_CLOSE_COMPLETE, on_close_complete, void*, callback_context);
MOCKABLE_FUNCTION(, int, xio_impl_send_async, CONCRETE_IO_HANDLE, xio, const void*, buffer, size_t, size, ON_SEND_COMPLETE, on_send_complete, void*, callback_context);
MOCKABLE_FUNCTION(, void, xio_impl_dowork, CONCRETE_IO_HANDLE, xio);
MOCKABLE_FUNCTION(, int, xio_impl_setoption, CONCRETE_IO_HANDLE, xio, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, OPTIONHANDLER_HANDLE, xio_impl_retrieveoptions, CONCRETE_IO_HANDLE, xio);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* XIO_IMPL_H */
