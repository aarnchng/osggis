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

#include <osgGIS/OGR_Feature>
#include <osgGIS/OGR_Utils>
#include <ogr_api.h>
#include <osg/Notify>
#include <algorithm>

using namespace osgGIS;

OGR_Feature::OGR_Feature( void* _handle, SpatialReference* _sr )
{
    handle = _handle;
	spatial_ref = _sr;
    load( _handle );
}


OGR_Feature::~OGR_Feature()
{
    OGR_SCOPE_LOCK();
    OGR_F_Destroy( handle );
}


const FeatureOID&
OGR_Feature::getOID() const
{
    return oid;
}


const GeoShapeList&
OGR_Feature::getShapes() const
{
    return shapes;
}


GeoShapeList&
OGR_Feature::getShapes()
{
    return shapes;
}


const GeoExtent&
OGR_Feature::getExtent() const
{
    return extent;
}


void
OGR_Feature::load( void* handle )
{
    OGR_SCOPE_LOCK();

    oid = (FeatureOID)OGR_F_GetFID( handle );

    void* geom_handle = OGR_F_GetGeometryRef( handle );
	
	if ( geom_handle )
	{
		OGRwkbGeometryType wkb_type = OGR_G_GetGeometryType( geom_handle );
		GeoShape::ShapeType shape_type;

		if ( 
			wkb_type == wkbLineString ||
			wkb_type == wkbLineString25D ||
			wkb_type == wkbMultiLineString ||
			wkb_type == wkbMultiLineString25D )
		{
			shape_type = GeoShape::TYPE_LINE;
		}
		else if (
			wkb_type == wkbMultiPoint ||
			wkb_type == wkbMultiPoint25D ||
			wkb_type == wkbPoint ||
			wkb_type == wkbPoint25D )
		{
			shape_type = GeoShape::TYPE_POINT;
		}
		else if (
			wkb_type == wkbMultiPolygon ||
			wkb_type == wkbMultiPolygon25D ||
			wkb_type == wkbPolygon ||
			wkb_type == wkbPolygon25D )
		{
			shape_type = GeoShape::TYPE_POLYGON;
		}
		else // unsupported type.
		{
			osg::notify( osg::WARN ) << "Unsupported WKB shape type:" << wkb_type << std::endl;
            //TODO: set @invald@ code??
			return;
		}

		int dim = OGR_G_GetCoordinateDimension( geom_handle );

        bool multi_shape = wkb_type == wkbMultiPolygon || wkb_type == wkbMultiPolygon25D;

        if ( !multi_shape )
        {
            GeoShape shape = decodeShape( geom_handle, dim, shape_type );
            if ( shape.getParts().size() > 0 )
            {
                shapes.push_back( shape );
                extent.expandToInclude( shape.getExtent() );
            }
        }
        else
        {
            int num_shapes = OGR_G_GetGeometryCount( geom_handle );
            for( int n=0; n<num_shapes; n++ )
            {
                void* shape_handle = OGR_G_GetGeometryRef( geom_handle, n );
                GeoShape shape = decodeShape( shape_handle, dim, shape_type );
                if ( shape.getParts().size() )
                {
                    shapes.push_back( shape );
                    extent.expandToInclude( shape.getExtent() );
                }
            }
        }
	}
}


void
decodePart( void* handle, GeoShape& shape, int dim )
{
    OGR_SCOPE_LOCK();
    int num_points = OGR_G_GetPointCount( handle );
    GeoPointList& part = shape.addPart( num_points );

    for( int v = num_points-1, j=0; v >= 0; v--, j++ ) // reserve winding
    {
        double x, y, z = 0.0;
        OGR_G_GetPoint( handle, v, &x, &y, &z );

        part[j] = dim == 2?
            GeoPoint( x, y, z, shape.getSRS() ) :
            GeoPoint( x, y, shape.getSRS() );
    }
}


GeoShape
OGR_Feature::decodeShape( void* geom_handle, int dim, GeoShape::ShapeType shape_type )
{
    OGR_SCOPE_LOCK();
    int num_parts = OGR_G_GetGeometryCount( geom_handle );

    GeoShape shape( shape_type, spatial_ref.get() );

    if ( num_parts == 0 )
    {
        //osg::notify( osg::WARN ) << "NUMPARTS = 0..." << std::endl;
        decodePart( geom_handle, shape, dim );
    }
    else
    {
        for( int p = 0; p < num_parts; p++ )
        {
            void* part_handle = OGR_G_GetGeometryRef( geom_handle, p );
            if ( part_handle )
            {
                decodePart( part_handle, shape, dim );
            }
        }
    }

    return shape;
}


Attribute
OGR_Feature::getAttribute( const std::string& key ) const
{
    AttributeTable::const_iterator i = user_attrs.find( key );
    if ( i != user_attrs.end() )
    {
        return i->second;
    }
    else
    {
        OGR_SCOPE_LOCK();
        int index = OGR_F_GetFieldIndex( handle, key.c_str() );
        if ( index > 0 )
        {
            void* field_handle_ref = OGR_F_GetFieldDefnRef( handle, index );
            OGRFieldType ft = OGR_Fld_GetType( field_handle_ref );
            switch( ft ) {
                case OFTInteger:
                    return Attribute( key, OGR_F_GetFieldAsInteger( handle, index ) );
                    break;
                case OFTReal:
                    return Attribute( key, OGR_F_GetFieldAsDouble( handle, index ) );
                    break;
                case OFTString:
                    return Attribute( key, OGR_F_GetFieldAsString( handle, index ) );
                    break;
            }
        }
    }  

    return invalid_attr;
}


void 
OGR_Feature::setAttribute( const std::string& key, const std::string& value )
{
    //TODO
    //don't forget the mutex
}

void 
OGR_Feature::setAttribute( const std::string& key, int value )
{
    //TODO
    //don't forget the mutex
}

void 
OGR_Feature::setAttribute( const std::string& key, double value )
{
    //TODO
    //don't forget the mutex
}
