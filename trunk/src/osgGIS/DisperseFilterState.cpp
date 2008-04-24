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
//#include <osgGIS/DisperseFilterState>
//#include <osgGIS/CollectionFilterState>
//#include <osgGIS/DrawableFilterState>
//#include <osgGIS/FeatureFilterState>
//#include <osgGIS/NodeFilterState>
//#include <osg/Notify>
//
//using namespace osgGIS;
//
//
//DisperseFilterState::DisperseFilterState( DisperseFilter* _filter )
//{
//    filter = _filter;
//}
//
//void 
//DisperseFilterState::push( const FeatureList& input )
//{
//    for( FeatureList::const_iterator i = input.begin(); i != input.end(); i++ )
//        if ( i->get()->hasShapeData() )
//            features.push_back( i->get() );
//    //features.insert( features.end(), input.begin(), input.end() );
//}
//
//
//void
//DisperseFilterState::push( Feature* input )
//{
//    if ( input && input->hasShapeData() )
//        features.push_back( input );
//}
//
//
//void 
//DisperseFilterState::push( const FragmentList& input )
//{
//    fragments.insert( fragments.end(), input.begin(), input.end() );
//}
//
//
//void
//DisperseFilterState::push( Fragment* input )
//{
//    fragments.push_back( input );
//}
//
//
//void 
//DisperseFilterState::push( const osg::NodeList& input )
//{
//    nodes.insert( nodes.end(), input.begin(), input.end() );
//}
//
//
//void
//DisperseFilterState::push( osg::Node* input )
//{
//    nodes.push_back( input );
//}
//
//
//template<typename D, typename T>
//static bool
//traverse_each( D& data, T* state, FilterEnv* env )
//{
//    for( D::const_iterator i = data.begin(); i != data.end(); i++ )
//    {
//        state->push( i->get() );
//        if ( !state->traverse( env ) )
//            return false;
//    }
//    return true;
//}
//
//bool
//DisperseFilterState::traverse( FilterEnv* in_env )
//{
//    bool ok = true;
//    FilterEnv* env = in_env->clone();
//
//    FilterState* next = getNextState();
//    if ( next )
//    {
//        if ( dynamic_cast<FeatureFilterState*>( next ) )
//        {
//            FeatureFilterState* state = static_cast<FeatureFilterState*>( next );
//            if ( !features.empty() )
//                ok = traverse_each( features, state, env );
//            else
//                ok = false;
//        }
//        else if ( dynamic_cast<DrawableFilterState*>( next ) )
//        {
//            DrawableFilterState* state = static_cast<DrawableFilterState*>( next );
//            if ( !features.empty() )
//                ok = traverse_each( features, state, env );
//            else if ( !fragments.empty() )
//                ok = traverse_each( fragments, state, env );
//            else
//                ok = false;
//        }
//        else if ( dynamic_cast<NodeFilterState*>( next ) )
//        {
//            NodeFilterState* state = static_cast<NodeFilterState*>( next );
//            if ( !features.empty() )
//                ok = traverse_each( features, state, env );
//            else if ( !fragments.empty() )
//                ok = traverse_each( fragments, state, env );
//            else if ( !nodes.empty() )
//                ok = traverse_each( nodes, state, env );
//            else
//                ok = false;
//        }
//        else if ( dynamic_cast<CollectionFilterState*>( next ) )
//        {
//            CollectionFilterState* state = static_cast<CollectionFilterState*>( next );
//            if ( !features.empty() )
//                ok = traverse_each( features, state, env );
//            else if ( !fragments.empty() )
//                ok = traverse_each( fragments, state, env );
//            else if ( !nodes.empty() )
//                ok = traverse_each( nodes, state, env );
//            else
//                ok = false;
//        }
//        else if ( dynamic_cast<DisperseFilterState*>( next ) )
//        {
//            DisperseFilterState* state = static_cast<DisperseFilterState*>( next );
//            if ( !features.empty() )
//                ok = traverse_each( features, state, env );
//            else if ( !fragments.empty() )
//                ok = traverse_each( fragments, state, env );
//            else if ( !nodes.empty() )
//                ok = traverse_each( nodes, state, env );
//            else
//                ok = false;
//        }
//        else
//        {
//            ok = false;
//        }
//    }
//
//    // clean up
//    features.clear();
//    fragments.clear();
//    nodes.clear();
//
//    return ok;
//}