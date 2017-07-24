#ifndef __SSL_MSU_H__
#define __SSL_MSU_H__

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include "generic_msu.h"
#include "generic_msu_queue.h"
#include "generic_msu_queue_item.h"

#define MAX_REQUEST_LEN 256

enum ssl_data_type {
    READ=0,
    WRITE=1
};

void SSLErrorCheck (int err);

struct ssl_data_payload {
    uint32_t ipAddress;
    uint32_t port;
    uint32_t socketfd;
    int sslMsuId;
    enum ssl_data_type type;
    char msg[MAX_REQUEST_LEN];
    SSL *state;
};

#endif /* __SSL_MSU_H__ */
