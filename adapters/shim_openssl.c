// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "shim_openssl.h"

#if defined(USE_OPENSSL_DYNAMIC)

#include <stdio.h>
#include <dlfcn.h>
#include "azure_c_shared_utility/xlogging.h"

// N.B. at this stage, defines are already in places, and references to shimmed
//      function X as a symbol are already being replaced by X_ptr.

static void *libssl;

// Define all function pointers.
#define REQUIRED_FUNCTION(fn) __typeof(fn) fn##_ptr;
FOR_ALL_OPENSSL_FUNCTIONS
#undef REQUIRED_FUNCTION

void load_libssl()
{
#if USE_OPENSSL_1_1_0_OR_UP
    libssl = dlopen("libssl.so.1.1", RTLD_LAZY);
#else
    libssl = dlopen("libssl.so.1.0.2", RTLD_LAZY);

    if (!libssl) libssl = dlopen("libssl.so.1.0.0", RTLD_LAZY);

    if (!libssl) libssl = dlopen("libssl.so.10", RTLD_LAZY);
#endif
    LogInfo("libssl RTLD_NOLOAD: %lx\n", libssl);

    int errorCount = 0;

#define REQUIRED_FUNCTION(fn) \
    if (!(fn##_ptr = (__typeof(fn))(dlsym(libssl, #fn)))) { \
        errorCount++; \
        LogError("Cannot get required symbol " #fn " from libssl\n"); \
    } else { \
        LogInfo( #fn " function is: %lx\n", fn##_ptr); \
    }
    FOR_ALL_OPENSSL_FUNCTIONS
#undef REQUIRED_FUNCTION

    // TODO error. Also, check versions.
}

#endif
