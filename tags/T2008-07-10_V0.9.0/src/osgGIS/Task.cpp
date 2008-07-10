#include <osgGIS/Task>

using namespace osgGIS;

Task::Task()
{
    setName( "Unnamed task" );
}

Task::Task( const std::string& _name )
{
    setName( _name );
}

void
Task::setName( const std::string& _name )
{
    name = _name;
}

const std::string&
Task::getName() const
{
    return name;
}
