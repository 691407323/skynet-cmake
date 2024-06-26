cmake_minimum_required(VERSION 3.10)

project(skynet)

# Linux
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    message(STATUS "current platform: Linux")
    add_compile_definitions(NOUSE_JEMALLOC LUA_USE_LINUX LUA_USE_READLINE)

    # 把lua头文件添加到全局搜索路径
    include_directories(skynet/3rd/lua)

    # 添加编译选项
    add_compile_options(-g)

    # 编译 skynet 可执行文件
    aux_source_directory(skynet/skynet-src SKYNET_SRC)
    aux_source_directory(skynet/3rd/lua LUA_LIB_SRC)
    list(REMOVE_ITEM LUA_LIB_SRC "skynet/3rd/lua/lua.c" "skynet/3rd/lua/luac.c" "skynet/3rd/lua/onelua.c")
    message(STATUS "The value of LUA_LIB_SRC is: ${LUA_LIB_SRC}")
    message(STATUS "The value of SKYNET_SRC is: ${SKYNET_SRC}")
    add_executable(skynet ${SKYNET_SRC} ${LUA_LIB_SRC})
    target_include_directories(skynet PRIVATE skynet/skynet-src)
    target_link_options(skynet PRIVATE "LINKER:-E")
    target_link_libraries(skynet m pthread dl rt)

    message(STATUS "The value of CMAKE_BINARY_DIR is: ${CMAKE_BINARY_DIR}")

    # 重新设置动态库输出路径
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cservice)

    # 生成c服务
    file(GLOB CSERVICE_FILE_LIST "skynet/service-src/*.c")
    message(STATUS "The value of CSERVICE_FILE_LIST is: ${CSERVICE_FILE_LIST}")
    foreach(FILE_PATH IN LISTS CSERVICE_FILE_LIST)
        message(STATUS "Processing file: ${FILE_PATH}")
        string(REGEX REPLACE ".*skynet/service-src/service_" "" LIB_NAME "${FILE_PATH}")
        string(REGEX REPLACE ".c$" "" LIB_NAME "${LIB_NAME}")
        message(STATUS "The value of LIB_NAME is: ${LIB_NAME}")
        add_library(${LIB_NAME} SHARED ${FILE_PATH})
        target_include_directories(${LIB_NAME} PRIVATE skynet/skynet-src)
        set_target_properties(${LIB_NAME} PROPERTIES PREFIX "")
    endforeach()

    # 重新设置动态库输出路径
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/luaclib)

    # 生成动态库 skynet.so
    set(LUA_CLIB_SKYNET_SRC
        skynet/lualib-src/lua-skynet.c
        skynet/lualib-src/lua-seri.c
        skynet/lualib-src/lua-socket.c
        skynet/lualib-src/lua-mongo.c
        skynet/lualib-src/lua-netpack.c
        skynet/lualib-src/lua-memory.c
        skynet/lualib-src/lua-multicast.c
        skynet/lualib-src/lua-cluster.c
        skynet/lualib-src/lua-crypt.c
        skynet/lualib-src/lsha1.c
        skynet/lualib-src/lua-sharedata.c
        skynet/lualib-src/lua-stm.c
        skynet/lualib-src/lua-debugchannel.c
        skynet/lualib-src/lua-datasheet.c
        skynet/lualib-src/lua-sharetable.c
        )
    add_library(skynetso SHARED ${LUA_CLIB_SKYNET_SRC})
    target_include_directories(skynetso PRIVATE skynet/skynet-src skynet/lualib-src)
    set_target_properties(skynetso PROPERTIES OUTPUT_NAME skynet)
    set_target_properties(skynetso PROPERTIES PREFIX "")

    # 生成动态库 bson.so
    add_library(bson SHARED skynet/lualib-src/lua-bson.c)
    target_include_directories(bson PRIVATE skynet/skynet-src)
    set_target_properties(bson PROPERTIES PREFIX "")

    # 生成动态库 md5.so
    aux_source_directory(skynet/3rd/lua-md5 LUA_MD5_SRC)
    add_library(md5 SHARED ${LUA_MD5_SRC})
    set_target_properties(md5 PROPERTIES PREFIX "")

    # 生成动态库 client.so
    add_library(client SHARED skynet/lualib-src/lua-clientsocket.c skynet/lualib-src/lua-crypt.c skynet/lualib-src/lsha1.c)
    set_target_properties(client PROPERTIES PREFIX "")
    target_link_libraries(client pthread)

    # 生成动态库 sproto.so
    add_library(sproto SHARED skynet/lualib-src/sproto/sproto.c skynet/lualib-src/sproto/lsproto.c)
    set_target_properties(sproto PROPERTIES PREFIX "")

    # 生成动态库 lpeg.so
    aux_source_directory(skynet/3rd/lpeg LPEG_SRC)
    add_library(lpeg SHARED ${LPEG_SRC})
    set_target_properties(lpeg PROPERTIES PREFIX "")
endif ()

# Windows
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    message(STATUS "current platform: Windows")
    add_compile_definitions(NOUSE_JEMALLOC LUA_BUILD_AS_DLL)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-int-conversion -Wno-implicit-function-declaration -Wno-deprecated-declarations")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

    # 把lua头文件添加到全局搜索路径
    include_directories(skynet/3rd/lua 3rd/posix 3rd/pthread-win32 skynet/skynet-src)
    add_compile_options(/FI ${CMAKE_SOURCE_DIR}/3rd/posix/unistd.h)

    message(STATUS "The value of CMAKE_BINARY_DIR is: ${CMAKE_BINARY_DIR}")

    aux_source_directory(skynet/skynet-src LIB_SKYNET_SRC)
    list(REMOVE_ITEM LIB_SKYNET_SRC skynet/skynet-src/skynet_main.c)
    aux_source_directory(skynet/3rd/lua LUA_LIB_SRC)
    list(REMOVE_ITEM LUA_LIB_SRC "skynet/3rd/lua/lua.c" "skynet/3rd/lua/luac.c" "skynet/3rd/lua/onelua.c")
    aux_source_directory(3rd/posix POSIX_SRC)

    add_subdirectory(3rd/pthread-win32)

    message(STATUS "The value of LUA_LIB_SRC is: ${LUA_LIB_SRC}")
    add_library(liblua SHARED ${LUA_LIB_SRC})
    add_library(libposix SHARED ${POSIX_SRC})
    target_link_libraries(libposix ws2_32)
    add_library(libskynet SHARED ${LIB_SKYNET_SRC})
    target_link_libraries(libskynet liblua libposix pthreadVC3)

    # 编译 skynet 可执行文件
    add_executable(skynet skynet/skynet-src/skynet_main.c)
    target_link_libraries(skynet ws2_32 liblua libposix libskynet pthreadVC3)

    # 生成动态库 skynet.so
    set(LUA_CLIB_SKYNET_SRC
        skynet/lualib-src/lua-skynet.c
        skynet/lualib-src/lua-seri.c
        skynet/lualib-src/lua-socket.c
        skynet/lualib-src/lua-mongo.c
        skynet/lualib-src/lua-netpack.c
        skynet/lualib-src/lua-memory.c
        skynet/lualib-src/lua-multicast.c
        skynet/lualib-src/lua-cluster.c
        skynet/lualib-src/lua-crypt.c
        skynet/lualib-src/lsha1.c
        skynet/lualib-src/lua-sharedata.c
        skynet/lualib-src/lua-stm.c
        skynet/lualib-src/lua-debugchannel.c
        skynet/lualib-src/lua-datasheet.c
        skynet/lualib-src/lua-sharetable.c
        )
    message(STATUS "The value of LIB_SKYNET_SRC is: ${LIB_SKYNET_SRC}")
    add_library(skynetso SHARED ${LUA_CLIB_SKYNET_SRC})
    target_compile_definitions(skynetso PUBLIC LUA_LIB)
    set_target_properties(skynetso PROPERTIES OUTPUT_NAME skynet)
    target_link_libraries(skynetso ws2_32 libskynet)

    # 生成动态库 bson.so
    add_library(bson SHARED skynet/lualib-src/lua-bson.c)
    target_link_libraries(bson ws2_32 liblua)

    # 生成动态库 md5.so
    aux_source_directory(skynet/3rd/lua-md5 LUA_MD5_SRC)
    add_library(md5 SHARED ${LUA_MD5_SRC})
    target_link_libraries(md5 liblua)

    # 生成动态库 client.so
    add_library(client SHARED skynet/lualib-src/lua-clientsocket.c skynet/lualib-src/lua-crypt.c skynet/lualib-src/lsha1.c)
    target_link_libraries(client ws2_32 liblua libposix pthreadVC3)

    # 生成动态库 sproto.so
    add_library(sproto SHARED skynet/lualib-src/sproto/sproto.c skynet/lualib-src/sproto/lsproto.c)
    target_link_libraries(sproto liblua)

    # 生成动态库 lpeg.so
    aux_source_directory(skynet/3rd/lpeg LPEG_SRC)
    add_library(lpeg SHARED ${LPEG_SRC})
    target_link_libraries(lpeg liblua)

    set_target_properties(lpeg sproto client md5 bson skynetso PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/luaclib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/luaclib
        PREFIX ""
        SUFFIX .so
        )

    # 生成c服务
    file(GLOB CSERVICE_FILE_LIST "skynet/service-src/*.c")
    message(STATUS "The value of CSERVICE_FILE_LIST is: ${CSERVICE_FILE_LIST}")
    foreach(FILE_PATH IN LISTS CSERVICE_FILE_LIST)
        message(STATUS "Processing file: ${FILE_PATH}")
        string(REGEX REPLACE ".*skynet/service-src/service_" "" LIB_NAME "${FILE_PATH}")
        string(REGEX REPLACE ".c$" "" LIB_NAME "${LIB_NAME}")
        message(STATUS "The value of LIB_NAME is: ${LIB_NAME}")
        add_library(${LIB_NAME} SHARED ${FILE_PATH})
        target_link_libraries(${LIB_NAME} ws2_32 libskynet)
        set_target_properties(${LIB_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/cservice
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cservice
            PREFIX ""
            SUFFIX .so
            ENABLE_EXPORTS ON
            )
    endforeach()

endif ()

# MacOS
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message(STATUS "current platform: Mac OS X")
    add_compile_definitions(NOUSE_JEMALLOC LUA_USE_MACOSX LUA_USE_READLINE)
    message(FATAL_ERROR "Unsupport platform: Mac OS X")
endif ()

# 拷贝其他文件
# CMAKE_SOURCE_DIR
message(STATUS "The value of CMAKE_SOURCE_DIR is: ${CMAKE_SOURCE_DIR}")
file(COPY ${CMAKE_SOURCE_DIR}/skynet/lualib/ DESTINATION lualib NO_SOURCE_PERMISSIONS)
file(COPY ${CMAKE_SOURCE_DIR}/skynet/test/ DESTINATION test NO_SOURCE_PERMISSIONS)
file(COPY ${CMAKE_SOURCE_DIR}/skynet/service/ DESTINATION service NO_SOURCE_PERMISSIONS)
file(COPY ${CMAKE_SOURCE_DIR}/skynet/examples/ DESTINATION examples NO_SOURCE_PERMISSIONS)
