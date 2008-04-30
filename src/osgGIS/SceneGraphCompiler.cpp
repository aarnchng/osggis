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

#include <osgGIS/SceneGraphCompiler>
#include <osgGIS/Filter>
#include <osgGIS/FeatureFilter>
#include <osgGIS/FragmentFilter>
#include <osgGIS/NodeFilter>
#include <osgGIS/Registry>
#include <osgGIS/Utils>
#include <osg/Notify>

using namespace osgGIS;


SceneGraphCompiler::SceneGraphCompiler( FeatureLayer* _layer, FilterGraph* _graph )
{
    layer = _layer;
    graph = _graph;
}


SceneGraphCompiler::~SceneGraphCompiler()
{
}


FeatureLayer*
SceneGraphCompiler::getFeatureLayer() 
{
    return layer.get();
}


FilterGraph*
SceneGraphCompiler::getFilterGraph()
{
    return graph.get();
}


Session*
SceneGraphCompiler::getSession()
{
    if ( !session.valid() )
        session = new Session();
    return session.get();
}


osg::Group*
SceneGraphCompiler::compile()
{
    osg::ref_ptr<FilterEnv> env = getSession()->createFilterEnv();
    return compile( env.get() );
}


osg::Group*
SceneGraphCompiler::compile( FilterEnv* env_template )
{
    osg::ref_ptr<osg::Group> result = new osg::Group();

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
    osg::NodeList output;

    FilterGraphResult r = graph->computeNodes( cursor, env.get(), output );
    if ( r.isOK() )
    {
        for( osg::NodeList::const_iterator i = output.begin(); i != output.end(); i++ )
            result->addChild( i->get() );
    }

    // if there's no geometry, just return NULL.
    return GeomUtils::getNumGeodes( result.get() ) > 0? result.release() : NULL;
}


osg::Group*
SceneGraphCompiler::compile( FeatureCursor& cursor )
{
    osg::ref_ptr<osg::Group> result = new osg::Group();

    // Set up the initial filter environment:
    osg::ref_ptr<FilterEnv> env = getSession()->createFilterEnv();

    env->setInputSRS( layer->getSRS() );
    env->setOutputSRS( layer->getSRS() );

    // compute the extent
    GeoExtent extent;
    for( cursor.reset(); cursor.hasNext(); )
    {
        Feature* f = cursor.next();
        if ( !extent.isValid() )
            extent = f->getExtent();
        else 
            extent.expandToInclude( f->getExtent() );
    }
    env->setExtent( extent );

    cursor.reset();

    // Run the filter graph.
    osg::NodeList output;

    FilterGraphResult r = graph->computeNodes( cursor, env.get(), output );
    if ( r.isOK() )
    {
        for( osg::NodeList::const_iterator i = output.begin(); i != output.end(); i++ )
            result->addChild( i->get() );
    }

    // if there's no geometry, just return NULL.
    return GeomUtils::getNumGeodes( result.get() ) > 0? result.release() : NULL;
}