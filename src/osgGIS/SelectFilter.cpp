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

#include <osgGIS/SelectFilter>
#include <osgGIS/Script>
#include <osg/Notify>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( SelectFilter );


SelectFilter::SelectFilter()
{
    //NOP
}

SelectFilter::SelectFilter( const SelectFilter& rhs )
: FeatureFilter( rhs ),
  select_script( rhs.select_script.get() )
{
    //NOP
}

SelectFilter::~SelectFilter()
{
    //NOP
}

void
SelectFilter::setSelectScript( Script* value )
{
    select_script = value;
}

Script*
SelectFilter::getSelectScript() const
{
    return select_script.get();
}

void
SelectFilter::setProperty( const Property& p )
{
    if ( p.getName() == "select" )
        setSelectScript( new Script( p.getValue() ) );
    FeatureFilter::setProperty( p );
}

Properties
SelectFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getSelectScript() )
        p.push_back( Property( "select", getSelectScript()->getCode() ) );
    return p;
}

FeatureList
SelectFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    if ( getSelectScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getSelectScript(), input, env );

        if ( r.isValid() && r.asBool( false ) )
            output.push_back( input );
    }

    return output;
}

