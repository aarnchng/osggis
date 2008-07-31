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

#include <osgGIS/ResourceLibrary>
#include <osgGIS/Utils>
#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <osgUtil/Optimizer>
#include <OpenThreads/ScopedLock>
#include <algorithm>
#include <osg/Notify>

using namespace osgGIS;
using namespace OpenThreads;

static std::string EMPTY_STRING = "";


static std::string
normalize( const std::string& input )
{
    std::string output = input;
    std::transform( output.begin(), output.end(), output.begin(), tolower );
    return output;
}

ResourceLibrary::ResourceLibrary( ReentrantMutex& _mut )
: mut( _mut )
{
    //NOP
}

//ResourceLibrary::ResourceLibrary( ResourceLibrary* _parent_lib )
//: parent_lib( _parent_lib ),
//  mut( ReentrantMutex() )
//{
//    //NOP
//}

void
ResourceLibrary::addResource( Resource* resource )
{
    if ( resource )
    {
        ScopedLock<ReentrantMutex> sl( mut );

        if ( dynamic_cast<SkinResource*>( resource ) )
        {
            SkinResource* skin = static_cast<SkinResource*>( resource );
            skins.push_back( skin );
            osg::notify( osg::INFO ) << "ResourceLibrary: added skin " << skin->getAbsoluteURI() << std::endl;
        }
        else if ( dynamic_cast<ModelResource*>( resource ) )
        {
            ModelResource* model = static_cast<ModelResource*>( resource );
            models.push_back( model );
            osg::notify( osg::INFO ) << "ResourceLibrary: added model " << model->getAbsoluteURI() << std::endl;

            osgDB::Registry::instance()->getDataFilePathList().push_back(
                osgDB::getFilePath( model->getAbsoluteURI() ) );
        }
        else if ( dynamic_cast<RasterResource*>( resource ) )
        {
            RasterResource* raster = static_cast<RasterResource*>( resource );
            rasters.push_back( raster );
            osg::notify( osg::INFO ) << "ResourceLibrary: added raster " << raster->getAbsoluteURI() << std::endl;
        }
        else if ( dynamic_cast<FeatureLayerResource*>( resource ) )
        {
            FeatureLayerResource* flr = static_cast<FeatureLayerResource*>( resource );
            feature_layers.push_back( flr );
            osg::notify( osg::INFO ) << "ResourceLibrary: added feature layer " << flr->getAbsoluteURI() << std::endl;
        }
        else if ( dynamic_cast<SRSResource*>( resource ) )
        {
            SRSResource* srsr = static_cast<SRSResource*>( resource );
            srs_list.push_back( srsr );
            osg::notify( osg::INFO ) << "ResourceLibrary: added SRS " << srsr->getName() << std::endl;
        }
        else if ( dynamic_cast<PathResource*>( resource ) )
        {
            PathResource* pr = static_cast<PathResource*>( resource );
            paths.push_back( pr );
            osg::notify( osg::INFO ) << "ResourceLibrary: added path " << pr->getAbsoluteURI() << std::endl;
        }

        resource->setMutex( mut );
    }
}

void
ResourceLibrary::removeResource( Resource* resource )
{
    bool done = false;

    if ( resource )
    {
        ScopedLock<ReentrantMutex> sl( mut );

        if ( dynamic_cast<SkinResource*>( resource ) )
        {
            for( SkinResources::iterator i = skins.begin(); i != skins.end(); i++ )
            {
                if ( i->get() == resource )
                {
                    skins.erase( i );
                    osg::notify( osg::INFO ) << "ResourceLibrary: Removed skin \"" << resource->getName() << "\"" << std::endl;
                    done = true;
                    break;
                }
            }
        }
        else if ( dynamic_cast<ModelResource*>( resource ) )
        {
            for( ModelResources::iterator i = models.begin(); i != models.end(); i++ )
            {
                if ( i->get() == resource )
                {
                    models.erase( i );
                    osg::notify( osg::INFO ) << "ResourceLibrary: Removed model \"" << resource->getName() << "\"" << std::endl;
                    done = true;
                    break;
                }
            }
        }
        else if ( dynamic_cast<RasterResource*>( resource ) )
        {
            for( RasterResourceVec::iterator i = rasters.begin(); i != rasters.end(); i++ )
            {
                if ( i->get() == resource )
                {
                    rasters.erase( i );
                    osg::notify( osg::INFO ) << "ResourceLibrary: Removed raster \"" << resource->getName() << "\"" << std::endl;
                    done = true;
                    break;
                }
            }
        }
        else if ( dynamic_cast<FeatureLayerResource*>( resource ) )
        {
            for( FeatureLayerResourceVec::iterator i = feature_layers.begin(); i != feature_layers.end(); i++ )
            {
                if ( i->get() == resource )
                {
                    feature_layers.erase( i );
                    osg::notify( osg::INFO ) << "ResourceLibrary: Removed feature layer \"" << resource->getName() << "\"" << std::endl;
                    done = true;
                    break;
                }
            }
        }
        else if ( dynamic_cast<SRSResource*>( resource ) )
        {
            for( SRSResourceVec::iterator i = srs_list.begin(); i != srs_list.end(); i++ )
            {
                if ( i->get() == resource )
                {
                    srs_list.erase( i );
                    osg::notify( osg::INFO ) << "ResourceLibrary: Removed SRS \"" << resource->getName() << "\"" << std::endl;
                    done = true;
                    break;
                }
            }
        }
        else if ( dynamic_cast<PathResource*>( resource ) )
        {
            for( PathResourceVec::iterator i = paths.begin(); i != paths.end(); i++ )
            {
                if ( i->get() == resource )
                {
                    paths.erase( i );
                    osg::notify( osg::INFO ) << "ResourceLibrary: Removed Path \"" << resource->getAbsoluteURI() << "\"" << std::endl;
                    done = true;
                    break;
                }
            }
        }
    }
}

Resource*
ResourceLibrary::getResource( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    Resource* result = NULL;
    result = getSkin( name );
    if ( !result ) result = getModel( name );
    if ( !result ) result = getRaster( name );
    if ( !result ) result = getPathResource( name );
    //if ( !result && parent_lib.valid() ) result = parent_lib->getResource( name );
    return result;
}

SkinResource* 
ResourceLibrary::getSkin( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( SkinResources::const_iterator i = skins.begin(); i != skins.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get();
    }
    return NULL;
}

ResourceList
ResourceLibrary::getSkins()
{
    ScopedLock<ReentrantMutex> sl( mut );

    ResourceList result;

    for( SkinResources::const_iterator i = skins.begin(); i != skins.end(); i++ )
        result.push_back( i->get() );

    return result;
}


ResourceList
ResourceLibrary::getSkins( const SkinResourceQuery& q )
{
    ScopedLock<ReentrantMutex> sl( mut );

    ResourceList result;

    for( SkinResources::const_iterator i = skins.begin(); i != skins.end(); i++ )
    {
        SkinResource* r = i->get();

        if ( q.hasTextureHeight() && ( q.getTextureHeight() < r->getMinTextureHeightMeters() || q.getTextureHeight() >= r->getMaxTextureHeightMeters() ) )
            continue;
        
        if ( q.hasMinTextureHeight() && q.getMinTextureHeight() > r->getMaxTextureHeightMeters() )
            continue;

        if ( q.hasMaxTextureHeight() && q.getMaxTextureHeight() <= r->getMinTextureHeightMeters() )
            continue;

        if ( q.hasRepeatsVertically() && q.getRepeatsVertically() != r->getRepeatsVertically() )
            continue;
        
        if ( q.getTags().size() > 0 && !r->containsTags( q.getTags() ) )
            continue;

        result.push_back( r );
    }

    return result;
}

//osg::StateSet*
//ResourceLibrary::getStateSet( SkinResource* skin )
//{
//    ScopedLock<ReentrantMutex> sl( mut );
//
//    osg::StateSet* result = NULL;
//    if ( skin )
//    {
//        SkinStateSets::iterator i = skin_state_sets.find( skin );
//        if ( i == skin_state_sets.end() )
//        {
//            result = skin->createStateSet();
//            skin_state_sets[skin] = result;
//        }
//        else
//        {
//            result = i->second.get();
//        }
//    }
//    return result;
//}



ModelResource* 
ResourceLibrary::getModel( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( ModelResources::const_iterator i = models.begin(); i != models.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get();
    }
    return NULL;
}


ResourceList
ResourceLibrary::getModels()
{
    ScopedLock<ReentrantMutex> sl( mut );

    ResourceList result;

    for( ModelResources::const_iterator i = models.begin(); i != models.end(); i++ )
        result.push_back( i->get() );

    return result;
}


ResourceList
ResourceLibrary::getModels( const ModelResourceQuery& q )
{
    ScopedLock<ReentrantMutex> sl( mut );

    ResourceList result;

    for( ModelResources::const_iterator i = models.begin(); i != models.end(); i++ )
    {
        ModelResource* r = i->get();

        if ( q.getTags().size() > 0 && !r->containsTags( q.getTags() ) )
            continue;

        result.push_back( r );
    }

    return result;
}


//osg::Node*
//ResourceLibrary::getNode( ModelResource* model, bool optimize )
//{
//    ScopedLock<ReentrantMutex> sl( mut );
//
//    osg::Node* result = NULL;
//    if ( model )
//    {
//        ModelNodes::iterator i = model_nodes.find( model->getAbsoluteURI() );
//        if ( i == model_nodes.end() )
//        {
//            bool simplify_extrefs = true; //TODO
//            result = model->createNode();
//            if ( result )
//            {
//                if ( optimize )
//                {
//                    GeomUtils::setDataVarianceRecursively( result, osg::Object::STATIC );
//                    osgUtil::Optimizer o;
//                    o.optimize( result );
//                }
//                model_nodes[model->getAbsoluteURI()] = result;
//
//                // prevent optimization later when the object might be shared!
//                //result->setDataVariance( osg::Object::DYNAMIC ); //gw 7/8/08
//            }
//        }
//        else
//        {
//            result = i->second.get();
//        }
//    }
//    return result;
//}


//osg::Node*
//ResourceLibrary::getProxyNode( ModelResource* model )
//{
//    ScopedLock<ReentrantMutex> sl( mut );
//
//    osg::Node* result = NULL;
//    if ( model )
//    {
//        ModelNodes::iterator i = model_nodes.find( model->getAbsoluteURI() );
//        if ( i == model_nodes.end() )
//        {
//            bool simplify_extrefs = true; //TODO
//            result = model->createProxyNode();
//            model_nodes[model->getAbsoluteURI()] = result;
//        }
//        else
//        {
//            result = i->second.get();
//        }
//    }
//    return result;
//}


SkinResourceQuery 
ResourceLibrary::newSkinQuery()
{
    return SkinResourceQuery();
}


RasterResource*
ResourceLibrary::getRaster( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( RasterResourceVec::const_iterator i = rasters.begin(); i != rasters.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get();
    }
    return NULL;
}

FeatureLayer*
ResourceLibrary::getFeatureLayer( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( FeatureLayerResourceVec::const_iterator i = feature_layers.begin(); i != feature_layers.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get()->getFeatureLayer();
    }
    return NULL;
}

SpatialReference*
ResourceLibrary::getSRS( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( SRSResourceVec::const_iterator i = srs_list.begin(); i != srs_list.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get()->getSRS();
    }
    return NULL;
}

std::string
ResourceLibrary::getPath( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( PathResourceVec::const_iterator i = paths.begin(); i != paths.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get()->getAbsoluteURI();
    }
    return EMPTY_STRING;
}


PathResource*
ResourceLibrary::getPathResource( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( PathResourceVec::const_iterator i = paths.begin(); i != paths.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get();
    }
    return NULL;
}

