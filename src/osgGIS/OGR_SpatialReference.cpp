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

#include <osgGIS/OGR_SpatialReference>
#include <osgGIS/OGR_Utils>
#include <osgGIS/Registry>
#include <ogr_api.h>
#include <ogr_spatialref.h>
#include <osg/Notify>

using namespace osgGIS;

OGR_SpatialReference::OGR_SpatialReference(void* _handle, 
                                           bool _delete_handle,
                                           const osg::Matrixd& _ref_frame )
{
	handle        = _handle;
	owns_handle   = _delete_handle;
    ref_frame     = _ref_frame;
    inv_ref_frame = _ref_frame.isIdentity()? _ref_frame : osg::Matrixd::inverse( _ref_frame );
    getWKT();

    // intialize the basis ellipsoid
    OGR_SCOPE_LOCK();
    int err;
    double semi_major_axis = OSRGetSemiMajor( handle, &err );
    double semi_minor_axis = OSRGetSemiMinor( handle, &err );
    ellipsoid = Ellipsoid( semi_major_axis, semi_minor_axis );
    
    is_geographic = OSRIsGeographic( handle ) != 0;
    is_projected = OSRIsProjected( handle ) != 0;
}


OGR_SpatialReference::~OGR_SpatialReference()
{
	if ( handle && owns_handle )
	{
      OGR_SCOPE_LOCK();
		OSRDestroySpatialReference( handle );
	}
	handle = NULL;
}


void*
OGR_SpatialReference::getHandle() 
{
    return handle;
}


SpatialReference* 
OGR_SpatialReference::cloneWithNewReferenceFrame( const osg::Matrixd& new_rf ) const
{
    OGR_SCOPE_LOCK();
    return new OGR_SpatialReference( handle, false, new_rf );
}


const std::string&
OGR_SpatialReference::getWKT() const
{
	if ( handle && wkt.length() == 0 )
	{
        OGR_SCOPE_LOCK();
		char* buf;
		if ( OSRExportToWkt( handle, &buf ) == OGRERR_NONE )
		{
			((OGR_SpatialReference*)this)->wkt = buf; //std::string( buf );
			OGRFree( buf );
		}		
	}
	return wkt;
}


GeoPoint
OGR_SpatialReference::transform( const GeoPoint& input ) const
{
    GeoPoint result = input; // copy it
    return transformInPlace( result )? result : GeoPoint::invalid();
}


bool
OGR_SpatialReference::transformInPlace( GeoPoint& input ) const
{
    if ( !handle || !input.isValid() ) {
        osg::notify( osg::WARN ) << "Spatial reference or input point is invalid" << std::endl;
        return false;
    }

	OGR_SpatialReference* input_sr = (OGR_SpatialReference*)input.getSRS();
    if ( !input_sr ) {
        osg::notify( osg::WARN ) << "SpatialReference: input point has no SRS" << std::endl;
        return false;
    }

    // first check whether the input point is geocentric - and if so, pre-convert it to geographic:
    if ( input_sr->isGeocentric() )
    {
        input.set( input * input_sr->getInverseReferenceFrame() );
        osg::Vec3 temp = input_sr->getBasisEllipsoid().geocentricToLatLong( input );
        input = GeoPoint( temp, input_sr->getBasisSRS() ); 
        input_sr = static_cast<OGR_SpatialReference*>( input.getSRS() );
    }

    osg::Vec3d input_vec = input;
    bool crs_equiv = false;
    bool mat_equiv = false;
    testEquivalence( input_sr, /*out*/crs_equiv, /*out*/mat_equiv );

    // pull it out of its source frame:
    if ( !mat_equiv )
    {
        input.set( input * input_sr->inv_ref_frame );
    }    

    bool result = false;

    if ( !crs_equiv )
    {
        OGR_SCOPE_LOCK();

        void* xform_handle = OCTNewCoordinateTransformation( input_sr->handle, this->handle );
        if ( !xform_handle ) {
            osg::notify( osg::WARN ) << "Spatial Reference: SRS xform not possible" << std::endl
                << "    From => " << input_sr->getName() << std::endl
                << "    To   => " << this->getName() << std::endl;
            return false;
        }

        //TODO: figure out why xforming GEOCS x=-180 to another GEOCS doesn't work
        if ( OCTTransform( xform_handle, 1, &input.x(), &input.y(), &input.z() ) )
        {
            result = true;
        }
        else
        {
            osg::notify( osg::WARN ) << "Spatial Reference: Failed to xform a point from "
                << input_sr->getName() << " to " << this->getName()
                << std::endl;
        }

        OCTDestroyCoordinateTransformation( xform_handle );
    }
    else
    {
        result = true;
    }

    // put it into the new ref frame:
    if ( !mat_equiv )
    {
        input.set( input * ref_frame );
    }

    if ( result == true )
    {
        applyTo( input );
    }

    return result;
}

GeoExtent
OGR_SpatialReference::transform( const GeoExtent& input ) const
{
    GeoPoint sw = transform( input.getSouthwest() );
    GeoPoint ne = transform( input.getNortheast() );
    return GeoExtent( sw, ne );
}


GeoShape
OGR_SpatialReference::transform( const GeoShape& input ) const
{
    GeoShape result = input; // copy
    return transformInPlace( result )? result : input; // GeoShape::invalid();
}


bool
OGR_SpatialReference::transformInPlace( GeoShape& input ) const
{	
    if ( !handle ) {
        osg::notify( osg::WARN ) << "OGR_SpatialReference: SRS is invalid" << std::endl;
        return false;
    }

	OGR_SpatialReference* input_sr = (OGR_SpatialReference*)input.getSRS();
    //if ( SpatialReference::equivalent( this, input_sr ) ) {
    //    return true; // same SRS; no action required
    //}
    
    bool crs_equiv = false;
    bool mat_equiv = false;
    testEquivalence( input_sr, /*out*/crs_equiv, /*out*/mat_equiv );
    if ( crs_equiv && mat_equiv )
        return true;

    OGR_SCOPE_LOCK();

	void* xform_handle = NULL;
    
    if ( !crs_equiv )
    {
        xform_handle = OCTNewCoordinateTransformation( input_sr->handle, this->handle );

        if ( !xform_handle ) {
            osg::notify( osg::WARN ) << "OGR_SpatialReference: SRS xform not possible" << std::endl;
            return false;
        }
    }
    
    //struct XformVisitor : public GeoPointVisitor {
    //    XformVisitor( void* _h, const osg::Matrixd& _src_rf, const osg::Matrixd& _to_rf )
    //        : handle( _h ), src_rf(_src_rf), to_rf(_to_rf) { }
    //    void* handle;
    //    osg::Matrixd src_rf, to_rf;

    //    bool visitPoint( GeoPoint& p )
    //    {
    //        bool ok = true;

    //        // pull it out of the source reference frame:
    //        p.set( p * src_rf );

    //        // reproject it:
    //        if ( handle )
    //            ok = OCTTransform( handle, 1, &p.x(), &p.y(), &p.z() ) != 0;

    //        // push it into the new reference frame:
    //        p.set( p * to_rf );

    //        return ok;
    //    }
    //};
    
    struct XformVisitor : public GeoPointVisitor {
        XformVisitor( void* _h, const osg::Matrixd& _src_rf, const SpatialReference* _to_srs )
            : handle( _h ), src_rf(_src_rf), to_srs(_to_srs) { }
        void* handle;
        osg::Matrixd src_rf;
        const osgGIS::SpatialReference* to_srs;

        bool visitPoint( GeoPoint& p )
        {
            bool ok = true;

            // pull it out of the source reference frame:
            p.set( p * src_rf );

            // reproject it:
            if ( handle )
                ok = OCTTransform( handle, 1, &p.x(), &p.y(), &p.z() ) != 0;

            // push it into the new reference frame:
            p.set( p * to_srs->getReferenceFrame() );

            p = p.getDim() == 2? GeoPoint( p.x(), p.y(), to_srs ) : GeoPoint( p.x(), p.y(), p.z(), to_srs );

            return ok;
        }
    };

    bool result = false;

    XformVisitor visitor( 
        xform_handle, 
        input_sr->getInverseReferenceFrame(),
        this );
        //this->getReferenceFrame() );

    if ( input.accept( visitor ) )
    {
        applyTo( input );
        result = true;
    }
    else
    {
        osg::notify( osg::WARN ) << "Failed to xform a point from "
            << input_sr->getName() << " to " << this->getName()
            << std::endl;
    }

    if ( xform_handle )
    {
        OCTDestroyCoordinateTransformation( xform_handle );
    }

    return result;
}


bool
OGR_SpatialReference::isGeographic() const
{
    return is_geographic;
}


bool
OGR_SpatialReference::isProjected() const
{
    return is_projected;
}


bool
OGR_SpatialReference::isGeocentric() const
{
    return false;
}

const SpatialReference*
OGR_SpatialReference::getBasisSRS() const
{
    if ( !basis.valid() )
    {
        if ( this->isGeographic() )
        {
            ((OGR_SpatialReference*)this)->basis = (SpatialReference*)this;
        }
	    else if ( handle )
	    {
            OGR_SCOPE_LOCK();

		    void* new_handle = OSRNewSpatialReference( NULL );
		    int err = OSRCopyGeogCSFrom( new_handle, handle );
		    if ( err == OGRERR_NONE )
		    {
                ((OGR_SpatialReference*)this)->basis = 
                    new OGR_SpatialReference( new_handle, true, osg::Matrixd() );
		    }
		    else
		    {
			    OSRDestroySpatialReference( new_handle );
		    }
	    }
    }

	return basis.get();
}


const Ellipsoid&
OGR_SpatialReference::getBasisEllipsoid() const
{
    return ellipsoid;
}


const osg::Matrixd&
OGR_SpatialReference::getReferenceFrame() const
{
    return ref_frame;
}


const osg::Matrixd&
OGR_SpatialReference::getInverseReferenceFrame() const
{
    return inv_ref_frame;
}


std::string
OGR_SpatialReference::getAttrValue( const std::string& name, int child_num ) const
{
    OGR_SCOPE_LOCK();
	const char* val = OSRGetAttrValue( handle, name.c_str(), child_num );
	return val? std::string( val ) : "";
}


std::string
OGR_SpatialReference::getName() const
{
	return std::string(
		isGeographic()? getAttrValue( "GEOGCS", 0 ) : getAttrValue( "PROJCS", 0 ) );
}


bool
OGR_SpatialReference::equivalentTo( const SpatialReference* rhs ) const
{
    if ( !rhs ) return false;
    bool crs_e, mat_e;
    testEquivalence( (const OGR_SpatialReference*)rhs, crs_e, mat_e );
    return crs_e && mat_e;
}


void
OGR_SpatialReference::testEquivalence(const OGR_SpatialReference* rhs,
                                      bool& out_crs_equiv,
                                      bool& out_mat_equiv ) const
{
    // for now, we consider all GEOGRAPHIC SRS's to be equivalent. We can
    // fix this later
    out_crs_equiv = 
        this == rhs ||
        ( this && rhs && this->isGeographic() && rhs->isGeographic() );

    out_mat_equiv = this->getRefFrame() == rhs->getRefFrame();
}