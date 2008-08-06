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
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/PlaneIntersector>
#include <osg/PagedLOD>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( ClampFilter );


ClampFilter::ClampFilter()
{
    ignore_z = false;
}

ClampFilter::ClampFilter( const ClampFilter& rhs )
: FeatureFilter( rhs ),
  ignore_z( rhs.ignore_z ),
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
    //if ( getElevationResourceScript() )
    //    p.push_back( Property( "elevation", getElevationResourceScript()->getCode() ) );

    return p;
}

#define ALTITUDE_EXTENSION 250000
#define DEFAULT_OFFSET_EXTENSION 0


// custom intersection visitor that can "fall back" on lower resolution tiles when
// a pages lod subtile cannot be found.
//
// NOTE: I submitted this change to the OSG core -- it will become the default behavior in
// the 2.6 stable release.
class RelaxedIntersectionVisitor : public osgUtil::IntersectionVisitor
{
public:
    RelaxedIntersectionVisitor() { }

    virtual void apply( osg::PagedLOD& plod ) // override
    {
        if (!enter(plod)) return;

        if (plod.getNumFileNames()>0)
        {
            osg::ref_ptr<osg::Node> highestResChild;

            if (plod.getNumFileNames() != plod.getNumChildren() && _readCallback.valid())
            {
                highestResChild = _readCallback->readNodeFile( plod.getDatabasePath() + plod.getFileName(plod.getNumFileNames()-1) );
            }

            if ( !highestResChild.valid() && plod.getNumChildren() > 0 )
            {
                highestResChild = plod.getChild( plod.getNumChildren()-1 );
            }

            if (highestResChild.valid())
            {
                highestResChild->accept(*this);
            }
        }

        leave();
    }
};


static int
clampPointPartToTerrain(GeoPointList&           part,
                        osg::Node*              terrain,
                        const SpatialReference* srs,
                        bool                    ignore_z,
                        SmartReadCallback*      read_cache,
                        double&                 out_clamped_z )
{
    int clamps = 0;
    out_clamped_z = DBL_MAX;

    RelaxedIntersectionVisitor iv;
    //osgUtil::IntersectionVisitor iv;

    if ( read_cache )
        iv.setReadCallback( read_cache );

    for( GeoPointList::iterator i = part.begin(); i != part.end(); i++ )
    {
        GeoPoint& p = *i;

        osg::Vec3d p_world = p * srs->getInverseReferenceFrame();
        osg::Vec3d clamp_vec;

        double lat = 0.0, lon = 0.0;
        double hat = 0.0;
        //double z = (p.getDim() > 2 && !ignore_z)? p.z() : DEFAULT_OFFSET_EXTENSION;

        osg::ref_ptr<LineSegmentIntersector2> isector;

        if ( srs->isGeocentric() )
        {
            clamp_vec = p_world;
            clamp_vec.normalize();
            
            srs->getEllipsoid().xyzToLatLonHeight( p_world.x(), p_world.y(), p_world.z(), lat, lon, hat );

            // to get rid of the HAT:
            osg::Vec3d xyz = srs->getEllipsoid().latLongToGeocentric(
                osg::Vec3d( osg::RadiansToDegrees(lon), osg::RadiansToDegrees(lat), 0.0 ) );
            
            isector = new LineSegmentIntersector2(
                clamp_vec * srs->getEllipsoid().getSemiMajorAxis() * 1.2,
                osg::Vec3d( 0, 0, 0 ) );
            
            // calculate the HAT for later:
            if ( !ignore_z )
            {
                srs->getEllipsoid().xyzToLatLonHeight( p_world.x(), p_world.y(), p_world.z(), lat, lon, hat );
            }
        }
        else
        {
            clamp_vec = osg::Vec3d( 0, 0, 1 );
            osg::Vec3d ext_vec = clamp_vec * ALTITUDE_EXTENSION;
            isector = new LineSegmentIntersector2(
                p_world + ext_vec,
                p_world - ext_vec );

            if ( p.getDim() > 2 && !ignore_z )
            {
                hat = p.z();
            }
        }

        iv.setIntersector( isector.get() );

        // try the read cache's MRU node if it intersects out point:
        osg::Node* target = terrain;
        if ( read_cache )
        {
            target = read_cache->getMruNodeIfContains( p_world, terrain );

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
            osg::Vec3d offset_point = new_point + clamp_vec * hat;
            p.set( offset_point * srs->getReferenceFrame() );
            clamps++;
            if ( read_cache )
                read_cache->setMruNode( isect.nodePath.back().get() );

            // record the HAT value:
            if ( srs->isGeocentric() )
            {
                if ( hat < out_clamped_z )
                    out_clamped_z = hat;

                //double lat, lon, h;
                //srs->getEllipsoid().xyzToLatLonHeight( offset_point.x(), offset_point.y(), offset_point.z(), lat, lon, h );
                //if ( h < out_clamped_z )
                //    out_clamped_z = h;
            }
            else
            {
                if ( offset_point.z() < out_clamped_z )
                    out_clamped_z = offset_point.z();
            }
            
            //osg::notify( osg::WARN ) << "Found an intersection for point " 
            //    << osg::RadiansToDegrees(lat) << ", " << osg::RadiansToDegrees(lon) << ", " << hat
            //    << std::endl;
        }

        else 
        {
            //osg::notify( osg::WARN ) << "MISSED an intersection for point " 
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

            planes[2] = osg::Plane( midpoint_normal, osg::Vec3d(0,0,0) );

            osg::Polytope polytope( planes );

            //osg::Plane plane( p0_world, p1_world, midpoint );
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
                    read_cache->setMruNode( isect.nodePath.back() );

                // add the new polyline part to the result set:
                if ( new_part.size() >= 2 )
                    out_parts.push_back( new_part );

                //TESTING - just read the first result:
                //break;
            }
        }
        else
        {
            osg::notify( osg::INFO )
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
