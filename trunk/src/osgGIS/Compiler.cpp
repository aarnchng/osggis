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
#include <osg/Notify>

using namespace osgGIS;



Compiler::Compiler( FeatureLayer* _layer, Script* _script )
{
    layer = _layer;
    script = _script;
}


Compiler::~Compiler()
{
}


FeatureLayer*
Compiler::getFeatureLayer() 
{
    return layer.get();
}


Script*
Compiler::getScript()
{
    return script.get();
}


osg::Group*
Compiler::compile()
{
    osg::ref_ptr<FilterEnv> env = new FilterEnv();
    return compile( env.get() );
}


osg::Group*
Compiler::compile( FilterEnv* env_template )
{
    osg::Group* result = new osg::Group();

    // A context will hold the filter environment stack and other
    // script-scoped information:
    osg::ref_ptr<ScriptContext> context = new ScriptContext();
//    script->resetFilters( context.get() );

    // Set up the initial filter environment:
    osg::ref_ptr<FilterEnv> env = env_template?
        new FilterEnv( *env_template ) : new FilterEnv();

    env->setInputSRS( layer->getSRS() );
    env->setOutputSRS( layer->getSRS() );

    // Get the input feature set:
    osg::ref_ptr<FeatureCursor> cursor = layer->createCursor( 
        env->getExtent() );

    // Run the script.
    osg::NodeList output;

    if ( script->run( cursor.get(), env.get(), output ) )
    {
        //osg::NodeList output = script->getOutput();
        for( osg::NodeList::iterator i = output.begin(); i != output.end(); i++ )
            result->addChild( i->get() );
    }
    return result;
}