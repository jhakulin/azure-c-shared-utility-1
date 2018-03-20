// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xio_endpoint_config.h"
#include "xio_endpoint_config_tls.h"
#include "azure_c_shared_utility/xio_impl.h"
#include "xio_endpoint_config_tls.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#define MAX_VALID_PORT 0xffff

/* Codes_SRS_XIO_ENDPOINT_CONFIG_30_000: [ The  xio_endpoint_config_destroy  shall release all allocated resources and then release xio_endpoint_config. ]*/
/* Codes_SRS_XIO_ENDPOINT_CONFIG_30_001: [ The  xio_endpoint_config  parameter is guaranteed to be non-NULL by the calling  xio_impl , so concrete implementations shall not add redundant checking. ] */
static void xio_endpoint_config_tls_destroy(XIO_ENDPOINT_CONFIG_HANDLE xep_config)
{
    TLS_CONFIG_DATA* cfg = (TLS_CONFIG_DATA*)xep_config;
    free((void*)cfg->hostname);
    free(xep_config);
}

/* Codes_SRS_XIO_ENDPOINT_CONFIG_30_010: [ The xio_endpoint_config, optionName, and value parameters are guaranteed to be non-NULL by the calling xio_impl, so concrete implementations shall not add redundant checking. ] */
static int xio_endpoint_config_tls_setoption(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config, const char* optionName, const void* value)
{
    // Non-NULL is guaranteed by the only caller, xio_impl.c
    TLS_CONFIG_DATA* config_data = (TLS_CONFIG_DATA*)xio_endpoint_config;
    /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_010: [ The xio_endpoint_config_tls component shall delegate setoption to its contained options member. ] */
    return tlsio_options_set(&config_data->options, optionName, value);
}

/* Codes_SRS_XIO_ENDPOINT_CONFIG_30_020: [ The xio_endpoint_config parameter is guaranteed to be non-NULL by the calling xio_impl, so concrete implementations shall not add redundant checking. ]*/
static OPTIONHANDLER_HANDLE xio_endpoint_config_tls_retrieveoptions(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config)
{
    TLS_CONFIG_DATA* config_data = (TLS_CONFIG_DATA*)xio_endpoint_config;
    /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_020: [ The xio_endpoint_config_tls component shall delegate retrieveoptions to its contained options member. ] */
    return tlsio_options_retrieve_options(&config_data->options, xio_impl_setoption);
}

static XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config_tls_create(TLSIO_CONFIG* tlsio_config, int option_caps)
{
    TLS_CONFIG_DATA* result;
    /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_005: [ If the  TLS_CONFIG  supplied during tlsio creation or its contained  hostname  are NULL, the  xio_create  function shall log an error and return NULL. ]*/
    if (tlsio_config == NULL || tlsio_config->hostname == NULL)
    {
        LogError("NULL tlsio_config or tlsio_config->hostname");
        result = NULL;
    }
    /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_006: [ If the supplied  port  member of  TLS_CONFIG  is less than 0 or greater than 0xffff, the  xio_create  function shall log an error and return NULL. ]*/
    else if (tlsio_config->port < 0 || tlsio_config->port > MAX_VALID_PORT)
    {
        LogError("tlsio_config->port out of range");
        result = NULL;
    }
    else if ((result = malloc(sizeof(TLS_CONFIG_DATA))) == NULL)
    {
        LogError("malloc TLS_CONFIG_DATA failed");
    }
    else
    {
        memset(result, 0, sizeof(TLS_CONFIG_DATA));
        /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_004: [ The  TLS_CONFIG  supplied during tlsio creation shall be used to initialize the  hostname  and  port  members of the  TLS_CONFIG_DATA . ]*/
        TLS_CONFIG_DATA* config_data = (TLS_CONFIG_DATA*)result;
         if (0 != mallocAndStrcpy_s((char**)&config_data->hostname, tlsio_config->hostname))
        {
            LogError("mallocAndStrcpy_s failed");
            xio_endpoint_config_tls_destroy(result);
            result = NULL;
        }
        else
        {
            config_data->port = tlsio_config->port;
            /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_003: [ The supplied  option_caps  shall be used to initialize the  options  member of the  TLS_CONFIG_DATA. ] */
            tlsio_options_initialize(&config_data->options, option_caps);
        }
    }
    return result;
}

static IO_INTERFACE_DESCRIPTION xio_interface_exposed;
static int option_caps_for_tls = TLSIO_OPTION_BIT_NONE;
static const XIO_ENDPOINT_INTERFACE* xio_endpoint_for_tls = NULL;
static XIO_ENDPOINT_CONFIG_INTERFACE xio_endpoint_config_interface_for_tls =
{
    xio_endpoint_config_tls_setoption,
    xio_endpoint_config_tls_retrieveoptions,
    xio_endpoint_config_tls_destroy
};

static CONCRETE_IO_HANDLE xio_local_create(void* io_create_parameters)
{
    CONCRETE_IO_HANDLE result;
    TLSIO_CONFIG* tlsio_config_in = (TLSIO_CONFIG*)io_create_parameters;
    XIO_ENDPOINT_CONFIG_HANDLE config_instance = xio_endpoint_config_tls_create(tlsio_config_in, option_caps_for_tls);
    if (config_instance == NULL)
    {
        LogError("Failed to create config instance");
        result = NULL;
    }
    else
    {
        result = xio_impl_create(xio_endpoint_for_tls, &xio_endpoint_config_interface_for_tls, config_instance);
    }
    
    return result;
}

const IO_INTERFACE_DESCRIPTION* tlsio_basic_get_interface_description(const XIO_ENDPOINT_INTERFACE* xio_endpoint, int option_caps)
{
    IO_INTERFACE_DESCRIPTION* result;
    if (xio_endpoint == NULL)
    {
        /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_000: [ If the  xio_endpoint  parameter is NULL,  tlsio_basic_get_interface_description  shall log an error and return NULL. ]*/
        LogError("Null required xio_endpoint");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_002: [ The tlsio created via the returned interface shall be composed of an  xio_impl  configured with the supplied  xio_endpoint  plus a  TLS_CONFIG_DATA  struct as the  xio_endpoint_config  instance. ] */
        // Save the supplied xio_endpoint and options_caps for use by concrete create calls
        xio_endpoint_for_tls = xio_endpoint;
        option_caps_for_tls = option_caps;

        // Use all of xio_impl's interface functions except create
        xio_interface_exposed.concrete_io_destroy = xio_impl_destroy;
        xio_interface_exposed.concrete_io_open = xio_impl_open_async;
        xio_interface_exposed.concrete_io_close = xio_impl_close_async;
        xio_interface_exposed.concrete_io_send = xio_impl_send_async;
        xio_interface_exposed.concrete_io_dowork = xio_impl_dowork;
        xio_interface_exposed.concrete_io_setoption = xio_impl_setoption;
        xio_interface_exposed.concrete_io_retrieveoptions = xio_impl_retrieveoptions;

        // Supply our home-grown create
        xio_interface_exposed.concrete_io_create = xio_local_create;

        /* Codes_SRS_XIO_ENDPOINT_CONFIG_TLS_30_001: [ The  tlsio_basic_get_interface_description  shall return a standard  xio  interface for a tlsio. ] */
        result = &xio_interface_exposed;
    }

    return result;
}

