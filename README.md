# wx-cache-proxy

wx-cache-proxy是多进程，基于中心化的代理，后端支持多种缓存(redis, pika,memcache)

## 环境要求
- Linux操作系统 (内核大于等于4.6)
- gcc版本在4.8.5以上
- 安装必要的软件包 automake libtool autoreconf curl curl-devel

## 安装
```shell
yum -y install automake  libtool autoreconf aclocal
git clone https://github.com/tal-tech/wx-cache-proxy.git
cd wx-cache-proxy
./configure --prefix=/data/wx-cache-proxy
make -j 4
make install
```

## 启动
```shell
/data/wx-cache-proxy/sbin/wx-cache-proxy -d -c /data/wx-cache-proxy/conf/proxy.yml -o /data/wx-cache-proxy/log/proxy.log -n 4 -e /data/wx-cache-proxy/conf/etcd.conf -L /data/wx-cache-proxy/conf/trace.conf

```

## 启动参数说明
- -d 守护程序方式运行
- -c wx-cache-proxy主配置文件
- -o wx-cache-proxy主日志
- -e etcd配置文件
- -L 请求在proxy追踪日志

### trace.conf
```shell
{
     "dept_id": "部门id",
     "app_name": "业务名称",
     "log_filename": "/path/to/log",
     "room": "机房",
     "slow_threshold":1
}
```
