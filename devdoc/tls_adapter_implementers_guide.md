# tls_adapter implementer's guide

This document details how to implement a `tls_adapter`.

A `tls_adapter` is a component designed to work with the Azure IoT C SDK to implement
secure communication via TLS to an Azure IoT Hub. The `tls_adapter` is the lowest level
in a stack of components that compose what the Azure IoT C SDK refers to as a "tlsio adapter".
For more information, see the [tlsio_adapter overview](tlsio_adapter_overview.md).

### Can your TLS library work with a socket?

##### Libraries that like sockets
TLS libraries are often written to eliminate any dependency on the specific mechanism
of TCP/IP communication. The usual technique is for the user of the library to 
pass in a set of "bio" functions, which are very simple functions, written by the
library user, that the library uses when it needs to read or write. At least one
TLS library can work with a socket directly.

Examples of libraries that can work with sockets via "bio" functions include 
Windows Schannel, WolfSSL, Mbed TLS, OpenSSL, and CycloneSSL. At least one
TLS library, the OpenSSL implementation used in ESP32, can use a socket directly.

The Azure IoT SDK obliges libraries that like sockets by performing the 
work of socket maintenance, and it provides an abstracted socket called
`socket_async` to the library during initialization. This socket is already
open and ready for use. Libraries that use "bio" functions should implement
these functions in terms of the read and write functions provided in 
`socket_async.h`. Libraries that want to use a socket directly can simply
cast the provided `socket_async` object to a socket handle; nothing else
is required.

If you are using a library that likes sockets, you will:
* include `xio_state.c` in your project
* include `tlsio_adapter_with_sockets.c` in your project
* make sure you exclude `tlsio_adapter_basic.c` from your project 
* create a `tls_adapter_xxxxx.c` file that implements the functions in
  * `tls_adapter_common.h`, and
  * `tls_adapter_with_sockets.h`

That's it! You have completed writing your tlsio adapter. See 
[below](#One-time-init-and-de-init-functions) for details on how
to write each function.

##### Libraries that don't like sockets

Other TLS libraries have no use for sockets because they already have TCP/IP
functionality integrated. Examples include Arduino, Apple OSX, and Apple iOS.

For these libraries the SDK will not bother creating an unneeded `socket_async`.

If you are using a library that doesn't like sockets, you will:
* include `xio_state.c` in your project
* include `tlsio_adapter_basic.c` in your project
* make sure you exclude `tlsio_adapter_with_sockets.c` from your project 
* create a `tls_adapter_xxxxx.c` file that implements the functions in
  * `tls_adapter_common.h`, and
  * `tls_adapter_basic.h`
  
That's it! You have completed writing your tlsio adapter.

Now let's dive into the details of how to write each function.

## One-time init and de-init functions

```c
int tls_adapter_common_init(void);
```
This function is called before any other `tls_adapter` function calls
are made, and can be used for any required one-time setup. This call only
occurs once during startup. Most implementations will leave this function
empty.

```c
void tls_adapter_common_deinit(void);
```
This function is called only once, during normal shutdown of the containing
application. Notice that on microcontrollers there is usually no such thing as
an orderly application shutdown, so you cannot count on this function ever
being called. Most implementations will leave this function empty.

## Creation functions

Depending on whether your TLS library likes sockets or not, you will create
your `tls_adapter` with either 
```c
TLS_ADAPTER_INSTANCE_HANDLE tls_adapter_with_sockets_create(TLSIO_OPTIONS* tlsio_options,
        const char* hostname, SOCKET_ASYNC_HANDLE socket_async);
```
or 
```c
TLS_ADAPTER_INSTANCE_HANDLE tls_adapter_basic_create(TLSIO_OPTIONS* tlsio_options,
        const char* hostname, uint16_t port);
```
Very little work takes place in the creation functions. The typical usage is to `malloc` a
struct where you can store the supplied parameters plus any other data you may need
during the lifetime of your object, fill in that struct with the parameters, and
return a pointer to the struct. This struct then gets passed back to you in all the 
remaining function calls.

Most microcontrollers will only ever connect with a single Azure IoT Hub at a time **Static storage stuff here**
