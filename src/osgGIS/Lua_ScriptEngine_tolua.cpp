/*
** Lua binding: Lua_ScriptEngine_tolua
** Generated automatically by tolua 5.1b on 01/03/08 20:15:22.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include <osgGIS/Attribute>
#include <osgGIS/Feature>
#include <osgGIS/FilterEnv>
#include <osgGIS/Resource>
#include <osgGIS/ResourceLibrary>
#include <string>
using namespace osgGIS;

extern "C" {
#include "tolua.h"

/* Exported function */
TOLUA_API int tolua_Lua_ScriptEngine_tolua_open (lua_State* tolua_S);
LUALIB_API int luaopen_Lua_ScriptEngine_tolua (lua_State* tolua_S);

/* function to release collected object */
#ifdef __cplusplus
 static int tolua_collect_Attribute (lua_State* tolua_S)
{
 Attribute* self = (Attribute*) tolua_tousertype(tolua_S,1,0);
 delete self;
 return 0;
}
 static int tolua_collect_std_string (lua_State* tolua_S)
{
 std::string* self = (std::string*) tolua_tousertype(tolua_S,1,0);
 delete self;
 return 0;
}
#else
static int tolua_collect (lua_State* tolua_S)
{
 void* self = tolua_tousertype(tolua_S,1,0);
 free(self);
 return 0;
}
#endif


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"Feature");
 tolua_usertype(tolua_S,"std::string");
 tolua_usertype(tolua_S,"Attribute");
 tolua_usertype(tolua_S,"FilterEnv");
}

/* method: isValid of class  Attribute */
static int tolua_Lua_ScriptEngine_tolua_Attribute_isValid00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Attribute",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Attribute* self = (Attribute*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'isValid'",NULL);
#endif
 {
  bool tolua_ret = (bool)  self->isValid();
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'isValid'.",&tolua_err);
 return 0;
#endif
}

/* method: asString of class  Attribute */
static int tolua_Lua_ScriptEngine_tolua_Attribute_asString00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Attribute",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Attribute* self = (Attribute*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'asString'",NULL);
#endif
 {
  std::string tolua_ret = (std::string)  self->asString();
 {
#ifdef __cplusplus
 void* tolua_obj = new std::string(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_std_string),"std::string");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::string));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"std::string");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'asString'.",&tolua_err);
 return 0;
#endif
}

/* method: asInt of class  Attribute */
static int tolua_Lua_ScriptEngine_tolua_Attribute_asInt00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Attribute",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Attribute* self = (Attribute*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'asInt'",NULL);
#endif
 {
  int tolua_ret = (int)  self->asInt();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'asInt'.",&tolua_err);
 return 0;
#endif
}

/* method: asDouble of class  Attribute */
static int tolua_Lua_ScriptEngine_tolua_Attribute_asDouble00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Attribute",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Attribute* self = (Attribute*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'asDouble'",NULL);
#endif
 {
  double tolua_ret = (double)  self->asDouble();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'asDouble'.",&tolua_err);
 return 0;
#endif
}

/* method: asBool of class  Attribute */
static int tolua_Lua_ScriptEngine_tolua_Attribute_asBool00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Attribute",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Attribute* self = (Attribute*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'asBool'",NULL);
#endif
 {
  bool tolua_ret = (bool)  self->asBool();
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'asBool'.",&tolua_err);
 return 0;
#endif
}

/* method: getOID of class  Feature */
static int tolua_Lua_ScriptEngine_tolua_Feature_getOID00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Feature",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Feature* self = (Feature*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getOID'",NULL);
#endif
 {
  int tolua_ret = (int)  self->getOID();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getOID'.",&tolua_err);
 return 0;
#endif
}

/* method: getAttributeAsDouble of class  Feature */
static int tolua_Lua_ScriptEngine_tolua_Feature_getAttributeAsDouble00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Feature",0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Feature* self = (Feature*)  tolua_tousertype(tolua_S,1,0);
  const char* tolua_var_1 = ((const char*)  tolua_tostring(tolua_S,2,0));
  double tolua_var_2 = ((double)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getAttributeAsDouble'",NULL);
#endif
 {
  double tolua_ret = (double)  self->getAttributeAsDouble(tolua_var_1,tolua_var_2);
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getAttributeAsDouble'.",&tolua_err);
 return 0;
#endif
}

/* method: getAttribute of class  Feature */
static int tolua_Lua_ScriptEngine_tolua_Feature_getAttribute00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Feature",0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Feature* self = (Feature*)  tolua_tousertype(tolua_S,1,0);
  const char* tolua_var_3 = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getAttribute'",NULL);
#endif
 {
  Attribute tolua_ret = (Attribute)  self->getAttribute(tolua_var_3);
 {
#ifdef __cplusplus
 void* tolua_obj = new Attribute(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_Attribute),"Attribute");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(Attribute));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"Attribute");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getAttribute'.",&tolua_err);
 return 0;
#endif
}

/* Open lib function */
LUALIB_API int luaopen_Lua_ScriptEngine_tolua (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"Attribute","Attribute","",tolua_collect_Attribute);
#else
 tolua_cclass(tolua_S,"Attribute","Attribute","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"Attribute");
 tolua_function(tolua_S,"isValid",tolua_Lua_ScriptEngine_tolua_Attribute_isValid00);
 tolua_function(tolua_S,"asString",tolua_Lua_ScriptEngine_tolua_Attribute_asString00);
 tolua_function(tolua_S,"asInt",tolua_Lua_ScriptEngine_tolua_Attribute_asInt00);
 tolua_function(tolua_S,"asDouble",tolua_Lua_ScriptEngine_tolua_Attribute_asDouble00);
 tolua_function(tolua_S,"asBool",tolua_Lua_ScriptEngine_tolua_Attribute_asBool00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"Feature","Feature","",0);
#else
 tolua_cclass(tolua_S,"Feature","Feature","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"Feature");
 tolua_function(tolua_S,"getOID",tolua_Lua_ScriptEngine_tolua_Feature_getOID00);
 tolua_function(tolua_S,"getAttributeAsDouble",tolua_Lua_ScriptEngine_tolua_Feature_getAttributeAsDouble00);
 tolua_function(tolua_S,"getAttribute",tolua_Lua_ScriptEngine_tolua_Feature_getAttribute00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"FilterEnv","FilterEnv","",0);
#else
 tolua_cclass(tolua_S,"FilterEnv","FilterEnv","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"FilterEnv");
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}
/* Open tolua function */
TOLUA_API int tolua_Lua_ScriptEngine_tolua_open (lua_State* tolua_S)
{
 lua_pushcfunction(tolua_S, luaopen_Lua_ScriptEngine_tolua);
 lua_pushstring(tolua_S, "Lua_ScriptEngine_tolua");
 lua_call(tolua_S, 1, 0);
 return 1;
}

}