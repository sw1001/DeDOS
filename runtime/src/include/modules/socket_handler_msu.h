/**
 * @file socket_handler_msu.h
 * Listen/accept connection on a socket, then forward data to a given MSU
 */
#ifndef SOCKET_HANDLER_MSU_H_
#define SOCKET_HANDLER_MSU_H_

#include "generic_msu.h"

#define MAX_SOCKET_PER_MSU 12

/** All socket handler MSUs contain a reference to this type */
extern struct msu_type SOCKET_HANDLER_MSU_TYPE;

struct socket_handler_init_payload {
    int port;
    int domain; //AF_INET
    int type; //SOCK_STREAM
    int protocol; //0 most of the time. refer to `man socket`
    unsigned long bind_ip; //fill with inet_addr, inet_pton(x.y.z.y) or give IN_ADDRANY
    int target_msu_type;
};

struct socket_handler_state {
    int socketfd;
    int target_msu_type;
};

#endif /* SOCKET_HANDLER_MSU_H_ */
