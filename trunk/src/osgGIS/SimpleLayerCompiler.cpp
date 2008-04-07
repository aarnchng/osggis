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
#include <osgGIS/FilterGraph>
#include <osgGIS/SceneGraphCompiler>
#include <osgGIS/FilterEnv>
#include <osgGIS/FadeHelper>
#include <osg/Group>
#include <osg/LOD>
#include <osg/Notify>
#include <osg/Depth>
#include <osgDB/FileNameUtils>
#include <osgSim/LineOfSight> // for the DatabaseCacheReadCallback

using namespace osgGIS;

SimpleLayerCompiler::SimpleLayerCompiler()
{
    //NOP
}


osg::Node*
SimpleLayerCompiler::compileLOD( FeatureLayer* layer, FilterGraph* graph )
{
    osg::ref_ptr<FilterEnv> env = getSession()->createFilterEnv();
    env->setExtent( getAreaOfInterest( layer ) );
    env->setTerrainNode( terrain.get() );
    env->setTerrainSRS( terrain_srs.get() );
    env->setTerrainReadCallback( read_cb.get() );
    SceneGraphCompiler compiler( layer, graph );
    return compiler.compile( env.get() );
}


osg::Node*
SimpleLayerCompiler::compile( FeatureLayer* layer, const std::string& output_file )
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

    for( FilterGraphRangeList::iterator i = graph_ranges.begin(); i != graph_ranges.end(); i++ )
    {
        osg::Node* range = compileLOD( layer, i->graph.get() );
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

    if ( getRenderOrder() >= 0 )
    {
        const std::string& bin_name = result->getOrCreateStateSet()->getBinName();
        result->getOrCreateStateSet()->setRenderBinDetails( getRenderOrder(), bin_name );
        result->getOrCreateStateSet()->setAttributeAndModes( new osg::Depth( osg::Depth::ALWAYS ), osg::StateAttribute::ON );
    }

    localizeResourceReferences( result );
    finalizeLayer( osgDB::getFilePath( output_file ) );

    return result;
}