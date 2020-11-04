/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nc_core.h>
#include <nc_server.h>
#include <glib.h>
#include "cJSON.h"
#include "spdlog.h"
#include "trace_conf.h"
#include "nc_time.h"
#include "proto/nc_redis.h"

extern struct conf_trace_data *trace_conf;
extern int g_slow_threshold;

void format_json_data(GString *json_data, int dept_id, char *app_name, char *room, GString *client_addr, GString *proxy_addr, GString *server_addr, char *key, GString *req_type, char *req, int req_len, int rsp_len, char *duration, GString * recv_client_time, GString * send_server_time, GString * recv_server_time, GString * send_client_time)
 {
    char* msg = NULL;
    cJSON * root =  cJSON_CreateObject();

    cJSON_AddItemToObject(root, "dept_id", cJSON_CreateNumber(10));
    cJSON_AddItemToObject(root, "app_name", cJSON_CreateString(app_name));
    cJSON_AddItemToObject(root, "room", cJSON_CreateString(room));
    cJSON_AddItemToObject(root, "client_ip", cJSON_CreateString(client_addr->str));
    cJSON_AddItemToObject(root, "proxy_ip", cJSON_CreateString(proxy_addr->str));
    cJSON_AddItemToObject(root, "server_ip", cJSON_CreateString(server_addr->str));
    cJSON_AddItemToObject(root, "key", cJSON_CreateString(key));
    cJSON_AddItemToObject(root, "req_type", cJSON_CreateString(req_type->str));
    cJSON_AddItemToObject(root, "req", cJSON_CreateString(req));
    cJSON_AddItemToObject(root, "req_len", cJSON_CreateNumber(req_len));
    cJSON_AddItemToObject(root, "rsp_len", cJSON_CreateNumber(rsp_len));
    cJSON_AddItemToObject(root, "duration", cJSON_CreateString(duration));
    cJSON_AddItemToObject(root, "recv_client_time", cJSON_CreateString(recv_client_time->str));
    cJSON_AddItemToObject(root, "send_server_time", cJSON_CreateString(send_server_time->str));
    cJSON_AddItemToObject(root, "recv_server_time", cJSON_CreateString(recv_server_time->str));
    cJSON_AddItemToObject(root, "send_client_time", cJSON_CreateString(send_client_time->str));

    msg = cJSON_PrintUnformatted(root);
    g_string_sprintf(json_data, "%s", msg);
    free(msg);
    cJSON_Delete(root);
}

struct msg *
req_get(struct conn *conn)
{
    struct msg *msg;

    ASSERT(conn->client && !conn->proxy);

    msg = msg_get(conn, true, conn->redis);
    if (msg == NULL) {
        conn->err = errno;
    }
    return msg;
}

static void
req_log(struct msg *req)
{
    struct msg *rsp;           /* peer message (response) */
    int64_t req_time;          /* time cost for this request */
    char *peer_str;            /* peer client ip:port */
    uint32_t req_len, rsp_len; /* request and response length */
    struct string *req_type;   /* request type string */
    struct keypos *kpos;

    if (log_loggable(LOG_NOTICE) == 0) {
        return;
    }

    /* a fragment? */
    if (req->frag_id != 0 && req->frag_owner != req) {
        return;
    }

    /* conn close normally? */
    if (req->mlen == 0) {
        return;
    }
    /*
     * there is a race scenario where a requests comes in, the log level is not LOG_NOTICE,
     * and before the response arrives you modify the log level to LOG_NOTICE
     * using SIGTTIN OR SIGTTOU, then req_log() wouldn't have msg->start_ts set
     */
    if (req->start_ts == 0) {
        return;
    }

    req_time = nc_usec_now() - req->start_ts;

    rsp = req->peer;
    req_len = req->mlen;
    rsp_len = (rsp != NULL) ? rsp->mlen : 0;

    if (array_n(req->keys) < 1) {
        return;
    }

    /*
     * FIXME: add backend addr here
     * Maybe we can store addrstr just like server_pool in conn struct
     * when connections are resolved
     */
    peer_str = nc_unresolve_peer_desc(req->owner->sd);
    req_type = msg_type_string(req->type);

    
    
    uint64_t  recv_client_time = req->recv_client_time;
    uint64_t  send_server_time = req->send_server_time;
    uint64_t  recv_server_time = rsp->recv_server_time;
    uint64_t  send_client_time = req->send_client_time;

    log_debug(LOG_DEBUG, "recv_client_time %ld,  send_server_time %ld, recv_server_time %ld,  send_client_time %ld\n", recv_client_time, send_server_time, recv_server_time, send_client_time);
    
    //需要提前处理
//    char buf2[8192]={'0'};
//    struct mbuf *mbuf2;
//    mbuf2 = STAILQ_FIRST(&req->mhdr);
//    memcpy(buf2, mbuf2->start, mbuf2->last - mbuf2->start);
//    buf2[mbuf2->last - mbuf2->start] = '\0';   
//    printf("ggggggggggggggg %s \n", buf2);
    
    if (NULL == trace_conf || 0 == send_server_time) 
    {
        return;
    }
      
    int slow_threshold = 0;
    slow_threshold = g_slow_threshold > 0 ? g_slow_threshold : trace_conf->slow_threshold;

    if (0 == slow_threshold)
    {
        log_error("trace_conf is null in children or  trace_conf->slow_threshold eq %d", trace_conf->slow_threshold);
        return;
    }
        
    int dur = (int)(send_client_time - recv_client_time) / 1000;
    log_debug(LOG_NOTICE, "dur: %d  trace_conf->slow_threshold: %d", dur, trace_conf->slow_threshold);
    
    if(dur <= slow_threshold) 
    {
        return;
    }

    GString *req_type_str = g_string_new(NULL);
    g_string_printf(req_type_str, "%.*s", req_type->len, req_type->data);
    
    GString *client_addr = g_string_new(NULL);
    g_string_printf(client_addr, "%s", peer_str);

    GString *server_addr = g_string_new(NULL);
    g_string_printf(server_addr, "%s", nc_unresolve_peer_desc(rsp->owner->sd));

    log_debug(LOG_NOTICE, "recv_client_time %ld, send_server_time %ld , recv_server_time %ld, send_client_time %ld",
              req->recv_client_time, req->send_server_time, rsp->recv_server_time, req->send_client_time);
    
    struct conn *conn = rsp->owner;
    struct server *server =(struct server *)conn->owner;
    struct server_pool *pool = server->owner;

    GString *proxy_addr = g_string_new(NULL);
    g_string_printf(proxy_addr, "%s", nc_unresolve_desc(pool->p_conn->sd));
    
    GString *json_data = g_string_new(NULL);

    int dept_id = 0;
    char *app_name = NULL;
    char *room = NULL;
    
    if (NULL != trace_conf) 
    {
        dept_id = trace_conf->dept_id;
        app_name = trace_conf->app_name;
        room = trace_conf->room;
    }

    char *buf = NULL;
    int is_big_mem = 0;
    int mbuf_count = 0;
    long int total_len = 0;
    struct mbuf *mbuf;
    struct mbuf *nbuf;

    for (mbuf = STAILQ_FIRST(&req->mhdr); mbuf != NULL; mbuf = nbuf) 
    {
        nbuf = STAILQ_NEXT(mbuf, next);
        mbuf_count++;

        //记录多个mbuf中的数据长度
        total_len += mbuf->last - mbuf->start;
    }

    long int len;
    if(1 == mbuf_count )
    {
        mbuf = STAILQ_FIRST(&req->mhdr);
        len = mbuf->last - mbuf->start;
        if( len < 8192) 
        {
            char small_buf[8192]={'0'};
            buf = small_buf;
        }
        else 
        {
            char *big_buf = (char*)calloc(mbuf->last - mbuf->start+1, sizeof(char));
            buf = big_buf;
        }
        
        memcpy(buf, mbuf->start, mbuf->last - mbuf->start);
        buf[mbuf->last - mbuf->start] = '\0';   
    }
    else
    {
        buf =(char*)calloc(total_len+1, sizeof(char));

        for (mbuf = STAILQ_FIRST(&req->mhdr); mbuf != NULL; mbuf = nbuf) 
        {
            nbuf = STAILQ_NEXT(mbuf, next);

            if (mbuf_empty(mbuf))
            {
                continue;
            }
            
            strncat(buf, mbuf->pos, mbuf->last - mbuf->pos);
        }

        buf[total_len] = '\0';
    }

//    感觉没必要解析
//    GString *new_format_cmd= g_string_new(NULL);
//    gchar **t = NULL;
//    gchar **p = NULL;
//
//     printf("%s\n", buf);
//    t = g_strsplit(buf, "\r\n", 0);
//    int count = sizeof(*t);
//    printf("count %d\n", count);
//    int i=0;
//    for ( p = t; *p; p++ ) {
//        i++;
//        printf("i %d\n", i);
//        char *info = *p;
//        if(NULL == strstr(info, "*") && NULL == strstr(info, "$") && NULL == strstr(info, "\r\n")) {
//            g_string_append(new_format_cmd, info);
//            if(i <= count)
//            {
//                g_string_append_c(new_format_cmd, ' ');
//            }
//        }
//    }
//    g_strfreev(t);
    
    kpos = array_get(req->keys, 0);
    if (kpos->end != NULL) {
        *(kpos->end) = '\0';
    }

    log_debug(LOG_NOTICE, "req %"PRIu64" done on c %d req_time %"PRIi64".%03"PRIi64
              " msec type %.*s narg %"PRIu32" req_len %"PRIu32" rsp_len %"PRIu32
              " key0 '%s' peer '%s' done %d error %d",
              req->id, req->owner->sd, req_time / 1000, req_time % 1000,
              req_type->len, req_type->data, req->narg, req_len, rsp_len,
              kpos->start, peer_str, req->done, req->error);

    GString *recv_client_time_str = g_string_sized_new(sizeof("2004-01-01T00:00:00.000Z"));
    get_time_str(recv_client_time_str, recv_client_time/1000);
    log_debug(LOG_DEBUG, "recv_client_time_str %s", recv_client_time_str->str);

    GString *send_server_time_str = g_string_sized_new(sizeof("2004-01-01T00:00:00.000Z"));
    get_time_str(send_server_time_str, send_server_time/1000);
    log_debug(LOG_DEBUG, "send_server_time_str %s", send_server_time_str->str);

    GString *recv_server_time_str = g_string_sized_new(sizeof("2004-01-01T00:00:00.000Z"));
    get_time_str(recv_server_time_str, recv_server_time/1000);
    log_debug(LOG_DEBUG, "recv_server_time_str %s", recv_server_time_str->str);

    GString *send_client_time_str = g_string_sized_new(sizeof("2004-01-01T00:00:00.000Z"));
    get_time_str(send_client_time_str, send_client_time/1000);
    log_debug(LOG_DEBUG, "send_client_time_str %s", send_client_time_str->str);

    double duration = (send_client_time - recv_client_time) / 1000.0;
    char duration_str[10];
    sprintf(duration_str, "%.3f", duration);

    format_json_data(json_data, dept_id, app_name, room, client_addr, proxy_addr, server_addr, kpos->start, req_type_str, buf, req_len, rsp_len, duration_str, recv_client_time_str, send_server_time_str, recv_server_time_str, send_client_time_str);

    wirte_log(json_data->str);
    
    if(1 == mbuf_count)
    {
        if( len >= 8192) 
        {
            if(NULL != buf)
            {
                free(buf);
                buf = NULL;
            }
        }
    }
    else
    {
        if(NULL != buf)
        {
            free(buf);
            buf = NULL;
        }
    }

    g_string_free(client_addr, TRUE);
    g_string_free(proxy_addr, TRUE);
    g_string_free(server_addr, TRUE);
   
    g_string_free(req_type_str, TRUE);
    g_string_free(json_data, TRUE);
//    g_string_free(new_format_cmd, TRUE);

    g_string_free(recv_client_time_str, TRUE);
    g_string_free(send_server_time_str, TRUE);
    g_string_free(recv_server_time_str, TRUE);
    g_string_free(send_client_time_str, TRUE);
}

void
req_put(struct msg *msg)
{
    struct msg *pmsg; /* peer message (response) */

    ASSERT(msg->request);

    req_log(msg);

    pmsg = msg->peer;
    if (pmsg != NULL) {
        ASSERT(!pmsg->request && pmsg->peer == msg);
        msg->peer = NULL;
        pmsg->peer = NULL;
        rsp_put(pmsg);
    }

    msg_tmo_delete(msg);

    msg_put(msg);
}



/*
 * Return true if request is done, false otherwise
 *
 * A request is done, if we received response for the given request.
 * A request vector is done if we received responses for all its
 * fragments.
 */
bool
req_done(struct conn *conn, struct msg *msg)
{
    struct msg *cmsg, *pmsg; /* current and previous message */
    uint64_t id;             /* fragment id */
    uint32_t nfragment;      /* # fragment */

    ASSERT(conn->client && !conn->proxy);
    ASSERT(msg->request);

    if (!msg->done) {
        return false;
    }

    id = msg->frag_id;
    if (id == 0) {
        return true;
    }

    if (msg->fdone) {
        /* request has already been marked as done */

//        struct keypos *kpos;
//        kpos = array_get(req->keys, 0);
//        if (kpos->end != NULL) {
//            *(kpos->end) = '\0';
//        }
//
//        GString *data = g_string_new(NULL);
//        
        return true;
    }

    if (msg->nfrag_done < msg->nfrag) {
        return false;
    }

    /* check all fragments of the given request vector are done */

    for (pmsg = msg, cmsg = TAILQ_PREV(msg, msg_tqh, c_tqe);
         cmsg != NULL && cmsg->frag_id == id;
         pmsg = cmsg, cmsg = TAILQ_PREV(cmsg, msg_tqh, c_tqe)) {

        if (!cmsg->done) {
            return false;
        }
    }

    for (pmsg = msg, cmsg = TAILQ_NEXT(msg, c_tqe);
         cmsg != NULL && cmsg->frag_id == id;
         pmsg = cmsg, cmsg = TAILQ_NEXT(cmsg, c_tqe)) {

        if (!cmsg->done) {
            return false;
        }
    }

    /*
     * At this point, all the fragments including the last fragment have
     * been received.
     *
     * Mark all fragments of the given request vector to be done to speed up
     * future req_done calls for any of fragments of this request
     */

    msg->fdone = 1;
    nfragment = 0;

    for (pmsg = msg, cmsg = TAILQ_PREV(msg, msg_tqh, c_tqe);
         cmsg != NULL && cmsg->frag_id == id;
         pmsg = cmsg, cmsg = TAILQ_PREV(cmsg, msg_tqh, c_tqe)) {
        cmsg->fdone = 1;
        nfragment++;
    }

    for (pmsg = msg, cmsg = TAILQ_NEXT(msg, c_tqe);
         cmsg != NULL && cmsg->frag_id == id;
         pmsg = cmsg, cmsg = TAILQ_NEXT(cmsg, c_tqe)) {
        cmsg->fdone = 1;
        nfragment++;
    }

    ASSERT(msg->frag_owner->nfrag == nfragment);

    msg->post_coalesce(msg->frag_owner);

    log_debug(LOG_DEBUG, "req from c %d with fid %"PRIu64" and %"PRIu32" "
              "fragments is done", conn->sd, id, nfragment);

    return true;
}


/*
 * Return true if request is in error, false otherwise
 *
 * A request is in error, if there was an error in receiving response for the
 * given request. A multiget request is in error if there was an error in
 * receiving response for any its fragments.
 */
bool
req_error(struct conn *conn, struct msg *msg)
{
    struct msg *cmsg; /* current message */
    uint64_t id;
    uint32_t nfragment;

    ASSERT(msg->request && req_done(conn, msg));

    if (msg->error) {
        return true;
    }

    id = msg->frag_id;
    if (id == 0) {
        return false;
    }

    if (msg->ferror) {
        /* request has already been marked to be in error */
        return true;
    }

    /* check if any of the fragments of the given request are in error */

    for (cmsg = TAILQ_PREV(msg, msg_tqh, c_tqe);
         cmsg != NULL && cmsg->frag_id == id;
         cmsg = TAILQ_PREV(cmsg, msg_tqh, c_tqe)) {

        if (cmsg->error) {
            goto ferror;
        }
    }

    for (cmsg = TAILQ_NEXT(msg, c_tqe);
         cmsg != NULL && cmsg->frag_id == id;
         cmsg = TAILQ_NEXT(cmsg, c_tqe)) {

        if (cmsg->error) {
            goto ferror;
        }
    }

    return false;

ferror:

    /*
     * Mark all fragments of the given request to be in error to speed up
     * future req_error calls for any of fragments of this request
     */

    msg->ferror = 1;
    nfragment = 1;

    for (cmsg = TAILQ_PREV(msg, msg_tqh, c_tqe);
         cmsg != NULL && cmsg->frag_id == id;
         cmsg = TAILQ_PREV(cmsg, msg_tqh, c_tqe)) {
        cmsg->ferror = 1;
        nfragment++;
    }

    for (cmsg = TAILQ_NEXT(msg, c_tqe);
         cmsg != NULL && cmsg->frag_id == id;
         cmsg = TAILQ_NEXT(cmsg, c_tqe)) {
        cmsg->ferror = 1;
        nfragment++;
    }

    log_debug(LOG_DEBUG, "req from c %d with fid %"PRIu64" and %"PRIu32" "
              "fragments is in error", conn->sd, id, nfragment);

    return true;
}

void
req_server_enqueue_imsgq(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(msg->request);
    ASSERT(!conn->client && !conn->proxy);

    /*
     * timeout clock starts ticking the instant the message is enqueued into
     * the server in_q; the clock continues to tick until it either expires
     * or the message is dequeued from the server out_q
     *
     * noreply request are free from timeouts because client is not intrested
     * in the response anyway!
     */
    if (!msg->noreply) {
        msg_tmo_insert(msg, conn);
    }

    TAILQ_INSERT_TAIL(&conn->imsg_q, msg, s_tqe);

    stats_server_incr(ctx, conn->owner, in_queue);
    stats_server_incr_by(ctx, conn->owner, in_queue_bytes, msg->mlen);
}

void
req_server_enqueue_imsgq_head(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(msg->request);
    ASSERT(!conn->client && !conn->proxy);

    /*
     * timeout clock starts ticking the instant the message is enqueued into
     * the server in_q; the clock continues to tick until it either expires
     * or the message is dequeued from the server out_q
     *
     * noreply request are free from timeouts because client is not intrested
     * in the response anyway!
     */
    if (!msg->noreply) {
        msg_tmo_insert(msg, conn);
    }

    TAILQ_INSERT_HEAD(&conn->imsg_q, msg, s_tqe);

    stats_server_incr(ctx, conn->owner, in_queue);
    stats_server_incr_by(ctx, conn->owner, in_queue_bytes, msg->mlen);
}

void
req_server_dequeue_imsgq(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(msg->request);
    ASSERT(!conn->client && !conn->proxy);

    TAILQ_REMOVE(&conn->imsg_q, msg, s_tqe);

    stats_server_decr(ctx, conn->owner, in_queue);
    stats_server_decr_by(ctx, conn->owner, in_queue_bytes, msg->mlen);
}

void
req_client_enqueue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(msg->request);
    ASSERT(conn->client && !conn->proxy);

    TAILQ_INSERT_TAIL(&conn->omsg_q, msg, c_tqe);
}

void
req_server_enqueue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(msg->request);
    ASSERT(!conn->client && !conn->proxy);

    TAILQ_INSERT_TAIL(&conn->omsg_q, msg, s_tqe);

    stats_server_incr(ctx, conn->owner, out_queue);
    stats_server_incr_by(ctx, conn->owner, out_queue_bytes, msg->mlen);
}

void
req_client_dequeue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(msg->request);
    ASSERT(conn->client && !conn->proxy);

    TAILQ_REMOVE(&conn->omsg_q, msg, c_tqe);
}

void
req_server_dequeue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(msg->request);
    ASSERT(!conn->client && !conn->proxy);

    msg_tmo_delete(msg);

    TAILQ_REMOVE(&conn->omsg_q, msg, s_tqe);

    stats_server_decr(ctx, conn->owner, out_queue);
    stats_server_decr_by(ctx, conn->owner, out_queue_bytes, msg->mlen);
}

struct msg *
req_recv_next(struct context *ctx, struct conn *conn, bool alloc)
{
    struct msg *msg;

    ASSERT(conn->client && !conn->proxy);

    if (conn->eof) {
        msg = conn->rmsg;

        /* client sent eof before sending the entire request */
        if (msg != NULL) {
            conn->rmsg = NULL;

            ASSERT(msg->peer == NULL);
            ASSERT(msg->request && !msg->done);

            log_error("eof c %d discarding incomplete req %"PRIu64" len "
                      "%"PRIu32"", conn->sd, msg->id, msg->mlen);

            req_put(msg);
        }

        /*
         * TCP half-close enables the client to terminate its half of the
         * connection (i.e. the client no longer sends data), but it still
         * is able to receive data from the proxy. The proxy closes its
         * half (by sending the second FIN) when the client has no
         * outstanding requests
         */
        if (!conn->active(conn)) {
            conn->done = 1;
            log_debug(LOG_INFO, "c %d is done", conn->sd);
        }
        return NULL;
    }

    msg = conn->rmsg;
    if (msg != NULL) {
        ASSERT(msg->request);
        return msg;
    }

    if (!alloc) {
        return NULL;
    }

    msg = req_get(conn);
    if (msg != NULL) {
        conn->rmsg = msg;
    }

    return msg;
}

static rstatus_t
req_make_reply(struct context *ctx, struct conn *conn, struct msg *req)
{
    struct msg *rsp;

    rsp = msg_get(conn, false, conn->redis); /* replay */
    if (rsp == NULL) {
        conn->err = errno;
        return NC_ENOMEM;
    }

    req->peer = rsp;
    rsp->peer = req;
    rsp->request = 0;

    req->done = 1;
    conn->enqueue_outq(ctx, conn, req);

    return NC_OK;
}

static bool
req_filter(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(conn->client && !conn->proxy);

    if (msg_empty(msg)) {
        ASSERT(conn->rmsg == NULL);
        log_debug(LOG_VERB, "filter empty req %"PRIu64" from c %d", msg->id,
                  conn->sd);
        req_put(msg);
        return true;
    }

    /*
     * Handle "quit\r\n" (memcache) or "*1\r\n$4\r\nquit\r\n" (redis), which
     * is the protocol way of doing a passive close. The connection is closed
     * as soon as all pending replies have been written to the client.
     */
    if (msg->quit) {
        log_debug(LOG_INFO, "filter quit req %"PRIu64" from c %d", msg->id,
                  conn->sd);
        if (conn->rmsg != NULL) {
            log_debug(LOG_INFO, "discard invalid req %"PRIu64" len %"PRIu32" "
                      "from c %d sent after quit req", conn->rmsg->id,
                      conn->rmsg->mlen, conn->sd);
        }
        conn->eof = 1;
        conn->recv_ready = 0;
        req_put(msg);
        return true;
    }

    /*
     * If this conn is not authenticated, we will mark it as noforward,
     * and handle it in the redis_reply handler.
     */
    if (!conn_authenticated(conn)) {
        msg->noforward = 1;
    }

    return false;
}

static void
req_forward_error(struct context *ctx, struct conn *conn, struct msg *msg)
{
    rstatus_t status;

    ASSERT(conn->client && !conn->proxy);

    log_debug(LOG_INFO, "forward req %"PRIu64" len %"PRIu32" type %d from "
              "c %d failed: %s", msg->id, msg->mlen, msg->type, conn->sd,
              strerror(errno));

    msg->done = 1;
    msg->error = 1;
    msg->err = errno;

    /* noreply request don't expect any response */
    if (msg->noreply) {
        req_put(msg);
        return;
    }

    if (req_done(conn, TAILQ_FIRST(&conn->omsg_q))) {
        status = event_add_out(ctx->evb, conn);
        if (status != NC_OK) {
            conn->err = errno;
        }
    }
}

static void
req_forward_stats(struct context *ctx, struct server *server, struct msg *msg)
{
    ASSERT(msg->request);

    stats_server_incr(ctx, server, requests);
    stats_server_incr_by(ctx, server, request_bytes, msg->mlen);
}

static void
req_forward(struct context *ctx, struct conn *c_conn, struct msg *msg)
{
    rstatus_t status;
    struct conn *s_conn;
    struct server_pool *pool;
    uint8_t *key;
    uint32_t keylen;
    struct keypos *kpos;

    ASSERT(c_conn->client && !c_conn->proxy);

    /* enqueue message (request) into client outq, if response is expected */
    if (!msg->noreply) {
        c_conn->enqueue_outq(ctx, c_conn, msg);
    }

    pool = c_conn->owner;

    ASSERT(array_n(msg->keys) > 0);
    kpos = array_get(msg->keys, 0);
    key = kpos->start;
    keylen = (uint32_t)(kpos->end - kpos->start);

    s_conn = server_pool_conn(ctx, c_conn->owner, key, keylen);
    if (s_conn == NULL) {
        req_forward_error(ctx, c_conn, msg);
        return;
    }
    ASSERT(!s_conn->client && !s_conn->proxy);

    /* enqueue the message (request) into server inq */
    if (TAILQ_EMPTY(&s_conn->imsg_q)) {
        log_debug(LOG_VERB, "Ohhhhhhhhhhh 触发server conn的写操作");
        status = event_add_out(ctx->evb, s_conn);
        if (status != NC_OK) {
            req_forward_error(ctx, c_conn, msg);
            s_conn->err = errno;
            return;
        }
    }

    if (!conn_authenticated(s_conn)) {
        status = msg->add_auth(ctx, c_conn, s_conn);
        if (status != NC_OK) {
            req_forward_error(ctx, c_conn, msg);
            s_conn->err = errno;
            return;
        }
    }

    s_conn->enqueue_inq(ctx, s_conn, msg);

    req_forward_stats(ctx, s_conn->owner, msg);

    log_debug(LOG_VERB, "forward from c %d to s %d req %"PRIu64" len %"PRIu32
              " type %d with key '%.*s'", c_conn->sd, s_conn->sd, msg->id,
              msg->mlen, msg->type, keylen, key);
}

int writeCmd(int cmd)
{
    if( cmd == MSG_REQ_REDIS_DEL ||
        cmd == MSG_REQ_REDIS_EXPIRE ||
        cmd == MSG_REQ_REDIS_EXPIREAT ||
        cmd == MSG_REQ_REDIS_DECR ||
        cmd == MSG_REQ_REDIS_DECRBY ||
        cmd == MSG_REQ_REDIS_INCR ||
        cmd == MSG_REQ_REDIS_INCRBY ||
        cmd == MSG_REQ_REDIS_INCRBYFLOAT ||
        cmd == MSG_REQ_REDIS_PERSIST ||
        cmd == MSG_REQ_REDIS_PEXPIRE ||
        cmd == MSG_REQ_REDIS_GETSET ||
        cmd == MSG_REQ_REDIS_PSETEX ||
        cmd == MSG_REQ_REDIS_APPEND ||
        cmd == MSG_REQ_REDIS_PFADD ||
        cmd == MSG_REQ_REDIS_PEXPIREAT ||

        cmd == MSG_REQ_REDIS_MSET ||
        cmd == MSG_REQ_REDIS_SET ||
        cmd == MSG_REQ_REDIS_SETBIT ||
        cmd == MSG_REQ_REDIS_SETEX ||
        cmd == MSG_REQ_REDIS_SETNX ||
        cmd == MSG_REQ_REDIS_SETRANGE ||

        cmd == MSG_REQ_REDIS_HDEL ||
        cmd == MSG_REQ_REDIS_HINCRBY ||
        cmd == MSG_REQ_REDIS_HINCRBYFLOAT ||

        cmd == MSG_REQ_REDIS_HMSET ||
        cmd == MSG_REQ_REDIS_HSET ||

        cmd == MSG_REQ_REDIS_LPOP ||
        cmd == MSG_REQ_REDIS_LPUSH ||
        cmd == MSG_REQ_REDIS_LPUSHX ||
        cmd == MSG_REQ_REDIS_LREM ||
        cmd == MSG_REQ_REDIS_LSET ||

        cmd == MSG_REQ_REDIS_LINSERT ||
        cmd == MSG_REQ_REDIS_LTRIM ||
        //cmd == MSG_REQ_REDIS_RESTORE ||
        cmd == MSG_REQ_REDIS_HSETNX  ||

        cmd == MSG_REQ_REDIS_RPOP ||
        cmd == MSG_REQ_REDIS_RPOPLPUSH ||
        cmd == MSG_REQ_REDIS_RPUSH ||
        cmd == MSG_REQ_REDIS_RPUSHX ||

        cmd == MSG_REQ_REDIS_SADD ||
        cmd == MSG_REQ_REDIS_SREM ||

        cmd == MSG_REQ_REDIS_ZADD ||
        cmd == MSG_REQ_REDIS_ZINCRBY ||
        cmd == MSG_REQ_REDIS_ZREM ||
        cmd == MSG_REQ_REDIS_ZREMRANGEBYRANK ||
        cmd == MSG_REQ_REDIS_ZREMRANGEBYLEX ||
        cmd == MSG_REQ_REDIS_ZREMRANGEBYSCORE ||
        cmd == MSG_REQ_REDIS_EVAL )
    {
        return 0;        
    }

    return 1;
}

void
req_recv_done(struct context *ctx, struct conn *conn, struct msg *msg,
              struct msg *nmsg)
{

    rstatus_t status;
    struct server_pool *pool;
    struct msg_tqh frag_msgq;
    struct msg *sub_msg;
    struct msg *tmsg; 			/* tmp next message */

    ASSERT(conn->client && !conn->proxy);
    ASSERT(msg->request);
    ASSERT(msg->owner == conn);
    ASSERT(conn->rmsg == msg);
    ASSERT(nmsg == NULL || nmsg->request);

    /* enqueue next message (request), if any */
    conn->rmsg = nmsg;

    if (req_filter(ctx, conn, msg)) {
        return;
    }

    if (msg->noforward) {

        status = req_make_reply(ctx, conn, msg);
        if (status != NC_OK) {
            conn->err = errno;
            return;
        }
        
        status = msg->reply(msg);
        if (status != NC_OK) {
            conn->err = errno;
            return;
        }

        status = event_add_out(ctx->evb, conn);
        if (status != NC_OK) {
            conn->err = errno;
        }

        return;
    }

    /* do fragment */
    pool = conn->owner;
    TAILQ_INIT(&frag_msgq);
    status = msg->fragment(msg, pool->ncontinuum, &frag_msgq);
    if (status != NC_OK) {
        if (!msg->noreply) {
            conn->enqueue_outq(ctx, conn, msg);
        }
        req_forward_error(ctx, conn, msg);
    }
   


    /* if no fragment happened */
    if (TAILQ_EMPTY(&frag_msgq)) {
        req_forward(ctx, conn, msg);
        return;
    }

    status = req_make_reply(ctx, conn, msg);
    if (status != NC_OK) {
        if (!msg->noreply) {
            conn->enqueue_outq(ctx, conn, msg);
        }
        req_forward_error(ctx, conn, msg);
    }

    for (sub_msg = TAILQ_FIRST(&frag_msgq); sub_msg != NULL; sub_msg = tmsg) {
        tmsg = TAILQ_NEXT(sub_msg, m_tqe);

        TAILQ_REMOVE(&frag_msgq, sub_msg, m_tqe);
        req_forward(ctx, conn, sub_msg);
    }

    ASSERT(TAILQ_EMPTY(&frag_msgq));
    return;
}

/*
zhanging
该方法是req_forward触发了server conn中的写操作
在server conn发送数据时, 其实是从client conn中取一个可用的msg
*/
struct msg *
req_send_next(struct context *ctx, struct conn *conn)
{
    rstatus_t status;
    struct msg *msg, *nmsg; /* current and next message */

    ASSERT(!conn->client && !conn->proxy);

    if (conn->connecting) {
        server_connected(ctx, conn);
    }

    nmsg = TAILQ_FIRST(&conn->imsg_q);
    if (nmsg == NULL) {
        /* nothing to send as the server inq is empty */
        status = event_del_out(ctx->evb, conn);
        if (status != NC_OK) {
            conn->err = errno;
        }

        return NULL;
    }

    msg = conn->smsg;
    if (msg != NULL) {
        ASSERT(msg->request && !msg->done);
        nmsg = TAILQ_NEXT(msg, s_tqe);
    }

    conn->smsg = nmsg;

    if (nmsg == NULL) {
        return NULL;
    }

    ASSERT(nmsg->request && !nmsg->done);

    log_debug(LOG_VVERB, "send next req %"PRIu64" len %"PRIu32" type %d on "
              "s %d", nmsg->id, nmsg->mlen, nmsg->type, conn->sd);

    return nmsg;
}

void
req_send_done(struct context *ctx, struct conn *conn, struct msg *msg)
{
    ASSERT(!conn->client && !conn->proxy);
    ASSERT(msg != NULL && conn->smsg == NULL);
    ASSERT(msg->request && !msg->done);
    ASSERT(msg->owner != conn);
    
    //zhangming
    struct server * server  = (struct server *)conn->owner;
    log_debug(LOG_VVERB, "send done req %"PRIu64" len %"PRIu32" type %d on "
              "s %d, '%.*s'", msg->id, msg->mlen, msg->type, conn->sd, server->pname.len, server->pname.data);

    /* dequeue the message (request) from server inq */
    conn->dequeue_inq(ctx, conn, msg);

    /*
     * noreply request instructs the server not to send any response. So,
     * enqueue message (request) in server outq, if response is expected.
     * Otherwise, free the noreply request
     */
    if (!msg->noreply) {
        conn->enqueue_outq(ctx, conn, msg);
    } else {
        req_put(msg);
    }
}