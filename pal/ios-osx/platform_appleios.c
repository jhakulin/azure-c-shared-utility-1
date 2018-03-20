// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xio_impl.h"
#include "xio_endpoint_config_tls.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "xio_endpoint_tls_apple.h"

int platform_init(void)
{
    return 0;
}

const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{
    return tlsio_basic_get_interface_description(xio_endpoint_tls_apple_get_interface(), TLSIO_OPTION_BIT_NONE);
}

STRING_HANDLE platform_get_platform_info(void)
{
    STRING_HANDLE result;
    struct utsname nnn;

    if (uname(&nnn) == 0)
    {
        result = STRING_construct_sprintf("(%s; %s)", nnn.sysname, nnn.machine);
        
        if (result == NULL)
        {
            LogInfo("ERROR: Failed to create machine info string");
        }
    }
    else
    {
        LogInfo("WARNING: failed to find machine info.");
        result = STRING_construct("iOS");

		if (result == NULL)
		{
			LogInfo("ERROR: Failed to create machine info string");
		}
    }

    return result;
}

void platform_deinit(void)
{
}
