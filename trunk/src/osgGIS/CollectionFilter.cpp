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
//#include <osgGIS/FeatureFilter>
//#include <osgGIS/DrawableFilter>
//#include <osgGIS/NodeFilter>
#include <osg/Notify>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( CollectionFilter );

CollectionFilter::CollectionFilter()
{
    setMetering( 0 );
}


CollectionFilter::CollectionFilter( int _metering )
{
    setMetering( _metering );
}


CollectionFilter::~CollectionFilter()
{
    //NOP
}

FilterState*
CollectionFilter::newState()
{
    return new CollectionFilterState( this );
}

void 
CollectionFilter::setProperty( const Property& prop )
{
    if ( prop.getName() == "metering" )
        setMetering( prop.getIntValue( 0 ) );
    Filter::setProperty( prop );
}

Properties
CollectionFilter::getProperties() const
{
    Properties p = Filter::getProperties();
    p.push_back( Property( "metering", getMetering() ) );
    return p;
}

