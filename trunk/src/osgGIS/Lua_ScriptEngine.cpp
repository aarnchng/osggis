/**
/* osgGIS - GIS Library for OpenSceneGraph
 * Copyright 2007-2008 Glenn Waldron and Pelican Ventures, Inc.
 * http://osggis.org
 *
 * osgGIS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <osgGIS/Lua_ScriptEngine>
#include <osgGIS/FilterEnv>
#include <osg/Notify>
#include <sstream>
extern "C" {
#include "tolua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <osgGIS/Lua_ScriptEngine_tolua>
}

using namespace osgGIS;

Lua_ScriptEngine::Lua_ScriptEngine()
{
    L = lua_open();

    tolua_Lua_ScriptEngine_tolua_open( L ); //todo: global

    luaopen_base( L );
    luaopen_string( L );
    luaopen_math( L );

    // install built-in scripts
    install( new Script( "function color(a,b,c,d) return string.format(\"%f %f %f %f\",a,b,c,d); end" ) );
    install( new Script( "function vec4(a,b,c,d) return string.format(\"%f %f %f %f\",a,b,c,d); end" ) );
    install( new Script( "function vec3(a,b,c) return string.format(\"%f %f %f\",a,b,c); end" ) );
    install( new Script( "function vec2(a,b) return string.format(\"%f %f\",a,b); end" ) );
    install( new Script( "function attr_double(f,a) return f:getAttribute(a):asDouble(); end" ) );
    install( new Script( "function attr_string(f,a) return f:getAttribute(a):asString(); end" ) );
    install( new Script( "function attr_int(f,a) return f:getAttribute(a):asInt(); end" ) );
    install( new Script( "function attr_bool(f,a) return f:getAttribute(a):asBool(); end" ) );
}

Lua_ScriptEngine::~Lua_ScriptEngine()
{
    lua_close(L);
}

void 
Lua_ScriptEngine::install( Script* script )
{
    scripts.push_back( script );
} 

ScriptResult 
Lua_ScriptEngine::run( Script* script )
{
    std::stringstream stream;

    for( ScriptList::iterator i = scripts.begin(); i != scripts.end(); i++ )
        stream << i->get()->getCode() << std::endl;

    stream << script->getCode() << std::endl;
    std::string code = stream.str();

    bool ok = false;

    std::stringstream result;

    if ( luaL_loadstring( L, code.c_str() ) == 0 )
    {
        if ( lua_pcall( L, 0, 1, 0 ) == 0 )
        {
            const char* top = lua_tostring( L, lua_gettop(L) );
            if ( top )
                result << top;
            ok = true;
        }
        else
        {
            result << "Error in LUA script; from: " << script->getCode();
        }
        lua_pop( L, 1 );
    }
    else
    {
        result << "Error in LUA script; from: " << script->getCode();
        lua_pop( L, 1 );
    }

    if ( !ok )
    {
        osgGIS::notify(osg::WARN) << result.str() << std::endl;
    }

    return ok? ScriptResult( result.str() ) : ScriptResult::Error( result.str() );
}

ScriptResult 
Lua_ScriptEngine::run( Script* script, FilterEnv* env )
{
    std::stringstream stream;

    for( ScriptList::iterator i = scripts.begin(); i != scripts.end(); i++ )
        stream << i->get()->getCode() << std::endl;

    stream << "function feature_filter_batch_entry(env,resources)" << std::endl;
    stream << "  return " << script->getCode() << std::endl;
    stream << "end" << std::endl;
    std::string code = stream.str();

    bool ok = false;
    std::stringstream result;

    if ( luaL_loadstring( L, code.c_str() ) == 0 ) // loads the script
    {
        lua_pcall( L, 0, 0, 0 ); // to define the functions

        ResourceLibrary_Lua* reslib_wrapper = new ResourceLibrary_Lua(env->getSession()->getResources());

        lua_getglobal( L, "feature_filter_batch_entry" ); // loads the function to call
        tolua_pushusertype( L, env, "FilterEnv" ); // pushed the second argument
        tolua_pushusertype( L, reslib_wrapper, "ResourceLibrary_Lua" );
        if ( lua_pcall( L, 2, 1, 0 ) == 0 ) // calls the function with 2 in, 1 out
        {
            const char* top = lua_tostring( L, lua_gettop( L ) );
            if ( top )
                result << top;
            ok = true;
            lua_pop( L, 1 );
        }
        else
        {
            int error = lua_gettop( L );
            lua_pop( L, 1 );
            result << "Error in LUA script; from: " << script->getCode();
        }
        //lua_pop( L, 1 ); // pop the script
        
        delete reslib_wrapper;
    }
    else
    {
        result << "Error in LUA script; from: " << script->getCode();
        lua_pop( L, 1 );
    }

    if ( !ok )
    {
        osgGIS::notify(osg::WARN) << result.str() << std::endl;
    }

    return ok? ScriptResult( result.str() ) : ScriptResult::Error( result.str() );
}

ScriptResult 
Lua_ScriptEngine::run( Script* script, Feature* feature, FilterEnv* env )
{
    std::stringstream stream;

    for( ScriptList::iterator i = scripts.begin(); i != scripts.end(); i++ )
        stream << i->get()->getCode() << std::endl;

    stream << "function feature_filter_entry(feature, env, resources)" << std::endl;
    stream << "  return " << script->getCode() << std::endl;
    stream << "end" << std::endl;
    std::string code = stream.str();

    bool ok = false;
    std::stringstream result;

    if ( luaL_loadstring( L, code.c_str() ) == 0 ) // loads the script
    {
        lua_pcall( L, 0, 0, 0 ); // to define the functions

        ResourceLibrary_Lua* reslib_wrapper = new ResourceLibrary_Lua(env->getSession()->getResources());

        lua_getglobal( L, "feature_filter_entry" ); // loads the function to call
        tolua_pushusertype( L, feature, "Feature" ); // pushes the first argument
        tolua_pushusertype( L, env, "FilterEnv" ); // pushed the second argument
        tolua_pushusertype( L, reslib_wrapper, "ResourceLibrary_Lua" );
        if ( lua_pcall( L, 3, 1, 0 ) == 0 ) // calls the function with 2 in, 1 out
        {
            if ( lua_isboolean( L, lua_gettop( L ) ) )
            {
                bool rv = lua_toboolean( L, lua_gettop( L ) )? true : false;
                result << (rv? "true" : "false");
            }
            else
            {
                const char* top = lua_tostring( L, lua_gettop( L ) );
                if ( top )
                    result << top;
            }
            ok = true;
            lua_pop( L, 1 );
        }
        else
        {
            int error = lua_gettop( L );
            lua_pop( L, 1 );
            result << "Error in LUA script; from: " << script->getCode();
        }

        delete reslib_wrapper;
    }
    else
    {
        result << "Error in LUA script; from: " << script->getCode();
        lua_pop( L, 1 );
    }

    if ( !ok )
    {
        osgGIS::notify(osg::WARN) << result.str() << std::endl;
    }

    return ok? ScriptResult( result.str() ) : ScriptResult::Error( result.str() );
}

