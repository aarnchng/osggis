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
#include <osgGIS/FeatureCursorImpl>
#include <ogr_api.h>
#include <osg/notify>

using namespace osgGIS;


OGR_FeatureStore::OGR_FeatureStore( const std::string& abs_path )
: extent( GeoExtent::invalid() )
{
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
}


OGR_FeatureStore::~OGR_FeatureStore()
{
	if ( layer_handle )
	{
        // no need to release layer handle (held by ref)
		layer_handle = NULL;
	}

	if ( ds_handle )
	{
		OGRReleaseDataSource( ds_handle );
		ds_handle = NULL;
	}
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


FeatureCursor*
OGR_FeatureStore::createCursor()
{
    FeatureCursor* result = NULL;
    if ( layer_handle )
    {
        unsigned int feature_count = OGR_L_GetFeatureCount( layer_handle, 1 );
        FeatureOIDList oids( feature_count );
    
        OGR_L_ResetReading( layer_handle );
        void* feature_handle;
        int iter = 0;
        while( (feature_handle = OGR_L_GetNextFeature( layer_handle )) != NULL )
        {
            FeatureOID oid = (FeatureOID)OGR_F_GetFID( feature_handle );
            oids[iter++] = oid;
            OGR_F_Destroy( feature_handle );
        }
    
        result = new FeatureCursorImpl( oids, this );
    }
    return result;
}


int
OGR_FeatureStore::getFeatureCount() const
{
	//TODO: cache result
	return OGR_L_GetFeatureCount( layer_handle, 1 );
}


const GeoExtent
OGR_FeatureStore::getExtent()
{
	if ( !extent.isValid() )
	{
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
            osg::ref_ptr<FeatureCursor> cursor = createCursor();
			while( cursor->hasNext() )
			{
				Feature* feature = cursor->next();
                const GeoShapeList& shapes = feature->getShapes();
                for( GeoShapeList::const_iterator& i = shapes.begin(); i != shapes.end(); i++ )
                {
                    const GeoExtent& feature_extent = i->getExtent();
				    extent.expandToInclude( feature_extent );
                }
			}
		}
	}

	return extent;
}
