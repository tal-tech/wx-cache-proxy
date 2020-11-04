#include "cetcd.h"
//#include "cetcd_json_parser.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <curl/curl.h>
//#include <yajl/yajl_parse.h>
#include <sys/select.h>
#include <pthread.h>
#include "log.h"


typedef struct cetcd_request_t {
    enum HTTP_METHOD method;
    //enum ETCD_API_TYPE api_type;
    cetcd_string uri;
    cetcd_string url;
    cetcd_string data;
    cetcd_client *cli;
} cetcd_request;

static const char *http_method[] = {
    "GET",
    "POST",
    "PUT",
    "DELETE",
    "HEAD",
    "OPTION"
};

typedef struct cetcd_response_parser_t {
   int st;
    int http_status;
    //enum ETCD_API_TYPE api_type;
    cetcd_string buf;
    void *resp;
    //yajl_parser_context ctx;
    //yajl_handle json;
}cetcd_response_parser;

void cetcd_client_init(cetcd_client *cli, cetcd_array *addresses);

cetcd_client *cetcd_client_create(cetcd_array *addresses);

void cetcd_addresses_release(cetcd_array *addrs);
void cetcd_client_release(cetcd_client *cli);
void cetcd_client_destroy(cetcd_client *cli);

size_t cetcd_parse_response(char *ptr, size_t size, size_t nmemb, void *userdata);

void *cetcd_send_request(CURL *curl, cetcd_request *req);
void *cetcd_cluster_request(cetcd_client *cli, cetcd_request *req);

/*
client初始化
*/
void cetcd_client_init(cetcd_client *cli, cetcd_array *addresses) {
    size_t i;
    cetcd_array *addrs;
    cetcd_string addr;
    curl_global_init(CURL_GLOBAL_ALL);
    srand(time(0));

    cli->keys_space =   "v3alpha/";
//    cli->stat_space =   "v2/stat";
//    cli->member_space = "v2/members";
    cli->curl = curl_easy_init();

    addrs = cetcd_array_create(cetcd_array_size(addresses));
    for (i=0; i<cetcd_array_size(addresses); ++i) {
        addr = cetcd_array_get(addresses, i);
        if ( strncmp(addr, "http", 4)) {
            cetcd_array_append(addrs, sdscatprintf(sdsempty(), "http://%s", addr));
        } else {
            cetcd_array_append(addrs, sdsnew(addr));
        }
    }

    cli->addresses = cetcd_array_shuffle(addrs);
    cli->picked = rand() % (cetcd_array_size(cli->addresses));

    cli->settings.verbose = 0;
    cli->settings.connect_timeout = 1;
    cli->settings.read_timeout = 1;  /*not used now*/
    cli->settings.write_timeout = 1; /*not used now*/
    cli->settings.user = NULL;
    cli->settings.password = NULL;

    cetcd_array_init(&cli->watchers, 10);

    /* Set CURLOPT_NOSIGNAL to 1 to work around the libcurl bug:
     *  http://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame
     *  http://curl.haxx.se/mail/lib-2008-09/0197.html
     * */
    curl_easy_setopt(cli->curl, CURLOPT_NOSIGNAL, 1L);

#if LIBCURL_VERSION_NUM >= 0x071900
    curl_easy_setopt(cli->curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(cli->curl, CURLOPT_TCP_KEEPINTVL, 1L); /*the same as go-etcd*/
#endif
    curl_easy_setopt(cli->curl, CURLOPT_USERAGENT, "cetcd");
    curl_easy_setopt(cli->curl, CURLOPT_POSTREDIR, 3L);     /*post after redirecting*/
    curl_easy_setopt(cli->curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
}

cetcd_client *cetcd_client_create(cetcd_array *addresses){
    cetcd_client *cli;

    cli = calloc(1, sizeof(cetcd_client));
    cetcd_client_init(cli, addresses);
    return cli;
}

void cetcd_client_destroy(cetcd_client *cli) {
    cetcd_addresses_release(cli->addresses);
    cetcd_array_release(cli->addresses);
    sdsfree(cli->settings.user);
    sdsfree(cli->settings.password);
    curl_easy_cleanup(cli->curl);
    curl_global_cleanup();
    cetcd_array_destroy(&cli->watchers);
}

void cetcd_client_release(cetcd_client *cli){
    if (cli) {
        cetcd_client_destroy(cli);
        free(cli);
    }
}

void cetcd_addresses_release(cetcd_array *addrs){
    int count, i;
    cetcd_string s;
    if (addrs) {
        count = cetcd_array_size(addrs);
        for (i = 0; i < count; ++i) {
            s = cetcd_array_get(addrs, i);
            sdsfree(s);
        }
    }
}



//int cetcd_curl_setopt(CURL *curl, cetcd_watcher *watcher) {
//    cetcd_string url;
//
//    url = cetcd_watcher_build_url(watcher->cli, watcher);
//    curl_easy_setopt(curl,CURLOPT_URL, url);
//    sdsfree(url);
//
//    /* See above about CURLOPT_NOSIGNAL
//     * */
//    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
//
//    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, watcher->cli->settings.connect_timeout);
//#if LIBCURL_VERSION_NUM >= 0x071900
//    curl_easy_setopt(watcher->curl, CURLOPT_TCP_KEEPALIVE, 1L);
//    curl_easy_setopt(watcher->curl, CURLOPT_TCP_KEEPINTVL, 1L); /*the same as go-etcd*/
//#endif
//    curl_easy_setopt(curl, CURLOPT_USERAGENT, "cetcd");
//    curl_easy_setopt(curl, CURLOPT_POSTREDIR, 3L);     /*post after redirecting*/
//    curl_easy_setopt(curl, CURLOPT_VERBOSE, watcher->cli->settings.verbose);
//
//    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
//    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cetcd_parse_response);
//    curl_easy_setopt(curl, CURLOPT_WRITEDATA, watcher->parser);
//    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
//    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
//
//    curl_easy_setopt(curl, CURLOPT_PRIVATE, watcher);
//    watcher->curl = curl;
//
//    return 1;
//}

int checkResp(cJSON * root)
{
    cJSON * item = NULL;
    char* errMsg = NULL;

    if (NULL == root )
    {
        printf("error:%s\n", cJSON_GetErrorPtr());
        xes_log("error:%s", cJSON_GetErrorPtr());
        cJSON_Delete(root);
        return 0;
    }

    item = cJSON_GetObjectItem(root, "error");
    if (item)
    {
        errMsg = item->valuestring;
        printf("error %s\n", errMsg); 
        xes_log("error %s", errMsg); 
        return 0;
    }

    return 1;
}

int getTTL(cJSON * root, uint64_t* ttl)
{
    if(! checkResp(root)) 
    {
        printf("check out resp json error\n");
        return 0;
    }

    cJSON * item = NULL;
    
    item = cJSON_GetObjectItem(root, "TTL");
    if (item)
    {
        *ttl = strtoull(item->valuestring, NULL, 0);
    }

    return 1;
}

int getToken(cJSON * root, char* token)
{
    if(! checkResp(root)) 
    {
        printf("check out resp json error\n");
        return 0;
    }

    cJSON * item = NULL;
    
    if (NULL == token)
    {
		xes_log("param error token is null");
        return 0;
    }

    item = cJSON_GetObjectItem(root, "token");
    if (item)
    {
        memcpy(token, item->valuestring, strlen(item->valuestring));
    }

    return 1;
}

int getLease(cJSON * root, uint64_t ttl, char* lease)
{
    cJSON * item = NULL;
    uint64_t ttlRsp = 0;
    
    if(! checkResp(root)) 
    {
        printf("check out resp json error\n");
        return 0;
    }

    if (ttl ==0 || NULL == lease)
    {
        printf("param error ttl %ld, lease %p\n", ttl, lease);
		xes_log("param error ttl %ld, lease %p", ttl, lease);
        return 0;
    }
    
    item = cJSON_GetObjectItem(root, "TTL");
    if (item)
    {
        ttlRsp = atoi(item->valuestring);
    }

    if(ttl != ttlRsp)
    {
        printf("error ttl %ld not equal %ld\n", ttl, ttlRsp); 
		xes_log("error ttl %ld not equal %ld", ttl, ttlRsp); 
        return 0;	
    }

    char* id = NULL;
    item = cJSON_GetObjectItem(root, "ID");
    if (item)
    {
        memcpy(lease, item->valuestring, strlen(item->valuestring));
    }

    return 1;
}

/*
https://www.cnblogs.com/catgatp/p/6379955.html
*/
int getTTLAfterKeepAlive(cJSON * root, uint64_t* ttl)
{
    cJSON * item = NULL;
    uint64_t ttlRsp = 0;
    
    if(! checkResp(root)) 
    {
        printf("check out resp json error\n");
        return 0;
    }

    item = cJSON_GetObjectItem(root, "result");
    if (item)
    {
        item = cJSON_GetObjectItem(item, "TTL");
        if (item)
        {
            *ttl = atoi(item->valuestring);
        } else {
            return 0;
        }
    }

    return 1;
}

int cetcd_set(cetcd_client *cli, const char *key, const char *value, char* ttl) {
    cetcd_request req;
    cetcd_response *resp;
    cetcd_string params;
    char *value_escaped;

	char* msg;
	cJSON * root =  cJSON_CreateObject();

    cJSON_AddItemToObject(root, "key", cJSON_CreateString(key));
    cJSON_AddItemToObject(root, "value", cJSON_CreateString(value));
    cJSON_AddItemToObject(root, "lease", cJSON_CreateString(ttl));

    msg = cJSON_PrintUnformatted(root);

    memset(&req, 0, sizeof(cetcd_request));
    req.method = ETCD_HTTP_POST;
    //req.api_type = ETCD_KEYS;
    req.uri = sdscatprintf(sdsempty(), "%s%s", cli->keys_space, "kv/put");
    value_escaped = curl_easy_escape(cli->curl, value, strlen(value));
	
    params = sdscatprintf(sdsempty(), "%s", msg);
    curl_free(value_escaped);

    req.data = params;
    resp = cetcd_cluster_request(cli, &req);
//	if(resp->err) {
//        printf("1111111111 error :%p, %d, %s (%s)\n", resp->err, resp->err->ecode, resp->err->message, resp->err->cause);
//    }
    sdsfree(req.uri);
    sdsfree(params);

	if (NULL != msg)
	{
		free(msg);
		msg = NULL;
	}
	cJSON_Delete(root);
    
    xes_log("cetcd_set 注册key结果 %s", resp->json_msg);

    root = cJSON_Parse(resp->json_msg); 
    if(! checkResp(root)) 
    {
        cJSON_Delete(root);
        cetcd_response_release(resp);
        return 0;
    }

    cJSON_Delete(root);
    cetcd_response_release(resp);

    return 1;
}

int cetcd_lease(cetcd_client *cli, uint64_t ttl, char* lease) {
    cetcd_request req;
    cetcd_response *resp;
    cetcd_string params;

    char* msg;
    cJSON * root =  cJSON_CreateObject();

    cJSON_AddItemToObject(root, "TTL", cJSON_CreateNumber(ttl));
    msg = cJSON_PrintUnformatted(root);

    memset(&req, 0, sizeof(cetcd_request));
    req.method = ETCD_HTTP_POST;
    //req.api_type = ETCD_KEYS;
    req.uri = sdscatprintf(sdsempty(), "%s%s", cli->keys_space, "lease/grant");

    params = sdscatprintf(sdsempty(), "%s", msg);
   
    req.data = params;
    resp = cetcd_cluster_request(cli, &req);
    sdsfree(req.uri);
    sdsfree(params);

	if (NULL != msg)
	{
		free(msg);
		msg = NULL;
	}
	cJSON_Delete(root);
    
    xes_log("cetcd_lease 获取lease结果 %s", resp->json_msg);
    
    root = cJSON_Parse(resp->json_msg); 
    if(NULL == getLease(root, ttl, lease))
    {  
        printf("cat not fetch lease\n");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_Delete(root);
    cetcd_response_release(resp);

    return 1;
}

uint64_t cetcd_timetolive(cetcd_client *cli, char* lease) {
    cetcd_request req;
    cetcd_response *resp;
    cetcd_string params;

    uint64_t ttl;
    char* msg;
    cJSON * root =  cJSON_CreateObject();

    cJSON_AddItemToObject(root, "ID", cJSON_CreateString(lease));
    msg = cJSON_PrintUnformatted(root);

    memset(&req, 0, sizeof(cetcd_request));
    req.method = ETCD_HTTP_POST;
    //req.api_type = ETCD_KEYS;
    req.uri = sdscatprintf(sdsempty(), "%s%s", cli->keys_space, "kv/lease/timetolive");

    params = sdscatprintf(sdsempty(), "%s", msg);

    req.data = params;
    resp = cetcd_cluster_request(cli, &req);
    sdsfree(req.uri);
    sdsfree(params);

    if(NULL != msg)
    {
        free(msg);
        msg = NULL;
    }
    cJSON_Delete(root);

    xes_log("cetcd_timetolive 保活结果 %s", resp->json_msg);

    root = cJSON_Parse(resp->json_msg); 
    if(! getTTL(root, &ttl)) 
    {
        xes_log("解析json有误");
        cJSON_Delete(root);
        cetcd_response_release(resp);
        return 0;
    }

    cJSON_Delete(root);
    cetcd_response_release(resp);

    return ttl;
}

/*
进行保活
*/
uint64_t cetcd_keepalive(cetcd_client *cli, char* lease) {
    cetcd_request req;
    cetcd_response *resp;
    cetcd_string params;

    uint64_t ttl;
    char* msg;
    cJSON * root =  cJSON_CreateObject();

    cJSON_AddItemToObject(root, "ID", cJSON_CreateString(lease));
    msg = cJSON_PrintUnformatted(root);

    memset(&req, 0, sizeof(cetcd_request));
    req.method = ETCD_HTTP_POST;
    //req.api_type = ETCD_KEYS;
    req.uri = sdscatprintf(sdsempty(), "%s%s", cli->keys_space, "lease/keepalive");

    params = sdscatprintf(sdsempty(), "%s", msg);

    req.data = params;
    resp = cetcd_cluster_request(cli, &req);
    sdsfree(req.uri);
    sdsfree(params);

    if(NULL != msg)
    {
        free(msg);
        msg = NULL;
    }
    cJSON_Delete(root);

    xes_log("cetcd_keepalive 保活结果 %s", resp->json_msg);

    root = cJSON_Parse(resp->json_msg); 
    if(! getTTLAfterKeepAlive(root, &ttl)) 
    {
    xes_log("解析json有误");
    cJSON_Delete(root);
    cetcd_response_release(resp);
    return 0;
    }

    cJSON_Delete(root);
    cetcd_response_release(resp);

    return ttl;
}

void cetcd_node_release(cetcd_response_node *node) {
    int i, count;
    cetcd_response_node *n;
    if (node->nodes) {
        count = cetcd_array_size(node->nodes);
        for (i = 0; i < count; ++i) {
            n = cetcd_array_get(node->nodes, i);
            cetcd_node_release(n);
        }
        cetcd_array_release(node->nodes);
    }
    if (node->key) {
        sdsfree(node->key);
    }
    if (node->value) {
        sdsfree(node->value);
    }
    free(node);
}
void cetcd_response_release(cetcd_response *resp) {
    if(resp) {
        if (resp->err) {
            cetcd_error_release(resp->err);
            resp->err = NULL;
        }
        if (resp->node) {
            cetcd_node_release(resp->node);
        }
        if (resp->prev_node) {
            cetcd_node_release(resp->prev_node);
        }

		if (resp->json_msg) {
            free(resp->json_msg);
            resp->json_msg = NULL;
        }

        free(resp);
    }
}

void cetcd_error_release(cetcd_error *err) {
    if (err) {
        if (err->message) {
            sdsfree(err->message);
        }
        if (err->cause) {
            sdsfree(err->cause);
        }
        free(err);
    }
}


size_t cetcd_parse_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    int len;
    char *key, *val;
    cetcd_response_parser *parser;
    //yajl_status status;
    cetcd_response *resp = NULL;
    //cetcd_array *addrs = NULL;
    int i;
    enum resp_parser_st {
        request_line_start_st,
        request_line_end_st,
        request_line_http_status_start_st,
        request_line_http_status_st,
        request_line_http_status_end_st,
        header_key_start_st,
        header_key_st,
        header_key_end_st,
        header_val_start_st,
        header_val_st,
        header_val_end_st,
        blank_line_st,
        json_start_st,
        json_end_st,
        response_discard_st
    };
    /* Headers we are interested in:
     * X-Etcd-Index: 14695
     * X-Raft-Index: 672930
     * X-Raft-Term: 12
     * */
    parser = userdata;
//    if (parser->api_type == ETCD_MEMBERS) {
//        addrs = parser->resp;
//    } else 
        
    
    resp = parser->resp;
    
    len = size * nmemb;
    for (i = 0; i < len; ++i) {
        if (parser->st == request_line_start_st) {
            if (ptr[i] == ' ') {
                parser->st = request_line_http_status_start_st;
            }
            continue;
        }
        if (parser->st == request_line_end_st) {
            if (ptr[i] == '\n') {
                parser->st = header_key_start_st;
            }
            continue;
        }
        if (parser->st == request_line_http_status_start_st) {
            parser->buf = sdscatlen(parser->buf, ptr+i, 1);
            parser->st = request_line_http_status_st;
            continue;
        }
        if (parser->st == request_line_http_status_st) {
            if (ptr[i] == ' ') {
                parser->st = request_line_http_status_end_st;
            } else {
                parser->buf = sdscatlen(parser->buf, ptr+i, 1);
                continue;
            }
        }
        if (parser->st == request_line_http_status_end_st) {
            val = parser->buf;
            parser->http_status = atoi(val);
            sdsclear(parser->buf);
            parser->st = request_line_end_st;
//            if (parser->api_type == ETCD_MEMBERS && parser->http_status != 200) {
//                parser->st = response_discard_st;
//            }

            if (parser->http_status != 200) {
                parser->st = response_discard_st;
                //resp->err->ecode = parser->http_status;
                //resp->err->message = sdsnew("http code error");
            }

            continue;
        }
        if (parser->st == header_key_start_st) {
            if (ptr[i] == '\r') {
                ++i;
            }
            if (ptr[i] == '\n') {
                parser->st = blank_line_st;
                if (parser->http_status >= 300 && parser->http_status < 400) {
                    /*this is a redirection, restart the state machine*/
                    parser->st = request_line_start_st;
                    break;
                }
                continue;
            }
            parser->st = header_key_st;
        }
        if (parser->st == header_key_st) {
            parser->buf = sdscatlen(parser->buf, ptr+i, 1);
            if (ptr[i] == ':') {
                parser->st = header_key_end_st;
            } else {
                continue;
            }
        }
        if (parser->st == header_key_end_st) {
            parser->st = header_val_start_st;
            continue;
        }
        if (parser->st == header_val_start_st) {
            if (ptr[i] == ' ') {
                continue;
            }
            parser->st = header_val_st;
        }
        if (parser->st == header_val_st) {
            if (ptr[i] == '\r') {
                ++i;
            }
            if (ptr[i] == '\n') {
                parser->st = header_val_end_st;
            } else {
                parser->buf = sdscatlen(parser->buf, ptr+i, 1);
                continue;
            }
        }
        if (parser->st == header_val_end_st) {
            parser->st = header_key_start_st;
//            if (parser->api_type == ETCD_MEMBERS) {
//                sdsclear(parser->buf);
//                continue;
//            }
			
            int count = 0;
            sds *kvs = sdssplitlen(parser->buf, sdslen(parser->buf), ":", 1, &count);
            sdsclear(parser->buf);
            if (count < 2) {
                sdsfreesplitres(kvs, count);
                continue;
            }
            key = kvs[0];
            val = kvs[1];
			
            if (strncmp(key, "X-Etcd-Index", sizeof("X-Etcd-Index")-1) == 0) {
                resp->etcd_index = atoi(val);
            } else if (strncmp(key, "X-Raft-Index", sizeof("X-Raft-Index")-1) == 0) {
                resp->raft_index = atoi(val);
            } else if (strncmp(key, "X-Raft-Term", sizeof("X-Raft-Term")-1) == 0) {
                resp->raft_term = atoi(val);
            }
            sdsfreesplitres(kvs, count);
            continue;
        }

		
		memcpy(resp->json_msg, ptr, strlen(ptr));
		

//        if (parser->st == blank_line_st) {
//            if (ptr[i] != '{') {
//                /*not a json response, discard*/
//                parser->st = response_discard_st;
//                if (resp->err == NULL && parser->api_type == ETCD_KEYS) {
//                    resp->err = calloc(1, sizeof(cetcd_error));
//                    resp->err->ecode = error_response_parsed_failed;
//                    resp->err->message = sdsnew("not a json response");
//                    resp->err->cause = sdsnewlen(ptr, len);
//                }
//                continue;
//            }
//            parser->st = json_start_st;
//            cetcd_array_init(&parser->ctx.keystack, 10);
//            cetcd_array_init(&parser->ctx.nodestack, 10);
//            if (parser->api_type == ETCD_MEMBERS) {
//                parser->ctx.userdata = addrs;
//                parser->json = yajl_alloc(&sync_callbacks, 0, &parser->ctx);
//            }
//            else {
//                if (parser->http_status != 200 && parser->http_status != 201) {
//                    resp->err = calloc(1, sizeof(cetcd_error));
//					printf("jjjjjjjjjjjjjjjjj\n");
//                    parser->ctx.userdata = resp->err;
//                    parser->json = yajl_alloc(&error_callbacks, 0, &parser->ctx);
//                } else {
//                    parser->ctx.userdata = resp;
//                    parser->json = yajl_alloc(&callbacks, 0, &parser->ctx);
//                }
//            }
//        }
//        if (parser->st == json_start_st) {
//            if (yajl_status_ok == yajl_parse(parser->json, (const unsigned char *)ptr + i, len - i)) {
//                //all text left has been parsed, break the for loop
//				//printf("sssssssss %s\n", parser->json);
//                break;
//            } else {
//                parser->st = json_end_st;
//            }
//        }

//        if (parser->st == json_end_st) {
//            status = yajl_complete_parse(parser->json);
//            /*parse failed, TODO set error message*/
//            if (status != yajl_status_ok) {
//                if ( parser->api_type == ETCD_KEYS && resp->err == NULL) {
//                    resp->err = calloc(1, sizeof(cetcd_error));
//                    resp->err->ecode = error_response_parsed_failed;
//                    resp->err->message = sdsnew("http response is invalid json format");
//                    resp->err->cause = sdsnewlen(ptr, len);
//                }
//                return 0;
//            }
//            break;
//        }
        if (parser->st == response_discard_st) {
            return len;
        }
   }

    return len;
}

void *cetcd_send_request(CURL *curl, cetcd_request *req) {
    CURLcode res;
    cetcd_response_parser parser;
    cetcd_response *resp = NULL ;
    //cetcd_array *addrs = NULL;

//    if (req->api_type == ETCD_MEMBERS) {
//        addrs = cetcd_array_create(10);
//        parser.resp = addrs;
//    } else 
    {
        resp = calloc(1, sizeof(cetcd_response));
        parser.resp = resp;
        resp->json_msg = (char*)calloc(5000, sizeof(char));
    }

    //parser.api_type = req->api_type;
    parser.st = 0; /*0 should be the start state of the state machine*/
    parser.buf = sdsempty();
    //parser.json = NULL;
    
    curl_easy_setopt(curl, CURLOPT_URL, req->url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, http_method[req->method]);

    if (req->method == ETCD_HTTP_PUT || req->method == ETCD_HTTP_POST) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->data);
        xes_log("curl 参数 %s", req->data);
    } else {
        /* We must clear post fields here:
         * We reuse the curl handle for all HTTP methods.
         * CURLOPT_POSTFIELDS would be set when issue a PUT request.
         * The field  pointed to the freed req->data. It would be
         * reused by next request.
         * */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    }
    if (req->cli->settings.user) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, req->cli->settings.user);
    }
    if (req->cli->settings.password) {
        curl_easy_setopt(curl, CURLOPT_PASSWORD, req->cli->settings.password);
    }
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &parser);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cetcd_parse_response);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, req->cli->settings.verbose);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, req->cli->settings.connect_timeout);
    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "Expect:");
    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    res = curl_easy_perform(curl);

    curl_slist_free_all(chunk);
    //release the parser resource
    sdsfree(parser.buf);
//    if (parser.json) {
//        yajl_free(parser.json);
//        cetcd_array_destroy(&parser.ctx.keystack);
//        cetcd_array_destroy(&parser.ctx.nodestack);
//    }

    if (res != CURLE_OK) {
		
//        if (req->api_type == ETCD_MEMBERS) {
//            return addrs;
//        }
        if (resp->err == NULL) {
            resp->err = calloc(1, sizeof(cetcd_error));
            resp->err->ecode = error_send_request_failed;
            resp->err->message = sdsnew(curl_easy_strerror(res));
            resp->err->cause = sdsdup(req->url);
        }
        return resp;
    }

    if (!parser.resp)
    {
        printf("ppppppppppp\n");
    }
    return parser.resp;
}

/*
 * cetcd_cluster_request tries to request the whole cluster. It round-robin to next server if the request failed
 * */
void *cetcd_cluster_request(cetcd_client *cli, cetcd_request *req) {
    size_t i, count;
    cetcd_string url;
    cetcd_error *err = NULL;
    cetcd_response *resp = NULL;
   // cetcd_array *addrs = NULL;
    void *res = NULL;
    
    count = cetcd_array_size(cli->addresses);

    for(i = 0; i < count; ++i) {
        url = sdscatprintf(sdsempty(), "%s/%s", (cetcd_string)cetcd_array_get(cli->addresses, cli->picked), req->uri);
        xes_log("当前url %s", url);

        req->url = url;
        req->cli = cli;
        res = cetcd_send_request(cli->curl, req);
        sdsfree(url);

//        if (req->api_type == ETCD_MEMBERS) {
//            addrs = res;
//            /* Got the result addresses, return*/
//            if (addrs && cetcd_array_size(addrs)) {
//                return addrs;
//            }
//            /* Empty or error ? retry */
//            if (addrs) {
//                cetcd_array_release(addrs);
//                addrs = NULL;
//            }
//            if (i == count - 1) {
//                break;
//            }
//        } else if (req->api_type == ETCD_KEYS) 
            
       {
            resp = res;
           
//            if(resp && resp->err && resp->err->ecode == error_send_request_failed) {
            if(resp && resp->err) {
                if (i == count - 1) {
                    break;
                }
                cetcd_response_release(resp);
            } else {
                /*got response, return*/
				if(resp->err) {
					printf("222222222222 error :%p, %d, %s (%s)\n", resp->err, resp->err->ecode, resp->err->message, resp->err->cause);
				}
                return resp;
            }

        }
        /*try next*/
        cli->picked = (cli->picked + 1) % count;
    }
    /*the whole cluster failed*/
   // if (req->api_type == ETCD_MEMBERS) return NULL;
    if (resp) {
        if(resp->err) {
            err = resp->err; /*remember last error*/
        }
        resp->err = calloc(1, sizeof(cetcd_error));
        resp->err->ecode = error_cluster_failed;
        resp->err->message = sdsnew("etcd_cluster_request: all cluster servers failed.");
        if (err) {
            resp->err->message = sdscatprintf(resp->err->message, " last error: %s", err->message);
            cetcd_error_release(err);
        }
        resp->err->cause = sdsdup(req->uri);
    }
    return resp;
}