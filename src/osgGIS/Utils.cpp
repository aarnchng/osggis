#include <osgGIS/Utils>
#include <osgGIS/LineSegmentIntersector2>
#include <osgGIS/Registry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osg/NodeVisitor>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <iostream>
#include <algorithm>
#include <float.h>
#include <sys/stat.h>
#include <string>

using namespace osgGIS;

void
TimeUtils::getHMSDuration(double total_seconds,
                          unsigned int& out_hours,
                          unsigned int& out_minutes,
                          unsigned int& out_seconds )
{
    out_hours = (unsigned int)(total_seconds/3600.0f);
    out_minutes = (unsigned int)::fmod( (double)(total_seconds/60.0f), 60.0 );
    out_seconds = (unsigned int)::fmod( (double)total_seconds, 60.0 );
}

bool
StringUtils::startsWith(const std::string& input,
                        const std::string& prefix,
                        bool case_sensitive )
{
    unsigned int prefix_len = prefix.length();
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
    unsigned int suffix_len = suffix.length();
    std::string input_p = input;
    std::string suffix_p = suffix;
    if ( !case_sensitive ) {
        std::transform( input_p.begin(), input_p.end(), input_p.begin(), ::tolower );
        std::transform( suffix_p.begin(), suffix_p.end(), suffix_p.begin(), ::tolower );
    }
    return input_p.length() >= suffix_len && input_p.substr( input_p.length()-suffix_len ) == suffix_p;
}


std::string&
StringUtils::replaceIn( std::string& s, const std::string& sub, const std::string& other)
{
    if ( sub.empty() ) return s;
    size_t b=0;
    for( ; ; )
    {
        b = s.find( sub, b );
        if ( b == s.npos ) break;
        s.replace( b, sub.size(), other );
        b += other.size();
    }
    return s;
}

std::string
StringUtils::toLower( const std::string& in )
{
    std::string output = in;
    std::transform( output.begin(), output.end(), output.begin(), ::tolower );
    return output;
}

std::string
StringUtils::trim( const std::string& in )
{
    // by Rodrigo C F Dias
    // http://www.codeproject.com/KB/stl/stdstringtrim.aspx
    std::string str = in;
    std::string::size_type pos = str.find_last_not_of(' ');
    if(pos != std::string::npos) {
        str.erase(pos + 1);
        pos = str.find_first_not_of(' ');
        if(pos != std::string::npos) str.erase(0, pos);
    }
    else str.erase(str.begin(), str.end());
    return str;
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


long
FileUtils::getFileTimeUTC(const std::string& abs_path)
{
    struct stat buf;
    if ( ::stat( abs_path.c_str(), &buf ) == 0 )
    {
        return (long)buf.st_mtime;
    }
    return 0L;
}


bool
GeomUtils::isPointInPolygon(const GeoPoint& point,
                            const GeoPointList& polygon )
{
    return polygon.intersects( GeoExtent( point, point ) );
}


double
GeomUtils::getPolygonArea2D( const GeoPointList& polygon )
{
    GeoPointList temp = polygon;
    openPolygon( temp );

    double sum = 0.0;
    for( GeoPointList::const_iterator i = temp.begin(); i != temp.end(); i++ )
    {
        const GeoPoint& p0 = *i;
        const GeoPoint& p1 = i != temp.end()-1? *(i+1) : *temp.begin();
        sum += p0.x()*p1.y() - p1.x()*p0.y();
    }
    return .5*sum;
}

bool 
GeomUtils::isPolygonCCW( const GeoPointList& points )
{
    return getPolygonArea2D( points ) >= 0.0;
}

bool
GeomUtils::isPolygonCW( const GeoPointList& points )
{
    return getPolygonArea2D( points ) < 0.0;
}

void
GeomUtils::openPolygon( GeoPointList& polygon )
{
    while( polygon.size() > 3 && polygon.front() == polygon.back() )
        polygon.erase( polygon.end()-1 );
}

void
GeomUtils::closePolygon( GeoPointList& polygon )
{
    if ( polygon.size() >= 1 )
    {
        if ( polygon[0] != polygon[polygon.size()-1] )
            polygon.push_back( polygon[0] );
    }
}

// note: none of the apply() overrides needs to call traverse(); this is intentional
struct HasDrawablesVisitor : public osg::NodeVisitor {
    HasDrawablesVisitor() : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ), count(0) { }
    void apply( osg::Geode& geode ) { count++; }
    void apply( osg::ProxyNode& proxy ) { count++; }
    void apply( osg::PagedLOD& plod ) { count++; }
    int count;
};

bool
GeomUtils::hasDrawables( osg::Node* node )
{
    int count = 0;
    if ( node )
    {
        HasDrawablesVisitor c;
        node->accept( c );
        count = c.count;
    }
    return count > 0;
}

struct DVSetter : public osg::NodeVisitor {
    DVSetter( const osg::Object::DataVariance& _dv ) : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ), dv( _dv ) { }
    void apply( osg::Node& node ) {
        node.setDataVariance( dv );
        osg::NodeVisitor::apply( node );
    }
    void apply( osg::Geode& geode ) {
        for( unsigned int i=0; i<geode.getNumDrawables(); i++ ) {
            geode.getDrawable(i)->setDataVariance( dv );
        }
        geode.setDataVariance( dv );
        osg::NodeVisitor::apply( geode );
    }
    osg::Object::DataVariance dv;
};

void
GeomUtils::setDataVarianceRecursively( osg::Node* node, const osg::Object::DataVariance& dv )
{
    if ( node )
    {
        DVSetter setter( dv );
        node->accept( setter );
    }
}

struct NamedNodeFinder : public osg::NodeVisitor {
    NamedNodeFinder( const std::string& _name ) : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ), name(_name) { }
    void apply( osg::Node& node ) {
        if ( node.getName() == name )
            result = &node;
        else
            traverse( node );
    }
    std::string name;
    osg::ref_ptr<osg::Node> result;
};

osg::Node*
GeomUtils::findNamedNode( const std::string& name, osg::Node* root )
{
    NamedNodeFinder v( name );
    root->accept( v );
    return v.result.get();
}

struct SimpleReader : public osgUtil::IntersectionVisitor::ReadCallback {
    osg::Node* readNodeFile( const std::string& filename ) {
        return osgDB::readNodeFile( filename );
    }
};


static void
calculateEndpoints( const GeoPoint& p_geo, osg::Vec3d& out_ep0, osg::Vec3d& out_ep1 )
{
    osg::ref_ptr<SpatialReference> ecef_srs = Registry::SRSFactory()->createGeocentricSRS( p_geo.getSRS() );
    GeoPoint p2 = p_geo;
    double ext = 0.1 * ecef_srs->getEllipsoid().getSemiMajorAxis();
    p2.z() = p2.getDim() < 3? ext : p2.z() + ext; p2.setDim(3);
    out_ep0 = ecef_srs->transform( p2 );
    GeoPoint p3 = p2;
    p3.z() -= 2.0 * ext;
    out_ep1 = ecef_srs->transform( p3 );
}

LineSegmentIntersector2*
GeomUtils::createClampingIntersector( const GeoPoint& p, double& out_hat )
{
    const SpatialReference* srs = p.getSRS();
    LineSegmentIntersector2* isector = NULL;

    if ( p.getSRS()->isGeographic() )
    {
        out_hat = p.getDim() > 2? p.z() : 0.0;
        osg::Vec3d ep0, ep1;
        calculateEndpoints( p, ep0, ep1 );
        isector = new LineSegmentIntersector2( ep0, ep1 );
    }

    else if ( p.getSRS()->isGeocentric() )
    {            
        GeoPoint p_geo = p.getSRS()->getGeographicSRS()->transform( p ); //p_world );
        out_hat = p_geo.getDim() > 2? p_geo.z() : 0.0;

        osg::Vec3d ep0, ep1;
        calculateEndpoints( p_geo, ep0, ep1 );
        isector = new LineSegmentIntersector2( ep0, ep1 );
    }
    else // srs->isProjected()
    {
        osg::Vec3d ext_vec(0,0,250000);
        isector = new LineSegmentIntersector2( p + ext_vec, p - ext_vec );
        out_hat = p.getDim() > 2? p.z() : 0.0;
    }

    return isector;
}


GeoPoint
GeomUtils::clampToTerrain( const GeoPoint& input, osg::Node* terrain, SpatialReference* terrain_srs, SmartReadCallback* reader )
{
    GeoPoint output = GeoPoint::invalid();

    if ( terrain && terrain_srs )
    {        
        double out_hat = 0;
        osg::ref_ptr<LineSegmentIntersector2> isector =
            createClampingIntersector( input, out_hat );

        //GeoPoint p_world = terrain_srs->transform( input );

        //osg::Vec3d clamp_vec;
        //osg::ref_ptr<osgUtil::LineSegmentIntersector> isector;

        //if ( terrain_srs->isGeocentric() )
        //{
        //    clamp_vec = p_world;
        //    clamp_vec.normalize();

        //    isector = new osgUtil::LineSegmentIntersector(
        //        clamp_vec * terrain_srs->getEllipsoid().getSemiMajorAxis() * 1.2,
        //        osg::Vec3d(0, 0, 0) );
        //}
        //else
        //{
        //    clamp_vec.set(0, 0, 1);
        //    osg::Vec3d ext_vec = clamp_vec * 1e6;
        //    isector = new LineSegmentIntersector(
        //        p_world + ext_vec,
        //        p_world - ext_vec );
        //}

        RelaxedIntersectionVisitor iv;
        iv.setIntersector( isector.get() );
        iv.setReadCallback( reader );

        //IntersectionVisitor iv;
        //iv.setIntersector( isector.get() );
        //iv.setReadCallback( new SimpleReader() );

        terrain->accept( iv );
        if ( isector->containsIntersections() )
        {
            output = GeoPoint( isector->getFirstIntersection().getWorldIntersectPoint(), terrain_srs );
        }
    }

    return output;
}

