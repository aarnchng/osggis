#include <osgGIS/ResourceLibrary>
#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <OpenThreads/ScopedLock>
#include <algorithm>
#include <osg/Notify>

using namespace osgGIS;
using namespace OpenThreads;

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
            osg::notify( osg::INFO ) << "...added skin " << skin->getAbsoluteURI() << std::endl;
        }
        else if ( dynamic_cast<ModelResource*>( resource ) )
        {
            ModelResource* model = static_cast<ModelResource*>( resource );
            models.push_back( model );
            osg::notify( osg::INFO ) << "...added model " << model->getAbsoluteURI() << std::endl;

            osgDB::Registry::instance()->getDataFilePathList().push_back(
                osgDB::getFilePath( model->getAbsoluteURI() ) );
        }
        else if ( dynamic_cast<RasterResource*>( resource ) )
        {
            RasterResource* raster = static_cast<RasterResource*>( resource );
            rasters.push_back( raster );
            osg::notify( osg::INFO ) << "...added raster " << raster->getAbsoluteURI() << std::endl;
        }

        resource->setMutex( mut );
    }
}

void
ResourceLibrary::removeResource( Resource* resource )
{
    if ( resource )
    {
        ScopedLock<ReentrantMutex> sl( mut );

        if ( dynamic_cast<SkinResource*>( resource ) )
        {
            for( SkinResourceVec::iterator i = skins.begin(); i != skins.end(); i++ )
            {
                if ( i->get() == resource )
                {
                    skins.erase( i );
                    osg::notify( osg::NOTICE ) << "ResourceLibrary: Removed skin \"" << resource->getName() << "\"" << std::endl;
                    break;
                }
            }
        }
        else if ( dynamic_cast<ModelResource*>( resource ) )
        {
            for( ModelResourceVec::iterator i = models.begin(); i != models.end(); i++ )
            {
                if ( i->get() == resource )
                {
                    models.erase( i );
                    osg::notify( osg::NOTICE ) << "ResourceLibrary: Removed model \"" << resource->getName() << "\"" << std::endl;
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
                    osg::notify( osg::NOTICE ) << "ResourceLibrary: Removed raster \"" << resource->getName() << "\"" << std::endl;
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
    return result;
}

SkinResource* 
ResourceLibrary::getSkin( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( SkinResourceVec::const_iterator i = skins.begin(); i != skins.end(); i++ )
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

    for( SkinResourceVec::const_iterator i = skins.begin(); i != skins.end(); i++ )
        result.push_back( i->get() );

    return result;
}


ResourceList
ResourceLibrary::getSkins( const SkinResourceQuery& q )
{
    ScopedLock<ReentrantMutex> sl( mut );

    ResourceList result;

    for( SkinResourceVec::const_iterator i = skins.begin(); i != skins.end(); i++ )
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

osg::StateSet*
ResourceLibrary::getStateSet( SkinResource* skin )
{
    ScopedLock<ReentrantMutex> sl( mut );

    osg::StateSet* result = NULL;
    if ( skin )
    {
        SkinStateSets::iterator i = skin_state_sets.find( skin );
        if ( i == skin_state_sets.end() )
        {
            result = skin->createStateSet();
            skin_state_sets[skin] = result;
        }
        else
        {
            result = i->second.get();
        }
    }
    return result;
}



ModelResource* 
ResourceLibrary::getModel( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    for( ModelResourceVec::const_iterator i = models.begin(); i != models.end(); i++ )
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

    for( ModelResourceVec::const_iterator i = models.begin(); i != models.end(); i++ )
        result.push_back( i->get() );

    return result;
}


ResourceList
ResourceLibrary::getModels( const ModelResourceQuery& q )
{
    ScopedLock<ReentrantMutex> sl( mut );

    ResourceList result;

    for( ModelResourceVec::const_iterator i = models.begin(); i != models.end(); i++ )
    {
        ModelResource* r = i->get();

        if ( q.getTags().size() > 0 && !r->containsTags( q.getTags() ) )
            continue;

        result.push_back( r );
    }

    return result;
}


osg::Node*
ResourceLibrary::getNode( ModelResource* model )
{
    ScopedLock<ReentrantMutex> sl( mut );

    osg::Node* result = NULL;
    if ( model )
    {
        ModelNodes::iterator i = model_nodes.find( model );
        if ( i == model_nodes.end() )
        {
            bool simplify_extrefs = true; //TODO
            result = model->createNode();
            model_nodes[model] = result;
        }
        else
        {
            result = i->second.get();
        }
    }
    return result;
}


osg::Node*
ResourceLibrary::getProxyNode( ModelResource* model )
{
    ScopedLock<ReentrantMutex> sl( mut );

    osg::Node* result = NULL;
    if ( model )
    {
        ModelNodes::iterator i = model_nodes.find( model );
        if ( i == model_nodes.end() )
        {
            bool simplify_extrefs = true; //TODO
            result = model->createProxyNode();
            model_nodes[model] = result;
        }
        else
        {
            result = i->second.get();
        }
    }
    return result;
}


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