# cf-like

## Quick Start

### 准备环节
* bazel
* linux
* cpp相关库下载
* 梯子（用于拉取bazel，可选）

### 运行方式

```
bazel run :cf-like
```
编译大概3-5分钟，首次编译较慢

### 开发模式（待优化）
```
bazel run :refresh_compile_commands
```
可以配置一定的高亮和跳转，没有关联proto。

## 文件目录介绍

demo形态时的文件状态如下：
```
.
├── BUILD.bazel
├── README.md
├── WORKSPACE
├── _Bazel
│   ├── leveldb.BUILD
│   ├── openssl.BUILD
│   ├── protobuf.BUILD
│   └── zlib.BUILD
├── brpc_workspace.bzl
├── cert.pem
├── config.json
├── key.pem
├── proto
│   ├── feed.proto
│   └── http.proto
└── src
    ├── com
    │   ├── baseStrategy.h
    │   └── context.h
    ├── constant
    │   └── status.h
    ├── datatype
    ├── handler
    │   ├── server.cpp
    │   └── server.h
    ├── main.cpp
    ├── strategy
    │   ├── filter
    │   ├── recall
    │   ├── response
    │   └── sort
    └── util

```
* bazel 前缀后缀相关均为bazel工具链。
* config.json 设计为获取参数，作为全局文件仅读一次，减少硬编码改动。
* proto 为protocbuff文件，用于生成结构化序列化格式进行数据传递（目前只用于结构体转json）
* src 为代码库。
* main.cpp 为启动函数
* handler 为主流程函数，后续逻辑链路均在该函数中实现。
* com为common库，用于存放基础模板和算子模板。
* datatype为特化类型库，暂不需要。
* constant为常量库。
* strategy为算子库。