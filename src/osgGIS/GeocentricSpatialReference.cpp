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

#include <osgGIS/GeocentricSpatialReference>
#include <osgGIS/GeoShape>

using namespace osgGIS;

GeocentricSpatialReference::GeocentricSpatialReference( const SpatialReference* _basis )
{
    basis = _basis? (SpatialReference*)_basis->getBasisSRS() : NULL;
}

GeocentricSpatialReference::GeocentricSpatialReference(const SpatialReference* _basis,
                                                       const osg::Matrixd&     _ref_frame)
{
    basis = _basis? (SpatialReference*)_basis->getBasisSRS() : NULL;
    ref_frame = _ref_frame;
    inv_ref_frame = osg::Matrixd::inverse( _ref_frame );
}


GeocentricSpatialReference::~GeocentricSpatialReference()
{
}


SpatialReference*
GeocentricSpatialReference::cloneWithNewReferenceFrame( const osg::Matrixd& new_rf ) const
{
    return new GeocentricSpatialReference( basis.get(), new_rf );
}


const std::string&
GeocentricSpatialReference::getWKT() const
{
    return basis->getWKT();
}

bool
GeocentricSpatialReference::isGeographic() const
{
    return false;
}

bool GeocentricSpatialReference::isProjected() const
{
    return false;
}

bool
GeocentricSpatialReference::isGeocentric() const
{
    return true;
}


const SpatialReference*
GeocentricSpatialReference::getBasisSRS() const
{
    return basis->getBasisSRS();
}


const Ellipsoid&
GeocentricSpatialReference::getBasisEllipsoid() const
{
    return basis->getBasisEllipsoid();
}


const osg::Matrixd&
GeocentricSpatialReference::getReferenceFrame() const
{
    return ref_frame;
}


const osg::Matrixd&
GeocentricSpatialReference::getInverseReferenceFrame() const
{
    return inv_ref_frame;
}


std::string
GeocentricSpatialReference::getName() const
{
    return "Geocentric SRS; " + basis->getName();
}


bool
GeocentricSpatialReference::equivalentTo( const SpatialReference* rhs ) const
{
    //TODO: address this; for now all geocentric SRS's are equivalent
    return rhs->isGeocentric() && ref_frame == ((SpatialReferenceBase*)rhs)->getRefFrame();
}


GeoPoint
GeocentricSpatialReference::transform( const GeoPoint& input ) const
{
    GeoPoint result( input );
    transformInPlace( result );
    return result;
}

bool
GeocentricSpatialReference::transformInPlace( GeoPoint& input ) const
{
    // transform to lat/lon
    // TODO: account for differing datums - for now this assumes the geographic
    //       reference systems are equivalent
    const SpatialReference* input_srs = input.getSRS();

    // first bring the input point out of its reference frame if necessary:
    if ( !input_srs->getReferenceFrame().isIdentity() )
    {
        input = input.getAbsolute();
    }

    // next transform it to lat/lon, if necessary:
    GeoPoint input_geog = input_srs->isGeographic()? 
        input : 
        input_srs->getBasisSRS()->transform( input );

    // next transform it to geocentric:
    // (TODO: use the proper ellipsoid devired from the SRS, not this default one)
    osg::Vec3d pt_geoc = ellipsoid.latLongToGeocentric( input_geog );

    // finally, make the point relative to the target reference frame if necessary:
    if ( !this->getReferenceFrame().isIdentity() )
    {
        pt_geoc = pt_geoc * this->getReferenceFrame();
    }

    input = GeoPoint( pt_geoc, this );
    return true;
}


GeoShape
GeocentricSpatialReference::transform( const GeoShape& input ) const
{
    GeoShape result( input ); //copy
    transformInPlace( result );
    return result;
}


bool
GeocentricSpatialReference::transformInPlace( GeoShape& input ) const
{
    struct XformVisitor : public GeoPointVisitor {
        XformVisitor( const SpatialReference* _sr, const Ellipsoid& _e ) 
            : sr(_sr), e(_e) { }
        const SpatialReference* sr;
        const Ellipsoid& e;
        bool visitPoint( GeoPoint& p ) {
            return sr->transformInPlace( p );
        }
    };

    if ( input.accept( XformVisitor( this, ellipsoid ) ) )
    {
        applyTo( input );
        return true;
    }
    else
    {
        return false;
    }
}
