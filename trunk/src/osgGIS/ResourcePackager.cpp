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

#include <osgGIS/ResourcePackager>
#include <osgGIS/Resource>
#include <osgGIS/Utils>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/ProxyNode>
#include <osg/Image>
#include <osg/StateSet>
#include <sstream>
#include <set>

using namespace osgGIS;

#define DEFAULT_COMPRESS_TEXTURES false
#define DEFAULT_MAX_TEX_SIZE      0      // 0 => no max
#define DEFAULT_INLINE_TEXTURES   false

ResourcePackager::ResourcePackager() 
: compress_textures( DEFAULT_COMPRESS_TEXTURES ),
  max_tex_size( DEFAULT_MAX_TEX_SIZE ),
  inline_textures( DEFAULT_INLINE_TEXTURES )
{
    //NOP
}

ResourcePackager*
ResourcePackager::clone() const
{
    ResourcePackager* copy = new ResourcePackager();
    copy->setArchive( archive.get() );
    copy->setOutputLocation( output_location );
    copy->setCompressTextures( compress_textures );
    copy->setMaxTextureSize( max_tex_size );
    copy->setInlineTextures( inline_textures );
    return copy;
}

void
ResourcePackager::setArchive( osgDB::Archive* value ) {
    archive = value;
}

void
ResourcePackager::setOutputLocation( const std::string& value ) {
    output_location = value;
}

void
ResourcePackager::setCompressTextures( bool value ) {
    compress_textures = value;
}

bool
ResourcePackager::getCompressTextures() const {
    return compress_textures;
}

void
ResourcePackager::setInlineTextures( bool value ) {
    inline_textures = value;
}

bool
ResourcePackager::getInlineTextures() const {
    return inline_textures;
}

void
ResourcePackager::setMaxTextureSize( unsigned int value ) {
    max_tex_size = value;
}

unsigned int
ResourcePackager::getMaxTextureSize() const {
    return max_tex_size;
}

void
ResourcePackager::rewriteResourceReferences( osg::Node* node )
{
    struct RewriteImageNamesVisitor : public osg::NodeVisitor 
    {
        RewriteImageNamesVisitor( bool _compress_textures, unsigned int _max_tex_size )
            : compress_textures( _compress_textures ),
              max_tex_size( _max_tex_size ),
              osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) 
        {
            osg::notify( osg::INFO ) << "ResourcePackager: rewriting resources references" << std::endl;
        }
        
        //std::string archive_name;
        bool compress_textures;
        unsigned int max_tex_size;
        std::set<osg::Image*> images;

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
            }
            osg::NodeVisitor::apply( geode );
        }

        virtual void apply( osg::ProxyNode& proxy )
        {
            //proxy.setDatabasePath( archive_name );
            std::string name = osgDB::getRealPath( proxy.getFileName( 0 ) );
            std::string simple = osgDB::getSimpleFileName( name );
            proxy.setFileName( 0, simple );
            osg::notify( osg::INFO ) << "  Rewrote " << name << " => " << simple << std::endl;
            osg::NodeVisitor::apply( proxy );
        }

        void rewrite( osg::StateSet* ss )
        {
            for( unsigned int i=0; i<ss->getTextureAttributeList().size(); i++ )
            {
                osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>( ss->getTextureAttribute( i, osg::StateAttribute::TEXTURE ) );
                if ( tex && tex->getImage() )
                {
                    images.insert( tex->getImage() );
                }
            }
        }

        void postProcess()
        {
            for( std::set<osg::Image*>::iterator i = images.begin(); i != images.end(); i++ )
            {
                osg::Image* image = *i;
                std::string name = osgDB::getRealPath( image->getFileName() );

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
                    
                    if ( max_tex_size > 0 )
                    {
                        std::stringstream buf;
                        buf << osgDB::getNameLessExtension( simple ) 
                            << "_" << (int)max_tex_size << "."
                            << osgDB::getLowerCaseFileExtension( simple );
                        simple = buf.str();
                    }

                    if ( compress_textures )
                    {
                        simple = osgDB::getNameLessExtension( simple ) + ".dds";
                    }

                    image->setFileName( simple );
                    osg::notify( osg::INFO ) << "  Rewrote " << name << " => " << simple << std::endl;
                }
            }
        }
    };

    if ( node )
    {
        RewriteImageNamesVisitor v( compress_textures, max_tex_size );
        node->accept( v );
        v.postProcess();
    }
}

void
ResourcePackager::packageResources( ResourceCache* resources, Report* report )
{
    // collect the resources marked as used.
    //ResourceList resources_to_localize = session->getResourcesUsed( true );

    osg::ref_ptr<osgDB::ReaderWriter::Options> local_options = new osgDB::ReaderWriter::Options();

    // The skin textures:
    SkinResources skins = resources->getSkins();
    for( SkinResources::iterator i = skins.begin(); i != skins.end(); i++ )
    {
        osg::ref_ptr<osg::Image> image = ImageUtils::getFirstImage( resources->getStateSet( i->get() ) );
        if ( image.valid() )
        {
            std::string filename = osgDB::getSimpleFileName( image->getFileName() );

            osg::ref_ptr<osg::Image> output_image = image.get();

            //TODO: check: this may be unnecessary if we already rewrote the references in the
            //      scene graph.
            if ( max_tex_size > 0 && max_tex_size < output_image->s() && max_tex_size < output_image->t() )
            {
                int new_s = std::min( (int)max_tex_size, output_image->s() );
                int new_t = std::min( (int)max_tex_size, output_image->t() );
                output_image = ImageUtils::resizeImage( output_image.get(), new_s, new_t );

                //std::stringstream buf;
                //buf << osgDB::getNameLessExtension( filename ) 
                //    << "_" << (int)max_tex_size << "."
                //    << osgDB::getLowerCaseFileExtension( filename );
                //filename = buf.str();
            }

            if ( compress_textures )
            {
                osg::ref_ptr<osg::Image> compressed_image = ImageUtils::convertRGBAtoDDS( output_image.get() );
                if ( compressed_image.valid() )
                {
                    output_image = compressed_image.get();
                    filename = osgDB::getNameLessExtension( filename ) + ".dds";
                    output_image->setFileName( filename );
                }
            }

            if ( inline_textures )
            {
                // replace the stateset's image with the new one:
                ImageUtils::setFirstImage( resources->getStateSet( i->get() ), output_image.get() );
            }

            else // NOT inlining textures
            {
                if ( archive.valid() && archive->fileExists( filename ) )
                {
                    osgDB::ReaderWriter::WriteResult r = archive->writeImage( *(output_image.get()), filename, local_options.get() );
                    if ( r.error() )
                    {
                        std::stringstream msg;
                        msg << "Failed to copy image " << filename << " into the archive";
                        report->warning( msg.str() );
                    }
                }
                else
                {
                    if ( osgDB::fileExists( output_location ) )
                    {
                        if ( !osgDB::writeImageFile( *(output_image.get()), PathUtils::combinePaths( output_location, filename ), local_options.get() ) )
                        {                            
                            std::stringstream msg;
                            msg << "Failed to copy image " << filename << " into the archive";
                            report->warning( msg.str() );
                        }
                    }
                    else
                    {     
                        std::stringstream msg;
                        msg << "Failed to copy image " << filename << ", output location " << output_location << " not found";
                        report->warning( msg.str() );
                    }
                }
            }
        }
    }

    ModelResources models = resources->getExternalReferenceModels();
    for( ModelResources::const_iterator i = models.begin(); i != models.end(); i++ )
    {
        const std::string& model_abs_uri = i->get()->getAbsoluteURI();
        //ModelResource* model = i->get(); //static_cast<ModelResource*>( resource );

        osg::notify(osg::INFO) << "Localizing extref " << model_abs_uri << std::endl;

        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( model_abs_uri );
        if ( node.valid() )
        {
            //TODO: troll the extref model for textures and run those textures though the 
            //      same process as the skins above before saving back out to the package
            //      location.

            std::string filename = osgDB::getSimpleFileName( model_abs_uri );
            if ( archive.valid() )
            {
                osgDB::ReaderWriter::WriteResult r = archive->writeNode( *(node.get()), filename, local_options.get() );
                if ( r.error() )
                { 
                    std::stringstream msg;
                    msg << "Failed to copy model " << filename << " into the archive";
                    report->warning( msg.str() );
                }
            }
            else
            {
                if ( osgDB::fileExists( output_location ) )
                {
                    if ( !osgDB::writeNodeFile( *(node.get()), osgDB::concatPaths( output_location, filename ), local_options.get() ) )
                    {
                        std::stringstream msg;
                        msg << "Failed to copy model " << filename << " into the folder " << output_location;
                        report->warning( msg.str() );
                    }
                }
                else
                {
                    std::stringstream msg;
                    msg << "Failed to localize model.. model " << filename << " into the folder " << output_location;
                    report->warning( msg.str() );
                }
            }
        }
    }
}

bool
ResourcePackager::packageNode( osg::Node* node, const std::string& abs_uri )
{
    //if ( archive.valid() )
    //{
    //    std::string file = osgDB::getSimpleFileName( abs_output_uri );
    //    osgDB::ReaderWriter::WriteResult r = archive->writeNode( *getResultNode(), file );
    //    if ( !r.success() )
    //    {
    //        result = FilterGraphResult::error( "Cell built OK, but failed to write to archive" );
    //    }
    //}
    //else
    //    {
    bool write_ok = false;
    if ( node )
    {
        osgDB::ReaderWriter::Options* global_options = osgDB::Registry::instance()->getOptions();

        osg::ref_ptr<osgDB::ReaderWriter::Options> options = global_options?
            static_cast<osgDB::ReaderWriter::Options*>( global_options->clone( osg::CopyOp::DEEP_COPY_ALL ) ) :
            new osgDB::ReaderWriter::Options();

        if ( StringUtils::endsWith( abs_uri, ".ive" ) )
        {
            std::string option_string = options->getOptionString();
            if ( inline_textures )
                options->setOptionString( StringUtils::replaceIn( option_string, "noTexturesInIVEFile", "" ) );
            else
                options->setOptionString( "noTexturesInIVEFile " + option_string );
        }
            
        write_ok = osgDB::makeDirectoryForFile( abs_uri );
        if ( write_ok )
        {
            write_ok = osgDB::writeNodeFile( *node, abs_uri, options.get() );
        }
    }
    //    }

    return write_ok;
}