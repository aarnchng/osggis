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

#include <osgGISProjects/SimpleMapLayerCompiler>
#include <osgGIS/Report>
#include <osgGIS/Session>
#include <osgGIS/Utils>
#include <osgGIS/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/NodeVisitor>
#include <sstream>
#include <list>

using namespace osgGIS;
using namespace osgGISProjects;
using namespace OpenThreads;

/*****************************************************************************/

static MapLayerLevelOfDetail*
getLodForKey( unsigned int key, MapLayer* map_layer )
{
    unsigned int level = 0;
    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++, level++ )
    {
        if ( key == level )
            return i->get();
    }
    return NULL;
}

//static void
//collectGeometryKeys( GridProfile* profile, MapLayer* map_layer, GridCellKeyList& out_keys )
//{
//    for( unsigned int col = 0; col < profile->getNumColumns(); col++ )
//    {
//        for( unsigned int row = 0; row < profile->getNumRows(); row++ )
//        {
//            for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
//            {
//                MapLayerLevelOfDetail* level_def = i->get();
//                GridCellKey key( col, row, level_def->getDepth(), profile );
//                out_keys.push_back( key );
//            }
//        }
//    }
//}
//
//static Task*
//createTask( const GridCellKey& key, MapLayerCompiler* compiler )
//{
//    Task* task = NULL;
//
//    MapLayerLevelOfDetail* def = getLodForKey( key, compiler->getMapLayer() );
//    if ( def )
//    {
//        // construct a filter environment template to use for all tasks:
//        osg::ref_ptr<FilterEnv> cell_env = compiler->getSession()->createFilterEnv();
//
//        cell_env->setTerrainNode( compiler->getTerrainNode() );
//        cell_env->setTerrainSRS( compiler->getTerrainSRS() );
//
//        std::string abs_path = compiler->createAbsPathFromTemplate( "g" + key.toString() );
//
//        GeoExtent extent = compiler->getMapLayer()->getAreaOfInterest().getSRS()->transform( key.getExtent() );
//
//        cell_env->setInputSRS( def->getFeatureLayer()->getSRS() );
//        cell_env->setExtent( extent );
//        cell_env->setProperty( Property( "compiler.cell_id", key.toString() ) );
//
//        task = new MapLayerCompiler::CellCompiler(
//            abs_path,
//            def->getFeatureLayer(),
//            def->getFilterGraph(),
//            cell_env.get(),
//            def->getResourcePackager()? def->getResourcePackager() : compiler->getResourcePackager(),
//            compiler->getArchive() );
//
//        osgGIS::notify( osg::INFO )
//            << "Task: Key = " << key.toString() << ", LOD = " << key.getLevel() << ", Extent = " << extent.toString() 
//            << " (w=" << extent.getWidth() << ", h=" << extent.getHeight() << ")"
//            << std::endl;
//    }
//
//    return task;
//}

/*****************************************************************************/

SimpleMapLayerCompiler::SimpleMapLayerCompiler( MapLayer* _layer, Session* _session )
: MapLayerCompiler( _layer, _session )
{
    //NOP
}

MapLayerCompiler::Profile*
SimpleMapLayerCompiler::createProfile()
{
    return new Profile();
}

unsigned int
SimpleMapLayerCompiler::queueTasks( MapLayerCompiler::Profile* _profile, TaskManager* task_man )
{
    unsigned int level = 0;
    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++, level++ )
    {
        MapLayerLevelOfDetail* level_def = i->get();

        std::stringstream s;
        s << level;

        osg::ref_ptr<FilterEnv> cell_env = getSession()->createFilterEnv();
        cell_env->setExtent( GeoExtent::infinite() );
        cell_env->setTerrainNode( getTerrainNode() );
        cell_env->setTerrainSRS( getTerrainSRS() );

        Task* task = new MapLayerCompiler::CellCompiler(
            s.str(),
            level_def->getFeatureLayer(),
            level_def->getFilterGraph(),
            cell_env.get(),
            NULL );

        task_man->queueTask( task );
    }
    return level;
}

void
SimpleMapLayerCompiler::processCompletedTask( CellCompiler* task )
{
    if ( task->getResult().isOK() && task->getResultNode() )
    {
        if ( !lod.valid() )
        {
            lod = new osg::LOD();
        }

        unsigned int key = (unsigned int)::atoi( task->getName().c_str() );
        MapLayerLevelOfDetail* def = getLodForKey( key, getMapLayer() );
        if ( def )
        {
            lod->addChild( task->getResultNode(), def->getMinRange(), def->getMaxRange() );
        }
    }
}

// builds and writes all the index nodes.
void
SimpleMapLayerCompiler::buildIndex( Profile* _profile )
{
    scene_graph = new osg::Group();
    if ( lod.valid() )
    {
        scene_graph->addChild( lod.get() );
    }
}
