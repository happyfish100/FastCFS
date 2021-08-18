### 1. 网络连接异常

当日志出现`Connection refused`或者`Transport endpoint is not connected`之类的异常记录。
请检查防火墙设置。FastCFS 共有7个服务端口,这些对应的端口应该打开：

* fdir
  * 默认集群端口 `11011`
  * 默认服务端口 `11012`
* fauth
  * 默认集群端口 `31011`
  * 默认服务端口 `31012`
* fstore
  * 默认集群端口 `21014`
  * 默认副本端口 `21015`
  * 默认服务端口 `21016`


### 2. 服务有没有启动顺序？

有。应按顺序启动`fdir`， `fauth`（可选），`fstore`，`fuseclient`。

