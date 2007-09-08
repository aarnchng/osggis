#include <osgGIS/RemoveHolesFilter>

using namespace osgGIS;

RemoveHolesFilter::RemoveHolesFilter()
{
    //NOP
}


RemoveHolesFilter::~RemoveHolesFilter()
{
    //NOP
}


bool
isPartCW( GeoPointList& points )
{
    // find the ymin point:
    double ymin = DBL_MAX;
    int i_lowest = 0;

    for( GeoPointList::iterator i = points.begin(); i != points.end(); i++ )
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


FeatureList
RemoveHolesFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    for( GeoShapeList::iterator i = input->getShapes().begin(); i != input->getShapes().end(); i++ )
    {
        GeoShape& shape = *i;

        if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        {
            GeoPartList new_parts;

            for( GeoPartList::iterator j = shape.getParts().begin(); j != shape.getParts().end(); j++ )
            {
                if ( j->size() > 2 && isPartCW( *j ) )
                    new_parts.push_back( *j );
            }

            shape.getParts().swap( new_parts );
        }
    }

    output.push_back( input );
    return output;
}