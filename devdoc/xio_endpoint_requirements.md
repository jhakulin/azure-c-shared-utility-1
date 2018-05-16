# xio_endpoint

## Overview

This specification defines the `xio_endpoint` component, which provides stateless message handling
for `xio` components. It works in conjunction with an `xio_impl` component that implements
state handling and callbacks, plus an `xio_endpoint_config` component that implements configuration
and option handling.

**Important:** The `xio_endpoint` component is only used by `xio_impl`, which performs all state 
management. Therefore an `xio_endpoint` component should never perform any of its own 
`xio` state management. For example, there is no need to ensure that `close` has been
called before performing `destroy'.

There is no abstract `xio_endpoint` component. 

## References

[xio.h](/inc/azure_c_shared_utility/xio.h)</br>
[xio_endpoint.h](/inc/azure_c_shared_utility/xio_endpoint.h)</br>
[xio_endpoint_config.h](/inc/azure_c_shared_utility/xio_endpoint_config.h)</br>

## Exposed API

 Each `xio_endpoint` implements and export all the functions defined in `xio_endpoint.h`. 
```c
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
XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_create(void);
void xio_endpoint_destroy(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint);
XIO_ASYNC_RESULT xio_endpoint_open(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, XIO_ENDPOINT_CONFIG_HANDLE, xio_endpoint_config);
XIO_ASYNC_RESULT xio_endpoint_close(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance);
// read and write return 0 for waiting, non-zero for data transferred, 
// or XIO_ASYNC_RW_RESULT_FAILURE
int xio_endpoint_read(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, 
    uint8_t*, buffer uint32_t, buffer_size);
int xio_endpoint_write(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, 
    const uint8_t* buffer, uint32_t buffer_size);

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

```
**]**

## API Calls

###   xio_endpoint_create

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_ENDPOINT_INTERFACE`.

```c
XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_create(void);
```
**SRS_XIO_ENDPOINT_30_000: [** The `xio_endpoint_create` shall allocate and initialize all necessary resources and return an instance of the `xio_endpoint`. **]**

**SRS_XIO_ENDPOINT_30_001: [** If any resource allocation fails, `xio_endpoint_create` shall log an error and return NULL. **]**


###   xio_endpoint_destroy

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_ENDPOINT_INTERFACE`.

```c
void xio_endpoint_destroy(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint);
```

**SRS_XIO_ENDPOINT_30_010: [** The `xio_endpoint` parameter is guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

**SRS_XIO_ENDPOINT_30_011: [** The `xio_endpoint_destroy` shell release all of the `xio_endpoint` resources. **]**

###   xio_endpoint_open

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_ENDPOINT_INTERFACE`.
Implementation of `concrete_io_open`

```c
XIO_ASYNC_RESULT xio_endpoint_open(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, XIO_ENDPOINT_CONFIG_HANDLE, xio_endpoint_config);
```

**SRS_XIO_ENDPOINT_30_020: [** All parameters are guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

**SRS_XIO_ENDPOINT_30_021: [** The `xio_endpoint_open` shall do what is necessary to prepare to accept read and write calls. **]**

**SRS_XIO_ENDPOINT_30_022: [** On success, `xio_endpoint_open` shall return `XIO_ASYNC_RESULT_SUCCESS`. **]**

**SRS_XIO_ENDPOINT_30_023: [** On failure, `xio_endpoint_open` shall log an error and return `XIO_ASYNC_RESULT_FAILURE`. **]**

**SRS_XIO_ENDPOINT_30_024: [** If `xio_endpoint_open` needs to be called again to complete the opening process, it shall return `XIO_ASYNC_RESULT_WAITING`. **]

###   xio_endpoint_close

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_ENDPOINT_INTERFACE`.

```c
XIO_ASYNC_RESULT xio_endpoint_close(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance);
```

**SRS_XIO_ENDPOINT_30_030: [** All parameters are guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

**SRS_XIO_ENDPOINT_30_031: [** The `xio_endpoint_close` shall do what is necessary to close down its operation. **]**

**SRS_XIO_ENDPOINT_30_032: [** On completion, `xio_endpoint_close` shall return `XIO_ASYNC_RESULT_SUCCESS`. **]**</br>
Note: The `xio_endpoint_close` call never returns failure.

**SRS_XIO_ENDPOINT_30_033: [** If `xio_endpoint_close` needs to be called again to complete the shutdown process, it shall return `XIO_ASYNC_RESULT_WAITING`. **]**


###   xio_endpoint_read

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_ENDPOINT_INTERFACE`.

```c
int xio_endpoint_read(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, 
    uint8_t*, buffer uint32_t, buffer_size);
```
**SRS_XIO_ENDPOINT_30_040: [** All parameters are guaranteed to be non-NULL by the calling `xio_impl`, so concrete implementations shall not add redundant checking. **]**

**SRS_XIO_ENDPOINT_30_041: [** The `xio_endpoint_read` shall attempt to read `buffer_size` characters into `buffer`. **]**

**SRS_XIO_ENDPOINT_30_042: [** On success, `xio_endpoint_read` shall return the number of characters copied into `buffer`. **]**

**SRS_XIO_ENDPOINT_30_043: [** On failure, `xio_endpoint_read` shall log an error and return `XIO_ASYNC_RW_RESULT_FAILURE`. **]**

**SRS_XIO_ENDPOINT_30_044: [** If no data is available `xio_endpoint_read` shall return 0. **]**

If the remote host closes the connection, sockets usually report this as "end of file", which is not usually
considered an error. However, it is an error in the context of the Azure IoT C SDK.
**SRS_XIO_ENDPOINT_30_045: [** The `xio_endpoint_read` shall test for an "end of file" condition and treat it as an error. **]**

###   xio_endpoint_write

Do not use this function name in your concrete implementation. Instead, use an appropriate static
function and expose it within a `XIO_ENDPOINT_INTERFACE`.

```c
int xio_endpoint_write(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, 
    const uint8_t* buffer, uint32_t buffer_size);
```

**SRS_XIO_ENDPOINT_30_050: [** The pointer parameters are guaranteed to be non-NULL by the calling `xio_impl`, and `buffer_size` is guaranteed positive, so concrete implementations shall not add redundant checking. **]**

**SRS_XIO_ENDPOINT_30_051: [** The `xio_endpoint_write` shall attempt to write `buffer_size` characters from `buffer` to its underlying data sink. **]**

**SRS_XIO_ENDPOINT_30_052: [** On success, `xio_endpoint_write` shall return the number of characters from `buffer` that are sent. **]**

**SRS_XIO_ENDPOINT_30_053: [** On failure, `xio_endpoint_write` shall log an error and return `XIO_ASYNC_RW_RESULT_FAILURE`. **]**

**SRS_XIO_ENDPOINT_30_054: [** If the underlying data sink is temporarily unable to accept data, `xio_endpoint_write` shall return 0. **]**
