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

#include <osgGIS/GroupFilter>
#include <osg/Notify>
#include <sstream>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( GroupFilter );


GroupFilter::GroupFilter()
{
    //NOP
}

GroupFilter::GroupFilter( const GroupFilter& rhs )
: CollectionFilter( rhs ),
  group_expr( rhs.group_expr )
{
    //NOP
}

GroupFilter::~GroupFilter()
{
    //NOP
}

void
GroupFilter::setGroupExpr( const std::string& value )
{
    group_expr = value;
}

const std::string&
GroupFilter::getGroupExpr() const
{
    return group_expr;
}

void
GroupFilter::setProperty( const Property& p )
{
    if ( p.getName() == "group" )
        setGroupExpr( p.getValue() );
    CollectionFilter::setProperty( p );
}

Properties
GroupFilter::getProperties() const
{
    Properties p = CollectionFilter::getProperties();
    if ( getGroupExpr().length() > 0 )
        p.push_back( Property( "group", getGroupExpr() ) );
    return p;
}


std::string 
GroupFilter::assign( Feature* input, FilterEnv* env )
{
    if ( getGroupExpr().length() > 0 )
    {
        //TODO: new Script() is a memory leak in the following line!!
        ScriptResult r = env->getScriptEngine()->run( new Script( getGroupExpr() ), input, env );
        if ( r.isValid() )
            return r.asString();
    }
    return "";
}
