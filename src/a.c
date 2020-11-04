#include <stdio.h>
#include <stdlib.h>
int main() {
    char info[1000];
//sprintf(info, "{\"dept_id\":%d, \"app_name\":%s, \"client_addr\":%s, \"proxy_addr\":%s, \"redis_addr\":%s, \"recv_client_time\":%s, \"send_server_time222\":%s, \"recv_server_time\":\"cc\"}", 10, "abc", client_addr->str, proxy_addr->str, redis_addr->str, recv_client_time, "aaaa");
 // log_warn("QQQQQQQQQQQQ %s\n", info);

  char ab[1000];
    snprintf (ab, 1000, "%s", "{\"dept_id\":10, \"app_name\":200, proxy_ip\":111, client_ip:111, server_ip:10.90.100.13:6380,recv_client_time:1658063112,send_server_time:1658065470,recv_server_time:1658066911,send_client_time:1658067129}");
    printf("%s", ab);

return 0;
}