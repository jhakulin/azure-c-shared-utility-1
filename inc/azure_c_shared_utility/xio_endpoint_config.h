// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef XIO_ENDPOINT_CONFIG_H
#define XIO_ENDPOINT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/umock_c_prod.h"

typedef void* XIO_ENDPOINT_CONFIG_HANDLE;

typedef int(*XIO_ENDPOINT_CONFIG_SETOPTION)(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config, const char* optionName, const void* value);
typedef OPTIONHANDLER_HANDLE(*XIO_ENDPOINT_CONFIG_RETRIEVEOPTIONS)(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config);
typedef void(*XIO_ENDPOINT_CONFIG_DESTROY)(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config);

// Declarations for mocking only -- actual implementations are exposed only
// within an XIO_ENDPOINT_CONFIG_INTERFACE
MOCKABLE_FUNCTION(, int, xio_endpoint_config_setoption, XIO_ENDPOINT_CONFIG_HANDLE, xio_endpoint_config, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, OPTIONHANDLER_HANDLE, xio_endpoint_config_retrieveoptions, XIO_ENDPOINT_CONFIG_HANDLE, xio_endpoint_config);
MOCKABLE_FUNCTION(, void, xio_endpoint_config_destroy, XIO_ENDPOINT_CONFIG_HANDLE, xio_endpoint_config);

typedef struct XIO_ENDPOINT_CONFIG_INTERFACE_TAG
{
    // There is no create function because creation is only done by concrete implementations
    XIO_ENDPOINT_CONFIG_SETOPTION setoption;
    XIO_ENDPOINT_CONFIG_RETRIEVEOPTIONS retrieveoptions;
    XIO_ENDPOINT_CONFIG_DESTROY destroy;
} XIO_ENDPOINT_CONFIG_INTERFACE;

// Declarations for mocking only -- actual implementations are exposed only
// within an XIO_ENDPOINT_CONFIG_INTERFACE
MOCKABLE_FUNCTION(, XIO_ENDPOINT_CONFIG_INTERFACE*, xio_endpoint_config_get_interface);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* XIO_ENDPOINT_CONFIG_H */
