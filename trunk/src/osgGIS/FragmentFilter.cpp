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

#include <osgGIS/FragmentFilter>
#include <osgGIS/FragmentFilterState>
#include <osg/Notify>

using namespace osgGIS;

FragmentFilter::FragmentFilter()
{
    //NOP
}

FragmentFilter::FragmentFilter( const FragmentFilter& rhs )
: Filter( rhs )
{
    //NOP
}

FragmentFilter::~FragmentFilter()
{
    //NOP
}

FilterState* 
FragmentFilter::newState() const
{
    return new FragmentFilterState( static_cast<FragmentFilter*>( clone() ) );
}

FragmentList
FragmentFilter::process( Feature* f, FilterEnv* env )
{
    return FragmentList();
}


FragmentList
FragmentFilter::process( FeatureList& input, FilterEnv* env )
{
    FragmentList output;
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        FragmentList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


FragmentList
FragmentFilter::process( Fragment* drawable, FilterEnv* env )
{
    FragmentList output;
    output.push_back( drawable );
    return output;
}


FragmentList 
FragmentFilter::process( FragmentList& input, FilterEnv* env )
{
    FragmentList output;
    for( FragmentList::iterator i = input.begin(); i != input.end(); i++ )
    {
        FragmentList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}

