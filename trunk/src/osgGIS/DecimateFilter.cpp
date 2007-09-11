#include <osgGIS/DecimateFilter>
#include <osg/notify>

using namespace osgGIS;

DecimateFilter::DecimateFilter()
{
    threshold = 0.0;
}


DecimateFilter::DecimateFilter( double _threshold )
{
    threshold = _threshold;
}


DecimateFilter::~DecimateFilter()
{
    //NOP
}


void
decimatePart( GeoPointList& input, double threshold, int min_points, GeoPartList& output )
{
    GeoPointList new_part;
    GeoPoint last_point;
    double t2 = threshold * threshold;

    if ( input.size() >= min_points )
    {
        for( GeoPointList::iterator i = input.begin(); i != input.end(); i++ )
        {
            if ( i == input.begin() )// || i == input.end()-1 )
            {
                last_point = *i;
                new_part.push_back( *i );
            }
            else
            {
                double d2 = (*i - last_point).length2();
                if ( d2 >= t2 )
                {
                    new_part.push_back( *i );
                    last_point = *i;
                }
            }
        }

        if ( new_part.size() >= min_points )
        {
            output.push_back( new_part );
        }
    }
}


FeatureList
DecimateFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    if ( threshold > 0.0 )
    {
        GeoShapeList new_shapes;

        for( GeoShapeList::iterator i = input->getShapes().begin(); i != input->getShapes().end(); i++ )
        {
            int min_points = 
                i->getShapeType() == GeoShape::TYPE_POLYGON? 3 :
                i->getShapeType() == GeoShape::TYPE_LINE? 2 :
                1;

            GeoPartList new_parts;

            for( GeoPartList::iterator j = i->getParts().begin(); j != i->getParts().end(); j++ )
            {
                decimatePart( *j, threshold, min_points, new_parts );
            }

            i->getParts().swap( new_parts );
            
            if ( new_parts.size() > 0 )
            {
                new_shapes.push_back( *i );
            }
        }

        input->getShapes().swap( new_shapes );
    }

    if ( input->getShapes().size() > 0 )
    {
        output.push_back( input );
    }
    return output;
}