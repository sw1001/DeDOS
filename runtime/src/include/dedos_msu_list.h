#ifndef DEDOS_MSU_LIST_H_
#define DEDOS_MSU_LIST_H_

#define DEDOS_PICO_TCP_STACK_MSU_ID 100
#define DEDOS_TCP_DATA_MSU_ID 100

#define DEDOS_PICO_TCP_APP_TCP_ECHO_ID 101

#define DEDOS_TCP_HANDSHAKE_MSU_ID 400
#define DEDOS_TCP_HS_REQUEST_ROUTING_MSU_ID 402
#define DEDOS_SSL_HANDSHAKE_MSU_ID 700

// Webserver MSU's
#define DEDOS_SSL_WRITE_MSU_ID 503
#define DEDOS_SSL_READ_MSU_ID 500
#define DEDOS_WEBSERVER_MSU_ID 501
#define DEDOS_SSL_REQUEST_ROUTING_MSU_ID 502

// Event-driven webserver MSUs
#define DEDOS_WEBSERVER_READ_MSU_ID 551
#define DEDOS_WEBSERVER_HTTP_MSU_ID 552
#define DEDOS_WEBSERVER_REGEX_MSU_ID 553
#define DEDOS_WEBSERVER_WRITE_MSU_ID 554
#define DEDOS_WEBSERVER_REGEX_ROUTING_MSU_ID 560

// Socket handling MSU
#define DEDOS_SOCKET_HANDLER_MSU_ID 600
#define DEDOS_BLOCKING_SOCKET_HANDLER_MSU_ID 601
#define DEDOS_SOCKET_REGISTRY_MSU_ID 611


// Regex parsing MSU
#define DEDOS_REGEX_ROUTING_MSU_ID 504
#define DEDOS_REGEX_MSU_ID 505

/* DUMMY MSU FOR SHOWING USAGE, DO NOT USE THIS */
#define DEDOS_DUMMY_MSU_ID 888

// Baremetal MSU for Profiling runtime without other MSUs
#define DEDOS_BAREMETAL_MSU_ID 900

#endif /* DEDOS_MSU_LIST_H_ */
