#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

# Honor visibility properties for all target types
cmake_policy(SET CMP0063 NEW)

if (TARGET aziotsharedutil)
    RETURN()
endif()

project(azure_c_shared_utility)

FILE(READ ${CMAKE_CURRENT_LIST_DIR}/version.txt C_SHARED_VERSION)

# Include the common build rules for the C SDK
include(configs/azure_iot_build_rules.cmake)

#the following variables are project-wide and can be used with cmake-gui
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(skip_samples "set skip_samples to ON to skip building samples (default is OFF)[if possible, they are always built]" OFF)
option(use_http "set use_http to ON if http is to be used, set to OFF to not use http" ON)
option(use_condition "set use_condition to ON if the condition module and its adapters should be enabled" ON)
option(use_wsio "set use_wsio to ON to build WebSockets support (default is ON)" ON)
option(nuget_e2e_tests "set nuget_e2e_tests to ON to generate e2e tests to run with nuget packages (default is OFF)" OFF)
option(use_installed_dependencies "set use_installed_dependencies to ON to use installed packages instead of building dependencies from submodules" OFF)
option(use_default_uuid "set use_default_uuid to ON to use the out of the box UUID that comes with the SDK rather than platform specific implementations" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF). Chsare dutility does not have any e2e tests, but the option needs to exist to evaluate in IF statements" OFF)
option(use_builtin_httpapi "set use_builtin_httpapi to ON to use the built-in httpapi_compact that comes with C shared utility (default is OFF)" OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is ON)" ON)
option(suppress_header_searches "do not try to find headers - used when compiler check will fail" OFF)
option(use_custom_heap "use externally defined heap functions instead of the malloc family" OFF)

if(${use_custom_heap})
    add_definitions(-DGB_USE_CUSTOM_HEAP)
endif()

if(WIN32)
    option(use_schannel "set use_schannel to ON if schannel is to be used, set to OFF to not use schannel" ON)
    option(use_openssl "set use_openssl to ON if openssl is to be used, set to OFF to not use openssl" OFF)
    option(use_openssl_dynamic "set use_openssl_dynamic to ON if openssl is to be dynamically loaded (Linux only)" OFF)
    option(use_wolfssl "set use_wolfssl to ON if wolfssl is to be used, set to OFF to not use wolfssl" OFF)
    option(use_etw "set use_etw to ON if ETW logging is to be used. Default is OFF" OFF)
    option(use_socketio "set use_socketio to ON if socketio is to be included in the library, set to OFF if a different implementation will be provided" ON)
else()
    option(use_schannel "set use_schannel to ON if schannel is to be used, set to OFF to not use schannel" OFF)
    if(MACOSX)
        option(use_openssl "set use_openssl to ON if openssl is to be used, set to OFF to not use openssl" OFF)
        option(use_openssl_dynamic "set use_openssl_dynamic to ON if openssl is to be dynamically loaded (Linux only)" OFF)
        option(use_socketio "set use_socketio to ON if socketio is to be included in the library, set to OFF if a different implementation will be provided" OFF)
    else()
        option(use_openssl "set use_openssl to ON if openssl is to be used, set to OFF to not use openssl" ON)
        option(use_openssl_dynamic "set use_openssl_dynamic to ON if openssl is to be dynamically loaded (Linux only)" OFF)
        option(use_socketio "set use_socketio to ON if socketio is to be included in the library, set to OFF if a different implementation will be provided" ON)
    endif()    
    option(use_wolfssl "set use_wolfssl to ON if wolfssl is to be used, set to OFF to not use wolfssl" OFF)
endif()

option(no_logging "disable logging (default is OFF)" OFF)

# The options setting for use_socketio is not reliable. If openssl is used, make sure it's on,
# and if apple tls is used then use_socketio must be off.
if (MACOSX)
    if (${use_openssl})
        set(use_socketio ON)
    else()
        # MACOSX only has native tls and open ssl, so this must be the native apple tls
        set(use_applessl ON)
        set(use_socketio ON)
    endif()
endif()

if(${exclude_aziotsharedutil_networking})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEXCLUDE_AZIOTSHAREDUTIL_NETWORKING")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DEXCLUDE_AZIOTSHAREDUTIL_NETWORKING")
    option(use_http "set use_http to ON if http is to be used, set to OFF to not use http" OFF)
    option(use_wsio "set use_wsio to ON to build WebSockets support (default is ON)" OFF)
    option(use_builtin_httpapi "set use_builtin_httpapi to ON to use the built-in httpapi_compact that comes with C shared utility (default is OFF)" OFF)
    option(use_schannel "set use_schannel to ON if schannel is to be used, set to OFF to not use schannel" OFF)
    option(use_openssl "set use_openssl to ON if openssl is to be used, set to OFF to not use openssl" OFF)
    option(use_openssl_dynamic "set use_openssl_dynamic to ON if openssl is to be dynamically loaded (Linux only)" OFF)
    option(use_wolfssl "set use_wolfssl to ON if wolfssl is to be used, set to OFF to not use wolfssl" OFF)
    option(use_socketio "set use_socketio to ON if socketio is to be included in the library, set to OFF if a different implementation will be provided" OFF)
    option(use_httpapi_compact "set use_httpapi_compact to ON if httpapi compact implementation is to be included in the library, set to OFF if a different implementation will be provided" OFF)
endif()

if(${use_etw}) 
    #create res folder
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/res)
    
    #populate res folder with the ETW resources
    execute_process(COMMAND  ${CMAKE_CURRENT_LIST_DIR}/src/etw_provider_generate.cmd 
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src
    )
endif()

option(build_as_dynamic "build the C Shared libaries as shared"  OFF)

if(${memory_trace})
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
    add_definitions(-DGB_MEASURE_NETWORK_FOR_THIS -DGB_DEBUG_NETWORK)
endif()

if(${use_openssl})
    if("${OPENSSL_ROOT_DIR}" STREQUAL "" AND NOT ("$ENV{OpenSSLDir}" STREQUAL ""))
        set(OPENSSL_ROOT_DIR $ENV{OpenSSLDir} CACHE PATH "")
    endif()

    find_package(OpenSSL REQUIRED)
    include_directories(${OPENSSL_INCLUDE_DIR})
    message(STATUS "openssl headers found at ${OPENSSL_INCLUDE_DIR}, libs at ${OPENSSL_LIBRARIES}")
endif()

if(${use_applessl})
    # MACOSX only has native tls and open ssl, so use the native apple tls
    find_library(cf_foundation Foundation)
    find_library(cf_network CFNetwork)
endif()

if(${no_logging})
    add_definitions(-DNO_LOGGING)
endif()
# Start of variables used during install
set (LIB_INSTALL_DIR lib CACHE PATH "Library object file directory")

#Setup the platform files
include("${CMAKE_CURRENT_LIST_DIR}/configs/azure_c_shared_utilityFunctions.cmake")
set_platform_files(${CMAKE_CURRENT_LIST_DIR})

if(MSVC)
    if (WINCE)
        # WEC 2013 uses older VS compiler. Build some files as C++ files to resolve C99 related compile issues
        SET_SOURCE_FILES_PROPERTIES(${LOGGING_C_FILE} ${XLOGGING_C_FILE} src/map.c adapters/uniqueid_win32.c src/tlsio_schannel.c src/x509_schannel.c PROPERTIES LANGUAGE CXX)
        if (${use_httpapi_compact})
            SET_SOURCE_FILES_PROPERTIES(adapters/httpapi_compact.c PROPERTIES LANGUAGE CXX)
        else()
            SET_SOURCE_FILES_PROPERTIES(adapters/httpapi_wince.c PROPERTIES LANGUAGE CXX)
        endif()
    ELSE()
        if(${use_openssl})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENSSL")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENSSL")
        endif()
        if(${use_wolfssl})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_WOLFSSL")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_WOLFSSL")
        endif()

    endif()
elseif(UNIX) #LINUX OR APPLE
    if(${use_openssl})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENSSL")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENSSL")
    endif()
    if(${use_wolfssl})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_WOLFSSL")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_WOLFSSL")
    endif()
endif()


include_directories(${UMOCK_C_INC_FOLDER})

message(STATUS "use new submodule for azure c shared")

#these are the C source files
set(source_c_files
./src/base32.c
./src/base64.c
./src/buffer.c
./src/connection_string_parser.c
./src/constbuffer.c
${LOGGING_C_FILE}
./src/crt_abstractions.c
./src/constmap.c
./src/doublylinkedlist.c
./src/gballoc.c
./src/gbnetwork.c
./src/gb_stdio.c
./src/gb_time.c
./src/gb_rand.c
./src/hmac.c
./src/hmacsha256.c
./src/http_proxy_io.c
./src/xio.c
./src/singlylinkedlist.c
./src/map.c
./src/sastoken.c
./src/sha1.c
./src/sha224.c
./src/sha384-512.c
./src/strings.c
./src/string_token.c
./src/string_tokenizer.c
./src/uuid.c
./src/urlencode.c
./src/usha.c
./src/vector.c
${XLOGGING_C_FILE}
./src/optionhandler.c
./adapters/agenttime.c
${CONDITION_C_FILE}
${LOCK_C_FILE}
${PLATFORM_C_FILE}
${SOCKETIO_C_FILE}
${TICKCOUTER_C_FILE}
${THREAD_C_FILE}
#${UNIQUEID_C_FILE}
${ENVIRONMENT_VARIABLE_C_FILE}
)

if(UNIX) #LINUX OR APPLE
    set(source_c_files ${source_c_files}
        ./adapters/linux_time.c
    )
endif()

if(${use_http})
    set(source_c_files ${source_c_files}
        ./src/httpapiex.c
        ./src/httpapiexsas.c
        ./src/httpheaders.c
        ${HTTP_C_FILE}
    )
endif()

if(${use_schannel})
    set(source_c_files ${source_c_files}
        ./adapters/tlsio_schannel.c
        ./adapters/x509_schannel.c
    )
endif()
if(${use_wolfssl})
    set(source_c_files ${source_c_files}
        ./adapters/tlsio_wolfssl.c
    )
endif()
if(${use_openssl})
    set(source_c_files ${source_c_files}
        ./adapters/tlsio_openssl.c
        ./adapters/x509_openssl.c
    )
    if(${use_openssl_dynamic})
        set(source_c_files ${source_c_files}
            ./adapters/shim_openssl.c
        )
    endif()
endif()
if(${use_applessl})
    set(source_c_files ${source_c_files}
        ./pal/ios-osx/tlsio_appleios.c
        ./pal/tlsio_options.c
    )
    include_directories(./pal/ios-osx/)
endif()

#these are the C headers
set(source_h_files
./inc/azure_c_shared_utility/agenttime.h
./inc/azure_c_shared_utility/base32.h
./inc/azure_c_shared_utility/base64.h
./inc/azure_c_shared_utility/buffer_.h
./inc/azure_c_shared_utility/connection_string_parser.h
./inc/azure_c_shared_utility/crt_abstractions.h
./inc/azure_c_shared_utility/constmap.h
./inc/azure_c_shared_utility/condition.h
./inc/azure_c_shared_utility/const_defines.h
${LOGGING_H_FILE}
./inc/azure_c_shared_utility/doublylinkedlist.h
./inc/azure_c_shared_utility/gballoc.h
./inc/azure_c_shared_utility/gbnetwork.h
./inc/azure_c_shared_utility/gb_stdio.h
./inc/azure_c_shared_utility/gb_time.h
./inc/azure_c_shared_utility/gb_rand.h
./inc/azure_c_shared_utility/hmac.h
./inc/azure_c_shared_utility/hmacsha256.h
./inc/azure_c_shared_utility/http_proxy_io.h
./inc/azure_c_shared_utility/singlylinkedlist.h
./inc/azure_c_shared_utility/lock.h
./inc/azure_c_shared_utility/macro_utils.h
./inc/azure_c_shared_utility/map.h
./inc/azure_c_shared_utility/optimize_size.h
./inc/azure_c_shared_utility/platform.h
./inc/azure_c_shared_utility/refcount.h
./inc/azure_c_shared_utility/sastoken.h
./inc/azure_c_shared_utility/sha-private.h
./inc/azure_c_shared_utility/shared_util_options.h
./inc/azure_c_shared_utility/sha.h
./inc/azure_c_shared_utility/socketio.h
./inc/azure_c_shared_utility/stdint_ce6.h
./inc/azure_c_shared_utility/strings.h
./inc/azure_c_shared_utility/strings_types.h
./inc/azure_c_shared_utility/string_token.h
./inc/azure_c_shared_utility/string_tokenizer.h
./inc/azure_c_shared_utility/string_tokenizer_types.h
./inc/azure_c_shared_utility/tickcounter.h
./inc/azure_c_shared_utility/threadapi.h
./inc/azure_c_shared_utility/xio.h
./inc/azure_c_shared_utility/umock_c_prod.h
./inc/azure_c_shared_utility/uniqueid.h
./inc/azure_c_shared_utility/uuid.h
./inc/azure_c_shared_utility/urlencode.h
./inc/azure_c_shared_utility/vector.h
./inc/azure_c_shared_utility/vector_types.h
./inc/azure_c_shared_utility/vector_types_internal.h
./inc/azure_c_shared_utility/xlogging.h
./inc/azure_c_shared_utility/constbuffer.h
./inc/azure_c_shared_utility/tlsio.h
./inc/azure_c_shared_utility/optionhandler.h
)

if(UNIX) #LINUX OR APPLE
    set(source_h_files ${source_h_files}
        ./adapters/linux_time.h
    )
endif()

if(${use_openssl})
    set(source_h_files ${source_h_files}
        ./adapters/shim_openssl.h
    )
endif()
if(${use_wsio})
    set(source_h_files ${source_h_files}
        ./inc/azure_c_shared_utility/wsio.h
        ./inc/azure_c_shared_utility/uws_client.h
        ./inc/azure_c_shared_utility/uws_frame_encoder.h
        ./inc/azure_c_shared_utility/utf8_checker.h
    )
    set(source_c_files ${source_c_files}
        ./src/wsio.c
        ./src/uws_client.c
        ./src/uws_frame_encoder.c
        ./src/utf8_checker.c
    )
endif()

if(${use_http})
    set(source_h_files ${source_h_files}
        ./inc/azure_c_shared_utility/httpapi.h
        ./inc/azure_c_shared_utility/httpapiex.h
        ./inc/azure_c_shared_utility/httpapiexsas.h
        ./inc/azure_c_shared_utility/httpheaders.h
        )
endif()

if(${use_schannel})
    set(source_h_files ${source_h_files}
    ./inc/azure_c_shared_utility/tlsio_schannel.h
    ./inc/azure_c_shared_utility/x509_schannel.h
    )
endif()

if(${use_wolfssl})
    set(source_h_files ${source_h_files}
        ./inc/azure_c_shared_utility/tlsio_wolfssl.h
    )
    if(WIN32)
        include_directories($ENV{WolfSSLDir})
    endif()
endif()
if(${use_openssl})
    set(source_h_files ${source_h_files}
        ./inc/azure_c_shared_utility/tlsio_openssl.h
        ./inc/azure_c_shared_utility/x509_openssl.h
        )
endif()
if(${use_applessl})
    set(source_h_files ${source_h_files}
        ./pal/ios-osx/tlsio_appleios.h
    )
    include_directories(./pal/ios-osx/)
endif()

#this is the product (a library)
add_library(aziotsharedutil ${source_c_files} ${source_h_files})

if(WIN32 AND SPECTRE_MITIGATION)
    set_property(TARGET aziotsharedutil PROPERTY VS_GLOBAL_SpectreMitigation "Spectre")
endif()

setTargetBuildProperties(aziotsharedutil)

target_include_directories(aziotsharedutil PUBLIC $<BUILD_INTERFACE:${SHARED_UTIL_INC_FOLDER}>)

if(MSVC)
    set(source_h_files ${source_h_files}
        ./pal/windows/refcount_os.h
    )
    target_include_directories(aziotsharedutil PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>$<BUILD_INTERFACE:${SHARED_UTIL_FOLDER}/pal/windows>)
else()
    set(source_h_files ${source_h_files}
        ./pal/linux/refcount_os.h
    )
    target_include_directories(aziotsharedutil PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>$<BUILD_INTERFACE:${SHARED_UTIL_FOLDER}/pal/linux>)
endif()


if(${build_as_dynamic})
    #make sure we can link as dll/so
    add_library(aziotsharedutil_dll SHARED ./src/aziotsharedutil.def ${source_c_files} ${source_h_files})
    set_target_properties(aziotsharedutil_dll PROPERTIES OUTPUT_NAME "aziotsharedutil")
endif()

set(aziotsharedutil_target_libs)

if(${use_http})
    if(WIN32)
        if (WINCE)
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} crypt32 ws2)
        else()
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} crypt32 winhttp)
        endif()
    else()
        if (NOT use_builtin_httpapi)
            if (CMAKE_CROSSCOMPILING)
                # As mentioned at https://cmake.org/Wiki/CMake_Cross_Compiling the
                # pkg-config tool can not be used by cmake while cross compiling.
                message(STATUS "Cross compiling not using pkg-config")
            else()
                # try pkg-config first
                find_package(PkgConfig)
                if(PKG_CONFIG_FOUND)
                    pkg_check_modules(CURL libcurl)
                endif()
            endif()

            # if that didn't work, try CMake find_package
            if(NOT CURL_FOUND)
                find_package(CURL)
            endif()

            set(CURL_FIND_REQUIRED 1)
            find_package_handle_standard_args(CURL DEFAULT_MSG CURL_LIBRARIES)

            include_directories(${CURL_INCLUDE_DIRS})
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} ${CURL_LIBRARIES})
        endif(NOT use_builtin_httpapi)
    endif()
endif(${use_http})

if(${use_schannel})
    if(WIN32)
        if (WINCE)
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} secur32)
        else()
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} crypt32 ws2_32 secur32 advapi32 ncrypt)
        endif()
    endif()
endif()

if(${use_openssl})
    if (${use_openssl_dynamic})
      add_definitions(-DUSE_OPENSSL_DYNAMIC)
      set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} ${CMAKE_DL_LIBS})
    else()
      set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} ${OPENSSL_LIBRARIES})
    endif()

    if (WIN32)
        set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} crypt32 ws2_32 secur32)
    endif()
endif()

if(${use_wolfssl})
    if (WIN32)
        if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} $ENV{WolfSSLDir}/Debug/wolfssl.lib)
        else()
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} $ENV{WolfSSLDir}/Release/wolfssl.lib)
        endif()
    else()
        set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} wolfssl)
    endif()
endif()

if(${use_applessl})
    set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} ${cf_foundation} ${cf_network})
endif()

if(WIN32)
    if (NOT ${use_default_uuid})
        set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} rpcrt4.lib)
    endif()
endif()

if(LINUX)
    set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} pthread m rt)
    if (NOT ${use_default_uuid})
        if(APPLE)
            find_package(PkgConfig REQUIRED)
            pkg_search_module(UUID REQUIRED uuid)
            link_directories(${UUID_LIBRARY_DIRS})
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} -L${UUID_LIBRARY_DIRS} ${UUID_LIBRARIES})
        else()
            set(aziotsharedutil_target_libs ${aziotsharedutil_target_libs} uuid)
        endif()
    endif()
endif()

target_link_libraries(aziotsharedutil ${aziotsharedutil_target_libs})
if(${build_as_dynamic})
    target_link_libraries(aziotsharedutil_dll ${aziotsharedutil_target_libs})
endif()

if (${run_unittests} OR ${run_e2e_tests})
    include("dependencies-test.cmake")
    add_subdirectory(testtools)
    setTargetBuildProperties(ctest)
    setTargetBuildProperties(testrunnerswitcher)
    setTargetBuildProperties(umock_c)
    if (${run_unittests})
        add_subdirectory(tests)
    endif()
endif()

function(FindDllFromLib var libFile)
    get_filename_component(_libName ${libFile} NAME_WE)
    get_filename_component(_libDir ${libFile} DIRECTORY)

    while(NOT ("${_libDir}" STREQUAL ""))
        file(GLOB_RECURSE _dllList "${_libDir}/${_libName}.dll")
        list(LENGTH _dllList _listLen)
        if(${_listLen} GREATER 0)
            list(GET _dllList 0 _dll)
            set(${var} "${_dll}" PARENT_SCOPE)
            break()
        endif()
        get_filename_component(_libDir ${_libDir} DIRECTORY)
    endwhile()
endfunction()

if (NOT ${skip_samples})
    if(${use_openssl} AND WIN32)
        FindDllFromLib(SSL_DLL "${OPENSSL_SSL_LIBRARY}")
        FindDllFromLib(CRYPTO_DLL "${OPENSSL_CRYPTO_LIBRARY}")
    endif()
    add_subdirectory(samples)
endif()

# Set CMAKE_INSTALL_* if not defined
include(GNUInstallDirs)

if(${use_installed_dependencies})

    if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR "lib")
    endif()

    # Install Azure C Shared Utility
    set(package_location "cmake")
    
    if(${build_as_dynamic})
        set(targets aziotsharedutil aziotsharedutil_dll)
    else(${build_as_dynamic})
        set(targets aziotsharedutil)
    endif(${build_as_dynamic})
    
    install (TARGETS ${targets} EXPORT aziotsharedutilTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/../bin
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot
    )
    install (FILES ${source_h_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot/azure_c_shared_utility)
    install (FILES ${micromock_h_files_full_path} ${INSTALL_H_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)

    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${C_SHARED_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_file("configs/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
        COPYONLY
    )

    install(EXPORT aziotsharedutilTargets
        FILE
            "${PROJECT_NAME}Targets.cmake"
        DESTINATION
            ${package_location}
    )
    install(
        FILES
            "configs/${PROJECT_NAME}Config.cmake"
            "configs/${PROJECT_NAME}Functions.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION
            ${package_location}
    )
else()
    set(install_staticlibs
        aziotsharedutil
    )
    install(FILES ${source_h_files}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azure_c_shared_utility) 
    install(TARGETS ${install_staticlibs} 
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

