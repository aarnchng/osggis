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

#include <osgGIS/RecalculateExtentFilter>
#include <osg/Notify>
#include <sstream>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( RecalculateExtentFilter );


RecalculateExtentFilter::RecalculateExtentFilter()
{
    // sets single-feature metering so that the filter acts like
    // a normal feature filter
    setMetering( 1 );
}

RecalculateExtentFilter::RecalculateExtentFilter( const RecalculateExtentFilter& rhs )
: CollectionFilter( rhs )
{
    //NOP
}

RecalculateExtentFilter::~RecalculateExtentFilter()
{
    //NOP
}

void
RecalculateExtentFilter::setProperty( const Property& p )
{
    CollectionFilter::setProperty( p );
}

Properties
RecalculateExtentFilter::getProperties() const
{
    Properties p = CollectionFilter::getProperties();
    return p;
}


void 
RecalculateExtentFilter::preMeter( FeatureList& features, FilterEnv* env )
{
    GeoExtent new_extent;

    for( FeatureList::const_iterator i = features.begin(); i != features.end(); i++ )
    {
        if ( i == features.begin() )
            new_extent = i->get()->getExtent();
        else
            new_extent.expandToInclude( i->get()->getExtent() );
    }      

    env->setExtent( new_extent );
}
