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

#include <osgGIS/Compiler>
#include <osgGIS/Filter>
#include <osgGIS/FeatureFilter>
#include <osgGIS/DrawableFilter>
#include <osgGIS/NodeFilter>
#include <osgGIS/Registry>
#include <osg/Notify>

using namespace osgGIS;


Compiler::Compiler( FeatureLayer* _layer, FilterGraph* _graph )
{
    layer = _layer;
    graph = _graph;
}


Compiler::~Compiler()
{
}


FeatureLayer*
Compiler::getFeatureLayer() 
{
    return layer.get();
}


FilterGraph*
Compiler::getFilterGraph()
{
    return graph.get();
}


Session*
Compiler::getSession()
{
    if ( !session.valid() )
        session = new Session();
    return session.get();
}


osg::Group*
Compiler::compile()
{
    osg::ref_ptr<FilterEnv> env = getSession()->createFilterEnv();
    return compile( env.get() );
}


osg::Group*
Compiler::compile( FilterEnv* env_template )
{
    osg::Group* result = new osg::Group();

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
    osg::ref_ptr<FeatureCursor> cursor = layer->createCursor( 
        env->getExtent() );

    // Run the filter graph.
    osg::NodeList output;

    if ( graph->run( cursor.get(), env.get(), output ) )
    {
        for( osg::NodeList::iterator i = output.begin(); i != output.end(); i++ )
            result->addChild( i->get() );
    }
    return result;
}