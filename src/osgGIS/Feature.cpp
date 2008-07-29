#include <osgGIS/Feature>
#include <osgGIS/Utils>
#include <osgGIS/SpatialReference>
#include <osgGIS/Registry>

using namespace osgGIS;

bool
FeatureBase::hasShapeData() const
{
    return getExtent().isValid();
}

GeoShape::ShapeType
FeatureBase::getShapeType() const
{
    GeoShape::ShapeType result = GeoShape::TYPE_UNSPECIFIED;

    if ( getShapes().size() > 0 )
    {
        result = (*getShapes().begin()).getShapeType();
    }

    return result;
}

int
FeatureBase::getShapeDim() const
{
    int dim = 2;
    if ( getShapes().size() > 0 )
    {
        const GeoShape& shape0 = *getShapes().begin();
        if ( shape0.getPartCount() > 0 )
        {
            const GeoPointList& part0 = shape0.getPart( 0 );
            if ( part0.size() > 0 )
            {
                const GeoPoint& p0 = *part0.begin();
                dim = p0.getDim();
            }
        }
    }
    return dim;
}



//double
//FeatureBase::getArea() const
//{
//    double area = 0.0;
//    for( GeoShapeList::const_iterator i = getShapes().begin(); i != getShapes().end(); i++ )
//    {
//        for( GeoPartList::const_iterator j = (*i).getParts().begin(); j != (*i).getParts().end(); j++ )
//        area += fabs( GeomUtils::getPolygonArea2D( *j ) );
//    }
//    return area;
//}

#define CEA_METERS_PER_DEGREE 111120.0
#define CEA_DEGREES_PER_METER 8.9992800575953923686105111591073e-6

double
FeatureBase::getArea() const
{
    double area = 0.0;
    for( GeoShapeList::const_iterator i = getShapes().begin(); i != getShapes().end(); i++ )
    {
        if ( !(*i).getSRS()->isProjected() )
        {
            GeoShape shape = Registry::SRSFactory()->createCEA()->transform( *i );
            for( GeoPartList::const_iterator j = shape.getParts().begin(); j != shape.getParts().end(); j++ )
            {
                area += fabs( GeomUtils::getPolygonArea2D( *j ) );
            }
        }
        else
        {
            for( GeoPartList::const_iterator j = (*i).getParts().begin(); j != (*i).getParts().end(); j++ )
            area += fabs( GeomUtils::getPolygonArea2D( *j ) );
        }
    }
    return area;
}
