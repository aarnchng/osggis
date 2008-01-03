#include <osgGIS/Rule>

using namespace osgGIS;

Rule::Rule()
{
    //NOP
}

Rule::Rule( const std::string& _name )
{
    setName( _name );
}

Rule::~Rule()
{
    //NOP
}

void
Rule::setName( const std::string& _name )
{
    name = _name;
}

const std::string&
Rule::getName() const
{
    return name;
}
