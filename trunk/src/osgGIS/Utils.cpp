#include <osgGIS/Utils>
#include <osgDB/FileNameUtils>
#include <algorithm>

using namespace osgGIS;

bool
StringUtils::startsWith(const std::string& input,
                        const std::string& prefix,
                        bool case_sensitive )
{
    int prefix_len = prefix.length();
    std::string input_p = input;
    std::string prefix_p = prefix;
    if ( !case_sensitive ) {
        std::transform( input_p.begin(), input_p.end(), input_p.begin(), ::tolower );
        std::transform( prefix_p.begin(), prefix_p.end(), prefix_p.begin(), ::tolower );
    }
    return input_p.length() >= prefix_len && input_p.find( prefix_p ) == 0;
}

bool
StringUtils::endsWith(const std::string& input,
                      const std::string& suffix,
                      bool case_sensitive )
{
    int suffix_len = suffix.length();
    std::string input_p = input;
    std::string suffix_p = suffix;
    if ( !case_sensitive ) {
        std::transform( input_p.begin(), input_p.end(), input_p.begin(), ::tolower );
        std::transform( suffix_p.begin(), suffix_p.end(), suffix_p.begin(), ::tolower );
    }
    return input_p.length() >= suffix_len && input_p.substr( input_p.length()-suffix_len ) == suffix_p;
}



bool
PathUtils::isAbsPath( const std::string& path )
{
    return
        ( path.length() >= 2 && path[0] == '\\' && path[1] == '\\' ) ||
        ( path.length() >= 2 && path[1] == ':' ) ||
        ( path.length() >= 1 && path[0] == '/' );
}


std::string
PathUtils::combinePaths(const std::string& left,
                        const std::string& right)
{
    if ( left.length() == 0 )
        return right;
    else if ( right.length() == 0 )
        return left;
    else
        return osgDB::getRealPath( osgDB::concatPaths( left, right ) );
}


std::string
PathUtils::getAbsPath(const std::string& base_path,
                      const std::string& my_path)
{
    if ( isAbsPath( my_path ) )
        return my_path;
    else
        return combinePaths( base_path, my_path );
}