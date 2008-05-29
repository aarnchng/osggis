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
  group_script( rhs.group_script.get() )
{
    //NOP
}

GroupFilter::~GroupFilter()
{
    //NOP
}

void
GroupFilter::setGroupScript( Script* value )
{
    group_script = value;
}

Script*
GroupFilter::getGroupScript() const
{
    return group_script.get();
}

void
GroupFilter::setProperty( const Property& p )
{
    if ( p.getName() == "group" )
        setGroupScript( new Script( p.getValue() ) );
    CollectionFilter::setProperty( p );
}

Properties
GroupFilter::getProperties() const
{
    Properties p = CollectionFilter::getProperties();
    if ( getGroupScript() )
        p.push_back( Property( "group", getGroupScript()->getCode() ) );
    return p;
}


std::string 
GroupFilter::assign( Feature* input, FilterEnv* env )
{
    if ( getGroupScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getGroupScript(), input, env );
        if ( r.isValid() )
            return r.asString();
    }
    return "";
}
