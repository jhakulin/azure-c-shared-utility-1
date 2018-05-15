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






















#### XIO_IMPL_STATE_EXT_OPENING behaviors

Transitioning from XIO_IMPL_STATE_EXT_OPENING to XIO_IMPL_STATE_EXT_OPEN may require multiple calls to `xio_endpoint_dowork`. The number of calls required is not specified.

**SRS_XIO_ENDPOINT_30_080: [** The `xio_endpoint_dowork` shall delegate open behavior to the `xio_endpoint` provided during `xio_endpoint_open_async`. **]**

**SRS_XIO_ENDPOINT_30_081: [** If the `xio_endpoint` returns `XIO_ASYNC_RESULT_WAITING`, `xio_endpoint_dowork` shall remain in the XIO_IMPL_STATE_EXT_OPENING state.  **]**

**SRS_XIO_ENDPOINT_30_082: [** If the `xio_endpoint` returns `XIO_ASYNC_RESULT_FAILURE`, `xio_endpoint_dowork`  shall log an error, call `on_open_complete` with the `on_open_complete_context` parameter provided in `xio_endpoint_open_async` and `IO_OPEN_ERROR`, and [enter XIO_IMPL_STATE_EXT_CLOSED](#enter-XIO_IMPL_STATE_EXT_CLOSED "Forcibly close any existing connections then call the `on_close_complete` function and pass the `on_close_complete_context` that was supplied in `xio_endpoint_close_async`."). **]**

**SRS_XIO_ENDPOINT_30_083: [** If `xio_endpoint` returns `XIO_ASYNC_RESULT_SUCCESS`, `xio_endpoint_dowork` shall [enter XIO_IMPL_STATE_EXT_OPEN](#enter-XIO_IMPL_STATE_EXT_OPEN "Call the `on_open_complete` function and pass IO_OPEN_OK and the `on_open_complete_context` that was supplied in `xio_endpoint_open_async`."). **]**

#### Data transmission behaviors

**SRS_XIO_ENDPOINT_30_090: [** The `xio_endpoint_dowork` shall delegate send behavior to the `xio_endpoint` provided during `xio_endpoint_open_async`. **]**

**SRS_XIO_ENDPOINT_30_091: [** If `xio_endpoint_dowork` is able to send all the bytes in an enqueued message, it shall first dequeue the message then call the messages's `on_send_complete` along with its associated `callback_context` and `IO_SEND_OK`. **]**<br/>
The message needs to be dequeued before calling the callback because the callback may trigger a re-entrant
`xio_endpoint_close_async` call which will need a consistent message queue.

**SRS_XIO_ENDPOINT_30_093: [** If the `xio_endpoint` was not able to send an entire enqueued message at once, subsequent calls to `xio_endpoint_dowork` shall continue to send the remaining bytes. **]**

**SRS_XIO_ENDPOINT_30_095: [** If the send process fails before sending all of the bytes in an enqueued message, `xio_endpoint_dowork` shall [destroy the failed message](#destroy-the-failed-message "Remove the message from the queue and destroy it after calling the message's `on_send_complete` along with its associated `callback_context` and `IO_SEND_ERROR`.") and [enter XIO_IMPL_STATE_EXT_ERROR](#enter-XIO_IMPL_STATE_EXT_ERROR "Call the `on_io_error` function and pass the `on_io_error_context` that was supplied in `xio_endpoint_open_async`."). **]**

**SRS_XIO_ENDPOINT_30_096: [** If there are no enqueued messages available, `xio_endpoint_dowork` shall do nothing. **]**

#### Data reception behaviors

**SRS_XIO_ENDPOINT_30_101: [** The `xio_endpoint_dowork` shall delegate recieve behavior to the `xio_endpoint` provided during `xio_endpoint_open_async`. **]**

**SRS_XIO_ENDPOINT_30_100: [** If the `xio_endpoint` returns a positive number of bytes received, `xio_endpoint_dowork` shall repeatedly read this data and call `on_bytes_received` with the pointer to the buffer containing the data, the number of bytes received, and the `on_bytes_received_context`. **]**

**SRS_XIO_ENDPOINT_30_101: [** If the `xio_endpoint` returns `XIO_ASYNC_RW_RESULT_FAILURE` then `xio_endpoint_dowork` shall [enter XIO_IMPL_STATE_EXT_ERROR](#enter-XIO_IMPL_STATE_EXT_ERROR "Call the `on_io_error` function and pass the `on_io_error_context` that was supplied in `xio_endpoint_open_async`."). **]**

**SRS_XIO_ENDPOINT_30_102: [** If the `xio_endpoint` returns 0 bytes received then `xio_endpoint_dowork` shall not call the `on_bytes_received` callback. **]**

#### XIO_IMPL_STATE_EXT_CLOSING behaviors

**SRS_XIO_ENDPOINT_30_105: [** The `xio_endpoint_dowork` shall delegate close behavior to the `xio_endpoint` provided during `xio_endpoint_open_async`. **]**

**SRS_XIO_ENDPOINT_30_106: [** If the `xio_endpoint` returns `XIO_ASYNC_RESULT_FAILURE`, `xio_endpoint_dowork` shall log an error and [enter XIO_IMPL_STATE_EXT_CLOSED](#enter-XIO_IMPL_STATE_EXT_CLOSED "Forcibly close any existing connections then call the `on_close_complete` function and pass the `on_close_complete_context` that was supplied in `xio_endpoint_close_async`."). **]**

**SRS_XIO_ENDPOINT_30_107: [** If the `xio_endpoint` returns `XIO_ASYNC_RESULT_SUCCESS`, `xio_endpoint_dowork` shall [enter XIO_IMPL_STATE_EXT_CLOSED](#enter-XIO_IMPL_STATE_EXT_CLOSED "Forcibly close any existing connections then call the `on_close_complete` function and pass the `on_close_complete_context` that was supplied in `xio_endpoint_close_async`."). **]**

**SRS_XIO_ENDPOINT_30_108: [** If the `xio_endpoint` returns `XIO_ASYNC_RESULT_WAITING`, `xio_endpoint_dowork` shall remain in the XIO_IMPL_STATE_EXT_CLOSING state.  **]**

###   xio_endpoint_setoption
Implementation of `concrete_io_setoption`. Specific implementations must define the behavior of successful `xio_endpoint_setoption` calls.

The options are conceptually part of `xio_endpoint_create` in that options which are set 
persist until the instance is destroyed. 
```c
int xio_endpoint_setoption(CONCRETE_IO_HANDLE xio_endpoint_handle, const char* optionName, const void* value);
```
**SRS_XIO_ENDPOINT_30_120: [** If any of the the `xio_endpoint_handle`, `optionName`, or `value` parameters is NULL, `xio_endpoint_setoption` shall do nothing except log an error and return `_FAILURE_`. **]**

**SRS_XIO_ENDPOINT_30_121 [** `xio_impl` shall delegate the behavior of `xio_endpoint_setoption` to the `xio_endpoint_config` supplied in `xio_endpoint_create`. **]**


###   xio_endpoint_retrieveoptions
Implementation of `concrete_io_retrieveoptions` Specific implementations must 
define the behavior of successful `xio_endpoint_retrieveoptions` calls.

```c
OPTIONHANDLER_HANDLE xio_endpoint_retrieveoptions(CONCRETE_IO_HANDLE xio_endpoint_handle);
```

**SRS_XIO_ENDPOINT_30_160: [** If the `xio_endpoint_handle` parameter is NULL, `xio_endpoint_retrieveoptions` shall do nothing except log an error and return `_FAILURE_`. **]**

**SRS_XIO_ENDPOINT_30_161: [** `xio_impl` shall delegate the behavior of `xio_endpoint_retrieveoptions` to the `xio_endpoint_config` supplied in `xio_endpoint_create`. **]**

### Error Recovery Testing
Error recovery for `xio_impl` is performed by the higher level modules that own the component. 
There are a very large number of error recovery sequences which might be performed, and performing 
all of the possible retry sequences
is out-of-scope for this document. However, the two tests here represent a minimal test suite to mimic the retry
sequences that the higher level modules might perform.

The test conditions in this section are deliberately underspecified and left to the judgement of 
the implementer, and code commenting
in the unit tests themselves will be considered sufficient documentation for any further detail. 
Any of a number of possible
specific call sequences is acceptable as long as the unit test meets the criteria of the test requirement. 

The words "high-level retry sequence" as used in this section means that:
  1. A failure has been injected at some specified point
  2. `xio_endpoint_close_async` has been called and the `on_close_complete` callback has been received.
  3. `xio_endpoint_open_async` has been called successfully.
  4. `xio_endpoint_dowork` has been called as necessary to permit this sequence of events.
  5. The `on_open_complete` callback has been received with `IO_OPEN_OK`.

Note that the requirements in this section have corresponding entries in the unit test files, but do not
appear in the implementation code.

**SRS_XIO_ENDPOINT_30_200: [** The "high-level retry sequence" shall succeed after an injected fault which causes `on_open_complete` to return with `IO_OPEN_ERROR`. **]**

**SRS_XIO_ENDPOINT_30_201: [** The "high-level retry sequence" shall succeed after an injected fault which causes 
 `on_io_error` to be called. **]**
