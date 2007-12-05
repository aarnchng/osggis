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

#include <osgGIS/GriddedLayerCompiler>
#include <osgGIS/GeoExtent>
#include <osgGIS/Compiler>
#include <osg/Node>
#include <osg/LOD>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/Group>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include <osg/BlendFunc>
#include <osgUtil/Optimizer>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include <sstream>

using namespace osgGIS;


GriddedLayerCompiler::GriddedLayerCompiler()                            
{
    num_rows = 10;
    num_cols = 10;
    paged = false;
    fade_lods = false;
}


GriddedLayerCompiler::~GriddedLayerCompiler()
{
    //NOP
}

void
GriddedLayerCompiler::setNumRows( int value )
{
    num_rows = value;
}

int
GriddedLayerCompiler::getNumRows() const
{
    return num_rows;
}

void
GriddedLayerCompiler::setNumColumns( int value )
{
    num_cols = value;
}

int
GriddedLayerCompiler::getNumColumns() const
{
    return num_cols;
}

void
GriddedLayerCompiler::setPaged( bool value )
{
    paged = value;
}

bool
GriddedLayerCompiler::getPaged() const
{
    return paged;
}

void
GriddedLayerCompiler::setFadeLODs( bool value )
{
    fade_lods = value;
}

bool
GriddedLayerCompiler::getFadeLODs() const
{
    return fade_lods;
}


osg::Node*
GriddedLayerCompiler::compileLOD( FeatureLayer* layer, Script* script, const GeoExtent& extent )
{
    osg::ref_ptr<FilterEnv> env = new FilterEnv();
    env->setExtent( extent );
    env->setTerrainNode( terrain.get() );
    env->setTerrainSRS( terrain_srs.get() );
    env->setTerrainReadCallback( read_cb.get() );
    Compiler compiler( layer, script );
    return compiler.compile( env.get() );
}


osg::Node*
GriddedLayerCompiler::compile( FeatureLayer* layer )
{
    return compile( layer, "" );
}

static const char *fadelod_vert = {
    "varying vec3 position;\n"
    "void main(void)\n"
    "{\n"
    "    position = gl_ModelViewMatrix * gl_Vertex;\n"
    "    gl_Position = ftransform();\n"
    "    gl_FrontColor = gl_Color;\n"
    "}\n"
};

static const char *fadelod_frag = {
    "varying vec3 position;\n"
    "uniform float fade_in_dist;\n"
    "uniform float fade_out_dist;\n"
    "void main(void)\n"
    "{\n"
    "    float fade = 1.0 - smoothstep( fade_in_dist, fade_out_dist, length(position) );\n"
    "    gl_FragColor = vec4( gl_Color.rgb, gl_Color.a * fade );\n"
    "}\n"
};


osg::Node*
GriddedLayerCompiler::compile( FeatureLayer* layer, const std::string& output_file )
{
    std::string output_dir = osgDB::getFilePath( output_file );
    std::string output_prefix = osgDB::getStrippedName( output_file );
    std::string output_extension = osgDB::getFileExtension( output_file );

    osg::Group* root = new osg::Group();

    // determine the working extent:
    GeoExtent extent = layer->getExtent();
    if ( extent.isInfinite() || !extent.isValid() )
    {
        const SpatialReference* srs = layer->getSRS()->getBasisSRS();

        extent = GeoExtent(
            GeoPoint( -180.0, -90.0, srs ),
            GeoPoint(  180.0,  90.0, srs ) );
    }

    if ( fade_lods )
    {
        osg::Program* fade_program = new osg::Program();
        fade_program->addShader( new osg::Shader( osg::Shader::VERTEX, fadelod_vert ) );
        fade_program->addShader( new osg::Shader( osg::Shader::FRAGMENT, fadelod_frag ) );
        root->getOrCreateStateSet()->setAttributeAndModes( fade_program, osg::StateAttribute::ON );
    
        osg::BlendFunc* blend_func = new osg::BlendFunc();
        blend_func->setFunction( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        root->getOrCreateStateSet()->setAttributeAndModes( blend_func, osg::StateAttribute::ON );
        root->getOrCreateStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    }
    
    if ( extent.isValid() )
    {
        const GeoPoint& sw = extent.getSouthwest();
        const GeoPoint& ne = extent.getNortheast();
        osg::ref_ptr<SpatialReference> srs = extent.getSRS();

        double dx = (ne.x() - sw.x()) / num_cols;
        double dy = (ne.y() - sw.y()) / num_rows;

        for( unsigned int col = 0; col < num_cols; col++ )
        {
            for( unsigned int row = 0; row < num_rows; row++ )
            {
                read_cb = new SmartReadCallback();

                GeoExtent sub_extent(
                    GeoPoint( sw.x() + (double)col*dx, sw.y() + (double)row*dy, srs.get() ),
                    GeoPoint( sw.x() + (double)(col+1)*dx, sw.y() + (double)(row+1)*dy, srs.get() ) );

                osg::notify(osg::NOTICE) <<
                    "Building row=" << row << " col=" << col << ", extent=" << sub_extent.toString() << "... " << std::flush;
                    
                float min_range = FLT_MAX, max_range = FLT_MIN;
                osg::ref_ptr<osg::LOD> lod = new osg::LOD();
                for( ScriptRangeList::iterator i = script_ranges.begin(); i != script_ranges.end(); i++ )
                {
                    osg::Node* range = compileLOD( layer, i->script.get(), sub_extent );
                    if ( range )
                    {
                        lod->addChild( range, i->min_range, i->max_range );
                        if ( i->min_range < min_range ) min_range = i->min_range;
                        if ( i->max_range > max_range ) max_range = i->max_range;
                        //TODO: need to set centroid and cluster culling
                        
                        if ( fade_lods )
                        {
                            osg::Uniform* fade_out = new osg::Uniform( "fade_out_dist", (float)i->max_range );
                            range->getOrCreateStateSet()->addUniform( fade_out, osg::StateAttribute::ON );
                            osg::Uniform* fade_in = new osg::Uniform( "fade_in_dist", (float)(i->max_range - .2*(i->max_range - i->min_range)) );
                            range->getOrCreateStateSet()->addUniform( fade_in, osg::StateAttribute::ON );
                        }
                    }
                }

                if ( paged )
                {
                    osg::PagedLOD* plod = new osg::PagedLOD();
                    std::stringstream str;
                    str << output_prefix << "_r" << row << "_c" << col << "." << output_extension;
                    std::string tile_filename = str.str();
                    plod->setFileName( 0, tile_filename );
                    plod->setRange( 0, min_range, max_range );
                    plod->setCenter( lod->getBound().center() );
                    plod->setRadius( lod->getBound().radius() );
                    std::string tile_path = osgDB::concatPaths( output_dir, tile_filename );
                    osgDB::writeNodeFile( *(lod.get()), tile_path );
                    osg::notify(osg::NOTICE) << tile_filename << "... " << std::flush;
                    root->addChild( plod );
                }
                else
                {
                    root->addChild( lod.get() );
                }

                osg::notify(osg::NOTICE) << "done; hits = " << read_cb->getMruHitRatio() << std::endl;
            }
        }
    }

    osgUtil::Optimizer opt;
    opt.optimize( root, osgUtil::Optimizer::SPATIALIZE_GROUPS );

    return root;
}

