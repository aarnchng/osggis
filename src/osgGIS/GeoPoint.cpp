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

#include <osgGIS/GeoPoint>
#include <osgGIS/GeoExtent>
#include <osgGIS/Registry>
#include <sstream>

using namespace osgGIS;


GeoPoint::GeoPoint()
{
    spatial_ref = NULL;
    dim = 0;
}


GeoPoint
GeoPoint::invalid()
{
    return GeoPoint();
}


GeoPoint::GeoPoint( const GeoPoint& rhs )
: osg::Vec3d( rhs )
{
    dim = rhs.dim;
    spatial_ref = rhs.spatial_ref.get();
}


GeoPoint::GeoPoint( const osg::Vec2d& _input, const SpatialReference* _srs )
: osg::Vec3d( _input.x(), _input.y(), 0.0 )
{
    spatial_ref = (SpatialReference*)_srs;
    dim = 2;
}


GeoPoint::GeoPoint( const osg::Vec3d& _input, const SpatialReference* _srs )
: osg::Vec3d( _input )
{
    spatial_ref = (SpatialReference*)_srs;
    dim = 3;
}


GeoPoint::GeoPoint( double _x, double _y, const SpatialReference* _sr )
: osg::Vec3d( _x, _y, 0.0 )
{
    spatial_ref = (SpatialReference*)_sr;
    dim = 2;
}


GeoPoint::GeoPoint( double _x, double _y, double _z, const SpatialReference* _sr )
: osg::Vec3d( _x, _y, _z )
{
    spatial_ref = (SpatialReference*)_sr;
    dim = 3;
}


GeoPoint::~GeoPoint()
{
	//NOP
}

bool
GeoPoint::isValid() const
{
    return dim > 0 && spatial_ref.valid();
}


bool
GeoPoint::operator == ( const GeoPoint& rhs ) const
{
    return
        isValid() && rhs.isValid() &&
        SpatialReference::equivalent( getSRS(), rhs.getSRS() ) &&
        getDim() == rhs.getDim() &&
        x() == rhs.x() &&
        (getDim() < 2 || y() == rhs.y()) &&
        (getDim() < 3 || z() == rhs.z());
}


unsigned int
GeoPoint::getDim() const
{
    return dim;
}

void
GeoPoint::setDim( int _dim )
{
    dim = _dim > 0 && _dim <= 3? _dim : dim;
}


const SpatialReference*
GeoPoint::getSRS() const
{
	return spatial_ref.get();
}


SpatialReference*
GeoPoint::getSRS()
{
    return spatial_ref.get();
}


void
GeoPoint::setSpatialReference( const SpatialReference* sr )
{
    spatial_ref = (SpatialReference*)sr;
}


std::string
GeoPoint::toString() const
{
    std::stringstream str;
    if ( isValid() && getDim() == 2 )
        str << x() << ", " << y();
    else if ( isValid() && getDim() == 3 )
        str << x() << ", " << y() << ", " << z();
    else
        str << "INVALID";
    return str.str();
}


GeoPoint
GeoPoint::getAbsolute() const
{
    //osg::ref_ptr<SpatialReference> new_srs = 
    //    getSRS()->cloneWithNewReferenceFrame( osg::Matrixd() );

    //return new_srs->transform( *this );

    return GeoPoint(
        (*this) * getSRS()->getInverseReferenceFrame(),
        getSRS()->cloneWithNewReferenceFrame( osg::Matrix::identity() ) );
}


/**************************************************************************/

bool
GeoPointList::intersects( const GeoExtent& ex ) const
{
    if ( ex.isInfinite() )
        return true;

    //TODO: srs transform

    if ( ex.isPoint() ) // do a point-in-polygon test
    {
        const GeoPoint& point = ex.getSouthwest();
        bool result = false;
        const GeoPointList& polygon = *this;
        for( unsigned int i=0, j=polygon.size()-1; i<polygon.size(); j = i++ )
        {
            if ((((polygon[i].y() <= point.y()) && (point.y() < polygon[j].y())) ||
                 ((polygon[j].y() <= point.y()) && (point.y() < polygon[i].y()))) &&
                (point.x() < (polygon[j].x()-polygon[i].x()) * (point.y()-polygon[i].y())/(polygon[j].y()-polygon[i].y())+polygon[i].x()))
            {
                result = !result;
            }
        }
        return result;
    }
    else // check for all points within extent -- not actually correct //TODO
    {
        GeoExtent e;
        for( GeoPointList::const_iterator i = begin(); i != end(); i++ )
        {
            if ( i == begin() ) e = GeoExtent( *i, *i );
            else e.expandToInclude( *i );
        }
        return e.intersects( ex );
    }    
}

bool
GeoPointList::isClosed() const
{
    return size() >= 2 && front() == back();
}