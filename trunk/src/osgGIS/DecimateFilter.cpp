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

#include <osgGIS/DecimateFilter>
#include <osg/Notify>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( DecimateFilter );


DecimateFilter::DecimateFilter()
{
    distance_threshold = 0.0;
}


DecimateFilter::DecimateFilter( double _dist_threshold )
{
    distance_threshold = _dist_threshold;
}


DecimateFilter::~DecimateFilter()
{
    //NOP
}

void
DecimateFilter::setDistanceThreshold( double value )
{
    distance_threshold = value;
}

double
DecimateFilter::getDistanceThreshold() const
{
    return distance_threshold;
}


void
DecimateFilter::setProperty( const Property& p )
{
    if ( p.getName() == "distance_threshold" )
        setDistanceThreshold( p.getDoubleValue( getDistanceThreshold() ) );
    FeatureFilter::setProperty( p );
}

Properties
DecimateFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    p.push_back( Property( "distance_threshold", getDistanceThreshold() ) );
    return p;
}


void
decimatePart( GeoPointList& input, double threshold, int min_points, GeoPartList& output )
{
    GeoPointList new_part;
    GeoPoint last_point;
    double t2 = threshold * threshold;

    if ( input.size() >= min_points )
    {
        for( GeoPointList::iterator i = input.begin(); i != input.end(); i++ )
        {
            if ( i == input.begin() )// || i == input.end()-1 )
            {
                last_point = *i;
                new_part.push_back( *i );
            }
            else
            {
                double d2 = (*i - last_point).length2();
                if ( d2 > t2 )
                {
                    new_part.push_back( *i );
                    last_point = *i;
                }
            }
        }

        if ( new_part.size() >= min_points )
        {
            output.push_back( new_part );
        }
    }
}


FeatureList
DecimateFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    GeoShapeList new_shapes;

    for( GeoShapeList::iterator i = input->getShapes().begin(); i != input->getShapes().end(); i++ )
    {
        int min_points = 
            i->getShapeType() == GeoShape::TYPE_POLYGON? 3 :
            i->getShapeType() == GeoShape::TYPE_LINE? 2 :
            1;

        GeoPartList new_parts;

        for( GeoPartList::iterator j = i->getParts().begin(); j != i->getParts().end(); j++ )
        {
            decimatePart( *j, distance_threshold, min_points, new_parts );
        }

        i->getParts().swap( new_parts );
        
        if ( new_parts.size() > 0 )
        {
            new_shapes.push_back( *i );
        }
    }

    input->getShapes().swap( new_shapes );

    if ( input->getShapes().size() > 0 )
    {
        output.push_back( input );
    }
    return output;
}