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
    extent = GeoExtent::invalid();
    return shapes;
}


const GeoExtent&
OGR_Feature::getExtent() const
{
    if ( !extent.isValid() )
    {
        OGR_Feature* non_const_this = const_cast<OGR_Feature*>( this );
        GeoShapeList& shapes = non_const_this->getShapes();
        int j = 0;
        for( GeoShapeList::iterator i = shapes.begin(); i != shapes.end(); i++, j++ )
        {
            GeoShape& shape = *i;
            if ( j == 0 )
                non_const_this->extent = shape.getExtent();
            else
                non_const_this->extent.expandToInclude( shape.getExtent() );
        }
    }
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
            GeoPoint( x, y, shape.getSRS() ) :
            GeoPoint( x, y, z, shape.getSRS() );
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
    // TODO: need a mutex here? is there any caching going on? Ideally,
    // each OGR_Feature instance is unique relative to the compilation
    // context.
    AttributeTable::const_iterator i = getUserAttrs().find( key );
    if ( i != getUserAttrs().end() )
    {
        return i->second;
    }
    else //TODO: perhaps cache the features in the UserAttrs table?
    {
        OGR_SCOPE_LOCK();
        int index = OGR_F_GetFieldIndex( handle, key.c_str() );
        if ( index > 0 )
        {
            void* field_handle_ref = OGR_F_GetFieldDefnRef( handle, index );
            OGRFieldType ft = OGR_Fld_GetType( field_handle_ref );
            switch( ft )
            {
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


AttributeList
OGR_Feature::getAttributes() const
{
    AttributeTable attrs;

    // accumulate the attrs from the store:
    OGR_SCOPE_LOCK();
    int count = OGR_F_GetFieldCount( handle );
    for( int i=1; i<=count; i++ )
    {
        void* field_handle_ref = OGR_F_GetFieldDefnRef( handle, i );
        const char*  field_name  = OGR_Fld_GetNameRef( field_handle_ref );
        attrs[ std::string(field_name) ] = getAttribute( std::string(field_name) );
    }

    // finally add in the user attrs (overwriting the store attrs if necessary)
    for( AttributeTable::const_iterator i = getUserAttrs().begin(); i != getUserAttrs().end() ; i++ )
        attrs[ (*i).first ] = (*i).second;

    // shove it all into a list
    AttributeList result( attrs.size() );
    for( AttributeTable::const_iterator i = attrs.begin(); i != attrs.end(); i++ )
        result.push_back( (*i).second );

    return result;
}


AttributeSchemaList
OGR_Feature::getAttributeSchemas() const
{
    AttributeSchemaTable table;

    // first collect the in-store attrs:
    OGR_SCOPE_LOCK();
    int count = OGR_F_GetFieldCount( handle );
    for( int i=0; i<count; i++ )
    {
        void* field_handle_ref = OGR_F_GetFieldDefnRef( handle, i );

        OGRFieldType field_type  = OGR_Fld_GetType( field_handle_ref );
        const char*  field_name  = OGR_Fld_GetNameRef( field_handle_ref );
        int          field_width = OGR_Fld_GetWidth( field_handle_ref );
        int          field_just  = OGR_Fld_GetJustify( field_handle_ref );
        int          field_prec  = OGR_Fld_GetPrecision( field_handle_ref );

        Attribute::Type type;
        Properties props;

        switch( field_type )
        {
        case OFTInteger:
            type = Attribute::TYPE_INT;
            props.push_back( Property( "width", field_width ) );
            props.push_back( Property( "precision", field_prec ) );
            break;

        case OFTReal:
            type = Attribute::TYPE_DOUBLE;
            props.push_back( Property( "width", field_width ) );
            props.push_back( Property( "precision", field_prec ) );
            break;

        case OFTString:
            type = Attribute::TYPE_STRING;
            props.push_back( Property( "width", field_width ) );
            props.push_back( Property( "justification", field_just ) );
            break;

        default:
            type = Attribute::TYPE_UNSPECIFIED;
        }

        std::string name(field_name);
        table[ name ] = AttributeSchema( name, type, props );
    }

    // collect the user attrs second:
    for( AttributeTable::const_iterator i = getUserAttrs().begin(); i != getUserAttrs().end(); i++ )
    {
        table[ i->first ] = AttributeSchema( i->first, i->second.getType(), Properties() );
    }

    // convert to a list
    AttributeSchemaList result;

    for( AttributeSchemaTable::const_iterator i = table.begin(); i != table.end(); i++ )
    {
        result.push_back( i->second );
    }

    return result;
}