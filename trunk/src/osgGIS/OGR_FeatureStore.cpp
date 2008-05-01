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

#include <osgGIS/OGR_FeatureStore>
#include <osgGIS/OGR_Feature>
#include <osgGIS/OGR_SpatialReference>
#include <osgGIS/OGR_Utils>
#include <osgDB/FileNameUtils>
#include <osg/Notify>
#include <ogr_api.h>


using namespace osgGIS;


// opening an existing feature store.
OGR_FeatureStore::OGR_FeatureStore( const std::string& abs_path )
: extent( GeoExtent::invalid() )
{
    OGR_SCOPE_LOCK();
    uri = abs_path;
	bool for_update = false;
    supports_random_read = false;
	ds_handle = OGROpenShared( abs_path.c_str(), (for_update? 1 : 0), NULL );
	if ( ds_handle )
	{
		layer_handle = OGR_DS_GetLayer( ds_handle, 0 ); // default to layer 0 for now
        if ( layer_handle )
        {
            supports_random_read = OGR_L_TestCapability( layer_handle, OLCRandomRead ) == TRUE;
        }
	}

    if ( isReady() )
    {
        osg::notify(osg::NOTICE) << "Opened feature store at " << getName() << std::endl
            << "   Extent = " << getExtent().toString() << std::endl;
    }
}


// creating a new feautre store.
OGR_FeatureStore::OGR_FeatureStore(const std::string&         abs_path,
                                   const GeoShape::ShapeType& shape_type,
                                   const AttributeSchemaList& schemas,
                                   int                        dimensionality,
                                   const SpatialReference*    srs,
                                   const Properties&          props )
: extent( GeoExtent::invalid() )
{
    OGR_SCOPE_LOCK();

    uri = abs_path;
    supports_random_read = false;

    // pull the appropriate OGR driver, defaulting to shapefile.
    std::string driver_name = props.getValue( "ogr-driver", "ESRI Shapefile" );
    OGRSFDriverH driver_handle = OGRGetDriverByName( driver_name.c_str() );
    if ( driver_handle )
    {
        std::string dir_name = osgDB::getFilePath( abs_path );
        ds_handle = OGR_Dr_CreateDataSource( driver_handle, dir_name.c_str(), NULL );
        if ( ds_handle )
        {
            std::string layer_name = osgDB::getStrippedName( abs_path );

            OGRwkbGeometryType geom_type;
            
            if ( dimensionality > 2 )
            {
                geom_type =
                    shape_type == GeoShape::TYPE_POINT ?   wkbMultiPoint25D :
                    shape_type == GeoShape::TYPE_POLYGON ? wkbMultiPolygon25D :
                    shape_type == GeoShape::TYPE_LINE ?    wkbMultiLineString25D :
                    wkbNone;
            }
            else
            {
                geom_type =
                    shape_type == GeoShape::TYPE_POINT ?   wkbMultiPoint :
                    shape_type == GeoShape::TYPE_POLYGON ? wkbMultiPolygon :
                    shape_type == GeoShape::TYPE_LINE ?    wkbMultiLineString :
                    wkbNone;
            }

            // figure out the SRS:
            OGRSpatialReferenceH srs_handle = NULL;
            if ( srs )
            {
                SpatialReference* nc_srs = const_cast<SpatialReference*>( srs );
                if ( dynamic_cast<OGR_SpatialReference*>( nc_srs ) )
                {
                    srs_handle = static_cast<OGR_SpatialReference*>( nc_srs )->getHandle();
                }
                else
                {
                    srs_handle = OSRNewSpatialReference( nc_srs->getWKT().c_str() );
                }
            }

            // create the new layer:
            layer_handle = OGR_DS_CreateLayer( ds_handle, layer_name.c_str(), srs_handle, geom_type, NULL );
            if ( layer_handle )
            {
                // create the field schema:
                for( AttributeSchemaList::const_iterator i = schemas.begin(); i != schemas.end(); i++ )
                {
                    const AttributeSchema& schema = *i;

                    OGRFieldType field_type =
                        schema.getType() == Attribute::TYPE_DOUBLE ? OFTReal :
                        schema.getType() == Attribute::TYPE_INT ?    OFTInteger :
                        schema.getType() == Attribute::TYPE_STRING ? OFTString :
                        OFTString;

                    OGRFieldDefnH field_handle = ::OGR_Fld_Create( schema.getName().c_str(), field_type );

                    if ( schema.getProperties().exists( "justification" ) )
                        OGR_Fld_SetJustify( field_handle, (OGRJustification)schema.getProperties().getIntValue( "justification", 0 ) );

                    if ( schema.getProperties().exists( "width" ) )
                        OGR_Fld_SetWidth( field_handle, schema.getProperties().getIntValue( "width", 11 ) );

                    if ( schema.getProperties().exists( "precision" ) )
                        OGR_Fld_SetPrecision( field_handle, schema.getProperties().getIntValue( "precision", 8 ) );

                    //TODO: handle return error:
                    if ( OGR_L_CreateField( layer_handle, field_handle, true ) != OGRERR_NONE )
                    {
                        osg::notify(osg::WARN) << "Error: OGR_FeatureStore: OGR_L_CreateField failed" << std::endl;
                    }

                    OGR_Fld_Destroy( field_handle );
                }
            }
        }
    }
}


OGR_FeatureStore::~OGR_FeatureStore()
{
    OGR_SCOPE_LOCK();
	if ( layer_handle )
	{
        OGR_L_SyncToDisk( layer_handle );
        // no need to release layer handle (held by ref)
		layer_handle = NULL;
	}

	if ( ds_handle )
	{
		OGRReleaseDataSource( ds_handle );
		ds_handle = NULL;
	}

    //osg::notify(osg::NOTICE) << "Closed feature store at " << getName() << std::endl;
}


bool
OGR_FeatureStore::supportsRandomRead() const
{
    return supports_random_read;
}


const std::string&
OGR_FeatureStore::getName() const
{
    return uri;
}


SpatialReference*
OGR_FeatureStore::getSRS()
{
	if ( !spatial_ref.get() )
	{
        OGR_SCOPE_LOCK();
		SpatialReference* result = NULL;
		void* sr_handle = OGR_L_GetSpatialRef( layer_handle );
		if ( sr_handle )
		{
            result = new OGR_SpatialReference( sr_handle, false, osg::Matrixd() );
		}
		spatial_ref = result;
	}
	return spatial_ref.get();
}


bool
OGR_FeatureStore::isReady() const
{
	return ds_handle != NULL;
}

	
Feature*
OGR_FeatureStore::getFeature( const FeatureOID& oid )
{
	Feature* result = NULL;
    if ( supports_random_read )
    {
        OGR_SCOPE_LOCK();
	    void* feature_handle = OGR_L_GetFeature( layer_handle, oid );
	    if ( feature_handle )
	    {
		    result = new OGR_Feature( feature_handle, getSRS() );
	    }
    }
    else
    {
        osg::notify( osg::WARN ) << "Feature store does not support getFeature(OID)" << std::endl;
    }
	return result;
}


FeatureCursor
OGR_FeatureStore::getCursor()
{
    if ( layer_handle )
    {
        OGR_SCOPE_LOCK();
        unsigned int feature_count = OGR_L_GetFeatureCount( layer_handle, 1 );
        FeatureOIDList oids; //( feature_count );
        if ( feature_count > 0 )
            oids.reserve( feature_count );
    
        OGR_L_ResetReading( layer_handle );
        void* feature_handle;
        //int iter = 0;
        while( (feature_handle = OGR_L_GetNextFeature( layer_handle )) != NULL )
        {
            FeatureOID oid = (FeatureOID)OGR_F_GetFID( feature_handle );
            oids.push_back( oid );
            OGR_F_Destroy( feature_handle );
        }
    
        return FeatureCursor( oids, this, GeoExtent::infinite(), false );
    }
    else
    {
        return FeatureCursor(); // empty
    }
}


int
OGR_FeatureStore::getFeatureCount() const
{
	//TODO: cache result
    OGR_SCOPE_LOCK();
	return OGR_L_GetFeatureCount( layer_handle, 1 );
}


const GeoExtent&
OGR_FeatureStore::getExtent() const
{
    if ( !extent.isValid() )
    {
        (const_cast<OGR_FeatureStore*>(this))->calcExtent();
    }
    return extent;
}

void
OGR_FeatureStore::calcExtent()
{
    OGR_SCOPE_LOCK();

	OGREnvelope envelope;

	if ( OGR_L_GetExtent( layer_handle, &envelope, 1 ) == OGRERR_NONE )
	{
		SpatialReference* sr = getSRS();

		extent = GeoExtent(
			GeoPoint( envelope.MinX, envelope.MinY, sr ),
			GeoPoint( envelope.MaxX, envelope.MaxY, sr ),
			sr );
	}
	else
	{
		osg::notify( osg::WARN ) << "Unable to compute extent for feature store" << std::endl;
        extent = GeoExtent::invalid();
	}

	if ( extent.isValid() && extent.isEmpty() )
	{
        for( FeatureCursor cursor = getCursor(); cursor.hasNext(); )
		{
			Feature* feature = cursor.next();
            const GeoShapeList& shapes = feature->getShapes();
            for( GeoShapeList::const_iterator i = shapes.begin(); i != shapes.end(); i++ )
            {
                const GeoExtent& feature_extent = i->getExtent();
			    extent.expandToInclude( feature_extent );
            }
		}
	}
}


static OGRGeometryH
encodePart( const GeoPointList& part, OGRwkbGeometryType part_type )
{
    OGRGeometryH part_handle = OGR_G_CreateGeometry( part_type );

    for( int v = part.size()-1; v >= 0; v-- )
    {
        const GeoPoint& p = part[v];
        if ( p.getDim() > 2 )
            OGR_G_AddPoint( part_handle, p.x(), p.y(), p.z() );
        else
            OGR_G_AddPoint_2D( part_handle, p.x(), p.y() );
    }

    return part_handle;
}


static OGRGeometryH
encodeShape( const GeoShape& shape, OGRwkbGeometryType shape_type, OGRwkbGeometryType part_type )
{
    OGRGeometryH shape_handle = OGR_G_CreateGeometry( shape_type );
    if ( shape_handle )
    {
        for( GeoPartList::const_iterator i = shape.getParts().begin(); i != shape.getParts().end(); i++ )
        {
            OGRGeometryH part_handle = encodePart( *i, part_type );
            if ( part_handle )
            {
                OGR_G_AddGeometryDirectly( shape_handle, part_handle );
            }
        }
    }
    return shape_handle;
}


bool
OGR_FeatureStore::insertFeature( Feature* input )
{
    OGR_SCOPE_LOCK();
    OGRFeatureH feature_handle = OGR_F_Create( OGR_L_GetLayerDefn( layer_handle ) );
    if ( feature_handle )
    {
        // assign the attributes:
        int num_fields = OGR_F_GetFieldCount( feature_handle );
        for( int i=0; i<num_fields; i++ )
        {
            OGRFieldDefnH field_handle_ref = OGR_F_GetFieldDefnRef( feature_handle, i );
            std::string name = OGR_Fld_GetNameRef( field_handle_ref );
            int field_index = OGR_F_GetFieldIndex( feature_handle, name.c_str() );
            Attribute attr = input->getAttribute( name );
            if ( attr.isValid() )
            {
                switch( OGR_Fld_GetType( field_handle_ref ) )
                {
                case OFTInteger:
                    OGR_F_SetFieldInteger( feature_handle, field_index, attr.asInt() );
                    break;
                case OFTReal:
                    OGR_F_SetFieldDouble( feature_handle, field_index, attr.asDouble() );
                    break;
                case OFTString:
                    OGR_F_SetFieldString( feature_handle, field_index, attr.asString() );
                    break;                    
                }
            }
        }

        // assign the geometry:
        OGRFeatureDefnH def = ::OGR_L_GetLayerDefn( layer_handle );

        OGRwkbGeometryType reported_type = OGR_FD_GetGeomType( def );

        OGRwkbGeometryType group_type = 
            reported_type == wkbPolygon? wkbMultiPolygon : 
            reported_type == wkbPolygon25D? wkbMultiPolygon25D :
            wkbNone;

        OGRwkbGeometryType shape_type =
            reported_type == wkbPolygon || reported_type == wkbMultiPolygon ? wkbPolygon :
            reported_type == wkbPolygon25D || reported_type == wkbMultiPolygon25D? wkbPolygon25D :
            reported_type == wkbLineString || reported_type == wkbMultiLineString? wkbMultiLineString :
            reported_type == wkbLineString25D || reported_type == wkbMultiLineString25D? wkbMultiLineString25D :
            reported_type == wkbPoint || reported_type == wkbMultiPoint? wkbMultiPoint :
            reported_type == wkbPoint25D || reported_type == wkbMultiPoint25D? wkbMultiPoint25D :
            wkbNone;

        OGRwkbGeometryType part_type =
            shape_type == wkbPolygon || shape_type == wkbPolygon25D? wkbLinearRing :
            shape_type == wkbMultiLineString? wkbLineString :
            shape_type == wkbMultiLineString25D? wkbLineString25D :
            shape_type == wkbMultiPoint? wkbPoint :
            shape_type == wkbMultiPoint25D? wkbPoint25D :
            wkbNone;


        if ( group_type != wkbNone )
        {
            OGRGeometryH group_handle = OGR_G_CreateGeometry( group_type );
            for( GeoShapeList::const_iterator j = input->getShapes().begin(); j != input->getShapes().end(); j++ )
            {
                OGRGeometryH shape_handle = encodeShape( *j, shape_type, part_type );
                if ( shape_handle )
                {
                    if ( OGR_G_AddGeometryDirectly( group_handle, shape_handle ) != OGRERR_NONE )
                    {
                        osg::notify( osg::WARN ) << "WARNING: OGR_FeatureStore, OGR_G_AddGeometryDirectly failed!" << std::endl;
                    }
                }
            }

            // transfers ownership to the feature:
            if ( OGR_F_SetGeometryDirectly( feature_handle, group_handle ) != OGRERR_NONE )
            {
                osg::notify( osg::WARN ) << "WARNING: OGR_FeatureStore, OGR_F_SetGeometryDirectly failed!" << std::endl;
            }
        }
        else if ( input->getShapes().size() > 0 )
        {
            const GeoShape& shape = *input->getShapes().begin();
            OGRGeometryH shape_handle = encodeShape( shape, shape_type, part_type );
            if ( shape_handle )
            {
                // transfers ownership to the feature:
                if ( OGR_F_SetGeometryDirectly( feature_handle, shape_handle ) != OGRERR_NONE )
                {
                    osg::notify( osg::WARN ) << "WARNING: OGR_FeatureStore, OGR_F_SetGeometryDirectly failed!" << std::endl;
                }
            }
        }

        if ( OGR_L_CreateFeature( layer_handle, feature_handle ) != OGRERR_NONE )
        {
            //TODO: handle error better
            osg::notify(osg::WARN) << "Error: OGR_FeatureStore, OGR_L_CreateFeature failed!" << std::endl;
            OGR_F_Destroy( feature_handle );
            return false;
        }

        // clean up the feature
        OGR_F_Destroy( feature_handle );
    }
    else
    {
            //TODO: handle error better
        osg::notify(osg::WARN) << "Error: OGR_FeatureStore, OGR_F_Create failed." << std::endl;
        return false;
    }

    return true;
}
