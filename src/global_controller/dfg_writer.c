/*
START OF LICENSE STUB
    DeDOS: Declarative Dispersion-Oriented Software
    Copyright (C) 2017 University of Pennsylvania, Georgetown University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
END OF LICENSE STUB
*/
#include <string.h>
#include <stdlib.h>
#include "runtime_communication.h"
#include "controller_stats.h"
#include "timeseries.h"
#include "dfg.h"
#include "controller_dfg.h"
#include "jsmn.h"
#include "logging.h"

#include <zmq.h>
#include <unistd.h>

#define JSON_LEN_INCREMENT 1024

#define CHECK_JSON_LEN(json, len)\
    while ( (int)((json).allocated_size - (json).length) < (len)) { \
        log(LOG_DFG_WRITER, "Reallocating to %d", (int)(json.allocated_size + JSON_LEN_INCREMENT)); \
        (json).string = realloc((json).string, (json).allocated_size + JSON_LEN_INCREMENT); \
        (json).allocated_size += JSON_LEN_INCREMENT; \
    }

#define START_JSON(json) \
    (json).length = 0

#define START_LIST(json) \
    do { \
        CHECK_JSON_LEN(json, 2); \
        (json).length += sprintf((json).string + (json).length, "[ "); \
    } while (0)


// This is a hacky trick. Steps back by one to overwrite the previous comma
// If list/obj is empty, it will overwrite the space at the start of the list/obj
#define END_LIST(json) \
    do { \
        CHECK_JSON_LEN(json, 2); \
        (json).length += sprintf((json).string + (json).length - 1, "],") - 1; \
    } while (0)

#define START_OBJ(json)\
    do { \
        CHECK_JSON_LEN(json, 2); \
        (json).length += sprintf((json).string + (json).length, "{ " ); \
    } while (0)

#define END_OBJ(json) \
    do { \
        CHECK_JSON_LEN(json, 2); \
        (json).length += sprintf((json).string + (json).length - 1 , "},") - 1; \
    } while (0)

#define KEY_VAL(json, key, fmt, value, value_len) \
    do { \
        CHECK_JSON_LEN(json, value_len + strlen(key) + 8); \
        (json).length += sprintf((json).string + (json).length, "\"" key "\":" fmt ",", value); \
    } while (0)

#define KEY_INTVAL(json, key, value) \
    KEY_VAL(json, key, "%d", value, 128)

#define KEY_STRVAL(json, key, value) \
    KEY_VAL(json, key, "\"%s\"", value, strlen(value))

#define FMT_KEY_VAL(json, key_fmt, key, key_len, val_fmt, value, value_len) \
    do { \
        CHECK_JSON_LEN(json, key_len + value_len + 8); \
        (json).length += sprintf((json).string + (json).length, "\"" key_fmt "\":\"" val_fmt "\",", key, value); \
    } while (0)

#define KEY(json, key)\
    do { \
        CHECK_JSON_LEN(json, strlen(key) + 4); \
        (json).length += sprintf((json).string + (json).length, "\"" key "\":"); \
    } while (0)

#define VALUE(json, fmt, value, value_len) \
    do { \
        CHECK_JSON_LEN(json, value_len + 4); \
        (json).length += sprintf((json).string + (json).length,  fmt ",", value); \
    } while (0)

#define END_JSON(json) \
    (json).string[(json).length-1] = '\0'

struct json_output {
    char *string;
    size_t allocated_size;
    int list_cnt;
    int length;
};


static char *meta_routing_to_json(struct dfg_meta_routing *meta_routing) {
    static struct json_output json;

    START_JSON(json);
    START_OBJ(json);

    KEY(json, "src_types");
    START_LIST(json);
    for (int i=0; i<meta_routing->n_src_types; i++) {
        VALUE(json, "%d", meta_routing->src_types[i]->id, 8);
    }
    END_LIST(json);

    KEY(json, "dst_types");
    START_LIST(json);
    for (int i=0; i<meta_routing->n_dst_types; i++) {
        VALUE(json, "%d", meta_routing->dst_types[i]->id, 8);
    }
    END_LIST(json);

    END_OBJ(json);
    END_JSON(json);
    return json.string;
}

static char *dependency_to_json(struct dfg_dependency *dep) {
    static struct json_output json;

    START_JSON(json);
    START_OBJ(json);
    KEY_INTVAL(json, "type", dep->type->id);
    KEY_STRVAL(json, "locality", dep->locality == MSU_IS_LOCAL ? "local" : "remote");
    END_OBJ(json);
    END_JSON(json);

    return json.string;
}

static char *msu_type_to_json(struct dfg_msu_type *type) {
    static struct json_output json;

    START_JSON(json);
    START_OBJ(json);

    KEY_INTVAL(json, "id", type->id);
    KEY_STRVAL(json, "name", type->name);

    char *meta_routing = meta_routing_to_json(&type->meta_routing);
    KEY(json, "meta_routing");
    VALUE(json, "%s", meta_routing, strlen(meta_routing));

    KEY(json, "dependencies");
    START_LIST(json);
    for (int i=0; i<type->n_dependencies; i++) {
        char *dependency = dependency_to_json(type->dependencies[i]);
        VALUE(json, "%s", dependency, strlen(dependency));
    }
    END_LIST(json);

    KEY_INTVAL(json, "cloneable", type->cloneable);
    KEY_INTVAL(json, "colocation_group", type->colocation_group);

    END_OBJ(json);
    END_JSON(json);
    return json.string;
}

static char *scheduling_to_json(struct dfg_scheduling *sched) {
    static struct json_output json;

    START_JSON(json);
    START_OBJ(json);

    KEY_INTVAL(json, "runtime", sched->runtime->id);
    KEY_INTVAL(json, "thread_id", sched->thread->id);

    KEY(json, "routes");
    START_LIST(json);
    for (int i=0; i<sched->n_routes; i++) {
        VALUE(json, "%d", sched->routes[i]->id, 8);
    }
    END_LIST(json);

    END_OBJ(json);
    END_JSON(json);

    return json.string;
}

static char *msu_to_json(struct dfg_msu *msu) {
    static struct json_output json;

    START_JSON(json);
    START_OBJ(json);

    KEY_INTVAL(json, "id", msu->id);
    if (msu->vertex_type & ENTRY_VERTEX_TYPE && msu->vertex_type & EXIT_VERTEX_TYPE) {
        KEY_STRVAL(json, "vertex_type", "entry/exit");
    } else if (msu->vertex_type & ENTRY_VERTEX_TYPE) {
        KEY_STRVAL(json, "vertex_type", "entry");
    } else if (msu->vertex_type & EXIT_VERTEX_TYPE) {
        KEY_STRVAL(json, "vertex_type", "exit");
    } else {
        KEY_STRVAL(json, "vertex_type", "nop");
    }

    KEY_STRVAL(json, "init_data", msu->init_data.init_data);
    KEY_INTVAL(json, "type", msu->type->id);
    KEY_STRVAL(json, "blocking_mode",
               msu->blocking_mode == BLOCKING_MSU ? "blocking" : "non-blocking");

    char *scheduling = scheduling_to_json(&msu->scheduling);
    KEY_VAL(json, "scheduling", "%s", scheduling, strlen(scheduling));

    /*
    if (n_stats > 0) {
        char *stats = msu_stats_to_json(msu->id, n_stats);
        KEY_VAL(json, "stats", "%s", stats, strlen(stats));
    }
    */


    END_OBJ(json);
    END_JSON(json);
    return json.string;
}

static char *endpoint_to_json(struct dfg_route_endpoint *ep) {
    static struct json_output json;
    START_JSON(json);
    START_OBJ(json);

    KEY_INTVAL(json, "key", ep->key);
    KEY_INTVAL(json, "msu", ep->msu->id);

    END_OBJ(json);
    END_JSON(json);
    return json.string;
}

static char *route_to_json(struct dfg_route *route) {
    static struct json_output json;

    START_JSON(json);
    START_OBJ(json);

    KEY_INTVAL(json, "id", route->id);
    KEY_INTVAL(json, "type", route->msu_type->id);

    KEY(json, "endpoints");
    START_LIST(json);
    for (int i=0; i<route->n_endpoints; i++) {
        char *ep = endpoint_to_json(route->endpoints[i]);
        VALUE(json, "%s", ep, strlen(ep));
    }
    END_LIST(json);
    END_OBJ(json);
    END_JSON(json);

    return json.string;
}


static char *runtime_to_json(struct dfg_runtime *rt) {
    static struct json_output json;

    START_JSON(json);
    START_OBJ(json);

    KEY_INTVAL(json, "id", rt->id);
    char ip[32];
    struct in_addr addr = {rt->ip};
    inet_ntop(AF_INET, &addr, ip, 32);
    KEY_STRVAL(json, "ip", ip);

    KEY_INTVAL(json, "port", rt->port);
    KEY_INTVAL(json, "n_cores", rt->n_cores);

    KEY_INTVAL(json, "n_pinned_threads", rt->n_pinned_threads);
    KEY_INTVAL(json, "n_unpinned_threads", rt->n_unpinned_threads);

    KEY_INTVAL(json, "connected", runtime_fd(rt->id) > 0 ? 1 : 0);

    KEY(json, "routes");
    START_LIST(json);
    for (int i=0; i<rt->n_routes; i++) {
        char *route = route_to_json(rt->routes[i]);
        VALUE(json, "%s", route, strlen(route));
    }
    END_LIST(json);
    END_OBJ(json);
    END_JSON(json);

    return json.string;
}

static pthread_mutex_t json_lock;
static int initialized = 0;

char *dfg_to_json(struct dedos_dfg *dfg) {
    static struct json_output json;
    if (!initialized) {
        pthread_mutex_init(&json_lock, NULL);
        initialized = 1;
    }
    pthread_mutex_lock(&json_lock);

    START_JSON(json);
    START_OBJ(json);

    KEY_STRVAL(json, "application_name", dfg->application_name);
    char ip[32];
    struct in_addr addr = {htons(dfg->global_ctl_ip)};
    inet_ntop(AF_INET, &addr, ip, 32);
    log(LOG_TEST, "IP IS %s", ip);
    KEY_STRVAL(json, "global_ctl_ip", ip);

    KEY_INTVAL(json, "global_ctl_port", dfg->global_ctl_port);

    KEY(json, "MSU_types");
    START_LIST(json);
    for (int i=0; i<dfg->n_msu_types; i++) {
        char *type = msu_type_to_json(dfg->msu_types[i]);
        VALUE(json, "%s", type, strlen(type));
    }
    END_LIST(json);

    KEY(json, "MSUs");
    START_LIST(json);
    for (int i=0; i<dfg->n_msus; i++) {
        char *msu = msu_to_json(dfg->msus[i]);
        VALUE(json, "%s", msu, strlen(msu));
    }
    END_LIST(json);

    KEY(json, "runtimes");
    START_LIST(json);
    for (int i=0; i<dfg->n_runtimes; i++) {
        char *rt = runtime_to_json(dfg->runtimes[i]);
        VALUE(json, "%s", rt, strlen(rt));
    }
    END_LIST(json);

    END_OBJ(json);
    END_JSON(json);

    pthread_mutex_unlock(&json_lock);
    return json.string;
}

void dfg_to_file(char *filename) {
    lock_dfg();
    struct dedos_dfg *dfg = get_dfg();
    char *dfg_json = dfg_to_json(dfg);
    unlock_dfg();
    int json_size = strlen(dfg_json);
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        log_perror("Cannot write DFG to %s", filename);
        return;
    }
    fwrite(dfg_json, sizeof(char), json_size, file);
    fwrite("\n", sizeof(char), 1, file);
    fclose(file);
}

#define ZMQ_TOPIC "DFG "

int dfg_to_zmq(void *zmq_socket) {
    lock_dfg();
    struct dedos_dfg *dfg = get_dfg();
    char *dfg_json = dfg_to_json(dfg);
    unlock_dfg();

    size_t json_size = strlen(dfg_json);

    int rtn = zmq_send(zmq_socket, ZMQ_TOPIC, strlen(ZMQ_TOPIC), ZMQ_SNDMORE);
    if (rtn < 0) {
        log_perror("Error publishing topic to zmq socket");
    }

    rtn = zmq_send(zmq_socket, dfg_json, json_size, 0);
    if (rtn < 0) {
        log_perror("Error publishing dfg to zmq socket");
        return -1;
    }
    return 0;
}

int dfg_to_fd(int fd) {
    struct dedos_dfg *dfg = get_dfg();
    char *dfg_json = dfg_to_json(dfg);
    unlock_dfg();

    size_t json_size = strlen(dfg_json);

    size_t written = 0;
    while (written < json_size) {
        ssize_t rtn = write(fd, dfg_json + written, json_size - written);
        if (rtn < 0) {
            log_error("error writing dfg to fd %d", fd);
            return -1;
        }
        written += rtn;
    }
    return 0;
}
