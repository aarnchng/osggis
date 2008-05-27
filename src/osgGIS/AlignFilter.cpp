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

#include <osgGIS/AlignFilter>
#include <osgGIS/FeatureLayerResource>
#include <osgGIS/Utils>
#include <osg/Notify>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( AlignFilter );

#define DEFAULT_RADIUS 50.0

AlignFilter::AlignFilter()
{
    setRadius( DEFAULT_RADIUS );
}

AlignFilter::AlignFilter( const AlignFilter& rhs )
: FeatureFilter( rhs ),
  radius( rhs.radius ),
  output_attribute( rhs.output_attribute ),
  alignment_layer_name( rhs.alignment_layer_name )
{
    //NOP
}

AlignFilter::~AlignFilter()
{
    //NOP
}

void
AlignFilter::setRadius( double value )
{
    radius = value;
}

double
AlignFilter::getRadius() const
{
    return radius;
}

void
AlignFilter::setAlignmentLayerResourceName( const std::string& value )
{
    alignment_layer_name = value;
}

const std::string&
AlignFilter::getAlignmentLayerResourceName() const
{
    return alignment_layer_name;
}

void
AlignFilter::setOutputAttribute( const std::string& value )
{
    output_attribute = value;
}

const std::string&
AlignFilter::getOutputAttribute() const
{
    return output_attribute;
}

void
AlignFilter::setProperty( const Property& p )
{
    if ( p.getName() == "radius" )
        setRadius( p.getDoubleValue( getRadius() ) );
    else if ( p.getName() == "output_attribute" )
        setOutputAttribute( p.getValue() );
    else if ( p.getName() == "alignment_layer" )
        setAlignmentLayerResourceName( p.getValue() );

    FeatureFilter::setProperty( p );
}

Properties
AlignFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getRadius() != DEFAULT_RADIUS )
        p.push_back( Property( "radius", getRadius() ) );
    if ( getOutputAttribute().length() > 0 )
        p.push_back( Property( "output_attribute", getOutputAttribute() ) );
    if ( getAlignmentLayerResourceName().length() > 0 )
        p.push_back( Property( "alignment_layer", getAlignmentLayerResourceName() ) );
    return p;
}

#define METERS_IN_ONE_DEG_OF_LAT 111120.0


struct SegmentVisitor : public GeoPartVisitor
{
    SegmentVisitor( const GeoPoint& _point, GeoShape::ShapeType _type ) : 
        point( _point ),
        type( _type ),
        best_heading( 0.0 ),
        best_distance( DBL_MAX ) { }

    bool
    visitPart( const GeoPointList& part )
    {
        // skip over CW polygons (i.e. holes)
        if ( type == GeoShape::TYPE_POLYGON && !GeomUtils::isPolygonCCW( part ) )
            return true;
        
        if ( type == GeoShape::TYPE_LINE )
        {
            for( GeoPointList::const_iterator i = part.begin(); i != part.end()-1; i++ )
            {
                visitSegment( *i, *(i+1) );
            }
        }
        else if ( type == GeoShape::TYPE_POLYGON )
        {
            for( GeoPointList::const_iterator i = part.begin(); i != part.end(); i++ )
            {
                const GeoPoint& p1 = i != part.end()-1? *(i+1) : *(part.begin() );
                visitSegment( *i, p1 );
            }
        }

        return true;
    }

    void
    visitSegment( const GeoPoint& p0, const GeoPoint& p1 )
    {
        double ax = p0.x(), ay = p0.y(), bx = p1.x(), by = p1.y(), cx = point.x(), cy = point.y();

        // shortest-distance-from-point-to-line-segment algorithm by Philip Nicoletti
        // http://www.codeguru.com/forum/printthread.php?t=194400

        double r_numerator = (cx-ax)*(bx-ax) + (cy-ay)*(by-ay);
	    double r_denomenator = (bx-ax)*(bx-ax) + (by-ay)*(by-ay);
        if ( r_denomenator != 0.0 )
        {
	        double r = r_numerator / r_denomenator;
            double px = ax + r*(bx-ax);
            double py = ay + r*(by-ay);
            double s =  ((ay-cy)*(bx-ax)-(ax-cx)*(by-ay) ) / r_denomenator;
	        double distance_line = fabs(s)*sqrt(r_denomenator);
            // (xx,yy) is the point on the lineSegment closest to (cx,cy)
	        double xx = px;
	        double yy = py;
            double distance_segment = 0.0;
        	if ( (r >= 0) && (r <= 1) )
	        {
		        distance_segment = distance_line;
	        }
	        else
	        {
        		double dist1 = (cx-ax)*(cx-ax) + (cy-ay)*(cy-ay);
        		double dist2 = (cx-bx)*(cx-bx) + (cy-by)*(cy-by);
        		if (dist1 < dist2)
        		{
			        xx = ax;
			        yy = ay;
			        distance_segment = sqrt(dist1);
		        }
		        else
		        {
			        xx = bx;
			        yy = by;
			        distance_segment = sqrt(dist2);
		        
                }
            }

            if ( distance_segment < best_distance )
            {
                best_distance = distance_segment;
                best_heading = osg::RadiansToDegrees( atan2( xx-cx, yy-cy ) );
            }
        }
    }

    const GeoPoint& point;
    GeoShape::ShapeType type;

    double best_distance;
    double best_heading;
};


FeatureList
AlignFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    if (input->getShapeType() == GeoShape::TYPE_POINT &&
        getAlignmentLayerResourceName().length() > 0 && 
        getRadius() > 0.0 &&
        getOutputAttribute().length() > 0 )
    {
        // load up and cache the alignment layer the first time through:
        if ( !alignment_layer.valid() )
        {
            alignment_layer = env->getSession()->getResources()->getFeatureLayer( getAlignmentLayerResourceName() );
            
            if ( !alignment_layer.valid() )
            {
                osg::notify(osg::WARN) << "AlignFilter: Error, cannot access alignment layer " 
                    << getAlignmentLayerResourceName()
                    << std::endl;
            }
        }

        if ( alignment_layer.valid() )
        {
            double radius_srs = radius;

            // adjust the search radius for geographic space if necessary:
            if ( env->getInputSRS()->isGeographic() )
                radius_srs = radius/METERS_IN_ONE_DEG_OF_LAT;

            // get the point's coordinates and transform them into absolute space:
            GeoPoint c = input->getExtent().getCentroid().getAbsolute();

            GeoExtent search_extent( 
                c.x()-radius_srs, c.y()-radius_srs,
                c.x()+radius_srs, c.y()+radius_srs,
                c.getSRS() );

            double best_distance = DBL_MAX;
            double best_heading  = 0.0;

            FeatureCursor cursor = alignment_layer->getCursor( search_extent );
            while( cursor.hasNext() )
            {
                Feature* a = cursor.next();

                SegmentVisitor vis( c, a->getShapeType() );
                const GeoShapeList& shapes = a->getShapes();
                shapes.accept( vis );

                if ( vis.best_distance < best_distance )
                {
                    best_distance = vis.best_distance;
                    best_heading = vis.best_heading;
                }
            }

            input->setAttribute( getOutputAttribute(), best_heading );

            //osg::notify(osg::NOTICE)
            //    << "Feature " << input->getOID() << ", heading = " << best_heading << std::endl;
        }
    }

    output.push_back( input );
    return output;
}