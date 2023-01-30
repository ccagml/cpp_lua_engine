# 项目名称

## cpp_lua_engine 一个 c++ 和lua结合的玩具
- c++处理网络通信、MySQL相关的操作
- lua脚本层处理业务逻辑

## 目录

- [开发前的配置要求](#开发前的配置要求)
- [安装步骤](#安装步骤)
- [文件目录说明](#文件目录说明)
- [部署](#部署)
- [使用到的框架](#使用到的框架)
- [版本控制](#版本控制)



## 开发前的配置要求

1. Ubuntu 22.04 LTS
2. cmake version 3.22.1

## **安装步骤**

#### 克隆本项目
- git clone https://github.com/ccagml/stunning.git

#### 加载项目子模块
- git submodule update

#### 下载编译Boost
- sudo apt-get update
- sudo apt-get install build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev 
- wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
- tar -zxvf boost_1_81_0.tar.gz
- cd boost_1_81_0
- ./bootstrap.sh
- ./b2
- sudo ./b2 install
### 

#### 执行cmake编译项目
- cmake ./CMakeLists.txt -B build/ -DCMAKE_BUILD_TYPE=Debug
- cd build
- make

## 文件目录说明

```
filetree 
├── LICENSE.txt
├── README.md
├── /3rd/  依赖的第三方包Mysql、LuaJit等等
├── /src/
│  ├── config_manager.cpp 配置相关操作管理
│  ├── database_manager.cpp 数据库相关操作管理
│  ├── global_define.hpp 全局定义
│  ├── main.cpp 启动目录
│  ├── main_manager.cpp 整个程序主要管理
│  ├── net_manager.cpp 网络相关操作管理
│  ├── script_manager.cpp lua脚本引擎相关操作管理
│  ├── script_protobuf.hpp protobuf相关支持

```


## 部署

- ./z.sh

## 使用到的框架

- [Boost](https://www.boost.org/)
- [libmysqlclient](https://dev.mysql.com/downloads/c-api/)
- [LuaJIT](https://luajit.org/)


## 版本控制

该项目使用Git进行版本管理。您可以在repository参看当前可用版本。


## 版权说明

该项目签署了MIT 授权许可，详情请参阅 [LICENSE.txt](https://github.com/ccagml/cpp_lua_engine/blob/master/LICENSE.txt)


<!-- links -->
[your-project-path]:ccagml/cpp_lua_engine
[contributors-shield]: https://img.shields.io/github/contributors/ccagml/cpp_lua_engine.svg?style=flat-square
[contributors-url]: https://github.com/ccagml/cpp_lua_engine/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/ccagml/cpp_lua_engine.svg?style=flat-square
[forks-url]: https://github.com/ccagml/cpp_lua_engine/network/members
[stars-shield]: https://img.shields.io/github/stars/ccagml/cpp_lua_engine.svg?style=flat-square
[stars-url]: https://github.com/ccagml/cpp_lua_engine/stargazers
[issues-shield]: https://img.shields.io/github/issues/ccagml/cpp_lua_engine.svg?style=flat-square
[issues-url]: https://img.shields.io/github/issues/ccagml/cpp_lua_engine.svg
[license-shield]: https://img.shields.io/github/license/ccagml/cpp_lua_engine.svg?style=flat-square
[license-url]: https://github.com/ccagml/cpp_lua_engine/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=flat-square&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/shaojintian



