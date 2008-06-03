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

#include <osgGIS/Registry>
#include <osgGIS/DefaultFeatureStoreFactory>
#include <osgGIS/OGR_SpatialReferenceFactory>
#include <osgGIS/DefaultRasterStoreFactory>
#include <osgGIS/Lua_ScriptEngine>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/Notify>
#include <fstream>
#include <algorithm>

using namespace osgGIS;

osgGIS::Registry* osgGIS::Registry::singleton = NULL;

static std::string
normalize( std::string input )
{
    std::string output = input;
    std::replace( output.begin(), output.end(), '-', '_' );
    std::transform( output.begin(), output.end(), output.begin(), tolower );
    return output;
}

Registry::Registry()
{
	setSRSFactory( new OGR_SpatialReferenceFactory() );
	setFeatureStoreFactory( new DefaultFeatureStoreFactory() );
    setRasterStoreFactory( new DefaultRasterStoreFactory() );
}


Registry::~Registry()
{
	//NOP
}


osgGIS::Registry*
Registry::instance()
{
	if ( singleton == NULL )
	{
		singleton = new Registry();
	}
	return singleton;
}


osgGIS::SpatialReferenceFactory*
Registry::SRSFactory()
{
    return Registry::instance()->getSRSFactory();
}


FeatureLayer*
Registry::createFeatureLayer( const std::string& uri )
{
    FeatureLayer* result = NULL;
    if ( getFeatureStoreFactory() )
    {
        osg::ref_ptr<FeatureStore> store = 
            getFeatureStoreFactory()->connectToFeatureStore( uri );

        if ( store.valid() && store->isReady() )
        {
            result = new FeatureLayer( store.get() );

            // if the store doesn't provide a spatial reference, try to load one from
            // a PRJ file:
            if ( result && result->getSRS() == NULL )
            {
                if ( osgDB::fileExists( uri ) ) // make sure it's a file:
                {            
                    std::string prj_file = osgDB::getNameLessExtension( uri ) + ".prj";
                    std::fstream is;
                    is.open( prj_file.c_str() );
                    if ( is.is_open() )
                    {
                        is.seekg (0, std::ios::end);
                        int length = is.tellg();
                        is.seekg (0, std::ios::beg);
                        char* buf = new char[length];
                        is.read( buf, length );
                        is.close();
                        std::string prj = buf;
                        delete[] buf;
                        const SpatialReference* prj_srs = 
                            Registry::instance()->getSRSFactory()->createSRSfromWKT( prj );
                        result->setSRS( prj_srs );
                    }
                }
            }
        }
    }
    return result;
}


SpatialReferenceFactory*
Registry::getSRSFactory()
{
	return spatial_ref_factory.get();
}


void
Registry::setSRSFactory( SpatialReferenceFactory* _factory )
{
	spatial_ref_factory = _factory;
}


FeatureStoreFactory*
Registry::getFeatureStoreFactory()
{
	return feature_store_factory.get();
}


void
Registry::setFeatureStoreFactory( FeatureStoreFactory* _factory )
{
	feature_store_factory = _factory;
}


RasterStoreFactory*
Registry::getRasterStoreFactory()
{
    return raster_store_factory.get();
}

void
Registry::setRasterStoreFactory( RasterStoreFactory* value )
{
    raster_store_factory = value;
}


Filter* 
Registry::createFilterByType( const std::string& type )
{
    std::string n = normalize(type);
    FilterFactoryMap::const_iterator i = filter_factories.find( n );
    return i != filter_factories.end()? i->second->createFilter() : NULL;
}


bool 
Registry::addFilterType( const std::string& type, FilterFactory* factory )
{
    std::string n = normalize(type);
    filter_factories[n] = factory;
    osg::notify( osg::DEBUG_INFO ) << "osgGIS::Registry: Registered filter type " << type << std::endl;
    return true;
}


Resource* 
Registry::createResourceByType( const std::string& type )
{
    std::string n = normalize(type);
    ResourceFactoryMap::const_iterator i = resource_factories.find( n );
    return i != resource_factories.end()? i->second->createResource() : NULL;
}

bool 
Registry::addResourceType( const std::string& type, ResourceFactory* factory )
{
    std::string n = normalize(type);
    resource_factories[n] = factory;
    osg::notify( osg::DEBUG_INFO ) << "osgGIS::Registry: Registered resource type " << type << std::endl;
    return true;
}

ScriptEngine*
Registry::createScriptEngine()
{
    return new Lua_ScriptEngine();
}

void
Registry::setWorkDirectory( const std::string& value )
{
    work_dir = value;
}

const std::string&
Registry::getWorkDirectory() const
{
    return work_dir;
}

bool
Registry::hasWorkDirectory() const
{
    return work_dir.length() > 0;
}


