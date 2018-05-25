# xio_filter_config

## Overview

This specification defines the behavior of `xio_filter_config` components, which convert 
parameters supplied to`xio_create` and options set using `xio_setoption` into type-safe
values for direct use by `xio_filter` components. An `xio_filter_config` also
implements `xio_setoption` and `xio_retrieveoptions` for its owning `xio_impl` component.

This spec does not define the type-safe structure that a specific concrete `xio_filter_config`
component exposes. Instead, these type-safe structures are defined by their concrete implementations.

## References

[xio.h](/inc/azure_c_shared_utility/xio.h)</br>
[xio_filter.h](/inc/azure_c_shared_utility/xio_filter_config.h)</br>
[xio_filter_config.h](/inc/azure_c_shared_utility/xio_filter_config.h)</br>



## Exposed API

**SRS_XIO_FILTER_CONFIG_30_001: [** The `xio_filter_config` shall implement and export all the functions defined 
in `xio_filter_config.h`.
```c
typedef void* XIO_FILTER_CONFIG_HANDLE;

typedef int(*XIO_FILTER_CONFIG_SETOPTION)(XIO_FILTER_CONFIG_HANDLE xio_filter_config, const char* optionName, const void* value);
typedef OPTIONHANDLER_HANDLE(*XIO_FILTER_CONFIG_RETRIEVEOPTIONS)(XIO_FILTER_CONFIG_HANDLE xio_filter_config);
typedef void(*XIO_FILTER_CONFIG_DESTROY)(XIO_FILTER_CONFIG_HANDLE xio_filter_config);

// Declarations for mocking only -- actual implementations are exposed only
// within an XIO_FILTER_CONFIG_INTERFACE
int xio_filter_config_setoption(XIO_FILTER_CONFIG_HANDLE xio_filter_config, const char* optionName, const void* value);
OPTIONHANDLER_HANDLE xio_filter_config_retrieveoptions(XIO_FILTER_CONFIG_HANDLE, xio_filter_config);
void, xio_filter_config_destroy(XIO_FILTER_CONFIG_HANDLE xio_filter_config);

typedef struct XIO_FILTER_CONFIG_INTERFACE_TAG
{
    // There is no create function because creation is only done by concrete implementations
    XIO_FILTER_CONFIG_SETOPTION setoption;
    XIO_FILTER_CONFIG_RETRIEVEOPTIONS retrieveoptions;
    XIO_FILTER_CONFIG_DESTROY destroy;
} XIO_FILTER_CONFIG_INTERFACE;

// Declaration for mocking only -- actual implementations are exposed only
// as a uniquely-named concrete function.
XIO_FILTER_CONFIG_INTERFACE* xio_filter_config_get_interface(void);

```
**]**




## API Calls


###   Creation of xio_filter_config components

There is no abstract creation of `xio_filter_config` components, so there is no definition
of a create function. Instead, implementers are free to define their concrete create functions
as they see fit.

The current design follows a simple create/destroy pattern, but if future design changes make
sharing of `xio_filter_config` desirable, then it would make sense to change this 
to an addref/release pattern.

###   xio_filter_config_destroy

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_FILTER_CONFIG_INTERFACE`.
```c
void xio_filter_config_destroy(XIO_FILTER_CONFIG_HANDLE xio_filter_config);
```

**SRS_XIO_FILTER_CONFIG_30_000: [** The `xio_filter_config_destroy` shall release all allocated resources and then release `xio_filter_config`. **]**

**SRS_XIO_FILTER_CONFIG_30_001: [** The `xio_filter_config` parameter is guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

###   xio_filter_config_setoption

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_FILTER_CONFIG_INTERFACE`.

The options are conceptually part of `xio_impl_create` in that options which are set 
persist until the instance is destroyed. 
```c
int xio_filter_config_setoption(XIO_FILTER_CONFIG_HANDLE xio_filter_config, 
    const char* optionName, const void* value);
```
**SRS_XIO_FILTER_CONFIG_30_010: [** The `xio_filter_config`, `optionName`, and `value` parameters are guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

###   xio_filter_config_retrieveoptions
Implementation of `concrete_io_retrieveoptions` Specific implementations must 
define the behavior of successful `xio_impl_retrieveoptions` calls.

```c
OPTIONHANDLER_HANDLE xio_impl_retrieveoptions(XIO_FILTER_CONFIG_HANDLE xio_filter_config);
```

**SRS_XIO_FILTER_CONFIG_30_020: [** The `xio_filter_config` parameter is guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

