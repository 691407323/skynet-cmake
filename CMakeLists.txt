cmake_minimum_required(VERSION 3.11)

project(skynet)

# 添加编译选项
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SKYNET_LUACLIB_SRC
    core/skynet/lualib-src/lua-skynet.c
    core/skynet/lualib-src/lua-seri.c
    core/skynet/lualib-src/lua-socket.c
    core/skynet/lualib-src/lua-mongo.c
    core/skynet/lualib-src/lua-netpack.c
    core/skynet/lualib-src/lua-memory.c
    core/skynet/lualib-src/lua-multicast.c
    core/skynet/lualib-src/lua-cluster.c
    core/skynet/lualib-src/lua-crypt.c
    core/skynet/lualib-src/lsha1.c
    core/skynet/lualib-src/lua-sharedata.c
    core/skynet/lualib-src/lua-stm.c
    core/skynet/lualib-src/lua-debugchannel.c
    core/skynet/lualib-src/lua-datasheet.c
    core/skynet/lualib-src/lua-sharetable.c
)

set(LUACLIB_TARGETS)

function(add_luaclib name)
    cmake_parse_arguments(A "" "SRC_DIR" "SRCS;LINKS;INCLUDES;DEFS" ${ARGN})

    if(A_SRC_DIR AND A_SRCS)
        message(FATAL_ERROR "add_luaclib(${name}): 不能同时指定 SRC_DIR 和 SRCS")
    endif()

    if(A_SRC_DIR)
        aux_source_directory(${A_SRC_DIR} ${name}_SRC)
        set(srcs ${${name}_SRC})
    else()
        set(srcs ${A_SRCS})
    endif()

    add_library(${name} SHARED ${srcs})
    # set(LUACLIB_TARGETS "${LUACLIB_TARGETS};${name}" PARENT_SCOPE)
    if(NOT ${name} IN_LIST LUACLIB_TARGETS)
        set(LUACLIB_TARGETS "${LUACLIB_TARGETS};${name}" PARENT_SCOPE)
    else()
        message(WARNING "add_luaclib(${name}): 模块 ${name} 已经添加过了，可能会导致重复定义")
    endif()

    if(A_LINKS)
        target_link_libraries(${name} ${A_LINKS})
    else()
        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            target_link_libraries(${name} liblua)
        endif()
    endif()

    if(A_INCLUDES)
        target_include_directories(${name} PRIVATE core/skynet/3rd/lua/ ${A_INCLUDES})
    else()
        target_include_directories(${name} PRIVATE core/skynet/3rd/lua/)
    endif()
    if(A_DEFS)
        target_compile_definitions(${name} PRIVATE ${A_DEFS})
    endif()
endfunction()

# 把lua头文件添加到全局搜索路径
include_directories( core/skynet/3rd/lua/ )

message(STATUS "current platform: ${CMAKE_SYSTEM_NAME}")

# 平台特定编译选项
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-int-conversion -Wno-implicit-function-declaration -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
    add_compile_options(/FI ${CMAKE_SOURCE_DIR}/core/posix/unistd.h)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_compile_options(-g)
endif()

# 平台特定链接选项
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CMAKE_C_FLAGS "-O2 -Wall")
    set(CMAKE_SHARED_LINKER_FLAGS " -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup")
endif()

# ============================================================
# 平台特定变量设置（必须在平台构建块之前定义）
# ============================================================
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(PLATFORM_COMPILE_DEFS NOUSE_JEMALLOC)
    set(PLATFORM_LINK_LIBS ws2_32 pthreadVC3)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(PLATFORM_COMPILE_DEFS NOUSE_JEMALLOC LUA_USE_LINUX LUA_USE_READLINE)
    set(PLATFORM_LINK_LIBS m pthread dl rt)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(PLATFORM_COMPILE_DEFS NOUSE_JEMALLOC LUA_USE_MACOSX LUA_USE_READLINE)
    set(PLATFORM_LINK_LIBS m pthread dl)
endif()

add_compile_definitions(${PLATFORM_COMPILE_DEFS})

# ============================================================
# Windows 特定设置与构建
# ============================================================
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    if (NOT DEFINED CMAKE_GENERATOR_PLATFORM)
        set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "Default platform is x64" FORCE)
    endif()

    message(STATUS "The value of CMAKE_BINARY_DIR is: ${CMAKE_BINARY_DIR}")

    # 把lua头文件添加到全局搜索路径
    include_directories(core/skynet/3rd/lua core/posix core/pthread-win32 core/skynet/skynet-src)

    aux_source_directory(core/skynet/skynet-src SKYNET_CORE_SRC)
    list(REMOVE_ITEM SKYNET_CORE_SRC core/skynet/skynet-src/skynet_main.c)
    aux_source_directory(core/skynet/3rd/lua LUA_STANDALONE_SRC)
    list(REMOVE_ITEM LUA_STANDALONE_SRC "core/skynet/3rd/lua/lua.c" "core/skynet/3rd/lua/luac.c" "core/skynet/3rd/lua/onelua.c")
    aux_source_directory(core/posix POSIX_SRC)

    add_subdirectory(core/pthread-win32)

    message(STATUS "The value of LUA_STANDALONE_SRC is: ${LUA_STANDALONE_SRC}")
    add_library(liblua SHARED ${LUA_STANDALONE_SRC})
    target_include_directories(liblua PRIVATE core/skynet/3rd/lua/ core/skynet/skynet-src)
    target_compile_definitions(liblua PRIVATE LUA_BUILD_AS_DLL)
    add_library(libposix SHARED ${POSIX_SRC})
    target_include_directories(libposix PRIVATE core/posix core/pthread-win32)
    target_link_libraries(libposix ws2_32)
    add_library(libskynet SHARED ${SKYNET_CORE_SRC})
    target_include_directories(libskynet PRIVATE core/skynet/skynet-src core/skynet/3rd/lua/)
    target_link_libraries(libskynet liblua libposix pthreadVC3)

    add_executable(skynet core/skynet/skynet-src/skynet_main.c)
    target_include_directories(skynet PRIVATE core/skynet/skynet-src core/skynet/3rd/lua/)
    target_link_libraries(skynet ws2_32 liblua libposix libskynet pthreadVC3)

    message(STATUS "The value of SKYNET_CORE_SRC is: ${SKYNET_CORE_SRC}")
else ()

    # 编译 skynet 可执行文件
    aux_source_directory(core/skynet/skynet-src SKYNET_SRC)
    aux_source_directory(core/skynet/3rd/lua LUA_STANDALONE_SRC)
    list(REMOVE_ITEM LUA_STANDALONE_SRC "core/skynet/3rd/lua/lua.c" "core/skynet/3rd/lua/luac.c" "core/skynet/3rd/lua/onelua.c")
    message(STATUS "The value of LUA_STANDALONE_SRC is: ${LUA_STANDALONE_SRC}")
    message(STATUS "The value of SKYNET_SRC is: ${SKYNET_SRC}")
    add_executable(skynet ${SKYNET_SRC} ${LUA_STANDALONE_SRC})
    target_include_directories(skynet PRIVATE core/skynet/skynet-src core/skynet/3rd/lua/)
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        target_link_options(skynet PRIVATE "LINKER:-E")
    endif()
    target_link_libraries(skynet ${PLATFORM_LINK_LIBS})

    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        message(STATUS " Darwin The value of CMAKE_BINARY_DIR is: ${CMAKE_BINARY_DIR}")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
    endif()

endif ()

# ============================================================
# 统一 luaclib（逐个 add_luaclib，平台差异直接在参数中处理）
# ============================================================

# 生成动态库 skynet.so
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_luaclib(skynetso SRCS ${SKYNET_LUACLIB_SRC}
        INCLUDES "core/skynet/skynet-src;core/skynet/lualib-src"
        LINKS "libskynet"
    )
else()
    add_luaclib(skynetso SRCS ${SKYNET_LUACLIB_SRC}
        INCLUDES "core/skynet/skynet-src;core/skynet/lualib-src"
    )
endif()
set_target_properties(skynetso PROPERTIES OUTPUT_NAME skynet PREFIX "" SUFFIX .so)

# 生成动态库 bson.so
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_luaclib(bson SRCS "core/skynet/lualib-src/lua-bson.c" LINKS "ws2_32;liblua")
else()
    add_luaclib(bson SRCS "core/skynet/lualib-src/lua-bson.c"
        INCLUDES "core/skynet/skynet-src"
    )
endif()

# 生成动态库 md5.so
add_luaclib(md5 SRC_DIR core/skynet/3rd/lua-md5)

# 生成动态库 client.so
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_luaclib(client SRCS "core/skynet/lualib-src/lua-clientsocket.c;core/skynet/lualib-src/lua-crypt.c;core/skynet/lualib-src/lsha1.c" LINKS "${PLATFORM_LINK_LIBS};liblua;libposix")
else()
    add_luaclib(client SRCS "core/skynet/lualib-src/lua-clientsocket.c;core/skynet/lualib-src/lua-crypt.c;core/skynet/lualib-src/lsha1.c" LINKS "pthread")
endif()

# 生成动态库 sproto.so
add_luaclib(sproto SRC_DIR core/skynet/lualib-src/sproto)

# 生成动态库 lpeg.so
add_luaclib(lpeg SRC_DIR core/skynet/3rd/lpeg)

# 生成动态库 pb.so
add_luaclib(pb SRC_DIR luaclib-src/lua-protobuf)

# 生成动态库 lfs.so
add_luaclib(lfs SRC_DIR luaclib-src/luafilesystem)

# 生成动态库 fmt.so
add_luaclib(fmt SRC_DIR luaclib-src/lua-fmt)

# 生成动态库 yyjson.so
add_luaclib(yyjson SRC_DIR luaclib-src/lua-yyjson)

# 生成动态库 zset.so
add_luaclib(zset SRC_DIR luaclib-src/lua-zset)

# 生成动态库 aoi.so
add_luaclib(aoi SRC_DIR luaclib-src/lua-aoi)

# 生成动态库 navmesh.so
add_luaclib(navmesh SRC_DIR luaclib-src/lua-navmesh
    INCLUDES "luaclib-src/recastnavigation/Detour/Include/;luaclib-src/recastnavigation/DetourCrowd/Include/;luaclib-src/recastnavigation/DetourTileCache/Include/;luaclib-src/recastnavigation/Recast/Include/"
)
aux_source_directory(luaclib-src/recastnavigation/Detour/Source Detour_SRC)
aux_source_directory(luaclib-src/recastnavigation/DetourCrowd/Source DetourCrowd_SRC)
aux_source_directory(luaclib-src/recastnavigation/DetourTileCache/Source DetourTileCache_SRC)
aux_source_directory(luaclib-src/recastnavigation/Recast/Source Recast_SRC)
target_sources(navmesh PRIVATE ${Detour_SRC} ${DetourCrowd_SRC} ${DetourTileCache_SRC} ${Recast_SRC})

# 生成动态库 rp3d.so
# aux_source_directory(luaclib-src/lua-rp3d RP3D_LUA_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/body/ RP3D_body_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/collision/ RP3D_collision_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/collision/broadphase/ RP3D_collision_broadphase_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/collision/narrowphase/ RP3D_collision_narrowphase_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/collision/narrowphase/GJK/ RP3D_collision_narrowphase_GJK_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/collision/narrowphase/SAT/ RP3D_collision_narrowphase_SAT_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/collision/shapes/ RP3D_collision_shapes_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/components/ RP3D_components_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/constraint/ RP3D_constraint_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/engine/ RP3D_engine_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/mathematics/ RP3D_mathematics_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/memory/ RP3D_memory_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/systems/ RP3D_systems_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/utils/ RP3D_utils_SRC)
# aux_source_directory(luaclib-src/reactphysics3d/src/utils/quickhull/ RP3D_utils_quickhull_SRC)
# # message(STATUS ">>> [rp3d] LUA_SRC=${RP3D_LUA_SRC}")
# # message(STATUS ">>> [rp3d] body_SRC=${RP3D_body_SRC}")
# # message(STATUS ">>> [rp3d] collision_SRC=${RP3D_collision_SRC}")
# set(RP3D_ALL_SRC "${RP3D_LUA_SRC};${RP3D_body_SRC};${RP3D_collision_SRC};${RP3D_collision_broadphase_SRC};${RP3D_collision_narrowphase_SRC};${RP3D_collision_narrowphase_GJK_SRC};${RP3D_collision_narrowphase_SAT_SRC};${RP3D_collision_shapes_SRC};${RP3D_components_SRC};${RP3D_constraint_SRC};${RP3D_engine_SRC};${RP3D_mathematics_SRC};${RP3D_memory_SRC};${RP3D_systems_SRC};${RP3D_utils_SRC};${RP3D_utils_quickhull_SRC}")
# # message(STATUS ">>> [rp3d] ALL_SRC=${RP3D_ALL_SRC}")
# add_luaclib(rp3d SRCS "${RP3D_ALL_SRC}"
#     INCLUDES "luaclib-src/reactphysics3d/include/;luaclib-src/reactphysics3d/include/reactphysics3d/body/;luaclib-src/reactphysics3d/include/reactphysics3d/collision/;luaclib-src/reactphysics3d/include/reactphysics3d/collision/broadphase/;luaclib-src/reactphysics3d/include/reactphysics3d/collision/narrowphase/;luaclib-src/reactphysics3d/include/reactphysics3d/collision/narrowphase/GJK/;luaclib-src/reactphysics3d/include/reactphysics3d/collision/narrowphase/SAT/;luaclib-src/reactphysics3d/include/reactphysics3d/collision/shapes/;luaclib-src/reactphysics3d/include/reactphysics3d/components/;luaclib-src/reactphysics3d/include/reactphysics3d/constraint/;luaclib-src/reactphysics3d/include/reactphysics3d/containers/;luaclib-src/reactphysics3d/include/reactphysics3d/engine/;luaclib-src/reactphysics3d/include/reactphysics3d/mathematics/;luaclib-src/reactphysics3d/include/reactphysics3d/memory/;luaclib-src/reactphysics3d/include/reactphysics3d/systems/;luaclib-src/reactphysics3d/include/reactphysics3d/utils/;luaclib-src/reactphysics3d/include/reactphysics3d/utils/quickhull/"
# )

# 生成动态库 math3d.so
add_luaclib(math3d SRC_DIR luaclib-src/math3d
    INCLUDES "luaclib-src/glm/include/"
)
target_compile_definitions(math3d PRIVATE -DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_QUAT_DATA_XYZW)

# 生成动态库 lsyslog.so（仅 Linux/macOS）
if (CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_luaclib(lsyslog SRC_DIR luaclib-src/luasyslog)
endif()

# 生成动态库 ecs.so
add_luaclib(ecs SRC_DIR luaclib-src/luaecs)

# 生成动态库 ltls.so（仅 Linux/macOS）
if (CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_luaclib(ltls SRCS "core/skynet/lualib-src/ltls.c"
        INCLUDES "luaclib-src/openssl/include/"
    )
    if(RP3D_OPENSSL_LIB_DIR)
        target_link_directories(ltls PRIVATE ${RP3D_OPENSSL_LIB_DIR})
    endif()
    target_link_libraries(ltls ssl crypto)
endif()

# 生成动态库 winhelp.so
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_luaclib(winhelp SRCS "luaclib-src/winStdin/lua-winhelp.c" LINKS "Kernel32;liblua")
endif()

# 生成动态库 cjson.so
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_luaclib(cjson SRCS "luaclib-src/lua-cjson/lua_cjson.c;luaclib-src/lua-cjson/strbuf.c;luaclib-src/lua-cjson/fpconv.c"
        LINKS "liblua"
        DEFS "strncasecmp=_strnicmp;strcasecmp=_stricmp;DISABLE_INVALID_NUMBERS"
    )
    # target_compile_definitions(cjson PRIVATE
    #     _CRT_SECURE_NO_WARNINGS
    #     inline=__inline
    #     snprintf=_snprintf
    # )
else()
    add_luaclib(cjson SRCS "luaclib-src/lua-cjson/lua_cjson.c;luaclib-src/lua-cjson/strbuf.c;luaclib-src/lua-cjson/fpconv.c")
endif()

# 平台特定 luaclib 输出目录
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_target_properties(${LUACLIB_TARGETS} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/luaclib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/luaclib
        PREFIX ""
        SUFFIX .so
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    set_target_properties(${LUACLIB_TARGETS} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/luaclib
        PREFIX ""
        SUFFIX .so
    )
endif()

# ============================================================
# 统一生成 c 服务（所有平台）
# ============================================================
function(add_cservice_from_dir SRC_DIR PATH_PREFIX)
    file(GLOB CSERVICE_FILE_LIST "${SRC_DIR}/*.c")
    foreach(FILE_PATH IN LISTS CSERVICE_FILE_LIST)
        string(REGEX REPLACE "${PATH_PREFIX}" "" LIB_NAME "${FILE_PATH}")
        string(REGEX REPLACE ".c$" "" LIB_NAME "${LIB_NAME}")
        add_library(${LIB_NAME} SHARED ${FILE_PATH})
        target_include_directories(${LIB_NAME} PRIVATE core/skynet/skynet-src)
        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            target_link_libraries(${LIB_NAME} ${PLATFORM_LINK_LIBS} libskynet)
        else()
            target_link_libraries(${LIB_NAME} ${PLATFORM_LINK_LIBS})
        endif()
        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            set_target_properties(${LIB_NAME} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/cservice
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cservice
                PREFIX ""
                SUFFIX .so
                ENABLE_EXPORTS ON
            )
        else()
            set_target_properties(${LIB_NAME} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cservice
                PREFIX ""
                SUFFIX .so
            )
        endif()
    endforeach()
endfunction()

add_cservice_from_dir("core/skynet/service-src" ".*core/skynet/service-src/service_")
add_cservice_from_dir("cservice-src" ".*cservice-src/service_")

# 拷贝其他文件
message(STATUS "The value of CMAKE_SOURCE_DIR is: ${CMAKE_SOURCE_DIR}")
file(COPY ${CMAKE_SOURCE_DIR}/core/skynet/lualib/ DESTINATION lualib NO_SOURCE_PERMISSIONS)
file(COPY ${CMAKE_SOURCE_DIR}/core/skynet/test/ DESTINATION test NO_SOURCE_PERMISSIONS)
file(COPY ${CMAKE_SOURCE_DIR}/core/skynet/service/ DESTINATION service NO_SOURCE_PERMISSIONS)
file(COPY ${CMAKE_SOURCE_DIR}/core/skynet/examples/ DESTINATION examples NO_SOURCE_PERMISSIONS)
