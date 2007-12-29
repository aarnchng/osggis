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

#include <osgGIS/SimpleLayerCompiler>
#include <osgGIS/Script>
#include <osgGIS/Compiler>
#include <osgGIS/FilterEnv>
#include <osgGIS/FadeHelper>
#include <osg/Group>
#include <osg/LOD>
#include <osg/Notify>
#include <osgSim/LineOfSight> // for the DatabaseCacheReadCallback

using namespace osgGIS;

SimpleLayerCompiler::SimpleLayerCompiler()
{
    //NOP
}


osg::Node*
SimpleLayerCompiler::compileLOD( FeatureLayer* layer, Script* script )
{
    osg::ref_ptr<FilterEnv> env = new FilterEnv();
    env->setExtent( layer->getExtent() );
    env->setTerrainNode( terrain.get() );
    env->setTerrainSRS( terrain_srs.get() );
    env->setTerrainReadCallback( read_cb.get() );
    Compiler compiler( layer, script );
    return compiler.compile( env.get() );
}


osg::Node*
SimpleLayerCompiler::compile( FeatureLayer* layer )
{
    osg::Node* result = NULL;

    if ( !layer ) {
        osg::notify( osg::WARN ) << "Illegal null feature layer" << std::endl;
        return result;
    }
    
    osg::LOD* lod = new osg::LOD();

    if ( getFadeLODs() )
    {
        FadeHelper::enableFading( lod->getOrCreateStateSet() );
    }

    for( ScriptRangeList::iterator i = script_ranges.begin(); i != script_ranges.end(); i++ )
    {
        osg::Node* range = compileLOD( layer, i->script.get() );
        if ( range )
        {
            lod->addChild( range, i->min_range, i->max_range );

            if ( getFadeLODs() )
            {
                FadeHelper::setOuterFadeDistance( i->max_range, range->getOrCreateStateSet() );
                FadeHelper::setInnerFadeDistance( i->max_range - .2*(i->max_range-i->min_range), range->getOrCreateStateSet() );
            }
        }
    }

    result = lod;

    if ( getOverlay() )
    {
        result = convertToOverlay( lod );
    }

    return result;
}