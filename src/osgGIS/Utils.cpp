#include <osgGIS/Utils>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <iostream>
#include <algorithm>
#include <float.h>

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


bool
GeomUtils::isPointInPolygon(const GeoPoint& point,
                            const GeoPointList& polygon )
{
    int i, j;
    bool result = false;
    for( i=0, j=polygon.size()-1; i<polygon.size(); j = i++ )
    {
        if ((((polygon[i].y() <= point.y()) && (point.y() < polygon[j].y())) ||
             ((polygon[j].y() <= point.y()) && (point.y() < polygon[i].y()))) &&
            (point.x() < (polygon[j].x()-polygon[i].x()) * (point.y()-polygon[i].y())/(polygon[j].y()-polygon[i].y())+polygon[i].x()))
        {
            result = !result;
        }
    }
    return result;
}




bool
GeomUtils::isPolygonCW( const GeoPointList& points )
{
    // find the ymin point:
    double ymin = DBL_MAX;
    int i_lowest = 0;

    for( GeoPointList::const_iterator i = points.begin(); i != points.end(); i++ )
    {
        if ( i->y() < ymin ) 
        {
            ymin = i->y();
            i_lowest = i-points.begin();
        }
    }

    // next cross the 2 vector converging at that point:
    osg::Vec3d p0 = *( points.begin() + ( i_lowest > 0? i_lowest-1 : points.size()-1 ) );
    osg::Vec3d p1 = *( points.begin() + i_lowest );
    osg::Vec3d p2 = *( points.begin() + ( i_lowest < points.size()-1? i_lowest+1 : 0 ) );

    osg::Vec3d cp = (p1-p0) ^ (p2-p1);

    //TODO: need to rotate into ref frame - for now just use this filter before xforming
    return cp.z() > 0;
}