#include <stdio.h>
#include <stdlib.h>
#include <luaT.h>
#include <string.h>

#include "THThread.h"
#include "luaTHRD.h"

#include <lua.h>
#include <lualib.h>

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

static int newthread(void *code_)
{
  char *code = code_;
  lua_State *L = luaL_newstate();

  if(!L) {
    printf("THREAD FATAL ERROR: could not create lua state\n");
    return -1;
  }
  luaL_openlibs(L);

  if(luaL_loadstring(L, code)) {
    printf("FATAL THREAD PANIC: (loadstring) %s\n", lua_tolstring(L, -1, NULL));
    free(code);
    lua_close(L);
    return -1;
  }
  free(code);
  if(lua_pcall(L, 0, 0, 0)) {
    printf("FATAL THREAD PANIC: (pcall) %s\n", lua_tolstring(L, -1, NULL));
    lua_close(L);
    return -1;
  }

  lua_close(L);
  return 0;
}

static int thread_new(lua_State *L)
{
  THThread *thread = NULL;
  size_t len = 0;
  const char *code = luaL_checklstring(L, 1, &len);
  char *code_dup = malloc(len+1);
  if(!code_dup)
    luaL_error(L, "threads: out of memory");
  memcpy(code_dup, code, len+1);

  thread = THThread_new(newthread, (void*)code_dup);
  if(!thread)
    luaL_error(L, "threads: thread new failed");
  luaTHRD_pushudata(L, thread, "threads.Thread");

  return 1;
}

static int thread_tostring(lua_State *L)
{
  char str[128];
  THThread *thread = luaTHRD_checkudata(L, 1, "threads.Thread");
  snprintf(str, 128, "threads.Thread <%lx>", THThread_id(thread));
  lua_pushstring(L, str);
  return 1;
}

static int thread_id(lua_State *L)
{
  THThread *thread = luaTHRD_checkudata(L, 1, "threads.Thread");
  lua_pushinteger(L, THThread_id(thread));
  return 1;
}

static int thread_free(lua_State *L)
{
  THThread *thread = luaTHRD_checkudata(L, 1, "threads.Thread");
  THThread_free(thread);
  return 0;
}

static int mutex_new(lua_State *L)
{
  THMutex *mutex = NULL;
  if(lua_gettop(L) == 0) {
    mutex = THMutex_new();
  }
  else if(lua_gettop(L) == 1) {
    long id = luaL_checklong(L, 1);
    mutex = THMutex_newWithId(id);
  }
  else
    luaL_error(L, "threads: mutex new invalid arguments");
  if(!mutex)
    luaL_error(L, "threads: mutex new failed");
  luaTHRD_pushudata(L, mutex, "threads.Mutex");
  return 1;
}

static int mutex_tostring(lua_State *L)
{
  char str[128];
  THMutex *mutex = luaTHRD_checkudata(L, 1, "threads.Mutex");
  snprintf(str, 128, "threads.Mutex <%lx>", THMutex_id(mutex));
  lua_pushstring(L, str);
  return 1;
}

static int mutex_id(lua_State *L)
{
  THMutex *mutex = luaTHRD_checkudata(L, 1, "threads.Mutex");
  lua_pushinteger(L, THMutex_id(mutex));
  return 1;
}

static int mutex_lock(lua_State *L)
{
  THMutex *mutex = luaTHRD_checkudata(L, 1, "threads.Mutex");
  if(THMutex_lock(mutex))
    luaL_error(L, "threads: mutex lock failed");
  return 0;
}

static int mutex_unlock(lua_State *L)
{
  THMutex *mutex = luaTHRD_checkudata(L, 1, "threads.Mutex");
  if(THMutex_unlock(mutex))
    luaL_error(L, "threads: mutex unlock failed");
  return 0;
}

static int mutex_free(lua_State *L)
{
  THMutex *mutex = luaTHRD_checkudata(L, 1, "threads.Mutex");
  THMutex_free(mutex);
  return 0;
}

static int condition_new(lua_State *L)
{
  THCondition *condition = NULL;
  if(lua_gettop(L) == 0) {
    condition = THCondition_new();
  }
  else if(lua_gettop(L) == 1) {
    long id = luaL_checklong(L, 1);
    condition = THCondition_newWithId(id);
  }
  else
    luaL_error(L, "threads: condition new invalid arguments");
  if(!condition)
    luaL_error(L, "threads: condition new failed");
  luaTHRD_pushudata(L, condition, "threads.Condition");
  return 1;
}

static int condition_tostring(lua_State *L)
{
  char str[128];
  THCondition *condition = luaTHRD_checkudata(L, 1, "threads.Condition");
  snprintf(str, 128, "threads.Condition <%lx>", THCondition_id(condition));
  lua_pushstring(L, str);
  return 1;
}

static int condition_id(lua_State *L)
{
  THCondition *condition = luaTHRD_checkudata(L, 1, "threads.Condition");
  lua_pushinteger(L, THCondition_id(condition));
  return 1;
}

static int condition_free(lua_State *L)
{
  THCondition *condition = luaTHRD_checkudata(L, 1, "threads.Condition");
  if(!condition)
    luaL_error(L, "threads: condition free failed");
  return 0;
}

static int condition_signal(lua_State *L)
{
  THCondition *condition = luaTHRD_checkudata(L, 1, "threads.Condition");
  if(THCondition_signal(condition))
    luaL_error(L, "threads: condition signal failed");
  return 0;
}

static int condition_wait(lua_State *L)
{
  THCondition *condition = luaTHRD_checkudata(L, 1, "threads.Condition");
  THMutex *mutex = luaTHRD_checkudata(L, 2, "threads.Mutex");
  if(THCondition_wait(condition, mutex))
    luaL_error(L, "threads: condition wait failed");
  return 0;
}

static const struct luaL_Reg thread__ [] = {
  {"new", thread_new},
  {"__tostring", thread_tostring},
  {"id", thread_id},
  {"free", thread_free},
  {NULL, NULL}
};

static const struct luaL_Reg mutex__ [] = {
  {"new", mutex_new},
  {"__tostring", mutex_tostring},
  {"id", mutex_id},
  {"lock", mutex_lock},
  {"unlock", mutex_unlock},
  {"free", mutex_free},
  {NULL, NULL}
};

static const struct luaL_Reg condition__ [] = {
  {"new", condition_new},
  {"__tostring", condition_tostring},
  {"id", condition_id},
  {"signal", condition_signal},
  {"wait", condition_wait},
  {"free", condition_free},
  {NULL, NULL}
};

static void thread_init_pkg(lua_State *L)
{
  if(!luaL_newmetatable(L, "threads.Thread"))
    luaL_error(L, "threads: threads.Thread type already exists");
  luaL_setfuncs(L, thread__, 0);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_rawset(L, -3);
  lua_pop(L, 1);

  if(!luaL_newmetatable(L, "threads.Mutex"))
    luaL_error(L, "threads: threads.Mutex type already exists");
  luaL_setfuncs(L, mutex__, 0);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_rawset(L, -3);
  lua_pop(L, 1);

  if(!luaL_newmetatable(L, "threads.Condition"))
    luaL_error(L, "threads: threads.Condition type already exists");
  luaL_setfuncs(L, condition__, 0);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_rawset(L, -3);
  lua_pop(L, 1);

  lua_pushstring(L, "Thread");
  luaTHRD_pushctortable(L, thread_new, "threads.Thread");
  lua_rawset(L, -3);

  lua_pushstring(L, "Mutex");
  luaTHRD_pushctortable(L, mutex_new, "threads.Mutex");
  lua_rawset(L, -3);

  lua_pushstring(L, "Condition");
  luaTHRD_pushctortable(L, condition_new, "threads.Condition");
  lua_rawset(L, -3);
}
