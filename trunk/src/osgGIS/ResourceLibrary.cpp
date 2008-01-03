#include <osgGIS/ResourceLibrary>
#include <algorithm>

using namespace osgGIS;

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
    if ( resource )
        resources[ normalize( resource->getName() ) ] = resource;
}

void
ResourceLibrary::removeResource( Resource* resource )
{
    if ( resource )
        resources.erase( normalize( resource->getName() ) );
}

void
ResourceLibrary::addRule( Rule* rule )
{
    if ( rule )
        rules[ normalize( rule->getName() ) ] = rule;
}

void
ResourceLibrary::removeRule( Rule* rule )
{
    if ( rule )
        rules.erase( normalize( rule->getName() ) );
}

Resource*
ResourceLibrary::getResource( const std::string& name )
{
    ResourcesByName::iterator i = resources.find( normalize( name ) );
    return i != resources.end()? i->second.get() : NULL;
}

ResourceSet
ResourceLibrary::getResourcesWithTag( const std::string& tag )
{
    ResourceSet result;

    for( ResourcesByName::iterator i = resources.begin(); i != resources.end(); i++ )
    {
        Resource* resource = i->second.get();
        if ( resource->containsTag( tag ) )
            result.insert( resource );
    }

    return result;
}

ResourceSet
ResourceLibrary::getResourcesWithTags( const std::set<std::string>& tags )
{
    ResourceSet result;

    for( ResourcesByName::iterator i = resources.begin(); i != resources.end(); i++ )
    {
        Resource* resource = i->second.get();
        if ( resource->containsTags( tags ) )
            result.insert( resource );
    }

    return result;
}

Rule*
ResourceLibrary::getRule( const std::string& name )
{
    RulesByName::iterator i = rules.find( normalize( name ) );
    return i != rules.end()? i->second.get() : NULL;
}