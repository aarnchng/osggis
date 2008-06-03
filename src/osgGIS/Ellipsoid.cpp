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

#include <osgGIS/Ellipsoid>
#include <osgGIS/GeoPoint>
#include <osgGIS/Registry>
#include <osg/CoordinateSystemNode>

using namespace osgGIS;


Ellipsoid::Ellipsoid()
{
    semi_major_axis = osg::WGS_84_RADIUS_EQUATOR;
    semi_minor_axis = osg::WGS_84_RADIUS_POLAR;
    double flattening = (semi_major_axis-semi_minor_axis)/semi_major_axis;
    ecc2 = 2*flattening - flattening*flattening;
}


Ellipsoid::Ellipsoid( double _semi_major_axis, double _semi_minor_axis )
{
    semi_major_axis = _semi_major_axis;
    semi_minor_axis = _semi_minor_axis;
    double flattening = (semi_major_axis-semi_minor_axis)/semi_major_axis;
    ecc2 = 2*flattening - flattening*flattening;
}


osg::Vec3d
Ellipsoid::latLongToGeocentric(const osg::Vec3d& input_deg ) const 
{
    double latitude = osg::DegreesToRadians( input_deg.y() );
    double longitude = osg::DegreesToRadians( input_deg.x() );
    double height = input_deg.z();
    double sin_latitude = sin( latitude );
    double cos_latitude = cos( latitude );
    double N = semi_major_axis/sqrt( 1.0 - ecc2*sin_latitude*sin_latitude);
    double X = (N+height)*cos_latitude*cos(longitude);
    double Y = (N+height)*cos_latitude*sin(longitude);
    double Z = (N*(1-ecc2)+height)*sin_latitude;
    return osg::Vec3d( X, Y, Z );
}

osg::Vec3d
Ellipsoid::geocentricToLatLong( const osg::Vec3d& input ) const
{
    double x = input.x(), y = input.y(), z = input.z();
    double lat_rad = 0, lon_rad = 0, height_m = 0;
    xyzToLatLonHeight( x, y, z, lat_rad, lon_rad, height_m );
    return osg::Vec3d( osg::RadiansToDegrees( lon_rad ), osg::RadiansToDegrees( lat_rad ), height_m );
}

void
Ellipsoid::xyzToLatLonHeight( double X, double Y, double Z, double& lat_rad, double& lon_rad, double& height ) const
{
    // http://www.colorado.edu/geography/gcraft/notes/datum/gif/xyzllh.gif
    double p = sqrt(X*X + Y*Y);
    double theta = atan2(Z*semi_major_axis , (p*semi_minor_axis));
    double eDashSquared = (semi_major_axis*semi_major_axis - semi_minor_axis*semi_minor_axis)/
                          (semi_minor_axis*semi_minor_axis);

    double sin_theta = sin(theta);
    double cos_theta = cos(theta);

    lat_rad = atan( (Z + eDashSquared*semi_minor_axis*sin_theta*sin_theta*sin_theta) /
                     (p - ecc2*semi_major_axis*cos_theta*cos_theta*cos_theta) );
    lon_rad = atan2(Y,X);

    double sin_latitude = sin(lat_rad);
    double N = semi_major_axis / sqrt( 1.0 - ecc2*sin_latitude*sin_latitude);

    height = p/cos(lat_rad) - N;
}


osg::Matrixd
Ellipsoid::createGeocentricInvRefFrame( const GeoPoint& input ) const
{
    // first make the point geocentric if necessary:
    GeoPoint p = input;
    const osgGIS::SpatialReference* p_srs = input.getSRS();
    if ( !p_srs->isGeocentric() )
    {
        p_srs = osgGIS::Registry::instance()->getSRSFactory()->createGeocentricSRS(
            p_srs->getGeographicSRS() );

        p_srs->transformInPlace( p );
    }

    //double lat_rad, lon_rad, height;
    //xyzToLatLonHeight( p.x(), p.y(), p.z(), lat_rad, lon_rad, height );

    double X = p.x(), Y = p.y(), Z = p.z();
    osg::Matrixd localToWorld;
    localToWorld.makeTranslate(X,Y,Z);

    // normalize X,Y,Z
    double inverse_length = 1.0/sqrt(X*X + Y*Y + Z*Z);
    
    X *= inverse_length;
    Y *= inverse_length;
    Z *= inverse_length;

    double length_XY = sqrt(X*X + Y*Y);
    double inverse_length_XY = 1.0/length_XY;

    // Vx = |(-Y,X,0)|
    localToWorld(0,0) = -Y*inverse_length_XY;
    localToWorld(0,1) = X*inverse_length_XY;
    localToWorld(0,2) = 0.0;

    // Vy = /(-Z*X/(sqrt(X*X+Y*Y), -Z*Y/(sqrt(X*X+Y*Y),sqrt(X*X+Y*Y))| 
    double Vy_x = -Z*X*inverse_length_XY;
    double Vy_y = -Z*Y*inverse_length_XY;
    double Vy_z = length_XY;
    inverse_length = 1.0/sqrt(Vy_x*Vy_x + Vy_y*Vy_y + Vy_z*Vy_z);            
    localToWorld(1,0) = Vy_x*inverse_length;
    localToWorld(1,1) = Vy_y*inverse_length;
    localToWorld(1,2) = Vy_z*inverse_length;

    // Vz = (X,Y,Z)
    localToWorld(2,0) = X;
    localToWorld(2,1) = Y;
    localToWorld(2,2) = Z;

    return localToWorld;
}


double
Ellipsoid::getSemiMajorAxis() const
{
    return semi_major_axis;
}


double
Ellipsoid::getSemiMinorAxis() const
{
    return semi_minor_axis;
}

