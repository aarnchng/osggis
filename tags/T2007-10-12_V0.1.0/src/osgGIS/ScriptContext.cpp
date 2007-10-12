/**
 * osgGIS - GIS Library for OpenSceneGraph
 * Copyright 2007 Glenn Waldron and Pelican Ventures, Inc.
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

#include <osgGIS/ScriptContext>

using namespace osgGIS;

ScriptContext::ScriptContext()
{
}


ScriptContext::ScriptContext( const ScriptContext& rhs )
{
    //filter_env_stack = rhs.filter_env_stack;
}


ScriptContext::~ScriptContext()
{
    //NOP
}


//void
//ScriptContext::pushEnv( const FilterEnv& env )
//{
//    filter_env_stack.push( env );
//}
//
//
//void
//ScriptContext::pushEnv()
//{
//    if ( filter_env_stack.size() > 0 )
//        filter_env_stack.push( filter_env_stack.top() );
//}
//
//
//void
//ScriptContext::popEnv()
//{
//    if ( filter_env_stack.size() > 0 )
//        filter_env_stack.pop();
//}
//
//
//FilterEnv&
//ScriptContext::getEnv()
//{
//    if ( filter_env_stack.size() == 0 )
//        filter_env_stack.push( FilterEnv() );
//    return filter_env_stack.top();
//}