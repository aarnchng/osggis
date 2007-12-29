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

#include <osgGIS/RandomGroupingFilter>
#include <osg/Notify>
#include <sstream>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( RandomGroupingFilter );


#define DEFAULT_NUM_GROUPS 5


RandomGroupingFilter::RandomGroupingFilter()
{
    setNumGroups( DEFAULT_NUM_GROUPS );
    setSeed( 0 );
}


RandomGroupingFilter::RandomGroupingFilter( int _num_groups )
{
    setNumGroups( _num_groups );
    setSeed( 0 );
}


RandomGroupingFilter::~RandomGroupingFilter()
{
    //NOP
}

void
RandomGroupingFilter::setNumGroups( int value )
{
    num_groups = value > 0? value : 1;
}

int
RandomGroupingFilter::getNumGroups() const
{
    return num_groups;
}

void
RandomGroupingFilter::setSeed( int value )
{
    seed = value;
    ::srand( seed );
}

int
RandomGroupingFilter::getSeed() const
{
    return seed;
}

void
RandomGroupingFilter::setProperty( const Property& p )
{
    if ( p.getName() == "num_groups" )
        setNumGroups( p.getIntValue( DEFAULT_NUM_GROUPS ) );
    CollectionFilter::setProperty( p );
}

Properties
RandomGroupingFilter::getProperties() const
{
    Properties p = CollectionFilter::getProperties();
    p.push_back( Property( "num_groups", getNumGroups() ) );
    return p;
}

std::string 
RandomGroupingFilter::assign( Feature* input )
{
    std::stringstream stream;
    stream << ( ::rand() % getNumGroups() );
    return stream.str();
}

std::string
RandomGroupingFilter::assign( osg::Drawable* input )
{
    std::stringstream stream;
    stream << ( ::rand() % getNumGroups() );
    return stream.str();
}

std::string
RandomGroupingFilter::assign( osg::Node* input )
{
    std::stringstream stream;
    stream << ( ::rand() % getNumGroups() );
    return stream.str();
}