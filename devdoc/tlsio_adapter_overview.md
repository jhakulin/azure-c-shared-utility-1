# tlsio_adapter overview

`tlsio_adapter` is layered set of software components that greatly simplifies the task of adapting new
hardware to work with the Azure IoT C SDK.

The [Azure IoT Porting Guide](porting_guide.md) shows several "adapters" which must be present to connect 
an unsupported hardware device to the Azure IoT SDK, and all of these are simple to write with the exception 
of the [tlsio adapter](tlsio_requirements.md), which is extremely complex to implement and can take 
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

The `tlsio_adapter` design splits code into three layers:
* `xio_state` 
