Sentinel（哨兵）是一个用纯 C++ 编写的轻量级后台进程，用于监控系统并根据接收到的指令执行相应任务。

## 编译部署

工程使用到现代 C++ 11 特性, 最低编译要求需要 gcc 4.7 以上版本

```shell
# 编译
make

# 默认启动在 127.0.0.1:8080 地址
./sentinel

# 启动时指定服务地址
./sentinel -addr 127.0.0.1:8000

# 启动时限定 shell 命令执行的目录, 多个目录使用 | 分开
./sentinel -allow_cmd_dir "/home/|/usr/local/"
```

注意: 远程执行命令的接口属于高危接口, 最小化限定 shell 命令执行的目录.

## 快发布实践

在现代化工程中, 业务服务通常使用 k8s 调用 Pod 来部署容器, 假设在业务开发过程中有个小改动需要发布到 Pod 中进行验证. 常规的办法是走漫长的流水线编译打包镜像, 然后走 kubectl 更新容器镜像. 这里存在的问题就是整个流程太长, 每一次改动需要等待好久. 而观察整个部署过程代码编译其实很快, 如果能够省去重新构建镜像部署容器, 就可以节省很多的时间.  

在这种背景下, 提高工作效率的解决方案是在容器打包时注入 sentinel 守护进程随容器启动, 之后通过外部远程上传二进制然后执行 shell 命令进行版本更新.

基本上所有的 linux 发行版都会安装有 curl 工具, 所以这里使用 curl 来演示, 简化上手难度

```shell
# 上传二进制
curl --location http://127.0.0.1:8080/upload --form 'filename=@"/home/xxx/web_service"'

# 执行远程命令
curl --location 'http://127.0.0.1:8080/command' \
--header 'Content-Type: text/plain' \
--data '/usr/local/start.sh start'
```

其中, 这里 shell 命令用户可以自由组合, 可以使用任何工程方式集成到工具进行服务的快速部署.

