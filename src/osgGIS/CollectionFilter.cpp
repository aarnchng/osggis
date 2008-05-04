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

#include <osgGIS/CollectionFilter>
#include <osgGIS/CollectionFilterState>
#include <osg/Notify>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( CollectionFilter );

#define DEFAULT_METERING 0

CollectionFilter::CollectionFilter()
{
    setMetering( DEFAULT_METERING );
}

CollectionFilter::CollectionFilter( const CollectionFilter& rhs )
: Filter( rhs ),
  metering( rhs.metering )
{
    //NOP
}

CollectionFilter::~CollectionFilter()
{
    //NOP
}

FilterState*
CollectionFilter::newState() const
{
    return new CollectionFilterState( static_cast<CollectionFilter*>( clone() ) );
}

void 
CollectionFilter::setProperty( const Property& prop )
{
    if ( prop.getName() == "metering" )
        setMetering( prop.getIntValue( DEFAULT_METERING ) );
    Filter::setProperty( prop );
}

Properties
CollectionFilter::getProperties() const
{
    Properties p = Filter::getProperties();
    if ( getMetering() != DEFAULT_METERING )
        p.push_back( Property( "metering", getMetering() ) );
    return p;
}
