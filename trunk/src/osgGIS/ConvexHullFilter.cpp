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
#include <osg/Notify>
#include <algorithm>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( ConvexHullFilter );

ConvexHullFilter::ConvexHullFilter()
{
    //NOP
}

ConvexHullFilter::ConvexHullFilter( const ConvexHullFilter& rhs )
: FeatureFilter( rhs )
{
    //NOP
}

ConvexHullFilter::~ConvexHullFilter()
{
    //NOP
}


// returns TRUE is a part is  not a hole)
static bool
isNotHole( GeoPointList& points )
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
                if ( k->size() > 2 ) //&& isNotHole( *k ) )
                {
                    output.insert( output.end(), k->begin(), k->end() );
                }
            }
        }
    }
}


struct LesserAngleWithP {
    LesserAngleWithP( GeoPoint& _p ) : p(_p) { }
    const GeoPoint& p;
    bool operator()( const GeoPoint& v0, const GeoPoint& v1 ) const {
        osg::Vec3d k0 = v0 - p, k1 = v1 - p;  
        return 
            k0.x() > 0 && k1.x() < 0? true :
            k1.x() > 0 && k0.x() < 0? false :
            k0.x() > 0 && k1.x() > 0? (k0.y()/k0.x() < k1.y()/k1.x()) :
            k0.x() < 0 && k1.x() < 0? (k0.y()/-k0.x() > k1.y()/-k1.x()) :
            false;
    }
};

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
    std::sort( input.begin(), input.end(), LesserAngleWithP(P) );

    // c) re-insert P at the beginning AND the end of the list.
    input.insert( input.begin(), P );
    input.push_back( P );
}


// traverses the sorted point list, building a new list representing
// the convex hull of in the input list
void
buildHull( GeoPointList& input, bool close_loop )
{
    if ( input.size() >= 3 )
    {
        for( GeoPointList::iterator i = input.begin()+2; i != input.end(); i++ )
        {
            if ( i >= input.begin()+2 )
            {
                const GeoPoint& p2 = *i;
                const GeoPoint& p1 = *(i-1);
                const GeoPoint& p0 = *(i-2);

                double cross2d =
                    (p1.x()-p0.x())*(p2.y()-p0.y()) - (p1.y()-p0.y())*(p2.x()-p0.x());

                bool is_left_turn = cross2d > 0;

                if ( !is_left_turn )
                {
                    i = input.erase( i-1 );
                    i--;
                }
            }
        }

        if ( !close_loop )
        {
            input.erase( input.end()-1 );
        }
    }
}


GeoShape::ShapeType
deriveType( FeatureList& input )
{
    if ( input.size() > 0 )
    {
        Feature* f = input.front().get();
        if ( f->getShapes().size() > 0 )
        {
            return f->getShapes().front().getShapeType();
        }
    }
    return GeoShape::TYPE_POLYGON;
}


FeatureList
ConvexHullFilter::process( FeatureList& input, FilterEnv* env )
{
    FeatureList output;

    // Contruct a convex hull from the feature set
    // Graham Scan algorithm:
    // http://en.wikipedia.org/wiki/Graham_scan
    
    // a) collect all points into a single array, discarding holes
    GeoPointList working_set;
    collectPoints( input, working_set );

    // c) sort remaining points by angle to P
    sortPoints( working_set );

    // d) traverse to build convex hull points
    GeoShape::ShapeType type = deriveType( input );
    buildHull( working_set, type == GeoShape::TYPE_LINE );

    // e) build new single convex hull feature.
    GeoShape hull_shape( type, env->getInputSRS() );
    hull_shape.addPart( working_set );
    Feature* hull_feature = new SimpleFeature();
    hull_feature->getShapes().push_back( hull_shape );
    
    output.push_back( hull_feature );
    return output;
}