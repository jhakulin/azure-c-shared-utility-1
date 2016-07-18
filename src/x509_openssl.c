// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/x509_openssl.h"
#include "azure_c_shared_utility/xlogging.h"
#include "openssl/bio.h"
#include "openssl/rsa.h"
#include "openssl/x509.h"
#include "openssl/pem.h"

/*return 0 if everything was ok, any other number to signal an error*/
/*this function inserts a x509certificate+x509privatekey to a SSL_CTX (ssl context) in order to authenticate the device with the service*/
int x509_openssl_add_credentials(SSL_CTX* ssl_ctx, const char* x509certificate, const char* x509privatekey)
{
    
    int result;
    if (
        (ssl_ctx == NULL) ||
        (x509certificate == NULL) ||
        (x509privatekey == NULL)
        )
    {
        LogError("invalid parameter detected: SSL_CTX* ssl_ctx=%p, const char* x509certificate=%p, const char* x509privatekey=%p", ssl_ctx, x509certificate, x509privatekey);
        result = __LINE__;
    }
    else
    { 
        BIO *bio_certificate;
        bio_certificate = BIO_new_mem_buf(x509certificate, -1);
        if (bio_certificate == NULL)
        {
            LogError("cannot create  BIO *bio_certificate");
            result = __LINE__;
        }
        else
        {
            X509 *cert = PEM_read_bio_X509(bio_certificate, NULL, 0, NULL);
            if (cert == NULL)
            {
                LogError("cannot create X509 *cert");
                result = __LINE__;
            }
            else
            {
                BIO *bio_privatekey;
                bio_privatekey = BIO_new_mem_buf(x509privatekey, -1);
                if (bio_privatekey == NULL)
                {
                    LogError("cannot create BIO *bio_privatekey;");
                    result = __LINE__;
                }
                else
                {
                    RSA* privatekey = PEM_read_bio_RSAPrivateKey(bio_privatekey, NULL, 0, NULL);
                    if (privatekey == NULL)
                    {
                        LogError("cannot create RSA* privatekey");
                        result = __LINE__; 
                    }
                    else
                    {
                        if (SSL_CTX_use_certificate((SSL_CTX*)ssl_ctx, cert) != 1)
                        {
                            LogError("cannot SSL_CTX_use_certificate");
                            result = __LINE__; 
                        }
                        else
                        {
                            if (SSL_CTX_use_RSAPrivateKey(ssl_ctx, privatekey) != 1)
                            {
                                LogError("cannot SSL_CTX_use_RSAPrivateKey");
                                result = __LINE__;
                            }
                            else
                            {
                                /*all is fine*/
                                result = 0;
                            }
                        }
                        RSA_free(privatekey);
                    }
                    BIO_free(bio_privatekey);
                }
                X509_free(cert);
            }
            BIO_free(bio_certificate);
        }
    }
    return result;
}