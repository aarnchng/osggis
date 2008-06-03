/*
** Lua binding: Lua_ScriptEngine_tolua
** Generated automatically by tolua 5.1b on 05/29/08 18:00:05.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua.h"


#include <osgGIS/Attribute>
#include <osgGIS/Feature>
#include <osgGIS/FilterEnv>
#include <osgGIS/Property>
#include <osgGIS/Resource>
#include <osgGIS/ResourceLibrary>
#include <osgGIS/Lua_ScriptEngine>
#include <string>
using namespace osgGIS;

extern "C" {
/* Exported function */
TOLUA_API int tolua_Lua_ScriptEngine_tolua_open (lua_State* tolua_S);
LUALIB_API int luaopen_Lua_ScriptEngine_tolua (lua_State* tolua_S);

/* function to release collected object */
#ifdef __cplusplus
 static int tolua_collect_SkinResourceQuery (lua_State* tolua_S)
{
 SkinResourceQuery* self = (SkinResourceQuery*) tolua_tousertype(tolua_S,1,0);
 delete self;
 return 0;
}
 static int tolua_collect_ResourceList_Lua (lua_State* tolua_S)
{
 ResourceList_Lua* self = (ResourceList_Lua*) tolua_tousertype(tolua_S,1,0);
 delete self;
 return 0;
}
 static int tolua_collect_GeoPoint (lua_State* tolua_S)
{
 GeoPoint* self = (GeoPoint*) tolua_tousertype(tolua_S,1,0);
 delete self;
 return 0;
}
 static int tolua_collect_Property (lua_State* tolua_S)
{
 Property* self = (Property*) tolua_tousertype(tolua_S,1,0);
 delete self;
 return 0;
}
 static int tolua_collect_Attribute (lua_State* tolua_S)
{
 Attribute* self = (Attribute*) tolua_tousertype(tolua_S,1,0);
 delete self;
 return 0;
}
 static int tolua_collect_GeoExtent (lua_State* tolua_S)
{
 GeoExtent* self = (GeoExtent*) tolua_tousertype(tolua_S,1,0);
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
 tolua_usertype(tolua_S,"FilterEnv");
 tolua_usertype(tolua_S,"SpatialReference");
 tolua_usertype(tolua_S,"ResourceLibrary_Lua");
 tolua_usertype(tolua_S,"Feature");
 tolua_usertype(tolua_S,"ResourceList_Lua");
 tolua_usertype(tolua_S,"SkinResourceQuery");
 tolua_usertype(tolua_S,"GeoPoint");
 tolua_usertype(tolua_S,"Property");
 tolua_usertype(tolua_S,"Attribute");
 tolua_usertype(tolua_S,"GeoExtent");
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
  const char* tolua_var_1 = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getAttribute'",NULL);
#endif
 {
  Attribute tolua_ret = (Attribute)  self->getAttribute(tolua_var_1);
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

/* method: getExtent of class  Feature */
static int tolua_Lua_ScriptEngine_tolua_Feature_getExtent00(lua_State* tolua_S)
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
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getExtent'",NULL);
#endif
 {
  GeoExtent tolua_ret = (GeoExtent)  self->getExtent();
 {
#ifdef __cplusplus
 void* tolua_obj = new GeoExtent(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_GeoExtent),"GeoExtent");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(GeoExtent));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"GeoExtent");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getExtent'.",&tolua_err);
 return 0;
#endif
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
  const char* tolua_ret = (const char*)  self->asString();
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'asString'.",&tolua_err);
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

/* method: isValid of class  Property */
static int tolua_Lua_ScriptEngine_tolua_Property_isValid00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Property",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Property* self = (Property*)  tolua_tousertype(tolua_S,1,0);
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

/* method: asString of class  Property */
static int tolua_Lua_ScriptEngine_tolua_Property_asString00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Property",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Property* self = (Property*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'asString'",NULL);
#endif
 {
  const char* tolua_ret = (const char*)  self->asString();
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'asString'.",&tolua_err);
 return 0;
#endif
}

/* method: asDouble of class  Property */
static int tolua_Lua_ScriptEngine_tolua_Property_asDouble00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Property",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Property* self = (Property*)  tolua_tousertype(tolua_S,1,0);
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

/* method: asInt of class  Property */
static int tolua_Lua_ScriptEngine_tolua_Property_asInt00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Property",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Property* self = (Property*)  tolua_tousertype(tolua_S,1,0);
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

/* method: asBool of class  Property */
static int tolua_Lua_ScriptEngine_tolua_Property_asBool00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"Property",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  Property* self = (Property*)  tolua_tousertype(tolua_S,1,0);
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

/* method: getExtent of class  FilterEnv */
static int tolua_Lua_ScriptEngine_tolua_FilterEnv_getExtent00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"FilterEnv",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  FilterEnv* self = (FilterEnv*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getExtent'",NULL);
#endif
 {
  GeoExtent tolua_ret = (GeoExtent)  self->getExtent();
 {
#ifdef __cplusplus
 void* tolua_obj = new GeoExtent(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_GeoExtent),"GeoExtent");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(GeoExtent));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"GeoExtent");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getExtent'.",&tolua_err);
 return 0;
#endif
}

/* method: getInputSRS of class  FilterEnv */
static int tolua_Lua_ScriptEngine_tolua_FilterEnv_getInputSRS00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"FilterEnv",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  FilterEnv* self = (FilterEnv*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getInputSRS'",NULL);
#endif
 {
  SpatialReference* tolua_ret = (SpatialReference*)  self->getInputSRS();
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"SpatialReference");
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getInputSRS'.",&tolua_err);
 return 0;
#endif
}

/* method: getProperty of class  FilterEnv */
static int tolua_Lua_ScriptEngine_tolua_FilterEnv_getProperty00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"FilterEnv",0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  FilterEnv* self = (FilterEnv*)  tolua_tousertype(tolua_S,1,0);
  const char* tolua_var_2 = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getProperty'",NULL);
#endif
 {
  Property tolua_ret = (Property)  self->getProperty(tolua_var_2);
 {
#ifdef __cplusplus
 void* tolua_obj = new Property(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_Property),"Property");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(Property));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"Property");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getProperty'.",&tolua_err);
 return 0;
#endif
}

/* method: getXMin of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_getXMin00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getXMin'",NULL);
#endif
 {
  double tolua_ret = (double)  self->getXMin();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getXMin'.",&tolua_err);
 return 0;
#endif
}

/* method: getYMin of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_getYMin00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getYMin'",NULL);
#endif
 {
  double tolua_ret = (double)  self->getYMin();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getYMin'.",&tolua_err);
 return 0;
#endif
}

/* method: getXMax of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_getXMax00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getXMax'",NULL);
#endif
 {
  double tolua_ret = (double)  self->getXMax();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getXMax'.",&tolua_err);
 return 0;
#endif
}

/* method: getYMax of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_getYMax00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getYMax'",NULL);
#endif
 {
  double tolua_ret = (double)  self->getYMax();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getYMax'.",&tolua_err);
 return 0;
#endif
}

/* method: contains of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_contains00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
  double x = ((double)  tolua_tonumber(tolua_S,2,0));
  double y = ((double)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'contains'",NULL);
#endif
 {
  bool tolua_ret = (bool)  self->contains(x,y);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'contains'.",&tolua_err);
 return 0;
#endif
}

/* method: contains of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_contains01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"GeoPoint",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
  GeoPoint p = *((GeoPoint*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'contains'",NULL);
#endif
 {
  bool tolua_ret = (bool)  self->contains(p);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 return tolua_Lua_ScriptEngine_tolua_GeoExtent_contains00(tolua_S);
}

/* method: contains of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_contains02(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"GeoExtent",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
  GeoExtent e = *((GeoExtent*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'contains'",NULL);
#endif
 {
  bool tolua_ret = (bool)  self->contains(e);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 return tolua_Lua_ScriptEngine_tolua_GeoExtent_contains01(tolua_S);
}

/* method: getArea of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_getArea00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getArea'",NULL);
#endif
 {
  double tolua_ret = (double)  self->getArea();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getArea'.",&tolua_err);
 return 0;
#endif
}

/* method: getCentroid of class  GeoExtent */
static int tolua_Lua_ScriptEngine_tolua_GeoExtent_getCentroid00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoExtent",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoExtent* self = (GeoExtent*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getCentroid'",NULL);
#endif
 {
  GeoPoint tolua_ret = (GeoPoint)  self->getCentroid();
 {
#ifdef __cplusplus
 void* tolua_obj = new GeoPoint(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_GeoPoint),"GeoPoint");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(GeoPoint));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"GeoPoint");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getCentroid'.",&tolua_err);
 return 0;
#endif
}

/* method: new of class  GeoPoint */
static int tolua_Lua_ScriptEngine_tolua_GeoPoint_new00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertable(tolua_S,1,"GeoPoint",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
 !tolua_isusertype(tolua_S,4,"SpatialReference",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  double x = ((double)  tolua_tonumber(tolua_S,2,0));
  double y = ((double)  tolua_tonumber(tolua_S,3,0));
  SpatialReference* srs = ((SpatialReference*)  tolua_tousertype(tolua_S,4,0));
 {
  GeoPoint* tolua_ret = (GeoPoint*)  new GeoPoint(x,y,srs);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"GeoPoint");
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'new'.",&tolua_err);
 return 0;
#endif
}

/* method: x of class  GeoPoint */
static int tolua_Lua_ScriptEngine_tolua_GeoPoint_x00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoPoint",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoPoint* self = (GeoPoint*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'x'",NULL);
#endif
 {
  double tolua_ret = (double)  self->x();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'x'.",&tolua_err);
 return 0;
#endif
}

/* method: y of class  GeoPoint */
static int tolua_Lua_ScriptEngine_tolua_GeoPoint_y00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoPoint",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoPoint* self = (GeoPoint*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'y'",NULL);
#endif
 {
  double tolua_ret = (double)  self->y();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'y'.",&tolua_err);
 return 0;
#endif
}

/* method: z of class  GeoPoint */
static int tolua_Lua_ScriptEngine_tolua_GeoPoint_z00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"GeoPoint",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  GeoPoint* self = (GeoPoint*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'z'",NULL);
#endif
 {
  double tolua_ret = (double)  self->z();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'z'.",&tolua_err);
 return 0;
#endif
}

/* method: transform of class  SpatialReference */
static int tolua_Lua_ScriptEngine_tolua_SpatialReference_transform00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"SpatialReference",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"GeoPoint",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  SpatialReference* self = (SpatialReference*)  tolua_tousertype(tolua_S,1,0);
  GeoPoint in = *((GeoPoint*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'transform'",NULL);
#endif
 {
  GeoPoint tolua_ret = (GeoPoint)  self->transform(in);
 {
#ifdef __cplusplus
 void* tolua_obj = new GeoPoint(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_GeoPoint),"GeoPoint");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(GeoPoint));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"GeoPoint");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'transform'.",&tolua_err);
 return 0;
#endif
}

/* method: getGeographicSRS of class  SpatialReference */
static int tolua_Lua_ScriptEngine_tolua_SpatialReference_getGeographicSRS00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"SpatialReference",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  SpatialReference* self = (SpatialReference*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getGeographicSRS'",NULL);
#endif
 {
  SpatialReference* tolua_ret = (SpatialReference*)  self->getGeographicSRS();
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"SpatialReference");
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getGeographicSRS'.",&tolua_err);
 return 0;
#endif
}

/* method: size of class  ResourceList_Lua */
static int tolua_Lua_ScriptEngine_tolua_ResourceList_Lua_size00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"ResourceList_Lua",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  ResourceList_Lua* self = (ResourceList_Lua*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'size'",NULL);
#endif
 {
  int tolua_ret = (int)  self->size();
 tolua_pushnumber(tolua_S,(double)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'size'.",&tolua_err);
 return 0;
#endif
}

/* method: getName of class  ResourceList_Lua */
static int tolua_Lua_ScriptEngine_tolua_ResourceList_Lua_getName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"ResourceList_Lua",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  ResourceList_Lua* self = (ResourceList_Lua*)  tolua_tousertype(tolua_S,1,0);
  int index = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getName'",NULL);
#endif
 {
  const char* tolua_ret = (const char*)  self->getName(index);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getName'.",&tolua_err);
 return 0;
#endif
}

/* method: setTextureHeight of class  SkinResourceQuery */
static int tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_setTextureHeight00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"SkinResourceQuery",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  SkinResourceQuery* self = (SkinResourceQuery*)  tolua_tousertype(tolua_S,1,0);
  double value = ((double)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'setTextureHeight'",NULL);
#endif
 {
  self->setTextureHeight(value);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'setTextureHeight'.",&tolua_err);
 return 0;
#endif
}

/* method: setMinTextureHeight of class  SkinResourceQuery */
static int tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_setMinTextureHeight00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"SkinResourceQuery",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  SkinResourceQuery* self = (SkinResourceQuery*)  tolua_tousertype(tolua_S,1,0);
  double value = ((double)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'setMinTextureHeight'",NULL);
#endif
 {
  self->setMinTextureHeight(value);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'setMinTextureHeight'.",&tolua_err);
 return 0;
#endif
}

/* method: setMaxTextureHeight of class  SkinResourceQuery */
static int tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_setMaxTextureHeight00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"SkinResourceQuery",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  SkinResourceQuery* self = (SkinResourceQuery*)  tolua_tousertype(tolua_S,1,0);
  double value = ((double)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'setMaxTextureHeight'",NULL);
#endif
 {
  self->setMaxTextureHeight(value);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'setMaxTextureHeight'.",&tolua_err);
 return 0;
#endif
}

/* method: setRepeatsVertically of class  SkinResourceQuery */
static int tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_setRepeatsVertically00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"SkinResourceQuery",0,&tolua_err) ||
 !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  SkinResourceQuery* self = (SkinResourceQuery*)  tolua_tousertype(tolua_S,1,0);
  bool value = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'setRepeatsVertically'",NULL);
#endif
 {
  self->setRepeatsVertically(value);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'setRepeatsVertically'.",&tolua_err);
 return 0;
#endif
}

/* method: addTag of class  SkinResourceQuery */
static int tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_addTag00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"SkinResourceQuery",0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  SkinResourceQuery* self = (SkinResourceQuery*)  tolua_tousertype(tolua_S,1,0);
  const char* value = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'addTag'",NULL);
#endif
 {
  self->addTag(value);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'addTag'.",&tolua_err);
 return 0;
#endif
}

/* method: newSkinQuery of class  ResourceLibrary_Lua */
static int tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_newSkinQuery00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"ResourceLibrary_Lua",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  ResourceLibrary_Lua* self = (ResourceLibrary_Lua*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'newSkinQuery'",NULL);
#endif
 {
  SkinResourceQuery tolua_ret = (SkinResourceQuery)  self->newSkinQuery();
 {
#ifdef __cplusplus
 void* tolua_obj = new SkinResourceQuery(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_SkinResourceQuery),"SkinResourceQuery");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(SkinResourceQuery));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"SkinResourceQuery");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'newSkinQuery'.",&tolua_err);
 return 0;
#endif
}

/* method: getSkins of class  ResourceLibrary_Lua */
static int tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_getSkins00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"ResourceLibrary_Lua",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"SkinResourceQuery",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  ResourceLibrary_Lua* self = (ResourceLibrary_Lua*)  tolua_tousertype(tolua_S,1,0);
  SkinResourceQuery query = *((SkinResourceQuery*)  tolua_tousertype(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getSkins'",NULL);
#endif
 {
  ResourceList_Lua tolua_ret = (ResourceList_Lua)  self->getSkins(query);
 {
#ifdef __cplusplus
 void* tolua_obj = new ResourceList_Lua(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_ResourceList_Lua),"ResourceList_Lua");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(ResourceList_Lua));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"ResourceList_Lua");
#endif
 }
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getSkins'.",&tolua_err);
 return 0;
#endif
}

/* method: getSkins of class  ResourceLibrary_Lua */
static int tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_getSkins01(lua_State* tolua_S)
{
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"ResourceLibrary_Lua",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  ResourceLibrary_Lua* self = (ResourceLibrary_Lua*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getSkins'",NULL);
#endif
 {
  ResourceList_Lua tolua_ret = (ResourceList_Lua)  self->getSkins();
 {
#ifdef __cplusplus
 void* tolua_obj = new ResourceList_Lua(tolua_ret);
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect_ResourceList_Lua),"ResourceList_Lua");
#else
 void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(ResourceList_Lua));
 tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,tolua_collect),"ResourceList_Lua");
#endif
 }
 }
 }
 return 1;
tolua_lerror:
 return tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_getSkins00(tolua_S);
}

/* method: getPath of class  ResourceLibrary_Lua */
static int tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_getPath00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"ResourceLibrary_Lua",0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  ResourceLibrary_Lua* self = (ResourceLibrary_Lua*)  tolua_tousertype(tolua_S,1,0);
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'getPath'",NULL);
#endif
 {
  const char* tolua_ret = (const char*)  self->getPath(name);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'getPath'.",&tolua_err);
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
 tolua_cclass(tolua_S,"Feature","Feature","",0);
#else
 tolua_cclass(tolua_S,"Feature","Feature","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"Feature");
 tolua_function(tolua_S,"getOID",tolua_Lua_ScriptEngine_tolua_Feature_getOID00);
 tolua_function(tolua_S,"getAttribute",tolua_Lua_ScriptEngine_tolua_Feature_getAttribute00);
 tolua_function(tolua_S,"getExtent",tolua_Lua_ScriptEngine_tolua_Feature_getExtent00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"Attribute","Attribute","",tolua_collect_Attribute);
#else
 tolua_cclass(tolua_S,"Attribute","Attribute","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"Attribute");
 tolua_function(tolua_S,"isValid",tolua_Lua_ScriptEngine_tolua_Attribute_isValid00);
 tolua_function(tolua_S,"asString",tolua_Lua_ScriptEngine_tolua_Attribute_asString00);
 tolua_function(tolua_S,"asDouble",tolua_Lua_ScriptEngine_tolua_Attribute_asDouble00);
 tolua_function(tolua_S,"asInt",tolua_Lua_ScriptEngine_tolua_Attribute_asInt00);
 tolua_function(tolua_S,"asBool",tolua_Lua_ScriptEngine_tolua_Attribute_asBool00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"Property","Property","",tolua_collect_Property);
#else
 tolua_cclass(tolua_S,"Property","Property","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"Property");
 tolua_function(tolua_S,"isValid",tolua_Lua_ScriptEngine_tolua_Property_isValid00);
 tolua_function(tolua_S,"asString",tolua_Lua_ScriptEngine_tolua_Property_asString00);
 tolua_function(tolua_S,"asDouble",tolua_Lua_ScriptEngine_tolua_Property_asDouble00);
 tolua_function(tolua_S,"asInt",tolua_Lua_ScriptEngine_tolua_Property_asInt00);
 tolua_function(tolua_S,"asBool",tolua_Lua_ScriptEngine_tolua_Property_asBool00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"FilterEnv","FilterEnv","",0);
#else
 tolua_cclass(tolua_S,"FilterEnv","FilterEnv","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"FilterEnv");
 tolua_function(tolua_S,"getExtent",tolua_Lua_ScriptEngine_tolua_FilterEnv_getExtent00);
 tolua_function(tolua_S,"getInputSRS",tolua_Lua_ScriptEngine_tolua_FilterEnv_getInputSRS00);
 tolua_function(tolua_S,"getProperty",tolua_Lua_ScriptEngine_tolua_FilterEnv_getProperty00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"GeoExtent","GeoExtent","",tolua_collect_GeoExtent);
#else
 tolua_cclass(tolua_S,"GeoExtent","GeoExtent","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"GeoExtent");
 tolua_function(tolua_S,"getXMin",tolua_Lua_ScriptEngine_tolua_GeoExtent_getXMin00);
 tolua_function(tolua_S,"getYMin",tolua_Lua_ScriptEngine_tolua_GeoExtent_getYMin00);
 tolua_function(tolua_S,"getXMax",tolua_Lua_ScriptEngine_tolua_GeoExtent_getXMax00);
 tolua_function(tolua_S,"getYMax",tolua_Lua_ScriptEngine_tolua_GeoExtent_getYMax00);
 tolua_function(tolua_S,"contains",tolua_Lua_ScriptEngine_tolua_GeoExtent_contains00);
 tolua_function(tolua_S,"contains",tolua_Lua_ScriptEngine_tolua_GeoExtent_contains01);
 tolua_function(tolua_S,"contains",tolua_Lua_ScriptEngine_tolua_GeoExtent_contains02);
 tolua_function(tolua_S,"getArea",tolua_Lua_ScriptEngine_tolua_GeoExtent_getArea00);
 tolua_function(tolua_S,"getCentroid",tolua_Lua_ScriptEngine_tolua_GeoExtent_getCentroid00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"GeoPoint","GeoPoint","",tolua_collect_GeoPoint);
#else
 tolua_cclass(tolua_S,"GeoPoint","GeoPoint","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"GeoPoint");
 tolua_function(tolua_S,"new",tolua_Lua_ScriptEngine_tolua_GeoPoint_new00);
 tolua_function(tolua_S,"x",tolua_Lua_ScriptEngine_tolua_GeoPoint_x00);
 tolua_function(tolua_S,"y",tolua_Lua_ScriptEngine_tolua_GeoPoint_y00);
 tolua_function(tolua_S,"z",tolua_Lua_ScriptEngine_tolua_GeoPoint_z00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"SpatialReference","SpatialReference","",0);
#else
 tolua_cclass(tolua_S,"SpatialReference","SpatialReference","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"SpatialReference");
 tolua_function(tolua_S,"transform",tolua_Lua_ScriptEngine_tolua_SpatialReference_transform00);
 tolua_function(tolua_S,"getGeographicSRS",tolua_Lua_ScriptEngine_tolua_SpatialReference_getGeographicSRS00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"ResourceList_Lua","ResourceList_Lua","",tolua_collect_ResourceList_Lua);
#else
 tolua_cclass(tolua_S,"ResourceList_Lua","ResourceList_Lua","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"ResourceList_Lua");
 tolua_function(tolua_S,"size",tolua_Lua_ScriptEngine_tolua_ResourceList_Lua_size00);
 tolua_function(tolua_S,"getName",tolua_Lua_ScriptEngine_tolua_ResourceList_Lua_getName00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"SkinResourceQuery","SkinResourceQuery","",tolua_collect_SkinResourceQuery);
#else
 tolua_cclass(tolua_S,"SkinResourceQuery","SkinResourceQuery","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"SkinResourceQuery");
 tolua_function(tolua_S,"setTextureHeight",tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_setTextureHeight00);
 tolua_function(tolua_S,"setMinTextureHeight",tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_setMinTextureHeight00);
 tolua_function(tolua_S,"setMaxTextureHeight",tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_setMaxTextureHeight00);
 tolua_function(tolua_S,"setRepeatsVertically",tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_setRepeatsVertically00);
 tolua_function(tolua_S,"addTag",tolua_Lua_ScriptEngine_tolua_SkinResourceQuery_addTag00);
 tolua_endmodule(tolua_S);
#ifdef __cplusplus
 tolua_cclass(tolua_S,"ResourceLibrary_Lua","ResourceLibrary_Lua","",0);
#else
 tolua_cclass(tolua_S,"ResourceLibrary_Lua","ResourceLibrary_Lua","",tolua_collect);
#endif
 tolua_beginmodule(tolua_S,"ResourceLibrary_Lua");
 tolua_function(tolua_S,"newSkinQuery",tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_newSkinQuery00);
 tolua_function(tolua_S,"getSkins",tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_getSkins00);
 tolua_function(tolua_S,"getSkins",tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_getSkins01);
 tolua_function(tolua_S,"getPath",tolua_Lua_ScriptEngine_tolua_ResourceLibrary_Lua_getPath00);
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


} // extern "C"
