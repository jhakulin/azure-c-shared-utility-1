# xio_endpoint_config

## Overview

This specification defines the behavior of `xio_endpoint_config` components, which convert 
parameters supplied to`xio_create` and options set using `xio_setoption` into type-safe
values for direct use by `xio_endpoint` components. An `xio_endpoint_config` also
implements `xio_setoption` and `xio_retrieveoptions` for its owning `xio_impl` component.

This spec does not define the type-safe structure that a specific concrete `xio_endpoint_config`
component exposes. Instead, these type-safe structures are defined by their concrete implementations.

## References

[xio.h](/inc/azure_c_shared_utility/xio.h)</br>
[xio_endpoint.h](/inc/azure_c_shared_utility/xio_endpoint_config.h)</br>
[xio_endpoint_config.h](/inc/azure_c_shared_utility/xio_endpoint_config.h)</br>



## Exposed API

**SRS_XIO_ENDPOINT_CONFIG_30_001: [** The `xio_endpoint_config` shall implement and export all the functions defined 
in `xio_endpoint_config.h`.
```c
typedef void* XIO_ENDPOINT_CONFIG_HANDLE;

typedef int(*XIO_ENDPOINT_CONFIG_SETOPTION)(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config, const char* optionName, const void* value);
typedef OPTIONHANDLER_HANDLE(*XIO_ENDPOINT_CONFIG_RETRIEVEOPTIONS)(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config);
typedef void(*XIO_ENDPOINT_CONFIG_DESTROY)(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config);

// Declarations for mocking only -- actual implementations are exposed only
// within an XIO_ENDPOINT_CONFIG_INTERFACE
int xio_endpoint_config_setoption(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config, const char* optionName, const void* value);
OPTIONHANDLER_HANDLE xio_endpoint_config_retrieveoptions(XIO_ENDPOINT_CONFIG_HANDLE, xio_endpoint_config);
void, xio_endpoint_config_destroy(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config);

typedef struct XIO_ENDPOINT_CONFIG_INTERFACE_TAG
{
    // There is no create function because creation is only done by concrete implementations
    XIO_ENDPOINT_CONFIG_SETOPTION setoption;
    XIO_ENDPOINT_CONFIG_RETRIEVEOPTIONS retrieveoptions;
    XIO_ENDPOINT_CONFIG_DESTROY destroy;
} XIO_ENDPOINT_CONFIG_INTERFACE;

// Declaration for mocking only -- actual implementations are exposed only
// as a uniquely-named concrete function.
XIO_ENDPOINT_CONFIG_INTERFACE* xio_endpoint_config_get_interface(void);

```
**]**




## API Calls


###   Creation of xio_endpoint_config components

There is no abstract creation of `xio_endpoint_config` components, so there is no definition
of a create function. Instead, implementers are free to define their concrete create functions
as they see fit.

The current design follows a simple create/destroy pattern, but if future design changes make
sharing of `xio_endpoint_config` desirable, then it would make sense to change this 
to an addref/release pattern.

###   xio_endpoint_config_destroy

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_ENDPOINT_CONFIG_INTERFACE`.
```c
void xio_endpoint_config_destroy(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config);
```

**SRS_XIO_ENDPOINT_CONFIG_30_000: [** The `xio_endpoint_config_destroy` shall release all allocated resources and then release `xio_endpoint_config`. **]**

**SRS_XIO_ENDPOINT_CONFIG_30_001: [** The `xio_endpoint_config` parameter is guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

###   xio_endpoint_config_setoption

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_ENDPOINT_CONFIG_INTERFACE`.

The options are conceptually part of `xio_impl_create` in that options which are set 
persist until the instance is destroyed. 
```c
int xio_endpoint_config_setoption(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config, 
    const char* optionName, const void* value);
```
**SRS_XIO_ENDPOINT_CONFIG_30_010: [** The `xio_endpoint_config`, `optionName`, and `value` parameters are guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

###   xio_endpoint_config_retrieveoptions
Implementation of `concrete_io_retrieveoptions` Specific implementations must 
define the behavior of successful `xio_impl_retrieveoptions` calls.

```c
OPTIONHANDLER_HANDLE xio_impl_retrieveoptions(XIO_ENDPOINT_CONFIG_HANDLE xio_endpoint_config);
```

**SRS_XIO_ENDPOINT_CONFIG_30_020: [** The `xio_endpoint_config` parameter is guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

