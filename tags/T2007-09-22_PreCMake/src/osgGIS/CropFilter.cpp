/**
 * osgGIS - GIS Library for OpenSceneGraph
 * Copyright 2007 Glenn Waldron and Pelican Ventures, Inc.
 * http://osggis.org
 *
 * osgGIS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <osgGIS/CropFilter>
#include <osg/notify>
#include <stack>
//#include <algorithm>

using namespace osgGIS;

#ifndef UINT
#  define UINT unsigned int
#endif

typedef std::vector<UINT> UINTList;


CropFilter::CropFilter()
{
    options = 0;
}


CropFilter::CropFilter( int _options )
{
    options = _options;
}


CropFilter::~CropFilter()
{
    //NOP
}


void
CropFilter::setShowCropLines( bool value )
{
    options = value?
        options | SHOW_CROP_LINES :
        options & ~SHOW_CROP_LINES;
}


bool
CropFilter::getShowCropLines() const
{
    return options & SHOW_CROP_LINES;
}


void
CropFilter::setProperty( const Property& p )
{
    if ( p.getName() == "show_crop_lines" )
        setShowCropLines( p.getBoolValue( getShowCropLines() ) );
    FeatureFilter::setProperty( p );
}

Properties
CropFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    p.push_back( Property( "show_crop_lines", getShowCropLines() ) );
    return p;
}

bool
findIsectSegmentAndPoint(const GeoPoint& p1,
                         const GeoPoint& p2,
                         const GeoPointList& input,
                         const UINT start_i,
                         int&      out_seg_i,
                         GeoPoint& out_isect_p )
{
    for( UINT i=0; i<input.size()-1; i++ )
    {
        // ignore the segments preceding and following the start index
        if ( (input.size()+start_i-1)%input.size() <= i && i <= (input.size()+start_i+1)%input.size() )
            continue;

        const GeoPoint& p3 = input[i];
        const GeoPoint& p4 = input[i+1];

        double denom = (p4.y()-p3.y())*(p2.x()-p1.x()) - (p4.x()-p3.x())*(p2.y()-p1.y());
        if ( denom != 0.0 )
        {
            double ua_num = (p4.x()-p3.x())*(p1.y()-p3.y()) - (p4.y()-p3.y())*(p1.x()-p3.x());
            double ub_num = (p2.x()-p1.x())*(p1.y()-p3.y()) - (p2.y()-p1.y())*(p1.x()-p3.x());

            double ua = ua_num/denom;
            double ub = ub_num/denom;

            if ( ua <= 1.0 ) {
                //osg::notify( osg::WARN ) 
                //    << " [" 
                //    << i << "] "
                //    << "isect point was found at on source segment (ua=" << ua << ")" 
                //    << std::endl
                //    << "  source segment = (" << p1.toString() << " => " << p2.toString() << "), "
                //    << "  target segment = (" << p3.toString() << " => " << p4.toString() << ")"
                //    << std::endl;
            }

            else if ( ub < 0.0 || ub > 1.0 ) 
            {
                //osg::notify( osg::WARN ) 
                //    << " [" 
                //    << i << "] "
                //    << "isect point was not found on target segment (ub=" << ub << ")" 
                //    << std::endl
                //    << "  source segment = (" << p1.toString() << " => " << p2.toString() << "), "
                //    << "  target segment = (" << p3.toString() << " => " << p4.toString() << ")"
                //    << std::endl;
            }
            else
            {
                double isect_x = p1.x() + ua*(p2.x()-p1.x());
                double isect_y = p1.y() + ua*(p2.y()-p1.y());
                out_seg_i = i;
                out_isect_p = p1.getDim() == 2?
                    GeoPoint( isect_x, isect_y, p4.getSRS() ) :
                    GeoPoint( isect_x, isect_y, p4.z(), p4.getSRS() );
                return true;
            }
        }
    }
    return false;
}


//// takes an arbitrary polygon and breaks it down into one or more convex polygons.
//bool
//convexify( const GeoPointList& input, GeoPartList& output )
//{
//    int input_size = input.size();
//    for( int i=0; i<input_size-2; i++ )
//    {
//        const GeoPoint& p1 = input[i];
//        const GeoPoint& p2 = input[i+1];
//        const GeoPoint& p3 = input[i+2];
//
//        osg::Vec3d cross = (p2-p1) ^ (p3-p2);
//        if ( cross.z() < 0 )
//        {
//            // the poly went concave. split it.
//
//            // extrapolate the first segment out to hit a future segment (which it must do)
//            GeoPoint isect_p;
//            int seg_i = 0;
//            if ( findIsectSegmentAndPoint( p1, p2, input, i, /*out*/ seg_i, /*out*/ isect_p ) )
//            {
//                GeoPointList part1;
//
//                part1.push_back( isect_p );
//                for( int j = seg_i+1; j != (i+1)%input_size; j = (j+1)%input_size )
//                    part1.push_back( input[j] );
//                part1.push_back( isect_p ); // close it off
//
//                GeoPointList part2;
//    
//                part2.push_back( isect_p );
//                for( int j = i+1; j != seg_i; j = (j+1)%input_size )
//                    part2.push_back( input[j] );
//                part2.push_back( input[seg_i] );
//                part2.push_back( isect_p ); // close it off
//
//                convexify( part1, output );
//                convexify( part2, output );
//
//                return true;
//            }
//            else
//            {
//                osg::notify( osg::WARN )
//                    << "ERROR: findIsectSegmentAndPoint failed"
//                    << std::endl;
//                return false;
//            }
//        }
//    }
//    output.push_back( input );
//    return true;
//}


bool
pointInsideOrOnLine( const GeoPoint& p, const GeoPoint& s1, const GeoPoint& s2 )
{
    osg::Vec3d cross = (s2-s1) ^ (p-s1);
    return cross.z() >= 0.0;
}


bool
extentInsideOrOnLine( const GeoExtent& p, const GeoPoint& s1, const GeoPoint& s2 )
{
    return 
        pointInsideOrOnLine( p.getSouthwest(), s1, s2 ) &&
        pointInsideOrOnLine( p.getNortheast(), s1, s2 );
}


bool
getIsectPoint( const GeoPoint& p1, const GeoPoint& p2, const GeoPoint& p3, const GeoPoint& p4, GeoPoint& out )
{
    double denom = (p4.y()-p3.y())*(p2.x()-p1.x()) - (p4.x()-p3.x())*(p2.y()-p1.y());
    if ( denom == 0.0 )
    {
        out = GeoPoint::invalid(); // parallel lines
        return false;
    }

    double ua_num = (p4.x()-p3.x())*(p1.y()-p3.y()) - (p4.y()-p3.y())*(p1.x()-p3.x());
    double ub_num = (p2.x()-p1.x())*(p1.y()-p3.y()) - (p2.y()-p1.y())*(p1.x()-p3.x());

    double ua = ua_num/denom;
    double ub = ub_num/denom;

    if ( ua < 0.0 || ua > 1.0 ) // || ub < 0.0 || ub > 1.0 )
    {
        out = GeoPoint::invalid(); // isect point is on line, but not on line segment
        return false;
    }

    double x = p1.x() + ua*(p2.x()-p1.x());
    double y = p1.y() + ua*(p2.y()-p1.y());
    double z = p1.getDim() > 2? p1.z() + ua*(p2.z()-p1.z()) : 0.0; //right?
    out = GeoPoint( x, y, z, p1.getSRS() );
    return true;
}



#define STAGE_SOUTH 0
#define STAGE_EAST  1
#define STAGE_NORTH 2
#define STAGE_WEST  3


void
spatialInsert( UINTList& list, UINT value, UINT stage, const GeoPointList& points )
{
    UINTList::iterator i;
    double x, y;
    
    switch( stage )
    {
    case STAGE_SOUTH:
        x = points[value].x();
        for( i = list.begin(); i != list.end() && x < points[*i].x(); i++ );
        list.insert( i, value );
        break;

    case STAGE_EAST:
        y = points[value].y();
        for( i = list.begin(); i != list.end() && y < points[*i].y(); i++ );
        list.insert( i, value );
        break;

    case STAGE_NORTH:
        x = points[value].x();
        for( i = list.begin(); i != list.end() && x > points[*i].x(); i++ );
        list.insert( i, value );
        break;

    case STAGE_WEST:
        y = points[value].y();
        for( i = list.begin(); i != list.end() && y > points[*i].y(); i++ );
        list.insert( i, value );
        break;
    }
}

UINT
findIndexOf( const UINTList& list, UINT value )
{
    for( UINT i=0; i<list.size(); i++ )
        if ( list[i] == value )
            return i;
    return 0;
}

class DeferredPart {
public:
    DeferredPart( const GeoPointList& a, UINT b, UINT c )
        : part( a ), 
          part_start_ptr( b ),
          waiting_on_ptr( c ) { }
    GeoPointList part;
    UINT part_start_ptr;
    UINT waiting_on_ptr;
};


// poly clipping algorithm Aug 2007
bool
cropNonConvexPolygonPart( const GeoPointList& initial_input, const GeoExtent& window, GeoPartList& final_outputs )
{
    // trivial rejection for a non-polygon:
    if ( initial_input.size() < 3 )
    {
        return false;
    }

    // check for trivial acceptance.
    if ( window.contains( initial_input ) )
    {
        final_outputs.push_back( initial_input );
        return true;
    }

    // prepare the list of input parts to process.
    GeoPartList inputs;
    inputs.push_back( initial_input );

    // run the algorithm against all four sides of the window in succession.
    for( UINT stage = 0; stage < 4; stage++ )
    {
        GeoPoint s1, s2;
        switch( stage )
        {
            case STAGE_SOUTH: s1 = window.getSouthwest(), s2 = window.getSoutheast(); break;
            case STAGE_EAST:  s1 = window.getSoutheast(), s2 = window.getNortheast(); break;
            case STAGE_NORTH: s1 = window.getNortheast(), s2 = window.getNorthwest(); break;
            case STAGE_WEST:  s1 = window.getNorthwest(), s2 = window.getSouthwest(); break;
        }

        // output parts to send to the next stage (or to return).
        GeoPartList outputs;

        // run against each input part.
        for( GeoPartList::iterator i = inputs.begin(); i != inputs.end(); i++ )
        {
            GeoPointList& input = *i;

            // trivially reject a degenerate part (should never happen ;)
            if ( input.size() < 3 )
            {
                continue;
            }
            
            // trivially accept if the window contains the entire extent of the part:
            GeoExtent input_extent;
            input_extent.expandToInclude( input );

            // trivially accept when the part lies entirely within the line:
            if ( extentInsideOrOnLine( input_extent, s1, s2 ) )
            {
                outputs.push_back( input );
                continue;
            }

            // trivially reject when there's no overlap:
            if ( !window.intersects( input_extent ) || extentInsideOrOnLine( input_extent, s2, s1 ) )
            {
                continue;
            }                

            // close the part in preparation for cropping. The cropping process with undo
            // this automatically.
            input.push_back( input.front() );

            // 1a. Traverse the part and find all intersections. Insert them into the input shape.
            // 1b. Create a traversal-order list, ordering the isect points in the order in which they are encountered.
            // 1c. Create a spatial-order list, ordering the isect points along the boundary segment in the direction of the segment.
            GeoPointList working;
            UINTList traversal_order;
            UINTList spatial_order;
            GeoPoint prev_p;
            bool was_inside = true;

            for( UINT input_i = 0; input_i < input.size(); input_i++ )
            {
                const GeoPoint& p = input[ input_i ];
                bool is_inside = pointInsideOrOnLine( p, s1, s2 );

                if ( input_i > 0 )
                {
                    if ( was_inside != is_inside ) // entering or exiting
                    {
                        GeoPoint isect_p;
                        if ( getIsectPoint( prev_p, p, s1, s2, /*out*/ isect_p ) )
                        {
                            working.push_back( isect_p );
                            traversal_order.push_back( working.size()-1 );
                            spatialInsert( spatial_order, working.size()-1, stage, working );
                        }
                        else
                        {
                            osg::notify( osg::WARN ) 
                                << "getIsectPoint failed" << std::endl;
                        }
                    }
                }
                
                working.push_back( p );
                prev_p = p;
                was_inside = is_inside;
            }

            if ( spatial_order.size() == 0 )
            {
                outputs.push_back( input );
                continue;
            }

            // 2. Start at the point preceding the first isect point (in spatial order). This will be the first
            //    outside point (since the first isect point is always an ENTRY.
            UINT overall_start_ptr = spatial_order[0];
            UINT shape_ptr = overall_start_ptr;

            // initialize the isect traversal pointer to start at the spatial order's first isect point:
            UINT trav_ptr = findIndexOf( traversal_order, spatial_order[0] );
            UINT traversals = 0;

            std::stack<DeferredPart> part_stack;
            GeoPointList current_part;

            // Process until we make it all the way around.
            while( traversals < traversal_order.size() )
            {
                // 4. We are outside. Find the next ENTRY point and either start a NEW part, or RESUME
                //    a previously deferred part that is on top of the stack.
                shape_ptr = traversal_order[trav_ptr]; // next ENTRY
                trav_ptr = (trav_ptr + 1) % traversal_order.size();
                traversals++;

                UINT part_start_ptr = shape_ptr;

                if ( part_stack.size() > 0 && part_stack.top().waiting_on_ptr == part_start_ptr )
                {
                    // time to resume a part that we deferred earlier.
                    DeferredPart& top = part_stack.top();
                    current_part = top.part;
                    part_start_ptr = top.part_start_ptr;
                    part_stack.pop();
                }
                else
                {
                    // start a new part
                    current_part = GeoPointList();
                }

                // 5. Traverse to the next EXIT, adding all points along the way. 
                //    Then check the spatial order of the EXIT against the part's starting point. If the former
                //    is ONE MORE than the latter, close out the part.
                for( bool part_done = false; !part_done && traversals < traversal_order.size(); )
                {
                    UINT next_exit_ptr = traversal_order[trav_ptr];
                    trav_ptr = (trav_ptr + 1) % traversal_order.size();
                    traversals++;
                
                    for( ; shape_ptr != next_exit_ptr; shape_ptr = (shape_ptr+1)%working.size() )
                    {
                        current_part.push_back( working[shape_ptr] );
                    }

                    // record the exit point:
                    current_part.push_back( working[next_exit_ptr] );

                    UINT part_start_order = findIndexOf( spatial_order, part_start_ptr );
                    UINT next_exit_order = findIndexOf( spatial_order, next_exit_ptr );
                    if ( next_exit_order - part_start_order == 1 )
                    {
                        outputs.push_back( current_part );
                        part_done = true;
                        continue;
                    }

                    // 6. Find the next ENTRY. If the spatial order of the ENTRY is one less than the 
                    //    spatial ordering of the preceding EXIT, continue on with the part.
                    UINT next_entry_ptr = traversal_order[trav_ptr];
                    trav_ptr = (trav_ptr + 1) % traversal_order.size();
                    traversals++;

                    // check whether we are back at the beginning:
                    if ( traversals >= traversal_order.size() )
                    {
                        current_part.push_back( working[next_entry_ptr] );
                        continue;
                    }

                    // check whether we are continuing the current part:
                    UINT next_entry_order = findIndexOf( spatial_order, next_entry_ptr );
                    if ( next_exit_order - next_entry_order == 1 )
                    {
                        shape_ptr = next_entry_ptr; // skip ahead to the entry point
                        continue;
                    }

                    // 7. We encountered an out-of-order traversal, so need to push the current part onto
                    //    the deferral stack until later, and start a new part.
                    part_stack.push( DeferredPart(
                        current_part,
                        part_start_ptr,
                        spatial_order[next_exit_order-1] ) );

                    current_part = GeoPointList();
                    current_part.push_back( working[next_entry_ptr] );
                    part_start_ptr = next_entry_ptr;
                }
            }

            // pop any parts left on the stack (they're complete now)
            while( part_stack.size() > 0 )
            {
                GeoPointList& part = part_stack.top().part;
                part.push_back( working[part_stack.top().waiting_on_ptr] );
                outputs.push_back( part );
                part_stack.pop();
            }
        }

        // set up for next iteration
        outputs.swap( inputs );
    }

    // go through and make sure no polys are "closed" (probably unnecessary).
    //for( GeoPartList::iterator k = inputs.begin(); k != inputs.end(); k++ )
    //{
    //    while ( k->size() > 3 && k->front() == k->back() )
    //        k->erase( k->end()-1 );
    //}

    final_outputs.swap( inputs );
    return true;
}



bool
cropLinePart( const GeoPointList& part, const GeoExtent& extent, GeoPartList& output )
{
    GeoPartList working_parts;
    working_parts.push_back( part );

    for( UINT s=0; s<4; s++ )
    {
        GeoPartList new_parts;

        GeoPoint s1, s2;
        switch( s )
        {
            case 0: s1 = extent.getSouthwest(), s2 = extent.getSoutheast(); break;
            case 1: s1 = extent.getSoutheast(), s2 = extent.getNortheast(); break;
            case 2: s1 = extent.getNortheast(), s2 = extent.getNorthwest(); break;
            case 3: s1 = extent.getNorthwest(), s2 = extent.getSouthwest(); break;
        }

        for( UINT part_i = 0; part_i < working_parts.size(); part_i++ )
        {
            bool inside = true;
            GeoPoint prev_p;
            GeoPointList current_part;

            const GeoPointList& working_part = working_parts[ part_i ];
            for( UINT point_i = 0; point_i < working_part.size(); point_i++ )
            {
                const GeoPoint& p = working_part[ point_i ];

                if ( pointInsideOrOnLine( p, s1, s2 ) )
                {
                    if ( inside )
                    {
                        current_part.push_back( p );
                    }
                    else if ( prev_p.isValid() )
                    {
                        GeoPoint isect_p;
                        if ( getIsectPoint( prev_p, p, s1, s2, /*out*/ isect_p ) )
                        {
                            current_part.push_back( isect_p );
                            current_part.push_back( p );
                        }
                    }
                    inside = true;
                }
                else // outside.
                {
                    if ( inside && point_i > 0 ) // inside heading out
                    {
                        GeoPoint isect_p;
                        if ( getIsectPoint( prev_p, p, s1, s2, /*out*/ isect_p ) )
                        {
                            current_part.push_back( isect_p );
                            if ( current_part.size() > 1 )
                                new_parts.push_back( current_part );
                            current_part = GeoPointList();
                        }
                    }
                    inside = false;
                }

                prev_p = p;
            }

            if ( current_part.size() > 1 )
                new_parts.push_back( current_part );
        }

        working_parts.swap( new_parts );
    }

    output.swap( working_parts );
    return true;
}


bool
cropPointPart( const GeoPointList& part, const GeoExtent& extent, GeoPartList& output )
{
    GeoPointList new_part;
    for( UINT i=0; i<part.size(); i++ )
    {
        if ( extent.contains( part[i] ) )
            new_part.push_back( part[i] );
    }
    output.push_back( new_part );
    return true;
}


bool
cropPart( const GeoPointList& part, const GeoExtent& extent, const GeoShape::ShapeType& type, GeoPartList& cropped_parts )
{
    GeoPartList outputs;

    if ( type == GeoShape::TYPE_POLYGON )
    {
        cropNonConvexPolygonPart( part, extent, outputs );
    }
    else if ( type == GeoShape::TYPE_LINE )
    {
        cropLinePart( part, extent, outputs );
    }
    else if ( type == GeoShape::TYPE_POINT )
    {
        cropPointPart( part, extent, outputs );
    }
    else
    {
        // unsupported type..?
        return false; 
    }
    
    cropped_parts.insert( cropped_parts.end(), outputs.begin(), outputs.end() );
    return true;
}



FeatureList
CropFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    const GeoExtent& crop_extent = env->getExtent();

    if ( crop_extent.contains( input->getExtent() ) )
    {
        //NOP
    }
    else if ( crop_extent.intersects( input->getExtent() ) )
    {
        GeoShapeList& shapes = input->getShapes();
        GeoShapeList new_shapes;

        for( GeoShapeList::const_iterator i = shapes.begin(); i != shapes.end(); i++ )
        {
            const GeoShape& shape = *i;
            if ( crop_extent.contains( shape.getExtent() ) )
            {
                new_shapes.push_back( shape );
            }
            else if ( crop_extent.intersects( shape.getExtent() ) ) // if crop_extent.contains( shape.getExtent() ) )
            {
                GeoPartList cropped_parts;
                for( UINT part_i = 0; part_i < shape.getPartCount(); part_i++ )
                {
                    const GeoPointList& part = shape.getPart( part_i );
                    cropPart( part, crop_extent, shape.getShapeType(), cropped_parts );
                }

                GeoShape new_shape = shape;
                new_shape.getParts().swap( cropped_parts );
                new_shapes.push_back( new_shape );
            }
        }

        if ( options & CropFilter::SHOW_CROP_LINES )
        {
            GeoShape extent_shape( GeoShape::TYPE_LINE, crop_extent.getSRS() );
            GeoPointList& part = extent_shape.addPart( 5 );
            part[0] = crop_extent.getSouthwest();
            part[1] = crop_extent.getSoutheast();
            part[2] = crop_extent.getNortheast();
            part[3] = crop_extent.getNorthwest();
            part[4] = part[0];
            new_shapes.push_back( extent_shape );
        }

        input->getShapes().swap( new_shapes );
    }

    output.push_back( input );
    return output;
}
