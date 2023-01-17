cmake_minimum_required(VERSION 3.22.1) # 最小版本
set (PROJECT "stunning")
project(${PROJECT} VERSION 0.1.0) #设置项目名称

# PROJECT_SOURCE_DIR：工程的根目录
# PROJECT_BINARY_DIR：运行cmake命令的目录，通常为${PROJECT_SOURCE_DIR}/build
# PROJECT_NAME：返回通过 project 命令定义的项目名称
# CMAKE_CURRENT_SOURCE_DIR：当前处理的 CMakeLists.txt 所在的路径
# CMAKE_CURRENT_BINARY_DIR：target 编译目录
# CMAKE_CURRENT_LIST_DIR：CMakeLists.txt 的完整路径
# EXECUTABLE_OUTPUT_PATH：重新定义目标二进制可执行文件的存放位置
# LIBRARY_OUTPUT_PATH：重新定义目标链接库文件的存放位置

set(CMAKE_CXX_STANDARD 11)

# include(CTest)
# enable_testing()

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
# aux_source_directory(. DIR_SRCS)

# 加入gcc警告
add_definitions(-Wall -Wextra)

#https://cmake.org/cmake/help/latest/command/file.html#glob-recurse
# 遍历所有src的文件,GLOB_RECURSE包括子目录
file(GLOB_RECURSE MyCpp src/*.cpp)

# # 添加 math 子目录
# add_subdirectory(math)

## 生成文件
add_executable(${PROJECT} ${MyCpp})

set(CPACK_PROJECT_NAME ${PROJECT_NAME}) #set 直接设置变量的值
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION}) #set 直接设置变量的值
include(CPack)

# 加入src头文件
target_include_directories(${PROJECT} PRIVATE src)

# 加入pthread https://stackoverflow.com/questions/5395309/how-do-i-force-cmake-to-include-pthread-option-during-compilation
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries( ${PROJECT} ${CMAKE_THREAD_LIBS_INIT} )

#======boost相关开始===================================================================================
# 查找boost #https://cmake.org/cmake/help/latest/module/FindBoost.html
# https://stackoverflow.com/questions/3808775/cmake-doesnt-find-boost
# find_package(Boost
#   [version] [EXACT]      # Minimum or EXACT version e.g. 1.67.0
#   [REQUIRED]             # Fail with error if Boost is not found
#   [COMPONENTS <libs>...] # Boost libraries by their canonical name
#                          # e.g. "date_time" for "libboost_date_time"
#   [OPTIONAL_COMPONENTS <libs>...]
#                          # Optional Boost libraries by their canonical name)
#   )                      # e.g. "date_time" for "libboost_date_time"

SET (DEBUG_BOOST ${PROJECT_SOURCE_DIR}/boost_1_81_0)
SET (BOOST_ROOT ${DEBUG_BOOST})
SET (BOOST_INCLUDEDIR ${DEBUG_BOOST}/boost)
SET (BOOST_LIBRARYDIR ${DEBUG_BOOST}/stage/lib)
set (Boost_NO_BOOST_CMAKE ON)
set(Boost_NO_WARN_NEW_VERSIONS 1) #旧版cmake和新版boost有警告
SET(Boost_USE_STATIC_LIBS ON) #静态链接
# ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK) # 动态链接

find_package(Boost 1.81.0 REQUIRED COMPONENTS system program_options filesystem date_time thread log)
#设置boost头文件
target_include_directories(${PROJECT} PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT} ${Boost_LIBRARIES})

#======boost相关结束===================================================================================

#======luajit开始==================================
 
 

SET(LUA_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/3rd/lua/include/)
SET(LUA_LIBRARIES ${PROJECT_SOURCE_DIR}/3rd/lua/lib/libluajit-5.1.so.2)
target_link_libraries(${PROJECT} ${LUA_LIBRARIES})
# # 包含整个目录
target_include_directories(${PROJECT} PUBLIC ${LUA_INCLUDE_DIR})
 
#======luajit结束==================================


#======mysql开始==================================
# include_directories(${PROJECT_SOURCE_DIR}/3rd/mysqlcapi/include)
# link_directories(${PROJECT_SOURCE_DIR}/3rd/mysqlcapi/lib)

SET(MYSQL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/3rd/mysqlcapi/include/)
SET(MYSQL_LIBRARIES ${PROJECT_SOURCE_DIR}/3rd/mysqlcapi/lib/libmysqlclient.so)
target_link_libraries(${PROJECT} ${MYSQL_LIBRARIES})
# # 包含整个目录
target_include_directories(${PROJECT} PUBLIC ${MYSQL_INCLUDE_DIR})

#======mysql结束==================================





#======ssl相关开始===================================================================================

# find_package(OpenSSL 1.1.0 REQUIRED)
# target_include_directories(${PROJECT} PRIVATE ${OPENSSL_INCLUDE_DIR})
# target_link_libraries(${PROJECT} ${OPENSSL_LIBRARIES})
# if(OPENSSL_VERSION VERSION_GREATER_EQUAL 1.1.1)
#     option(ENABLE_SSL_KEYLOG "Build with SSL KeyLog support" ON)
#     if(ENABLE_SSL_KEYLOG)
#         add_definitions(-DENABLE_SSL_KEYLOG)
#     endif()

#     option(ENABLE_TLS13_CIPHERSUITES "Build with TLS1.3 ciphersuites support" ON)
#     if(ENABLE_TLS13_CIPHERSUITES)
#         add_definitions(-DENABLE_TLS13_CIPHERSUITES)
#     endif()
# endif()
#======ssl相关结束===================================================================================

set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")

# 需要SO_REUSEPORT
add_definitions(-DENABLE_REUSE_PORT)

 

# https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
include(GNUInstallDirs)
install(TARGETS ${PROJECT} DESTINATION ${CMAKE_INSTALL_BINDIR})

# set_target_properties(${PROJECT} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "arg1 arg2 ... argN") 

# add_executable(demo demo.cpp) # 生成可执行文件
# add_library(common STATIC util.cpp) # 生成静态库
# add_library(common SHARED util.cpp) # 生成动态库或共享库

# 添加链接库
# target_link_libraries(Demo MathFunctions)


