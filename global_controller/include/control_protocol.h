#ifndef CONTROL_PROTOCOL_H_
#define CONTROL_PROTOCOL_H_

/* Request or Response control message between runtime and master */
#define REQUEST_MESSAGE 1
#define RESPONSE_MESSAGE 2
#define ACTION_MESSAGE 3 //just an action to do no response required

/* msg_code field in dedos_control_msg
 * Various types of request/responses between runtime and master */
#define GET_INIT_CONFIG 10
#define SET_INIT_CONFIG 11
#define GET_MSU_LIST 12
#define RUNTIME_ENDPOINT_ADD 13
#define SET_DEDOS_RUNTIMES_LIST 20 /* msg from master to runtime for connecting to other runtimes */
#define CREATE_PINNED_THREAD 21
#define STATISTICS_UPDATE 30

#define CREATE_MSU 2001
#define DESTROY_MSU 2002
#define FORWARD_DATA 2003
#define MSU_ROUTE_ADD 2004
#define MSU_ROUTE_DEL 2005

//Temporary declaration for socket handling MSU
#define DEDOS_SOCKET_HANDLER_MSU_ID 600
#define DEDOS_SSL_READ_MSU_ID 500

struct dedos_control_msg_manage_msu {
    int msu_id;
    int msu_type;
    int init_data_size;
    void *init_data;
};

struct msu_control_add_or_del_route {
	int peer_msu_id;
	int peer_msu_type;
	int peer_locality;
	int peer_ip; //if locality is remote, then some val else 0
};

struct dedos_control_msg {
    int msg_type; //request or response
    int msg_code;
    int header_len;
    int payload_len;
    void *payload;
};

struct msu_stats_data {
    int msu_id;
    int queue_item_processed[2];
    int memory_allocated[2];
    int data_queue_size[2];
};

struct socket_handler_init_payload {
    int port;
    int domain; //AF_INET
    int type; //SOCK_STREAM
    int protocol; //0 most of the time. refer to `man socket`
    unsigned long bind_ip; //fill with inet_addr, inet_pton(x.y.z.y) or give IN_ADDRANY
    int target_msu_type;
};

#endif /* CONTROL_PROTOCOL_H_ */
