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

#include <osgGIS/LayerCompiler>
#include <osgGIS/Resource>
#include <osgGIS/Session>
#include <osgGIS/Utils>
#include <osgSim/LineOfSight>
#include <osgSim/OverlayNode>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osg/TexEnv>
#include <osg/NodeVisitor>
#include <osg/Texture2D>
#include <osg/ProxyNode>
#include <OpenThreads/ScopedLock>
#include <map>
#include <string>
#include <queue>
#include <sstream>

using namespace osgGIS;
using namespace OpenThreads;


LayerCompiler::LayerCompiler()
{
    read_cb = new SmartReadCallback();
    render_order = -1;
    fade_lods = false;
    overlay = false;
    aoi_xmin = DBL_MAX;
    aoi_ymin = DBL_MAX;
    aoi_xmax = DBL_MIN;
    aoi_ymax = DBL_MIN;
    localize_resources = true;
    paged = false;
    compress_textures = false;
}

void
LayerCompiler::setTaskManager( TaskManager* _manager )
{
    task_manager = _manager;
}

TaskManager*
LayerCompiler::getTaskManager()
{
    return task_manager.get();
}

void
LayerCompiler::addFilterGraph( float min_range, float max_range, FilterGraph* graph )
{
    FilterGraphRange slice;
    slice.min_range = min_range;
    slice.max_range = max_range;
    slice.graph = graph;

    graph_ranges.push_back( slice );
}

LayerCompiler::FilterGraphRangeList&
LayerCompiler::getFilterGraphs()
{
    return graph_ranges;
}

void
LayerCompiler::setTerrain(osg::Node*              _terrain,
                          const SpatialReference* _terrain_srs,
                          const GeoExtent&        _terrain_extent )
{
    terrain        = _terrain;
    terrain_srs    = (SpatialReference*)_terrain_srs;
    terrain_extent = _terrain_extent;
}


void
LayerCompiler::setTerrain(osg::Node*              _terrain,
                          const SpatialReference* _terrain_srs )
{
    setTerrain( _terrain, _terrain_srs, GeoExtent::infinite() );
}

osg::Node*
LayerCompiler::getTerrainNode()
{
    return terrain.get();
}

SpatialReference* 
LayerCompiler::getTerrainSRS() 
{
    return terrain_srs.get();
}

const GeoExtent& 
LayerCompiler::getTerrainExtent() const
{
    return terrain_extent;
}

void
LayerCompiler::setArchive( osgDB::Archive* _archive, const std::string& _filename )
{
    archive = _archive;
    archive_filename = _filename;
}


osgDB::Archive*
LayerCompiler::getArchive() 
{
    return archive.get();
}

const std::string&
LayerCompiler::getArchiveFileName() const
{
    return archive_filename;
}

void
LayerCompiler::setRenderOrder( int value )
{
    render_order = value;
}

int
LayerCompiler::getRenderOrder() const
{
    return render_order;
}

void
LayerCompiler::setFadeLODs( bool value )
{
    fade_lods = value;
}

bool
LayerCompiler::getFadeLODs() const
{
    return fade_lods;
}

void
LayerCompiler::setPaged( bool value )
{
    paged = value;
}

bool
LayerCompiler::getPaged() const
{
    return paged;
}

void
LayerCompiler::setOverlay( bool value )
{
    overlay = value;
}

bool
LayerCompiler::getOverlay() const
{
    return overlay;
}

void
LayerCompiler::setCompressTextures( bool value )
{
    compress_textures = value;
}

bool
LayerCompiler::getCompressTextures() const
{
    return compress_textures;
}

osg::Node*
LayerCompiler::convertToOverlay( osg::Node* input )
{
    osgSim::OverlayNode* o_node = new osgSim::OverlayNode();
    o_node->getOrCreateStateSet()->setTextureAttribute( 1, new osg::TexEnv( osg::TexEnv::DECAL ) );
    o_node->setOverlaySubgraph( input );
    o_node->setOverlayTextureSizeHint( 1024 );
    return o_node;
}

Session*
LayerCompiler::getSession()
{
    if ( !session.valid() )
        session = new Session();

    return session.get();
}

void
LayerCompiler::setSession( Session* _session )
{
    session = _session;
}

void
LayerCompiler::setAreaOfInterest( double x0, double y0, double x1, double y1 )
{
    aoi_xmin = x0;
    aoi_ymin = y0;
    aoi_xmax = x1;
    aoi_ymax = y1;
}

GeoExtent
LayerCompiler::getAreaOfInterest( FeatureLayer* layer )
{
    if ( aoi_xmin < aoi_xmax && aoi_ymin < aoi_ymax )
    {
        return GeoExtent( aoi_xmin, aoi_ymin, aoi_xmax, aoi_ymax, layer->getSRS() );
    }
    else
    {
        return layer->getExtent();
    }
}

void 
LayerCompiler::setLocalizeResources( bool value )
{
    localize_resources = value;
}

bool 
LayerCompiler::getLocalizeResources() const
{
    return localize_resources;
}


Properties
LayerCompiler::getProperties()
{
    Properties props;
    //TODO - populate!
    return props;
}

void
LayerCompiler::setProperties( Properties& input )
{
    setPaged( input.getBoolValue( "paged", getPaged() ) );
    setFadeLODs( input.getBoolValue( "fade_lods", getFadeLODs() ) );
    setRenderOrder( input.getIntValue( "render_order", getRenderOrder() ) );
    setLocalizeResources( input.getBoolValue( "localize_resources", getLocalizeResources() ) );
    setCompressTextures( input.getBoolValue( "compress_textures", getCompressTextures() ) );

    aoi_xmin = input.getDoubleValue( "aoi_xmin", DBL_MAX );
    aoi_ymin = input.getDoubleValue( "aoi_ymin", DBL_MAX );
    aoi_xmax = input.getDoubleValue( "aoi_xmax", DBL_MIN );
    aoi_ymax = input.getDoubleValue( "aoi_ymax", DBL_MIN );
}


void
LayerCompiler::localizeResourceReferences( osg::Node* node )
{
    struct RewriteImageVisitor : public osg::NodeVisitor 
    {
        RewriteImageVisitor( const std::string& _archive_name, bool _compress_textures )
            : archive_name( osgDB::getSimpleFileName( _archive_name ) ),
              compress_textures( _compress_textures ),
              osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN )
        { 
            osg::notify( osg::INFO ) << "LayerCompiler: Localizing resources references" << std::endl;
        }
        
        std::string archive_name;
        bool compress_textures;

        virtual void apply( osg::Node& node )
        {
            osg::StateSet* ss = node.getStateSet();
            if ( ss ) rewrite( ss );
            osg::NodeVisitor::apply( node );
        }

        virtual void apply( osg::Geode& geode )
        {
            for( unsigned int i=0; i<geode.getNumDrawables(); i++ )
            {
                osg::StateSet* ss = geode.getDrawable( i )->getStateSet();
                if ( ss ) rewrite( ss );
                osg::NodeVisitor::apply( geode );
            }
        }

        virtual void apply( osg::ProxyNode& proxy )
        {
            proxy.setDatabasePath( archive_name );
            std::string name = proxy.getFileName( 0 );
            std::string simple = osgDB::getSimpleFileName( name );
            proxy.setFileName( 0, simple );
            osg::notify( osg::INFO ) << "  Rewrote " << name << " as " << simple << std::endl;
            osg::NodeVisitor::apply( proxy );
        }

        void rewrite( osg::StateSet* ss )
        {
            for( unsigned int i=0; i<ss->getTextureAttributeList().size(); i++ )
            {
                osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>( ss->getTextureAttribute( i, osg::StateAttribute::TEXTURE ) );
                if ( tex && tex->getImage() )
                {
                    const std::string& name = tex->getImage()->getFileName();

                    // fix the in-archive reference:
                    //if ( archive_name.length() > 0 )
                    //{
                    //    if ( !StringUtils::startsWith( name, archive_name ) )
                    //    {
                    //        std::string path = osgDB::concatPaths( archive_name, tex->getImage()->getFileName() );
                    //        tex->getImage()->setFileName( path );
                    //        osg::notify(osg::INFO) << "  Rewrote " << name << " as " << path << std::endl;
                    //    }
                    //}
                    //else
                    {
                        std::string simple = osgDB::getSimpleFileName( name );
                        
                        if ( compress_textures )
                        {
                            simple = osgDB::getNameLessExtension( simple ) + ".dds";
                        }

                        tex->getImage()->setFileName( simple );
                        osg::notify( osg::INFO ) << "  LayerCompiler::localizeResourceRefs, Rewrote " << name << " as " << simple << std::endl;
                    }
                }
            }
        }
    };

    if ( node && getLocalizeResources() )
    {
        RewriteImageVisitor v( getArchiveFileName(), getCompressTextures() );
        node->accept( v );
    }    
}


void
LayerCompiler::localizeResources( const std::string& output_folder )
{
    // collect the resources marked as used.
    ResourceList resources_to_localize = getSession()->getResourcesUsed( true );


    osg::ref_ptr<osgDB::ReaderWriter::Options> local_options = new osgDB::ReaderWriter::Options;

    for( ResourceList::const_iterator i = resources_to_localize.begin(); i != resources_to_localize.end(); i++ )
    {
        Resource* resource = i->get();

        // if we are localizing resources, attempt to copy each one to the output folder:
        if ( getLocalizeResources() )
        {
            if ( dynamic_cast<SkinResource*>( resource ) )
            {
                SkinResource* skin = static_cast<SkinResource*>( resource );

                osg::ref_ptr<osg::Image> image = skin->getImage();
                if ( image.valid() )
                {
                    std::string filename = osgDB::getSimpleFileName( image->getFileName() );

                    osg::ref_ptr<osg::Image> output_image = image.get();
                    if ( getCompressTextures() )
                    {
                        output_image = ImageUtils::convertRGBAtoDDS( image.get() );
                        filename = osgDB::getNameLessExtension( filename ) + ".dds";
                        output_image->setFileName( filename );
                    }

                    if ( getArchive() && !getArchive()->fileExists( filename ) )
                    {
                        osgDB::ReaderWriter::WriteResult r = getArchive()->writeImage( *(output_image.get()), filename, local_options.get() );
                        if ( r.error() )
                        {
                            osg::notify( osg::WARN ) << "  Failure to copy image " << filename << " into the archive" << std::endl;
                        }
                    }
                    else
                    {
                        if ( osgDB::fileExists( output_folder ) )
                        {
                            if ( !osgDB::writeImageFile( *(output_image.get()), PathUtils::combinePaths( output_folder, filename ), local_options.get() ) )
                            {
                                osg::notify( osg::WARN ) << "  FAILED to copy image " << filename << " into the folder " << output_folder << std::endl;
                            }
                        }
                        else
                        {
                            osg::notify( osg::WARN ) << "  FAILD to localize image " << filename << ", folder " << output_folder << " not found" << std::endl;
                        }
                    }
                }
            }

            else if ( dynamic_cast<ModelResource*>( resource ) )
            {
                ModelResource* model = static_cast<ModelResource*>( resource );

                osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( model->getAbsoluteURI() );
                if ( node.valid() )
                {
                    std::string filename = osgDB::getSimpleFileName( model->getAbsoluteURI() );
                    if ( getArchive() )
                    {
                        osgDB::ReaderWriter::WriteResult r = getArchive()->writeNode( *(node.get()), filename, local_options.get() );
                        if ( r.error() )
                        {
                            osg::notify( osg::WARN ) << "  Failure to copy model " << filename << " into the archive" << std::endl;
                        }
                    }
                    else
                    {
                        if ( osgDB::fileExists( output_folder ) )
                        {
                            if ( !osgDB::writeNodeFile( *(node.get()), osgDB::concatPaths( output_folder, filename ), local_options.get() ) )
                            {
                                osg::notify( osg::WARN ) << "  FAILED to copy model " << filename << " into the folder " << output_folder << std::endl;
                            }
                        }
                        else
                        {
                            osg::notify( osg::WARN ) << "  FAILD to localize model " << filename << ", folder " << output_folder << " not found" << std::endl;
                        }
                    }
                }
            }
        }

        // now remove any single-use (i.e. non-shared) resources (whether we are localizing them or not)
        if ( resource->isSingleUse() )
        {
            getSession()->getResources()->removeResource( resource );
        }
    }
}