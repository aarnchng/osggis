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

#include <osgGIS/DensifyFilter>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( DensifyFilter );


DensifyFilter::DensifyFilter()
{
    threshold = 0.0;
}

DensifyFilter::DensifyFilter( const DensifyFilter& rhs )
: FeatureFilter( rhs ),
  threshold( rhs.threshold )
{
    //NOP
}

DensifyFilter::~DensifyFilter()
{
    //NOP
}


void
DensifyFilter::setThreshold( double value )
{
    threshold = value;
}

double
DensifyFilter::getThreshold() const
{
    return threshold;
}

void
DensifyFilter::setProperty( const Property& p )
{
    if ( p.getName() == "threshold" )
        setThreshold( p.getDoubleValue( getThreshold() ) );
    FeatureFilter::setProperty( p );
}


Properties
DensifyFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    p.push_back( Property( "threshold", getThreshold() ) );
    return p;
}

FeatureList
DensifyFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    if ( threshold > 0.0 )
    {
        for( GeoShapeList::iterator i = input->getShapes().begin(); i != input->getShapes().end(); i++ )
        {
            GeoShape& shape = *i;
            if ( shape.getShapeType() != GeoShape::TYPE_POINT )
            {
                for( GeoPartList::iterator j = shape.getParts().begin(); j != shape.getParts().end(); j++ )
                {
                    GeoPointList& part = *j;
                    for( GeoPointList::iterator k = part.begin(); k != part.end()-1; k++ )
                    {
                        GeoPoint& a = *k;
                        GeoPoint& b = *(k+1);
                        if ( (b-a).length() > threshold )
                        {
                            osg::Vec3d unit = b-a;
                            unit.normalize();
                            osg::Vec3d v = a + unit*threshold;
                            GeoPoint p = a.getDim() == 2? 
                                GeoPoint( v.x(), v.y(), a.getSRS() ) :
                                GeoPoint( v.x(), v.y(), v.z(), a.getSRS() );
                            
                            k = part.insert( k+1, p );
                            k--;
                        }
                    }
                }
            }
        }
    }

    output.push_back( input );
    return output;
}

