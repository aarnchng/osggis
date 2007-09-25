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

#include <osgGIS/ConvexHullFilter>
#include <osgGIS/SimpleFeature>
#include <algorithm>

using namespace osgGIS;

ConvexHullFilter::ConvexHullFilter()
{
    //NOP
}


ConvexHullFilter::~ConvexHullFilter()
{
    //NOP
}


// returns TRUE is a part is would clockwise (i.e. is not a hole)
static bool
isPartCW( GeoPointList& points )
{
    // find the ymin point:
    double ymin = DBL_MAX;
    int i_lowest = 0;

    for( GeoPointList::iterator i = points.begin(); i != points.end(); i++ )
    {
        if ( i->y() < ymin ) 
        {
            ymin = i->y();
            i_lowest = i-points.begin();
        }
    }

    // next cross the 2 vector converging at that point:
    osg::Vec3d p0 = *( points.begin() + ( i_lowest > 0? i_lowest-1 : points.size()-1 ) );
    osg::Vec3d p1 = *( points.begin() + i_lowest );
    osg::Vec3d p2 = *( points.begin() + ( i_lowest < points.size()-1? i_lowest+1 : 0 ) );

    osg::Vec3d cp = (p1-p0) ^ (p2-p1);

    //TODO: need to rotate into ref frame - for now just use this filter before xforming
    return cp.z() > 0;
}


// collects all points from non-hole parts into a single list
void
collectPoints( FeatureList& input, GeoPointList& output )
{
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        GeoShapeList& shapes = i->get()->getShapes();
        for( GeoShapeList::iterator j = shapes.begin(); j != shapes.end(); j++ )
        {
            GeoPartList& parts = j->getParts();
            for( GeoPartList::iterator k = parts.begin(); k != parts.end(); k++ )
            {
                if ( k->size() > 2 && isPartCW( *k ) )
                {
                    output.insert( output.end(), k->begin(), k->end() );
                }
            }
        }
    }
}


// finds the y-min point and then sorts the remaining points by their 
// angle to the y-min point
void
sortPoints( GeoPointList& input )
{
    // a) find Y-min and remove it from the list.
    double y_min = DBL_MAX;
    double x_min = DBL_MAX;
    GeoPointList::iterator i = input.end();
    for( GeoPointList::iterator j = input.begin(); j != input.end(); j++ )
    {
        if ( j->y() < y_min || ( j->y() == y_min && j->x() < x_min ) )
        {
            y_min = j->y();
            x_min = j->x();
            i = j;
        }
    }
    if ( i == input.end() )
        return;

    GeoPoint P = *i;
    input.erase( i );

    // b) sort the remaining elements by their angle to P.
    struct AscendingAngleSort {
        AscendingAngleSort( GeoPoint& _p ) : p(_p) { }
        GeoPoint& p;
        bool operator()( GeoPoint& one, GeoPoint& two ) {
            return (p ^ one) < (p ^ two);
        }
    };

    AscendingAngleSort comp(P);
    std::sort( input.begin(), input.end(), comp );

    // c) re-insert P at the beginning of the list.
    input.insert( input.begin(), P );
}


// traverses the sorted point list, building a new list representing
// the convex hull of in the input list
void
buildHull( GeoPointList& input, GeoPointList& output )
{
    //TODO
}


FeatureList
ConvexHullFilter::process( FeatureList& input, FilterEnv* env )
{
    FeatureList output;
    
    // a) collect all points into a single array, discarding holes
    GeoPointList all_points;
    collectPoints( input, all_points );

    // c) sort remaining points by angle to P
    sortPoints( all_points );

    // d) traverse to build convex hull points
    GeoPointList hull_points;
    buildHull( all_points, hull_points );

    // e) build new single convex hull feature.
    GeoShape hull_shape( GeoShape::TYPE_POLYGON, env->getInputSRS() );
    hull_shape.addPart( hull_points );
    Feature* hull_feature = new SimpleFeature();
    hull_feature->getShapes().push_back( hull_shape );
    
    output.push_back( hull_feature );
    return output;
}