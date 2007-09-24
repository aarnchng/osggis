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

#include <osgGIS/ClampFilter>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/PlaneIntersector>

using namespace osgGIS;


ClampFilter::ClampFilter()
{
}

ClampFilter::~ClampFilter()
{
    //NOP
}

void
ClampFilter::setTechnique( const Technique& value )
{
    technique = value;
}

const ClampFilter::Technique&
ClampFilter::getTechnique() const
{
    return technique;
}

void
ClampFilter::setProperty( const Property& p )
{
    if ( p.getName() == "technique" ) {
        setTechnique( 
            p.getValue() == "simple"? TECHNIQUE_SIMPLE :
            p.getValue() == "conform"? TECHNIQUE_CONFORM :
            getTechnique() );
    }
    FeatureFilter::setProperty( p );
}

Properties
ClampFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    p.push_back( Property( "technique",
        getTechnique() == TECHNIQUE_SIMPLE? "simple" :
        getTechnique() == TECHNIQUE_CONFORM? "conform" :
        std::string() ) );
    return p;
}

#define ALTITUDE_EXTENSION 250000
#define OFFSET_EXTENSION 1

void
clampPointPart( GeoPointList& part, osg::Node* terrain, const SpatialReference* srs )
{
    osgUtil::IntersectionVisitor iv;

    for( GeoPointList::iterator i = part.begin(); i != part.end(); i++ )
    {
        GeoPoint& p = *i;

        osg::Vec3d p_world = p * srs->getInverseReferenceFrame();
        osg::Vec3d clamp_vec;

        osg::ref_ptr<osgUtil::LineSegmentIntersector> isector;

        if ( srs->isGeocentric() )
        {
            clamp_vec = p_world;
            clamp_vec.normalize();
            isector = new osgUtil::LineSegmentIntersector(
                //p_world + clamp_vec * ALTITUDE_EXTENSION,
                clamp_vec * srs->getBasisEllipsoid().getSemiMajorAxis() * 1.2,
                osg::Vec3d( 0, 0, 0 ) );
        }
        else
        {
            clamp_vec = osg::Vec3d( 0, 0, 1 );
            osg::Vec3d ext_vec = clamp_vec * ALTITUDE_EXTENSION;
            isector = new osgUtil::LineSegmentIntersector(
                p_world + ext_vec,
                p_world - ext_vec );
        }

        iv.setIntersector( isector.get() );
        terrain->accept( iv );
        if ( isector->containsIntersections() )
        {
            osgUtil::LineSegmentIntersector::Intersection isect = isector->getFirstIntersection();
            osg::Vec3d new_point = isect.getWorldIntersectPoint();
            osg::Vec3d offset_point = new_point + clamp_vec * OFFSET_EXTENSION;
            p.set( offset_point * srs->getReferenceFrame() );
        }
        else
        {
            osg::notify( osg::INFO )
                << "uh oh no isect.." << std::endl;
        }
    }
}


void
clampLinePart(GeoPointList&           in_part,
              osg::Node*              terrain, 
              const SpatialReference* srs,
              GeoPartList&            out_parts )
{
    osgUtil::IntersectionVisitor iv;

    for( GeoPointList::iterator i = in_part.begin(); i != in_part.end()-1; i++ )
    {
        GeoPoint& p0 = *i;
        GeoPoint& p1 = *(i+1);

        osg::Vec3d p0_world = p0 * srs->getInverseReferenceFrame();
        osg::Vec3d p1_world = p1 * srs->getInverseReferenceFrame();

        osg::ref_ptr<osgUtil::PlaneIntersector> isector;
       
        if ( srs->isGeocentric() )
        {
            osg::Polytope::PlaneList planes(3);
            
            osg::Vec3d p0_normal = p1_world-p0_world;
            double seg_length = p0_normal.length();

            p0_normal.normalize();
            planes[0] = osg::Plane( p0_normal, p0_world );
            planes[1] = osg::Plane( -p0_normal, p1_world );
            
            osg::Vec3d midpoint = p0_world + p0_normal*(.5*seg_length);
            osg::Vec3d midpoint_normal = midpoint;
            midpoint_normal.normalize();

            planes[2] = osg::Plane(
                midpoint_normal,
                osg::Vec3d(0,0,0) );
                //midpoint - midpoint_normal * ALTITUDE_EXTENSION );

            osg::Vec3d normal1 = p1_world;
            normal1.normalize();

            osg::Polytope polytope( planes );

            osg::Plane plane(
                p0_world,
                p1_world,
                osg::Vec3d(0,0,0) );
                //p1_world + normal1 * ALTITUDE_EXTENSION );

            isector = new osgUtil::PlaneIntersector( plane, polytope );
        }
        else
        {
            osg::Vec3d normal( 0, 0, 1 );

            osg::BoundingBox bbox;
            bbox.expandBy( p0_world + normal * ALTITUDE_EXTENSION );
            bbox.expandBy( p0_world - normal * ALTITUDE_EXTENSION );
            bbox.expandBy( p1_world + normal * ALTITUDE_EXTENSION );
            bbox.expandBy( p1_world - normal * ALTITUDE_EXTENSION );
            
            osg::Polytope polytope;
            polytope.setToBoundingBox( bbox );

            osg::Plane plane(
                p0_world,
                p1_world,
                p1_world + normal*ALTITUDE_EXTENSION );

            isector = new osgUtil::PlaneIntersector( plane, polytope );
        }
       
        osg::NotifySeverity sev = osg::getNotifyLevel(); // suppress annoying notices
        osg::setNotifyLevel( osg::WARN );

        iv.setIntersector( isector.get() );
        terrain->accept( iv );

        osg::setNotifyLevel( sev );

        if ( isector->containsIntersections() )
        {
            GeoPointList new_part;

            osgUtil::PlaneIntersector::Intersections& results = isector->getIntersections();
            for( osgUtil::PlaneIntersector::Intersections::iterator i = results.begin();
                 i != results.end();
                 i++ )
            {                
                osgUtil::PlaneIntersector::Intersection& isect = *i;
                for( osgUtil::PlaneIntersector::Intersection::Polyline::iterator j = isect.polyline.begin();
                     j != isect.polyline.end();
                     j++ )
                {
                    osg::Vec3d ip_world = (*j) * *(isect.matrix.get());
                    //osg::Vec3d offset = ip_world;
                    //offset.normalize();
                    //ip_world += offset * OFFSET_EXTENSION;
                    ip_world = ip_world * srs->getReferenceFrame();
                    new_part.push_back( GeoPoint( ip_world, srs ) );
                }
            }

            out_parts.push_back( new_part );
        }
        else
        {
            osg::notify( osg::INFO )
                << "uh oh no isect.." << std::endl;

            //out_parts.clear();
            //out_parts.push_back( in_part );
            //break;
        }
    }
}


void
clampPolyPart( GeoPointList& part, osg::Node* terrain, const SpatialReference* srs )
{
    //TODO
    clampPointPart( part, terrain, srs );
}


// removes coincident points.
void
cleansePart( GeoPointList& part )
{
    for( GeoPointList::iterator i = part.begin(); i != part.end(); i++ )
    {
        if ( i != part.begin() )
        {
            if ( *i == *(i-1) )
            {
                i = part.erase( i );
                i--;
            }
        }
    }
}


FeatureList
ClampFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    //osg::notify( osg::INFO ) << "Clamping feature " << input->getOID() << std::endl;

    osg::Node* terrain = env->getTerrainNode();

    for( GeoShapeList::iterator i = input->getShapes().begin(); i != input->getShapes().end(); i++ )
    {
        GeoShape& shape = *i;
        GeoPartList new_parts;

        for( GeoPartList::iterator j = shape.getParts().begin(); j != shape.getParts().end(); j++ )
        {
            GeoPointList& part = *j;

            switch( shape.getShapeType() )
            {
            case GeoShape::TYPE_POINT:
                clampPointPart( part, terrain, env->getInputSRS() );
                break;

            case GeoShape::TYPE_LINE:
                clampLinePart( part, terrain, env->getInputSRS(), new_parts );
                break;

            case GeoShape::TYPE_POLYGON:
                clampPolyPart( part, terrain, env->getInputSRS() );
                break;
            }

            cleansePart( part );
        }

        if ( new_parts.size() > 0 )
        {
            shape.getParts().swap( new_parts );
        }
    }

    output.push_back( input );
    return output;
}