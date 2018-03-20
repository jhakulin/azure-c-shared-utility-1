// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This file is made an integral part of tlsio_openssl_compact.c with a #include. It
// is broken out for readability. 

#ifndef FAKE_ENDPOINT_H
#define FAKE_ENDPOINT_H

typedef struct fake_config 
{
    int dummy;
} fake_config;

static fake_config config_instance = { 0 };

static XIO_ENDPOINT_CONFIG_INTERFACE endpoint_config_interface =
{
    xio_endpoint_config_setoption,
    xio_endpoint_config_retrieveoptions,
    xio_endpoint_config_destroy
};

typedef struct XIO_ENDPOINT_INTSTANCE_TAG
{
    int dummy;
} XIO_ENDPOINT_INTSTANCE;

static XIO_ENDPOINT_INTSTANCE endpoint_instance = { 0 };

static XIO_ENDPOINT_INTERFACE endpoint_interface =
{
    xio_endpoint_create,
    xio_endpoint_destroy,
    xio_endpoint_open,
    xio_endpoint_close,
    xio_endpoint_read,
    xio_endpoint_write
};

// 
uint8_t* SSL_send_buffer = (uint8_t*)"111111112222222233333333";
size_t SSL_send_message_size = sizeof(SSL_send_buffer) - 1;

#define SSL_TEST_MESSAGE_SIZE 64
#define SSL_WRITE_MAX_TEST_SIZE 60
#define SSL_SHORT_SENT_MESSAGE_SIZE 30
#define SSL_FAIL_ME_SENT_MESSAGE_SIZE 1700
#define SSL_SHORT_RECEIVED_MESSAGE_SIZE 15
#define SSL_LONG_RECEIVED_MESSAGE_SIZE 1500

static size_t fake_read_byte_out_count = 0;
static size_t fake_read_current_byte_out_count = 0;
static size_t fake_read_bytes_received = 0;


static void init_fake_read(size_t byte_count)
{
    fake_read_byte_out_count = byte_count;
    fake_read_current_byte_out_count = 0;
    fake_read_bytes_received = 0;
}

static void ASSERT_BYTE_RECEIVED(uint8_t byte)
{
    ASSERT_ARE_EQUAL(size_t, (size_t)byte, (size_t)(fake_read_bytes_received % 256));
    fake_read_bytes_received++;
}

int my_xio_endpoint_read(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, uint8_t* buffer, uint32_t buffer_size)
{
    size_t bytes_to_receive;
    size_t i;
    ASSERT_ARE_EQUAL(size_t, (size_t)xio_endpoint_instance, (size_t)&endpoint_instance);
    bytes_to_receive = fake_read_byte_out_count - fake_read_current_byte_out_count;
    bytes_to_receive = bytes_to_receive <= buffer_size ? bytes_to_receive : buffer_size;
    for (i = 0; i < bytes_to_receive; i++)
    {
        buffer[i] = (uint8_t)(fake_read_current_byte_out_count % 256);
        fake_read_current_byte_out_count++;
    }
    return (int)bytes_to_receive;
}

int my_xio_endpoint_write(XIO_ENDPOINT_INSTANCE_HANDLE xio_endpoint_instance, const uint8_t* buffer, uint32_t buffer_size)
{
    int result;
    // "Send" no more than SSL_WRITE_MAX_TEST_SIZE bytes
    (void)buffer; // not used
    ASSERT_ARE_EQUAL(size_t, (size_t)xio_endpoint_instance, (size_t)&endpoint_instance);
    if (buffer_size == SSL_FAIL_ME_SENT_MESSAGE_SIZE)
    {
        result = XIO_ASYNC_RW_RESULT_FAILURE;
    }
    else
    {
        if (buffer_size > SSL_WRITE_MAX_TEST_SIZE)
        {
            result = SSL_WRITE_MAX_TEST_SIZE;
        }
        else
        {
            result = (int)buffer_size;
        }
    }
    return result;
}


#endif // FAKE_ENDPOINT_H
