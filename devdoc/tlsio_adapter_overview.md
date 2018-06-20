# tlsio_adapter overview

`tlsio_adapter` is layered set of software components that greatly simplifies the task of adapting new
hardware to work with the Azure IoT C SDK.

The [Azure IoT Porting Guide](porting_guide.md) shows several "adapters" which must be present to connect 
an unsupported hardware device to the Azure IoT SDK, and all of these are simple to write with the exception 
of the [tlsio adapter](tlsio_requirements.md), which is complex to implement, difficult to verify, and can take 
many weeks of work by even very experienced developers.

The use of `tlsio_adapter` speeds the process of developing a tlsio adapter by at least an order of magnitude
via reusable components. 

## References
[xio_requirements](xio_requirements.md)</br>
[xio_adapter_requirements](xio_adapter_requirements.md)

[xio.h](/inc/azure_c_shared_utility/xio.h)</br>
[xio_adapter.h](/inc/azure_c_shared_utility/xio_adapter.h)</br>
[xio_state.h](/inc/azure_c_shared_utility/xio_state.h)</br>


## The major pieces

The `tlsio_adapter` design splits code into three layers. The first two layers are 
reusable code that is part of the SDK. Only the final layer (at most) needs to be implemented
for new devices. 
* **xio_state** The `xio_state` layer encapsulates the state handling, error handling,
message queuing, and most of the callbacks that are required for an `xio` component. This 
layer is a general solution for `xio` component code reuse; it is not specialized for 
`tlsio` components. The `xio_state` layer is implemented entirely in a single 
file: `xio_state.c`.
*  **tlsio_adapter** The `tlsio_adapter` layer encapsulates the remaining `xio` callbacks,
error handling,
option handling, platform adapter functions, read buffering, and the validation of 
`xio` creation parameters. The `tlsio_adapter` layer in combination with the `xio_state`
layer forms a standard `xio` component as required by the SDK.
* **tls_adapter** The upper two layers hide all of the `xio` complexity from the `tls_adapter`
layer, so the low-level `tls_adapter` layer is simple to implement.
 
## Two flavors of tlsio_adapter

Many TLS libraries are capable of either using Berkeley sockets directly (OpenSSL for ESP32)
or using socket-like functionality wrapped in a set of "bio" functions (Windows Schannel, 
WolfSSL, mbedtls, OpenSSL, CycloneSSL). 

Other TLS implementations have their TLS handling integrated with their TCP/IP 
libraries (Arduino, Mac OSX, and iOS).

There are two flavors of the `tlsio_adapter` middle layer to handle these two different cases.
* The **tlsio_adapter_basic** is for TLS implementations that don't use sockets. It consists of
two files: `tlsio_adapter_basic.c`, and the shared file `tlsio_adapter_common.c`.
These files encapsulate the final `xio` callbacks, error handling, option handling, platform
adapter functions, read buffering, and validation of `xio_create` parameters.
* The **tlsio_adapter_with_sockets** does all the same work as `tlsio_adapter_basic`, 
and in addition it provides the low-level `tls_adapter` with a socket wrapped in a 
platform-independent `socket_async` component. The `tlsio_adapter_with_sockets`
does all the work of maintaining the socket, including creation, deletion, configuration,
opening, and closing, so the low-level `tls_adapter` can simply use the `socket_async`
 as-provided. It consists of two files: `tlsio_adapter_with_sockets.c` and 
the shared file `tlsio_adapter_common.c`.