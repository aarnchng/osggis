///**
// * osgGIS - GIS Library for OpenSceneGraph
// * Copyright 2007 Glenn Waldron and Pelican Ventures, Inc.
// * http://osggis.org
// *
// * osgGIS is free software; you can redistribute it and/or modify
// * it under the terms of the GNU Lesser General Public License as published by
// * the Free Software Foundation; either version 2 of the License, or
// * (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// * GNU Lesser General Public License for more details.
// *
// * You should have received a copy of the GNU Lesser General Public License
// * along with this program.  If not, see <http://www.gnu.org/licenses/>
// */
//
//#include <osgGIS/DrawableFilterState>
//#include <osgGIS/CollectionFilterState>
//#include <osgGIS/DisperseFilterState>
//#include <osgGIS/NodeFilterState>
//#include <osg/Notify>
//
//using namespace osgGIS;
//
//DrawableFilterState::DrawableFilterState( DrawableFilter* _filter )
//{
//    filter = _filter;
//}
//
//void
//DrawableFilterState::push( Feature* input )
//{
//    if ( input && input->hasShapeData() )
//        in_features.push_back( input );
//}
//
//void
//DrawableFilterState::push( const FeatureList& input )
//{
//    for( FeatureList::const_iterator i = input.begin(); i != input.end(); i++ )
//        if ( i->get()->hasShapeData() )
//            in_features.push_back( i->get() );
//    //in_features.insert( in_features.end(), input.begin(), input.end() );
//}
//
//void
//DrawableFilterState::push( osg::Drawable* input )
//{
//    in_fragments.push_back( input );
//}
//
//void
//DrawableFilterState::push( const FragmentList& input )
//{
//    in_fragments.insert( in_fragments.end(), input.begin(), input.end() );
//}
//
//bool
//DrawableFilterState::traverse( FilterEnv* in_env )
//{
//    bool ok = true;
//
//    osg::ref_ptr<FilterEnv> env = in_env->advance();
//
//    FilterState* next = getNextState();
//    if ( next )
//    {
//        FragmentList output =
//            in_features.size() > 0? filter->process( in_features, env.get() ) :
//            in_fragments.size() > 0? filter->process( in_fragments, env.get() ) :
//            FragmentList();
//        
//        if ( dynamic_cast<NodeFilterState*>( next ) )
//        {
//            NodeFilterState* state = static_cast<NodeFilterState*>( next );
//            state->push( output );
//        }
//        else if ( dynamic_cast<DrawableFilterState*>( next ) )
//        {
//            DrawableFilterState* state = static_cast<DrawableFilterState*>( next );
//            state->push( output );
//        }
//        else if ( dynamic_cast<CollectionFilterState*>( next ) )
//        {
//            CollectionFilterState* state = static_cast<CollectionFilterState*>( next );
//            state->push( output );
//        }
//        else if ( dynamic_cast<DisperseFilterState*>( next ) )
//        {
//            DisperseFilterState* state = static_cast<DisperseFilterState*>( next );
//            state->push( output );
//        }
//
//        ok = next->traverse( env.get() );
//    }
//
//    in_features.clear();
//    in_fragments.clear();
//
//    return ok;
//}
