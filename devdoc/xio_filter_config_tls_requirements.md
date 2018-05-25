# xio_filter_config_tls

## Overview

This specification defines the behavior of the `xio_filter_config_tls` component, which is
a specialization of [`xio_filter_config`](xio_filter_config_requirements.md).

The `xio_filter_config_tls` component accepts the configuration information from the supplied
[`TLSIO_CONFIG`](/inc/azure_c_shared_utility/tlsio.h) during the creation of a `tlsio`, plus
any options set on the owner via `xio_state_retrieveoptions`, and exposes this information as
a type-safe struct.

The `xio_filter_config_tls` component is not exposed directly. Instead, it exposes a 
`tlsio_basic_get_interface_description` function which is a standard xio interface. This
interface will create an `xio` component composed of an `xio_state` component that manages state
and callbacks, an `xio_filter` (supplied to the `tlsio_basic_get_interface_description` function),
plus an `xio_filter_config_tls` component.

## References

[xio.h](/inc/azure_c_shared_utility/xio.h)</br>
[xio_filter.h](/inc/azure_c_shared_utility/xio_filter_config.h)</br>
[xio_filter_config.h](/inc/azure_c_shared_utility/xio_filter_config.h)</br>
[tlsio_options.h](/inc/azure_c_shared_utility/tlsio_options.h)</br>
[xio_filter_config_tls.h](/pal/inc/xio_filter_config_tls.h)</br>

## Applicable specs
[The `xio_filter_config` spec](xio_filter_config_requirements.md) is a parent spec to this
one, and its requirements apply to this component.

## Exposed API
 
The `xio_filter_config_tls` exposes its data as a `TLS_CONFIG_DATA` struct, and 
creates instances of itself via `tlsio_basic_get_interface_description`.

```c
// This is the public struct that the exposes config data for
// basic tlsio.
typedef struct TLS_CONFIG_DATA_TAG
{
    // Standard tlsio info
    const char* hostname;
    uint16_t port;
    TLSIO_OPTIONS options;
} TLS_CONFIG_DATA;

// Return an xio interface description using basic TLS config plus the supplied xio_filter
const IO_INTERFACE_DESCRIPTION* tlsio_basic_get_interface_description(
    const XIO_FILTER_INTERFACE*, xio_filter, int, option_caps);

```

## API Calls


###   tlsio_basic_get_interface_description

Returns an `xio` interface combining `xio_state`, the supplied `xio_filter` and `option_caps`.
```c
const IO_INTERFACE_DESCRIPTION* tlsio_basic_get_interface_description(
    const XIO_FILTER_INTERFACE* xio_filter, int option_caps);
```

**SRS_XIO_FILTER_CONFIG_TLS_30_000: [** If the `xio_filter` parameter is NULL,  `tlsio_basic_get_interface_description` shall log an error and return NULL. **]**

**SRS_XIO_FILTER_CONFIG_TLS_30_001: [** The `tlsio_basic_get_interface_description` shall return a standard `xio` interface for a tlsio. **]**

**SRS_XIO_FILTER_CONFIG_TLS_30_002: [** The tlsio created via the returned interface shall be composed of an `xio_state` configured with the supplied `xio_filter` plus a `TLS_CONFIG_DATA` struct as the `xio_filter_config` instance. **]**

**SRS_XIO_FILTER_CONFIG_TLS_30_003: [** The supplied `option_caps` shall be used to initialize the `options` member of the `TLS_CONFIG_DATA`. **]**

**SRS_XIO_FILTER_CONFIG_TLS_30_004: [** The `TLS_CONFIG` supplied during tlsio creation shall be used to initialize the `hostname` and `port` members of the `TLS_CONFIG_DATA`. **]**

**SRS_XIO_FILTER_CONFIG_TLS_30_005: [** If the `TLS_CONFIG` supplied during tlsio creation or its contained `hostname` are NULL, the `xio_create` function shall log an error and return NULL. **]**

**SRS_XIO_FILTER_CONFIG_TLS_30_006: [** If the supplied `port` member of `TLS_CONFIG` is less than 0 or greater than 0xffff, the `xio_create` function shall log an error and return NULL. **]**

###   xio_filter_config_setoption

**SRS_XIO_FILTER_CONFIG_TLS_30_010: [** The `xio_filter_config_tls` component shall delegate `setoption` to its contained `options` member. **]**

###   xio_filter_config_retrieveoptions

**SRS_XIO_FILTER_CONFIG_TLS_30_020: [** The `xio_filter_config_tls` component shall delegate `retrieveoptions` to its contained `options` member. **]**

