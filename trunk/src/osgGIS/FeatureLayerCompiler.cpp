/**
/* osgGIS - GIS Library for OpenSceneGraph
 * Copyright 2007-2008 Glenn Waldron and Pelican Ventures, Inc.
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

#include <osgGIS/FeatureLayerCompiler>
#include <osg/Notify>

using namespace osgGIS;

FeatureLayerCompiler::FeatureLayerCompiler(FeatureLayer* _layer,
                                           FilterGraph*  _graph,
                                           FilterEnv*    _env)
: layer( _layer ), filter_graph( _graph ), env( _env )
{
    //NOP
}

FeatureLayerCompiler::FeatureLayerCompiler(const std::string& _name,
                                           FeatureLayer*      _layer,
                                           FilterGraph*       _graph,
                                           FilterEnv*         _env)
: Task( _name ),
  layer( _layer ), filter_graph( _graph ), env( _env )
{
    //NOP
}

FilterGraphResult&
FeatureLayerCompiler::getResult() {
    return result;
}

osg::Node*
FeatureLayerCompiler::getResultNode() {
    return result_node.get();
}

void
FeatureLayerCompiler::run()
{
    if ( !layer.valid() )
    {
        result = FilterGraphResult::error( "Illegal: null feature layer" );
    }
    else if ( !filter_graph.valid() )
    {
        result = FilterGraphResult::error( "Illegal: null filter graph" );
    }
    else if ( !env.valid() )
    {
        result = FilterGraphResult::error( "Illegal: null filter environment" );
    }
    else 
    {
        // ensure the input SRS matches that of the layer:
        env->setInputSRS( layer->getSRS() );

        // retrieve the features in the given extent:
        FeatureCursor cursor = layer->getCursor( env->getExtent() );

        // and compile the filter graph:
        osg::Group* temp = NULL;
        result = filter_graph->computeNodes( cursor, env.get(), temp );
        result_node = temp;
    }
}
