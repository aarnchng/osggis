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

#include <osgGIS/OGR_SpatialReferenceFactory>
#include <osgGIS/OGR_SpatialReference>
#include <osgGIS/OGR_Utils>
#include <osgGIS/Registry>
#include <osgGIS/GeocentricSpatialReference>
#include <osgGIS/Utils>
#include <osg/Notify>
#include <osg/NodeVisitor>
#include <osg/CoordinateSystemNode>
#include <ogr_api.h>
#include <ogr_spatialref.h>
#include <fstream>
#include <iostream>

using namespace osgGIS;

#define WKT_WGS84 "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]"

OGR_SpatialReferenceFactory::OGR_SpatialReferenceFactory()
{
	OGR_Utils::registerAll();
}

OGR_SpatialReferenceFactory::~OGR_SpatialReferenceFactory()
{
	//TODO
}


SpatialReference*
OGR_SpatialReferenceFactory::createWGS84()
{
    return createSRSfromWKT( WKT_WGS84 );
}


SpatialReference*
OGR_SpatialReferenceFactory::createSRSfromWKT( const std::string& wkt )
{
    return createSRSfromWKT( wkt, osg::Matrixd() );
}


SpatialReference*
OGR_SpatialReferenceFactory::createSRSfromWKT(const std::string& wkt,
                                              const osg::Matrixd& ref_frame )
{
    OGR_SCOPE_LOCK();

    osg::ref_ptr<SpatialReference> result = NULL;

	void* handle = OSRNewSpatialReference( NULL );
    char buf[4096];
    char* buf_ptr = &buf[0];
	strcpy( buf, wkt.c_str() );
	if ( OSRImportFromWkt( handle, &buf_ptr ) == OGRERR_NONE )
	{
        result = new OGR_SpatialReference( handle, true, ref_frame );
        result = validateSRS( result.get() );
	}
	else 
	{
		osg::notify(osg::WARN) << "Unable to create spatial reference from WKT: " << wkt << std::endl;
		OSRDestroySpatialReference( handle );
	}

	return result.release();
}

SpatialReference*
OGR_SpatialReferenceFactory::createSRSfromWKTfile( const std::string& abs_path )
{
    std::ifstream in( abs_path.c_str() );
    if ( in.is_open() )
    {
        std::string wkt;
        std::getline( in, wkt );
        in.close();
        return createSRSfromWKT( wkt );
    }
    else
    {
        osg::notify(osg::WARN) << "Failed to load SRS from file " << abs_path << std::endl;
        return NULL;
    }
}


SpatialReference*
OGR_SpatialReferenceFactory::createSRSfromESRI( const std::string& esri )
{
    return createSRSfromESRI( esri, osg::Matrixd() );
}


SpatialReference*
OGR_SpatialReferenceFactory::createSRSfromESRI(const std::string& esri,
                                               const osg::Matrixd& ref_frame)
{
    OGR_SCOPE_LOCK();

    osg::ref_ptr<SpatialReference> result = NULL;

	void* handle = OSRNewSpatialReference( NULL );
    char buf[4096];
    char* buf_ptr = &buf[0];
	strcpy( buf, esri.c_str() );
	if ( OSRImportFromESRI( handle, &buf_ptr ) == OGRERR_NONE )
	{
		result = new OGR_SpatialReference( handle, true, ref_frame );
        result = validateSRS( result.get() );
	}
	else 
	{
		osg::notify(osg::WARN) << "Unable to create spatial reference from ESRI: " << esri << std::endl;
		OSRDestroySpatialReference( handle );
	}

	return result.release();
}


SpatialReference*
OGR_SpatialReferenceFactory::createSRSfromTerrain( osg::Node* node )
{
    SpatialReference* result = NULL;

    struct CSNodeVisitor : public osg::NodeVisitor {
        osg::CoordinateSystemNode* result;
        CSNodeVisitor() : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ), result(NULL) { }
        void apply( osg::CoordinateSystemNode& csnode ) {
            result = &csnode;
            //no traverse; end when found
        }
    };
    
    if ( node )
    {
        CSNodeVisitor v;
        node->accept( v );
        if ( v.result )
        {
            if ( v.result->getFormat() == "WKT" )
                result = createSRSfromWKT( v.result->getCoordinateSystem() );
            else if ( v.result->getFormat() == "ESRI" )
                result = createSRSfromESRI( v.result->getCoordinateSystem() );

            // if the CS node has a lat/lon SRS, it's really a geocentric model
            // based on that SRS.
            if ( result->isGeographic() )
            {
                result = createGeocentricSRS( result );
            }
        }
    }

    return result;
}


SpatialReference*
OGR_SpatialReferenceFactory::validateSRS( SpatialReference* input )
{
    OGR_SpatialReference* ogr_srs = dynamic_cast<OGR_SpatialReference*>( input );
    if ( ogr_srs )
    {
        // fix invalid ESRI LCC projections:
        if ( ogr_srs->getAttrValue( "PROJECTION", 0 ) == "Lambert_Conformal_Conic" )
        {
            bool has_2_sps =
                ogr_srs->getAttrValue( "Standard_Parallel_2", 0 ).length() > 0 ||
                ogr_srs->getAttrValue( "standard_parallel_2", 0 ).length() > 0;

            std::string new_wkt = ogr_srs->getWKT();
            if ( has_2_sps )
                StringUtils::replaceIn( new_wkt, "Lambert_Conformal_Conic", "Lambert_Conformal_Conic_2SP" );
            else
                StringUtils::replaceIn( new_wkt, "Lambert_Conformal_Conic", "Lambert_Conformal_Conic_1SP" );
            
            return createSRSfromWKT( new_wkt, input->getReferenceFrame() );
        }
    }

    return input;
}


SpatialReference*
OGR_SpatialReferenceFactory::createGeocentricSRS()
{
    return new GeocentricSpatialReference( createWGS84() );
}

SpatialReference*
OGR_SpatialReferenceFactory::createGeocentricSRS( const SpatialReference* basis )
{
    return new GeocentricSpatialReference( basis? basis : createWGS84() );
}


SpatialReference*
OGR_SpatialReferenceFactory::createGeocentricSRS(const SpatialReference* basis,
                                                 const osg::Matrixd& ref_frame)
{
    return new GeocentricSpatialReference( basis, ref_frame );
}


SpatialReference*
OGR_SpatialReferenceFactory::createGeocentricSRS(const osg::CoordinateSystemNode* cs_node)
{
    osg::ref_ptr<SpatialReference> basis = NULL;
    SpatialReference* result = NULL;
    const std::string& format = cs_node->getFormat();

    if ( format == "WKT" || format == "wkt" )
    {
        basis = createSRSfromWKT( cs_node->getCoordinateSystem() );
    }
    else if ( format == "ESRI" || format == "esri" )
    {
        basis = createSRSfromESRI( cs_node->getCoordinateSystem() );
    }
    else
    {
        //TODO... others...
    }

    if ( basis.valid() )
    {
        result = createGeocentricSRS( basis.get() );
    }
    return result;
}

