//
// Created by Vermouth on 2024/1/24.
//

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdatomic.h>
static atomic_bool input_ready = ATOMIC_VAR_INIT(FALSE);  // 原子变量，表示输入是否准备好
static BOOL input_end = FALSE;
static char input[256];                                   // 存储用户输入的全局变量
static HANDLE input_thread;

//static int hInConsoleEvents(HANDLE hIn){
//    //定义事件结构体
//    INPUT_RECORD record[128];
//
//    //存储记录
//    DWORD res;
//
//    GetNumberOfConsoleInputEvents(hIn, &res);
//    int flag = 0;
//    if (res){
//        //PeekConsoleInput //ReadConsoleInput
//        PeekConsoleInput(hIn, record, 128, &res);
//        for (int i = 0; i < res; ++i) {
//            if (record[i].EventType == KEY_EVENT && 13 == record[i].Event.KeyEvent.uChar.UnicodeChar){
////        record[i].Event.KeyEvent.uChar.UnicodeChar;
//                flag = 1;
//                break;
//            }
//        }
//    }
//
//    return flag;
//}
//
//static int lstdinEvent(lua_State *L) {
//    void* handle = NULL;
//    if(lua_islightuserdata(L, 1)){
//        handle = lua_touserdata(L, 1);
//    }
//
//    int event_end = 0;
//    if(handle) {
//        event_end = hInConsoleEvents(handle);
//    }
//    lua_pushnumber(L, event_end);
//    return 1;
//}
//
//static int lopenStdin(lua_State *L) {
//    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
//    if(hIn ==NULL){
//        lua_pushnil(L);
//    }else{
//        lua_pushlightuserdata(L, hIn);
//    }
//    return 1;
//}
//
//static int lopenHandle(lua_State *L) {
//
//    return 1;
//}
//
//static int lflushHandle(lua_State *L) {
//    void* handle = NULL;
//    if(lua_islightuserdata(L, 1)){
//        handle = lua_touserdata(L, 1);
//    }
//    if(handle) {
//        FlushConsoleInputBuffer(handle);
//    }
//    return 1;
//}
//
//static int lcloseHandle(lua_State *L) {
//    void* handle = NULL;
//    if(lua_islightuserdata(L, 1)){
//        handle = lua_touserdata(L, 1);
//    }
//    if(handle) {
//        CloseHandle(handle);
//    }
//    return 1;
//}

static DWORD WINAPI readInput(LPVOID lpParam) {
    while(!input_end){
        fgets(input, sizeof(input), stdin);  // 阻塞读取输入
        input_ready = TRUE;                  // 标记输入已准备好
    }
    return 0;
}

static int linitWinThread(lua_State *L){
    input_thread = CreateThread(
            NULL,          // 默认安全属性
            0,             // 默认堆栈大小
            readInput,     // 线程函数
            NULL,          // 参数
            0,             // 立即运行线程
            NULL           // 不需要线程ID
    );
    input_ready = FALSE;
    if (input_thread == NULL) {
        fprintf(stderr, "无法创建线程\n");
        return 1;
    }
    return 1;
}

static int lgetWinThreadResultFlag(lua_State *L){
    if(input_ready){
        lua_pushstring(L, input);
    }else{
        lua_pushnil(L);
    }
    input_ready = FALSE;
    return 1;
}

static int lcloseWinThread(lua_State *L){
    input_end = TRUE;
    WaitForSingleObject(input_thread, INFINITE);
    CloseHandle(input_thread);
}

static const luaL_Reg l[] = {
//        {"stdinEvent", lstdinEvent},
//        {"openStdin", lopenStdin},
//        {"closeHandle", lcloseHandle},
//        {"flushHandle",lflushHandle},
        {"initWinThread",linitWinThread},
        {"getWinThreadResultFlag",lgetWinThreadResultFlag},
        {"closeWinThread",lcloseWinThread},
        {NULL, NULL},
};

int luaopen_winhelp(lua_State *L) {
    luaL_newlib(L, l);
    return 1;
}