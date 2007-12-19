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

#include <osgGIS/CombineLinesFilter>
#include <osgGIS/SimpleFeature>
#include <osg/Notify>
#include <map>
#include <list>
#include <algorithm>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( CombineLinesFilter );


CombineLinesFilter::CombineLinesFilter()
{
    //NOP
}


CombineLinesFilter::~CombineLinesFilter()
{
    //NOP
}


void
CombineLinesFilter::setProperty( const Property& p )
{
    FeatureFilter::setProperty( p );
}

Properties
CombineLinesFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    return p;
}


struct Segment : public osg::Referenced {
    Segment( int _id, const GeoPointList& _points ) : id(_id), points(_points), active(true) { }
    Segment() : active(false) { }
    int id;
    GeoPointList points;
    bool active;
};

FeatureList
CombineLinesFilter::process( FeatureList& input, FilterEnv* env )
{
    FeatureList output;

    typedef std::list<osg::ref_ptr<Segment> > SegmentList;
    typedef std::map<GeoPoint,osg::ref_ptr<Segment> > EndpointMap;

    EndpointMap ep_map;
    SegmentList segments;
    int seg_id = 0;
    int num_active_segs = 0;

    // first collect all the parts into a segment list.
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        Feature* f = i->get();

        for( GeoShapeList::iterator& j = f->getShapes().begin(); j != f->getShapes().end(); j++ )
        {
            GeoShape& shape = *j;
            for( GeoPartList::iterator& k = shape.getParts().begin(); k != shape.getParts().end(); k++ )
            {
                GeoPointList& part = *k;
                if ( part.size() > 0 )
                {
                    segments.push_back( new Segment( seg_id++, part ) );
                    num_active_segs++;
                }
            }
        }
    }

    int start_count = segments.size();
    //osg::notify(osg::WARN) << "[Combine] combining " << segments.size() << " segments..." << std::endl;

    // next run through the segments list and combine segments w/ matching endpoints
    for( SegmentList::iterator i = segments.begin(); i != segments.end(); i++ )
    {
        Segment* seg = i->get();
        
        if ( !seg->active )
        {
            i = segments.erase( i ); i--;
            continue;
        }

        //osg::notify(osg::WARN)<< "[Combine]   segment " << seg->id << ": ";

        // see if one of this segment's endpoints is in the ep map:
        bool found = false;
        EndpointMap::iterator ep = ep_map.find( seg->points.front() );
        if ( ep != ep_map.end() && ep->second->active && seg->id != ep->second->id )
        {
            found = true;
            GeoPointList new_part;

            new_part.insert( new_part.begin(), seg->points.begin()+1, seg->points.end() );

            if ( seg->points.front() == ep->second->points.front() ) // front matches front
            {
                std::reverse( new_part.begin(), new_part.end() );
                new_part.insert( new_part.end(), ep->second->points.begin(), ep->second->points.end() );
            }
            else // front matches back
            {
                new_part.insert( new_part.begin(), ep->second->points.begin(), ep->second->points.end() );
            }

            // add the new combined segment to the end of our list (so we'll hit it later)
            Segment* new_seg = new Segment( seg_id++, new_part );
            segments.push_back( new_seg );
            num_active_segs--;
            
            //osg::notify(osg::WARN) << "combined with seg " << ep->second->id << " to form seg " << new_seg->id 
            //    << ", now there are " << num_active_segs << " active segs." << std::endl;

            // deactivate the old segments
            seg->active = false; 
            ep->second->active = false;
            i = segments.erase( i ); i--;
            ep_map.erase( ep );

            // stick it in the index too
            ep_map[new_seg->points.front()] = new_seg;
            ep_map[new_seg->points.back()] = new_seg;
        }
        else if ( seg->points.size() > 1 ) // next try the back point:
        {
            ep = ep_map.find( seg->points.back() );
            if ( ep != ep_map.end() && ep->second->active && seg->id != ep->second->id )
            {
                found = true;
                GeoPointList new_part;

                new_part.insert( new_part.begin(), seg->points.begin(), seg->points.end()-1 );

                if ( seg->points.back() == ep->second->points.front() ) // back matches front
                {
                    new_part.insert( new_part.end(), ep->second->points.begin(), ep->second->points.end() );
                }
                else // back matches back
                {
                    std::reverse( new_part.begin(), new_part.end() );
                    new_part.insert( new_part.begin(), ep->second->points.begin(), ep->second->points.end() );
                }           

                // add the new combined segment to the end of our list (so we'll hit it later)
                Segment* new_seg = new Segment( seg_id++, new_part );
                segments.push_back( new_seg );
                num_active_segs--;
                
                //osg::notify(osg::WARN) << "combined with seg " << ep->second->id << " to form seg " << new_seg->id 
                //    << ", now there are " << num_active_segs << " active segs." << std::endl;

                // deactivate the old segments
                seg->active = false; 
                ep->second->active = false;
                i = segments.erase( i ); i--;
                ep_map.erase( ep );

                // stick it in the index too
                ep_map[new_seg->points.front()] = new_seg;
                ep_map[new_seg->points.back()] = new_seg;
            }
        }

        if ( !found ) // no ep match, so just log this seg in the endpoint map.
        {
            ep_map[seg->points.front()] = seg;
            if ( seg->points.front() != seg->points.back() )
                ep_map[seg->points.back()] = seg;   

            //osg::notify(osg::WARN) << "added" << std::endl;
        }
    }

    // finally, collect all remaining "active" segments.
    for( SegmentList::iterator i = segments.begin(); i != segments.end(); i++ )
    {
        Segment* seg = i->get();
        if ( seg->active )
        {
            GeoShape shape( GeoShape::TYPE_LINE, env->getInputSRS() );
            shape.getParts().push_back( seg->points );
            Feature* f = new SimpleFeature();
            f->getShapes().push_back( shape );
            output.push_back( f );
        }
    }

    if ( start_count > 0 )
    {
        //osg::notify(osg::NOTICE) << "[Combine] combined " << start_count << " segments into " << output.size()
        //    << " features (" << (100-(int)(100.0f*(float)output.size()/(float)start_count)) << "% reduction)"
        //    << std::endl;
    }

    return output;
}