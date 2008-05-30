#include <osgGIS/TerrainUtils>

using namespace osgGIS;


osg::Node* 
TerrainUtils::findMinimumBoundingNode(osg::Node*       terrain,
                                      const GeoExtent& terrain_extent,
                                      const GeoExtent& aoi )
{

    return terrain;
}



// Calculates a sub-extent of a larger extent, given the number of children and
// the child number. This currently assumes the subdivision ordering used by 
// VirtualPlanetBuilder.
GeoExtent
TerrainUtils::getSubExtent(const GeoExtent& extent,
                           int              num_children,
                           int              child_no)
{
    GeoPoint centroid = extent.getCentroid();
    GeoExtent sub_extent;

    switch( num_children )
    {
    case 0:
    case 1:
        sub_extent = extent;
        break;

    case 2:
        if ( child_no == 0 )
        {
            sub_extent = GeoExtent(
                extent.getXMin(),
                extent.getYMin(),
                centroid.x(),
                extent.getYMax(),
                extent.getSRS() );
        }
        else
        {
            sub_extent = GeoExtent(
                centroid.x(),
                extent.getYMin(),
                extent.getXMax(),
                extent.getYMax(),
                extent.getSRS() );
        }
        break;

    case 4:
        if ( child_no == 2 )
        {
            sub_extent = GeoExtent(
                extent.getXMin(),
                centroid.y(),
                centroid.x(),
                extent.getYMax(),
                extent.getSRS() );
        }
        else if ( child_no == 3 )
        {
            sub_extent = GeoExtent(
                centroid.x(),
                centroid.y(),
                extent.getXMax(),
                extent.getYMax(),
                extent.getSRS() );
        }
        else if ( child_no == 0 )
        {
            sub_extent = GeoExtent(
                extent.getXMin(),
                extent.getYMin(),
                centroid.x(),
                centroid.y(),
                extent.getSRS() );
        }
        else if ( child_no == 1 )
        {
            sub_extent = GeoExtent(
                centroid.x(),
                extent.getYMin(),
                extent.getXMax(),
                centroid.y(),
                extent.getSRS() );
        }
    }

    return sub_extent;
}


// Calcuates all the extents of a parent's subtiles after subdivision.
void
TerrainUtils::calculateSubExtents(const GeoExtent& extent,
                                  unsigned int num_children,
                                  std::vector<GeoExtent>& out )
{
    for( unsigned int i=0; i<num_children; i++ )
    {
        out.push_back( getSubExtent( extent, num_children, i ) );
    }
}