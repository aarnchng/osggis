#include <osgGIS/Property>
#include <sstream>
#include <algorithm>

using namespace osgGIS;

Property::Property()
{
}

Property::Property( const std::string& _name, const std::string& _value )
{
    name = _name;
    value = _value;
}

Property::Property( const std::string& _name, int _value )
{
    name = _name;
    std::stringstream ss;
    ss << _value;
    value = ss.str();
}

Property::Property( const std::string& _name, float _value )
{
    name = _name;
    std::stringstream ss;
    ss << _value;
    value = ss.str();
}

Property::Property( const std::string& _name, double _value )
{
    name = _name;
    std::stringstream ss;
    ss << _value;
    value = ss.str();
}

Property::Property( const std::string& _name, bool _value )
{
    name = _name;
    value = _value? "true" : "false";
}

Property::Property( const std::string& _name, const osg::Vec2f& _v )
{
    name = _name;
    std::stringstream ss;
    ss << _v[0] << " " << _v[1];
    value = ss.str();
}

Property::Property( const std::string& _name, const osg::Vec3f& _v )
{
    name = _name;
    std::stringstream ss;
    ss << _v[0] << " " << _v[1] << " " << _v[2];
    value = ss.str();
}

Property::Property( const std::string& _name, const osg::Vec4f& _v )
{
    name = _name;
    std::stringstream ss;
    ss << _v[0] << " " << _v[1] << " " << _v[2] << " " << _v[3];
    value = ss.str();
}

Property::Property( const std::string& _name, const osg::Matrix& _v )
{
    name = _name;
    std::stringstream ss;
    const osg::Matrix::value_type* p = _v.ptr();
    for( int i=0; i<15; i++ ) ss << *p++ << " ";
    ss << *p;
    value = ss.str();
}

const std::string& 
Property::getName() const
{
    return name;
}

const std::string& 
Property::getValue() const
{
    return value;
}

int 
Property::getIntValue( int def ) const
{
    return value.length() > 0? ::atoi( value.c_str() ) : def;
}

float 
Property::getFloatValue( float def ) const
{
    return value.length() > 0? (float)::atof( value.c_str() ) : def;
}

double 
Property::getDoubleValue( double def ) const
{
    return value.length() > 0? ::atof( value.c_str() ) : def;
}

bool 
Property::getBoolValue( bool def ) const
{
    std::string temp = value;
    std::transform( temp.begin(), temp.end(), temp.begin(), tolower );
    return temp == "true" || temp == "yes" || temp == "on" ? true : false;
}

osg::Vec2f
Property::getVec2fValue() const
{
    osg::Vec2f v;
    std::stringstream ss( value );
    ss >> v[0] >> v[1];
    return v;
}

osg::Vec3f
Property::getVec3fValue() const
{
    osg::Vec3f v;
    std::stringstream ss( value );
    ss >> v[0] >> v[1] >> v[2];
    return v;
}

osg::Vec4f
Property::getVec4fValue() const
{
    osg::Vec4f v;
    std::stringstream ss( value );
    ss >> v[0] >> v[1] >> v[2] >> v[3];
    return v;
}

osg::Matrix
Property::getMatrixValue() const
{
    osg::Matrix m;
    std::stringstream ss( value );
    osg::Matrix::value_type* p = m.ptr();
    for( int i=0; i<16; i++ ) ss >> *p++;
    return m;
}
