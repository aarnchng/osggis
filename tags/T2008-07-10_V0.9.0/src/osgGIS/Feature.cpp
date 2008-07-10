#include <osgGIS/Feature>

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

