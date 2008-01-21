#include <osgGIS/ResourceLibrary>
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



ResourceLibrary::ResourceLibrary()
{
    //NOP
}

void
ResourceLibrary::addResource( Resource* resource )
{
    ScopedLock<ReentrantMutex> sl( mut );
    if ( resource )
    {
        if ( dynamic_cast<SkinResource*>( resource ) )
        {
            SkinResource* skin = static_cast<SkinResource*>( resource );
            skins.push_back( skin );
            osg::notify( osg::NOTICE ) << "...added skin " << skin->getTexturePath() << std::endl;
        }
    }
    //if ( resource )
    //    resources[ normalize( resource->getName() ) ] = resource;
}

Resource*
ResourceLibrary::getResource( const std::string& name )
{
    ScopedLock<ReentrantMutex> sl( mut );

    Resource* result = NULL;
    result = getSkin( name );
    //todo...check other types
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
            bool simplify_extrefs = true; //TODO
            result = skin->createStateSet( simplify_extrefs );
            skin_state_sets[skin] = result;
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