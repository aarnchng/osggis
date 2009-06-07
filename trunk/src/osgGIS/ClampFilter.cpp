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

#include <osgGIS/ClampFilter>
#include <osgGIS/SmartReadCallback>
#include <osgGIS/LineSegmentIntersector2>
#include <osgGIS/ElevationResource>
#include <osgGIS/Utils>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/PlaneIntersector>
#include <osg/PagedLOD>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( ClampFilter );


ClampFilter::ClampFilter()
{
    ignore_z = false;
    simulate = false;
}

ClampFilter::ClampFilter( const ClampFilter& rhs )
: FeatureFilter( rhs ),
  ignore_z( rhs.ignore_z ),
  simulate( rhs.simulate ),
  clamped_z_output_attribute( rhs.clamped_z_output_attribute ),
  elevation_resource_script( rhs.elevation_resource_script.get() )
{
    //NOP
}

ClampFilter::~ClampFilter()
{
    //NOP
}

void
ClampFilter::setIgnoreZ( bool value )
{
    ignore_z = value;
}

bool
ClampFilter::getIgnoreZ() const
{
    return ignore_z;
}

void
ClampFilter::setClampedZOutputAttribute( const std::string& value )
{
    clamped_z_output_attribute = value;
}

const std::string&
ClampFilter::getClampedZOutputAttribute() const
{
    return clamped_z_output_attribute;
}

void
ClampFilter::setSimulate( bool value )
{ 
    simulate = value;
}

bool
ClampFilter::getSimulate() const
{
    return simulate;
}

//void
//ClampFilter::setElevationResourceScript( Script* value )
//{
//    elevation_resource_script = value;
//}
//
//Script*
//ClampFilter::getElevationResourceScript() const
//{
//    return elevation_resource_script.get();
//}

void
ClampFilter::setProperty( const Property& p )
{
    if ( p.getName() == "ignore_z" )
        setIgnoreZ( p.getBoolValue( getIgnoreZ() ) );
    else if ( p.getName() == "clamped_z_output_attribute" )
        setClampedZOutputAttribute( p.getValue() );
    else if ( p.getName() == "simulate" )
        setSimulate( p.getBoolValue( getSimulate() ) );
    //else if ( p.getName() == "elevation" )
    //    setElevationResourceScript( new Script( p.getValue() ) );

    FeatureFilter::setProperty( p );
}

Properties
ClampFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();

    p.push_back( Property( "ignore_z", getIgnoreZ() ) );

    if ( getClampedZOutputAttribute().length() > 0 )
        p.push_back( Property( "clamped_z_output_attribute", getClampedZOutputAttribute() ) );
    if ( getSimulate() == true )
        p.push_back( Property( "simulate", getSimulate() ) );
    //if ( getElevationResourceScript() )
    //    p.push_back( Property( "elevation", getElevationResourceScript()->getCode() ) );

    return p;
}

#define ALTITUDE_EXTENSION 250000
#define DEFAULT_OFFSET_EXTENSION 0


//static void
//calculateEndpoints( const GeoPoint& p_geo, osg::Vec3d& out_ep0, osg::Vec3d& out_ep1 )
//{
//    osg::ref_ptr<SpatialReference> ecef_srs = Registry::SRSFactory()->createGeocentricSRS( p_geo.getSRS() );
//    GeoPoint p2 = p_geo;
//    double ext = 0.1 * ecef_srs->getEllipsoid().getSemiMajorAxis();
//    p2.z() = p2.getDim() < 3? ext : p2.z() + ext; p2.setDim(3);
//    out_ep0 = ecef_srs->transform( p2 );
//    GeoPoint p3 = p2;
//    p3.z() -= 2.0 * ext;
//    out_ep1 = ecef_srs->transform( p3 );
//}


static int
clampPointPartToTerrain(GeoPointList&           part,
                        osg::Node*              terrain,
                        const SpatialReference* srs,
                        bool                    ignore_z,
                        SmartReadCallback*      reader,
                        double&                 out_clamped_z,
                        bool                    simulate =false)
{
    int clamps = 0;
    out_clamped_z = DBL_MAX;

    RelaxedIntersectionVisitor iv;

    if ( reader )
        iv.setReadCallback( reader );

    GeoPoint simulated_p;

    for( GeoPointList::iterator i = part.begin(); i != part.end(); i++ )
    {
        if ( simulate ) simulated_p = *i;
        GeoPoint& p = simulate? simulated_p : *i;
        GeoPoint  p_world = p.getAbsolute();

        osg::Vec3d clamp_vec;
        double hat = 0.0;

        osg::ref_ptr<LineSegmentIntersector2> isector;

        isector = GeomUtils::createClampingIntersector( p_world, hat );

        iv.setIntersector( isector.get() );

        // try the read cache's MRU node if it intersects out point:
        osg::Node* target = terrain;
        if ( reader )
        {
            target = reader->getMruNodeIfContains( p_world, terrain );

            // First try the MRU target; then fall back on the whole terrain.
            target->accept( iv );
            if ( !isector->containsIntersections() && target != terrain )
            {
                terrain->accept( iv );
            }
        }
        else
        {
            terrain->accept( iv );
        }

        if ( isector->containsIntersections() )
        {
            LineSegmentIntersector2::Intersection isect = isector->getFirstIntersection();
            osg::Vec3d new_point = isect.getWorldIntersectPoint();
            osg::Vec3d offset_point = ignore_z? new_point : new_point + clamp_vec * hat;

            if ( !srs->isGeographic() )
            {
                p.set( offset_point * srs->getReferenceFrame() );
            }
            else
            {
                osg::ref_ptr<SpatialReference> ecef_srs = Registry::SRSFactory()->createGeocentricSRS( p.getSRS() );
                GeoPoint pp( offset_point, ecef_srs.get() );
                p = srs->transform( pp );
                p.set( p * srs->getReferenceFrame() ); // unlikely, but correct
            }

            clamps++;

            //TODO: can we replace the manual setMru with the SmartCB's MRU list?
            if ( reader )
                //reader->setMruNode( NULL );
                reader->setMruNode( isect.nodePath.back().get() );

            // record the HAT value:
            if ( srs->isGeocentric() )
            {
                if ( hat < out_clamped_z )
                    out_clamped_z = hat;
            }
            else
            {
                if ( offset_point.z() < out_clamped_z )
                    out_clamped_z = offset_point.z();
            }
            
            //osgGIS::notify( osg::WARN ) << "Found an intersection for point " 
            //    << osg::RadiansToDegrees(lat) << ", " << osg::RadiansToDegrees(lon) << ", " << hat
            //    << std::endl;
        }

        else 
        {
            //osgGIS::notify( osg::WARN ) << "MISSED an intersection for point " 
            //    << osg::RadiansToDegrees(lat) << ", " << osg::RadiansToDegrees(lon) << ", " << hat
            //    << std::endl;
            
            //osg::Vec3d offset_point = p_world + clamp_vec * 100.0;
            //p.set( offset_point * srs->getReferenceFrame() );
        }
    }

    return clamps;
}


static void
clampLinePartToTerrain(GeoPointList&           in_part,
                       osg::Node*              terrain, 
                       const SpatialReference* srs,
                       bool                    ignore_z,
                       SmartReadCallback*      read_cache,
                       GeoPartList&            out_parts )
{
    // For a geocentic part, we first clamp each point. Then we can use the polytope
    // intersector to refine the line in between.
    if ( srs->isGeocentric() )
    {
        double out_z = 0.0;
        clampPointPartToTerrain( in_part, terrain, srs, ignore_z, read_cache, out_z );
    }

    RelaxedIntersectionVisitor iv;

    if ( read_cache )
        iv.setReadCallback( read_cache );

    for( GeoPointList::iterator i = in_part.begin(); i != in_part.end()-1; i++ )
    {
        GeoPoint& p0 = *i;
        GeoPoint& p1 = *(i+1);

        // if the points are equals, move along; TODO: use an epsilon
        if ( p0 == p1 )
            continue;

        //TODO: fix: only works for projected...needs to work for geocentric too
        double z = p0.getDim() < 3 || ignore_z || !p0.getSRS()->isProjected()? 0.0 : p0.z();

        osg::Vec3d p0_world = p0 * srs->getInverseReferenceFrame();
        osg::Vec3d p1_world = p1 * srs->getInverseReferenceFrame();

        osg::ref_ptr<osgUtil::PlaneIntersector> isector;
       
        if ( srs->isGeocentric() )
        {
            osg::Polytope::PlaneList planes(3);
            
            osg::Vec3d p0_normal = p1_world-p0_world;
            double seg_length = p0_normal.length();

            // check again for "equivalent" points. TODO: use an epsilon
            if ( seg_length <= 0.0 )
                continue;

            p0_normal.normalize();
            planes[0] = osg::Plane( p0_normal, p0_world );
            planes[1] = osg::Plane( -p0_normal, p1_world );
            
            osg::Vec3d midpoint = p0_world + p0_normal*(.5*seg_length);
            osg::Vec3d midpoint_normal = midpoint;
            midpoint_normal.normalize();

            //midpoint -= midpoint_normal*seg_length;
            //planes[2].set( midpoint_normal, midpoint );

            //TODO: technically, the (0,0,0) is wrong. We don't usually notice since the
            //      points were pre-clamped, but it should really be an extension of
            //      the "clamp vector" based on two geographic heights. See clampPoint
            //      for the technique.

            planes[2] = osg::Plane( midpoint_normal, osg::Vec3d(0,0,0) );

            osg::Polytope polytope( planes );

            //TODO: again, the (0,0,0) is not quite right...see above
            osg::Plane plane( p0_world, p1_world, osg::Vec3d(0,0,0) );

            isector = new osgUtil::PlaneIntersector( plane, polytope );
        }
        else
        {
            osg::Vec3d normal( 0, 0, 1 );
            
            osg::Polytope::PlaneList planes( 2 );

            osg::Vec3d p0_normal = p1_world-p0_world;
            double seg_length = p0_normal.length();
            p0_normal.normalize();
            planes[0] = osg::Plane( p0_normal, p0_world );
            planes[1] = osg::Plane( -p0_normal, p1_world );

            osg::Polytope polytope( planes );

            osg::Plane plane(
                p0_world,
                p1_world,
                p1_world + normal*ALTITUDE_EXTENSION );

            isector = new osgUtil::PlaneIntersector( plane, polytope );
        }
       
        osg::NotifySeverity sev = osg::getNotifyLevel(); // suppress annoying notices
        osg::setNotifyLevel( osg::WARN );

        iv.setIntersector( isector.get() );
        
        // try the read cache's MRU node if it intersects our points:
        osg::Node* target = NULL;
        if ( read_cache )
            target = read_cache->getMruNodeIfContains( p0_world, p1_world, terrain );
        else
            target = terrain;

        if ( target != terrain )
        {
            // we're using the cache entry, so we must first ensure that each endpoint
            // intersects the patch. The getMru method only tests against bounding spheres
            // and therefore there's a chance one of the points doesn't actually fall
            // within the cached tile.
            GeoPointList test( 2 );
            test[0] = p0;
            test[1] = p1;
            double out_clamped_z = 0.0;
            if ( clampPointPartToTerrain( test, target, srs, ignore_z, NULL, out_clamped_z ) < 2 )
            {
                target = terrain;
            }
        }

        target->accept( iv );

        if ( isector->containsIntersections() )
        {
            osgUtil::PlaneIntersector::Intersections& results = isector->getIntersections();
            for( osgUtil::PlaneIntersector::Intersections::iterator k = results.begin();
                k != results.end();
                k++ )
            {                       
                GeoPointList new_part;

                osgUtil::PlaneIntersector::Intersection& isect = *k;
                for( osgUtil::PlaneIntersector::Intersection::Polyline::iterator j = isect.polyline.begin();
                     j != isect.polyline.end();
                     j++ )
                {
                    osg::Vec3d ip_world = (*j) * *(isect.matrix.get());

                    // reapply the original Z value
                    // TODO: we should interpolate Z between p0 and p1
                    if ( z > 0.0 )
                    {
                        if ( srs->isGeocentric() )
                        {
                            osg::Vec3d z_vec = ip_world;
                            z_vec.normalize();
                            ip_world = ip_world + z_vec * z;
                        }
                        else
                        {
                            ip_world = ip_world + osg::Vec3d(0,0,z);
                        }
                    }

                    ip_world = ip_world * srs->getReferenceFrame();

                    new_part.push_back( GeoPoint( ip_world, srs ) );
                }
                
                //TODO: i guess we really need a MRU queue instead..?
                if ( read_cache )
                    read_cache->setMruNode( NULL );
                    //read_cache->setMruNode( isect.nodePath.back() );

                // add the new polyline part to the result set:
                if ( new_part.size() >= 2 )
                    out_parts.push_back( new_part );

                //TESTING - just read the first result:
                //break;
            }
        }
        else
        {
            osgGIS::notify( osg::INFO )
                << "uh oh no isect.." << std::endl;
        }
        
        osg::setNotifyLevel( sev );
    }
}


static void
clampPolyPartToTerrain(GeoPointList&           part,
                       osg::Node*              terrain,
                       const SpatialReference* srs,
                       bool                    ignore_z,
                       SmartReadCallback*      read_cache,
                       double&                 out_clamped_z )
{
    clampPointPartToTerrain( part, terrain, srs, ignore_z, read_cache, out_clamped_z );
}


// removes coincident points.
static void
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

    osg::Node* terrain = env->getTerrainNode();

    // if no terrain is set, just pass the data through unaffected.
    if ( terrain )
    {
        double min_clamped_z = DBL_MAX;

        for( GeoShapeList::iterator i = input->getShapes().begin(); i != input->getShapes().end(); i++ )
        {
            GeoShape& shape = *i;
            GeoPartList new_parts;

            for( GeoPartList::iterator j = shape.getParts().begin(); j != shape.getParts().end(); j++ )
            {
                GeoPointList& part = *j;

                double out_clamped_z = DBL_MAX;

                if ( getSimulate() )
                {
                    clampPointPartToTerrain( part, terrain, env->getInputSRS(), ignore_z, env->getTerrainReadCallback(), out_clamped_z, true );
                }
                else
                {
                    switch( shape.getShapeType() )
                    {
                    case GeoShape::TYPE_POINT:
                        clampPointPartToTerrain( part, terrain, env->getInputSRS(), ignore_z, env->getTerrainReadCallback(), out_clamped_z );
                        break;

                    case GeoShape::TYPE_LINE:
                        clampLinePartToTerrain( part, terrain, env->getInputSRS(), ignore_z, env->getTerrainReadCallback(), new_parts );
                        break;

                    case GeoShape::TYPE_POLYGON:
                        clampPolyPartToTerrain( part, terrain, env->getInputSRS(), ignore_z, env->getTerrainReadCallback(), out_clamped_z );
                        break;
                    }

                    cleansePart( part );
                }


                if ( out_clamped_z < min_clamped_z )
                    min_clamped_z = out_clamped_z;
            }

            if ( new_parts.size() > 0 )
            {
                shape.getParts().swap( new_parts );
            }
        }

        if ( getClampedZOutputAttribute().length() > 0 )
        {
            input->setAttribute( getClampedZOutputAttribute(), min_clamped_z );
        }
    }

    output.push_back( input );
    return output;
}

