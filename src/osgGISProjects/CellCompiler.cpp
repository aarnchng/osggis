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
#include <osgGISProjects/CellCompiler>
#include <osgGIS/Utils>
#include <osgDB/FileUtils>

using namespace osgGISProjects;


CellCompiler::CellCompiler(const std::string& _cell_id,
                           const std::string& _abs_output_uri,
                           FeatureLayer*      layer,
                           FilterGraph*       graph,
                           float              _min_range,
                           float              _max_range,
                           FilterEnv*         env,
                           ResourcePackager*  _packager,
                           osgDB::Archive*    _archive,
                           osg::Referenced*   _user_data )
: FeatureLayerCompiler( _abs_output_uri, layer, graph, env ),
  packager( _packager ),
  cell_id( _cell_id ),
  abs_output_uri( _abs_output_uri ),
  min_range( _min_range ),
  max_range( _max_range ),
  archive( _archive )
{
    //TODO: maybe the FilterEnv should just have one of these by default.
    SmartReadCallback* smart = new SmartReadCallback();
    smart->setMinRange( min_range );
    env->setTerrainReadCallback( smart );
    setUserData( _user_data );
}

const std::string&
CellCompiler::getCellId() const {
    return cell_id;
}

const std::string&
CellCompiler::getLocation() const {
    return abs_output_uri;
}

void
CellCompiler::run() // overrides FeatureLayerCompiler::run()
{
    // first check to see whether this cell needs compiling:
    need_to_compile = archive.valid() || !osgDB::fileExists( abs_output_uri );

    has_drawables = false;

    if ( need_to_compile )
    {
        // Compile the cell:
        FeatureLayerCompiler::run();

        // Write the resulting node graph to disk, first ensuring that the output folder exists:
        // TODO: consider whether this belongs in the runSynchronousPostProcess() method
        if ( getResult().isOK() && getResultNode() )
        {
            has_drawables = GeomUtils::hasDrawables( getResultNode() );
        }
    }
    else
    {
        result = FilterGraphResult::ok();
        has_drawables = true;
    }
}

void
CellCompiler::runSynchronousPostProcess( Report* report )
{
    if ( need_to_compile )
    {
        if ( !getResult().isOK() )
        {
            osgGIS::notice() << getName() << " failed to compile: " << getResult().getMessage() << std::endl;
            return;
        }

        if ( !getResultNode() || !has_drawables )
        {
            osgGIS::info() << getName() << " resulted in no geometry" << std::endl;
            result_node = NULL;
            return;
        }

        if ( packager.valid() )
        {
            // TODO: we should probably combine the following two calls into one:

            // update any texture/model refs in preparation for packaging:
            packager->rewriteResourceReferences( getResultNode() );

            // copy resources to their final destination
            packager->packageResources( env->getResourceCache(), report );

            // write the node data itself
            osg::ref_ptr<osg::Node> node_to_package = getResultNode();

            if ( !packager->packageNode( node_to_package.get(), abs_output_uri ) ) //, env->getCellExtent(), min_range, max_range ) )
            {
                osgGIS::warn() << getName() << " failed to package node to output location" << std::endl;
                result = FilterGraphResult::error( "Cell built OK, but failed to deploy to disk/archive" );
            }
        }
    }
}