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

#include <osgGIS/BufferFilter>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( BufferFilter );


BufferFilter::BufferFilter()
{
    setDistance( 0.0 );
}

BufferFilter::BufferFilter( const BufferFilter& rhs )
: FeatureFilter( rhs ),
  distance( rhs.distance )
{
    //NOP
}

BufferFilter::BufferFilter( double _distance )
{
    setDistance( _distance );
}

BufferFilter::~BufferFilter()
{
    //NOP
}

void 
BufferFilter::setDistance( double value )
{
    distance = value;
}

double
BufferFilter::getDistance() const
{
    return distance;
}

void
BufferFilter::setProperty( const Property& p )
{
    if ( p.getName() == "distance" )
        setDistance( p.getDoubleValue( getDistance() ) );
    FeatureFilter::setProperty( p );
}

Properties
BufferFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getDistance() > 0.0 )
        p.push_back( Property( "distance", getDistance() ) );
    return p;
}


struct Segment {
    Segment( osg::Vec3& _p0, osg::Vec3& _p1 ) : p0(_p0), p1(_p1) { }
    osg::Vec3 p0, p1;
};
typedef std::vector<Segment> SegmentList;

static bool
getLineIntersection( Segment& s0, Segment& s1, osg::Vec3& output )
{
    osg::Vec3& p1 = s0.p0;
    osg::Vec3& p2 = s0.p1;
    osg::Vec3& p3 = s1.p0;
    osg::Vec3& p4 = s1.p1;

    double denom = (p4.y()-p3.y())*(p2.x()-p1.x()) - (p4.x()-p3.x())*(p2.y()-p1.y());
    if ( denom != 0.0 )
    {
        double ua_num = (p4.x()-p3.x())*(p1.y()-p3.y()) - (p4.y()-p3.y())*(p1.x()-p3.x());
        double ub_num = (p2.x()-p1.x())*(p1.y()-p3.y()) - (p2.y()-p1.y())*(p1.x()-p3.x());

        double ua = ua_num/denom;
        double ub = ub_num/denom;

        double isect_x = p1.x() + ua*(p2.x()-p1.x());
        double isect_y = p1.y() + ua*(p2.y()-p1.y());
        output.set( isect_x, isect_y, p2.z() );
    }
    else
    {
        output.set( p2 );
    }
    return true;
}

static void
bufferPolygons( const GeoShape& shape, double b, GeoPartList& output )
{
    for( GeoPartList::const_iterator i = shape.getParts().begin(); i != shape.getParts().end(); i++ )
    {
        const GeoPointList& part = *i;
        if ( part.size() < 3 )
            continue;

        GeoPointList new_part;

        // first build the buffered line segments:
        SegmentList segments;
        for( GeoPointList::const_iterator j = part.begin(); j != part.end(); j++ )
        {
            const osg::Vec3& p0 = *j;
            const osg::Vec3& p1 = (j+1) != part.end()? *(j+1) : *part.begin();

            osg::Vec3 d = p1-p0; d.normalize();

            osg::Vec3 b0( p0.x() + b*d.y(), p0.y() - b*d.x(), p1.z() );
            osg::Vec3 b1( p1.x() + b*d.y(), p1.y() - b*d.x(), p1.z() );
            segments.push_back( Segment( b0, b1 ) );
        }

        // then intersect each pair of segments to find the new verts:
        for( SegmentList::iterator k = segments.begin(); k != segments.end(); k++ )
        {
            Segment& s0 = *k;
            Segment& s1 = (k+1) != segments.end()? *(k+1) : *segments.begin();

            osg::Vec3 isect;
            if ( getLineIntersection( s0, s1, isect ) )
            {
                GeoPoint r( isect, part[0].getSRS() );
                r.setDim( part[0].getDim() );
                new_part.push_back( r );
            }
        }

        if ( new_part.size() > 2 )
            output.push_back( new_part );
    }
}

FeatureList
BufferFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    GeoShapeList& shapes = input->getShapes();
    for( GeoShapeList::iterator i = shapes.begin(); i != shapes.end(); i++ )
    {
        GeoPartList new_parts;
        GeoShape& shape = *i;

        if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        {
            bufferPolygons( shape, getDistance(), new_parts );
        }

        if ( new_parts.size() > 0 )
            shape.getParts().swap( new_parts );
    }

    output.push_back( input );
    return output;
}
