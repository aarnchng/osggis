#include <osgGIS/Feature>

using namespace osgGIS;
//
//Attribute
//FeatureBase::getAttribute( const std::string& key ) const
//{
//    AttributeTable::const_iterator i = user_attrs.find( key );
//    return i != user_attrs.end()? i->second : Attribute::invalid();
//}
//
//AttributeList
//FeatureBase::getAttributes() const
//{
//    AttributeList result;
//    for( AttributeTable::const_iterator i = user_attrs.begin(); i != user_attrs.end(); i++ )
//        result.push_back( i->second );
//    return result;
//}
//
//AttributeSchemaList
//FeatureBase::getAttributeSchemas() const
//{
//    AttributeSchemaList result;
//
//    for( AttributeTable::const_iterator i = user_attrs.begin(); i != user_attrs.end(); i++ )
//    {
//        result.push_back( AttributeSchema( i->first, i->second.getType(), Properties() ) );
//    }
//
//    return result;
//}
//
//void 
//FeatureBase::setAttribute( const std::string& key, const std::string& value )
//{
//    user_attrs[key] = Attribute( key, value );
//}
//
//void 
//FeatureBase::setAttribute( const std::string& key, int value )
//{
//    user_attrs[key] = Attribute( key, value );
//}
//
//void 
//FeatureBase::setAttribute( const std::string& key, double value )
//{
//    user_attrs[key] = Attribute( key, value );
//}
//
//void
//FeatureBase::setAttribute( const std::string& key, bool value )
//{
//    user_attrs[key] = Attribute( key, value );
//}
//
//const AttributeTable&
//FeatureBase::getUserAttrs() const
//{
//    return user_attrs;
//}

bool
FeatureBase::hasShapeData() const
{
    return getExtent().isValid();
        //getShapes().size() > 0 &&
        //(*getShapes().begin()).getPartCount() > 0 &&
        //(*getShapes().begin()).getPart(0).size() > 0;
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