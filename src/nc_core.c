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

#include <stdlib.h>
#include <unistd.h>
#include <nc_core.h>
#include <nc_conf.h>
#include <nc_server.h>
#include <nc_proxy.h>
#include <wait.h>

#include "etcd/etcd_conf.h"
#include "spdlog.h"
#include "trace_conf.h"
#include <glib.h>

//pid_t *core_worker_list = NULL;
struct worker_info* core_worker_list = NULL;

int core_worker_num = 1;
int core_is_master = 0;

static uint32_t ctx_id; /* context generation */

static int core_master_loop(struct instance *nci, struct context *ctx);

static rstatus_t
core_calc_connections(struct context *ctx)
{
    int status;
    struct rlimit limit;

    status = getrlimit(RLIMIT_NOFILE, &limit);
    if (status < 0) {
        log_error("getrlimit failed: %s", strerror(errno));
        return NC_ERROR;
    }

    ctx->max_nfd = (uint32_t)limit.rlim_cur;
    ctx->max_ncconn = ctx->max_nfd - ctx->max_nsconn - RESERVED_FDS;
    log_debug(LOG_NOTICE, "max fds %"PRIu32" max client conns %"PRIu32" "
              "max server conns %"PRIu32"", ctx->max_nfd, ctx->max_ncconn,
              ctx->max_nsconn);

    return NC_OK;
}

static struct context *
core_ctx_create(struct instance *nci)
{
    rstatus_t status;
    struct context *ctx;

    ctx = nc_alloc(sizeof(*ctx));
    if (ctx == NULL) {
        return NULL;
    }
    ctx->id = ++ctx_id;
    ctx->cf = NULL;
    ctx->stats = NULL;
    ctx->evb = NULL;
    array_null(&ctx->pool);
    ctx->max_timeout = nci->stats_interval;
    ctx->timeout = ctx->max_timeout;
    ctx->max_nfd = 0;
    ctx->max_ncconn = 0;
    ctx->max_nsconn = 0;

    /* parse and create configuration */
    ctx->cf = conf_create(nci->conf_filename);
    if (ctx->cf == NULL) {
        nc_free(ctx);
        return NULL;
    }

    /* initialize server pool from configuration */
    status = server_pool_init(&ctx->pool, &ctx->cf->pool, ctx);
    if (status != NC_OK) {
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

#ifdef SO_REUSEPORT
	if (core_worker_num > 1) {
		core_master_loop(nci, ctx);
	}
#endif

    /*
     * Get rlimit and calculate max client connections after we have
     * calculated max server connections
     */
    status = core_calc_connections(ctx);
    if (status != NC_OK) {
        server_pool_deinit(&ctx->pool);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* create stats per server pool */
    ctx->stats = stats_create(nci->stats_port, nci->stats_addr, nci->stats_interval,
                              nci->hostname, &ctx->pool);
    if (ctx->stats == NULL) {
        server_pool_deinit(&ctx->pool);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* initialize event handling for client, proxy and server */
    ctx->evb = event_base_create(EVENT_SIZE, &core_core);
    if (ctx->evb == NULL) {
        stats_destroy(ctx->stats);
        server_pool_deinit(&ctx->pool);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* preconnect? servers in server pool */
    status = server_pool_preconnect(ctx);
    if (status != NC_OK) {
        server_pool_disconnect(ctx);
        event_base_destroy(ctx->evb);
        stats_destroy(ctx->stats);
        server_pool_deinit(&ctx->pool);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* initialize proxy per server pool */
    status = proxy_init(ctx);
    if (status != NC_OK) {
        server_pool_disconnect(ctx);
        event_base_destroy(ctx->evb);
        stats_destroy(ctx->stats);
        server_pool_deinit(&ctx->pool);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    log_debug(LOG_VVERB, "created ctx %p id %"PRIu32"", ctx, ctx->id);

    return ctx;
}

static void
core_ctx_destroy(struct context *ctx)
{
    log_debug(LOG_VVERB, "destroy ctx %p id %"PRIu32"", ctx, ctx->id);
    proxy_deinit(ctx);
    server_pool_disconnect(ctx);
    event_base_destroy(ctx->evb);
    stats_destroy(ctx->stats);
    server_pool_deinit(&ctx->pool);
    conf_destroy(ctx->cf);
    nc_free(ctx);
}

struct context *
core_start(struct instance *nci)
{
    struct context *ctx;

    mbuf_init(nci);
    msg_init();
    conn_init();

    ctx = core_ctx_create(nci);
    if (ctx != NULL) {
        nci->ctx = ctx;
        return ctx;
    }

    conn_deinit();
    msg_deinit();
    mbuf_deinit();

    return NULL;
}

static rstatus_t core_worker_each(void *elem, void *data) {
	rstatus_t status;
	struct server_pool *pool = elem;
	struct conn *p;

	p = conn_get_proxy(pool);
	if (p == NULL) {
		return NC_ENOMEM;
	}

	ASSERT(p->proxy);

	int sock = socket(p->family, SOCK_STREAM, 0);
	if (sock < 0) {
		log_error("socket failed: %s", strerror(errno));
		return NC_ERROR;
	}

	status = nc_set_reuseaddr(sock);
	if (status < 0) {
		log_error("reuse of port '%.*s' for listening on p %d failed: %s",
				  pool->addrstr.len, pool->addrstr.data, p->sd,
				  strerror(errno));
		return NC_ERROR;
	}

	status = bind(sock, p->addr, p->addrlen);
	if (status < 0) {
		log_error("bind on p %d to addr '%.*s' failed: %s", p->sd,
				  pool->addrstr.len, pool->addrstr.data, strerror(errno));
		return NC_ERROR;
	}

	return NC_OK;
}

struct conf_trace_data *trace_conf = NULL;

static int core_master_loop(struct instance *nci, struct context *ctx) {
	int i;
	pid_t pid;
	rstatus_t status;
    
    ctx->environment_type = nci->environment_type;
    //printf("==== ctx->environment_type %d\n", ctx->environment_type);

	//core_worker_list = calloc(core_worker_num, sizeof(pid_t));
    core_worker_list = calloc(core_worker_num+1, sizeof(struct worker_info));
	if (!core_worker_list) {
		log_error("calloc() failed: %s", strerror(errno));
		exit(1);
	}

	status = array_each(&ctx->pool, core_worker_each, NULL);
	if (status != NC_OK) {
		return NC_ERROR;
		return status;
	}
    
	for (i = 0; i < core_worker_num; i++) {
		pid = fork();
		if (pid > 0) {
            core_is_master = 1;
            core_worker_list[i].pid = pid;
            core_worker_list[i].type = 1;
            continue;
		} else if (pid == 0) {
            core_is_master = 0;
            nci->stats_port++;
            break;
		} else {
			server_pool_deinit(&ctx->pool);
			conf_destroy(ctx->cf);
			nc_free(ctx);
			log_error("fork() failed: %s", strerror(errno));
			exit(1);
		}
	}
    
    int res;
	int child_status;
	pid_t new_pid;
	if (core_is_master) {
        char* etcd_conf_file = nci->etcd_conf_filename;
        if (NULL != etcd_conf_file)
        {
            struct conf_data* conf = parseConf(etcd_conf_file);
            res = set_timmer(conf);
            if (0 != res)
            {
                log_error("set_timmer failed");
                return NC_ERROR;
            }
        }

		while (1) {
            pid = wait(&child_status);
            if (pid > 0) {
				new_pid = fork();
                log_warn("child is killed, after fork, new_pid %d, pid %d",new_pid, pid);
                if (new_pid == 0) {
					break;
				} else if (new_pid > 0) {
                    for (i = 0; i < core_worker_num + 1; i++) {
                        if (core_worker_list[i].pid == pid) {
                            core_worker_list[i].pid = new_pid;
                            break;
                        }
                    }
                    continue;
                } else {
                    log_error("fork() failed: %s", strerror(errno));
                }
			}
		}
	} else {
        
        if (NULL != nci->trace_conf_filename)
        {
            log_warn("trace_conf_filename is %s", nci->trace_conf_filename);
            trace_conf = parse_trace_conf(nci->trace_conf_filename);
            if(NULL  == trace_conf) {
                log_error("trace_conf struct is null");
                return NC_ERROR;
            }
            
            GString *new_trace_log = g_string_new_len(trace_conf->log_filename, (gssize)strlen(trace_conf->log_filename)-4);
            g_string_append_printf(new_trace_log, "-%d.log", getpid());

            init_filehand(new_trace_log->str);
            g_string_free(new_trace_log, TRUE);
        }

        if (0 == ctx->environment_type)
        {
            free_trace_conf(trace_conf);
            return NC_OK;
        }
    }
	return NC_OK;
}

void
core_stop(struct context *ctx)
{
    conn_deinit();
    msg_deinit();
    mbuf_deinit();
    core_ctx_destroy(ctx);
}

static rstatus_t
core_recv(struct context *ctx, struct conn *conn)
{
    rstatus_t status;

    status = conn->recv(ctx, conn);
    if (status != NC_OK) {
        log_debug(LOG_INFO, "recv on %c %d failed: %s",
                  conn->client ? 'c' : (conn->proxy ? 'p' : 's'), conn->sd,
                  strerror(errno));
    }

    return status;
}

static rstatus_t
core_send(struct context *ctx, struct conn *conn)
{
    rstatus_t status;

    status = conn->send(ctx, conn);
    if (status != NC_OK) {
        log_debug(LOG_INFO, "send on %c %d failed: status: %d errno: %d %s",
                  conn->client ? 'c' : (conn->proxy ? 'p' : 's'), conn->sd,
                  status, errno, strerror(errno));
    }

    return status;
}

static void
core_close(struct context *ctx, struct conn *conn)
{
    rstatus_t status;
    char type, *addrstr;

    ASSERT(conn->sd > 0);

    if (conn->client) {
        type = 'c';
        addrstr = nc_unresolve_peer_desc(conn->sd);
    } else {
        type = conn->proxy ? 'p' : 's';
        addrstr = nc_unresolve_addr(conn->addr, conn->addrlen);
    }
    log_debug(LOG_NOTICE, "close %c %d '%s' on event %04"PRIX32" eof %d done "
              "%d rb %zu sb %zu%c %s", type, conn->sd, addrstr, conn->events,
              conn->eof, conn->done, conn->recv_bytes, conn->send_bytes,
              conn->err ? ':' : ' ', conn->err ? strerror(conn->err) : "");

    status = event_del_conn(ctx->evb, conn);
    if (status < 0) {
        log_warn("event del conn %c %d failed, ignored: %s",
                 type, conn->sd, strerror(errno));
    }

    conn->close(ctx, conn);
}

static void
core_error(struct context *ctx, struct conn *conn)
{
    rstatus_t status;
    char type = conn->client ? 'c' : (conn->proxy ? 'p' : 's');

    status = nc_get_soerror(conn->sd);
    if (status < 0) {
        log_warn("get soerr on %c %d failed, ignored: %s", type, conn->sd,
                  strerror(errno));
    }
    conn->err = errno;

    core_close(ctx, conn);
}

static void
core_timeout(struct context *ctx)
{
    for (;;) {
        struct msg *msg;
        struct conn *conn;
        int64_t now, then;

        msg = msg_tmo_min();
        if (msg == NULL) {
            ctx->timeout = ctx->max_timeout;
            return;
        }

        /* skip over req that are in-error or done */

        if (msg->error || msg->done) {
            msg_tmo_delete(msg);
            continue;
        }

        /*
         * timeout expired req and all the outstanding req on the timing
         * out server
         */

        conn = msg->tmo_rbe.data;
        then = msg->tmo_rbe.key;

        now = nc_msec_now();
        if (now < then) {
            int delta = (int)(then - now);
            ctx->timeout = MIN(delta, ctx->max_timeout);
            return;
        }

        log_debug(LOG_INFO, "req %"PRIu64" on s %d timedout", msg->id, conn->sd);

        msg_tmo_delete(msg);
        conn->err = ETIMEDOUT;

        core_close(ctx, conn);
    }
}

rstatus_t
core_core(void *arg, uint32_t events)
{
    rstatus_t status;
    struct conn *conn = arg;
    struct context *ctx;

    if (conn->owner == NULL) {
        log_warn("conn is already unrefed!");
        return NC_OK;
    }

    ctx = conn_to_ctx(conn);

    log_debug(LOG_VVERB, "event %04"PRIX32" on %c %d", events,
              conn->client ? 'c' : (conn->proxy ? 'p' : 's'), conn->sd);

    conn->events = events;

    /* error takes precedence over read | write */
    if (events & EVENT_ERR) {
        core_error(ctx, conn);
        return NC_ERROR;
    }

    /* read takes precedence over write */
    if (events & EVENT_READ) {
        status = core_recv(ctx, conn);
        if (status != NC_OK || conn->done || conn->err) {
            core_close(ctx, conn);
            return NC_ERROR;
        }
    }

    if (events & EVENT_WRITE) {
        status = core_send(ctx, conn);
        if (status != NC_OK || conn->done || conn->err) {
            core_close(ctx, conn);
            return NC_ERROR;
        }
    }

    return NC_OK;
}

rstatus_t
core_loop(struct context *ctx)
{
    int nsd;

    nsd = event_wait(ctx->evb, ctx->timeout);
    if (nsd < 0) {
        return nsd;
    }

    core_timeout(ctx);

    stats_swap(ctx->stats);

    return NC_OK;
}
