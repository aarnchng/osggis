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

#include <osgGIS/FeatureStoreCompiler>
#include <osgGIS/Filter>
#include <osgGIS/FeatureFilter>
#include <osgGIS/FragmentFilter>
#include <osgGIS/NodeFilter>
#include <osgGIS/Registry>
#include <osg/Notify>

using namespace osgGIS;


FeatureStoreCompiler::FeatureStoreCompiler( FeatureLayer* _layer, FilterGraph* _graph )
{
    layer = _layer;
    graph = _graph;
}


FeatureStoreCompiler::~FeatureStoreCompiler()
{
}


FeatureLayer*
FeatureStoreCompiler::getFeatureLayer() 
{
    return layer.get();
}


FilterGraph*
FeatureStoreCompiler::getFilterGraph()
{
    return graph.get();
}


Session*
FeatureStoreCompiler::getSession()
{
    if ( !session.valid() )
        session = new Session();
    return session.get();
}


bool
FeatureStoreCompiler::compile( const std::string& output_uri, FilterEnv* env_template )
{
    // Set up the initial filter environment:
    osg::ref_ptr<FilterEnv> env;
    if ( env_template )
    {
        env = new FilterEnv( *env_template );
        session = env->getSession();
    }
    else
    {
        env = getSession()->createFilterEnv();
    }

    env->setInputSRS( layer->getSRS() );
    env->setOutputSRS( layer->getSRS() );

    // Get the input feature set:
    FeatureCursor cursor = layer->getCursor( env->getExtent() );

    // Run the filter graph.
    FilterGraphResult r = graph->computeFeatureStore( cursor, env.get(), output_uri );
    return r.isOK();
}

