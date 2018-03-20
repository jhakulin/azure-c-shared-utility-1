// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef XIO_ENDPOINT_CONFIG_TLS_H
#define XIO_ENDPOINT_CONFIG_TLS_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/tlsio_options.h"
#include "azure_c_shared_utility/xio_endpoint.h"
#include "azure_c_shared_utility/xio_endpoint_config.h"
#include "azure_c_shared_utility/umock_c_prod.h"

// This is the public struct that the exposes config data for 
// basic tlsio.
typedef struct TLS_CONFIG_DATA_TAG
{
    // Standard tlsio info
    const char* hostname;
    uint16_t port;
    TLSIO_OPTIONS options;
} TLS_CONFIG_DATA;

// Return an xio interface description using basic TLS config plus the supplied xio_endpoint
MOCKABLE_FUNCTION(, const IO_INTERFACE_DESCRIPTION*, tlsio_basic_get_interface_description, const XIO_ENDPOINT_INTERFACE*, xio_endpoint, int, option_caps);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* XIO_ENDPOINT_CONFIG_TLS_H */
