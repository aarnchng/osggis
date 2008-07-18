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

using namespace osgGIS;

#define DEFAULT_COMPRESS_TEXTURES false

ResourcePackager::ResourcePackager() 
: compress_textures( DEFAULT_COMPRESS_TEXTURES )
{
    //NOP
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

void
ResourcePackager::rewriteResourceReferences( osg::Node* node )
{
    struct RewriteImageNamesVisitor : public osg::NodeVisitor 
    {
        RewriteImageNamesVisitor( bool _compress_textures )
            : compress_textures( _compress_textures ),
              osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) 
        {
            osg::notify( osg::INFO ) << "ResourcePackager: rewriting resources references" << std::endl;
        }
        
        //std::string archive_name;
        bool compress_textures;

        virtual void apply( osg::Node& node )
        {
            osg::StateSet* ss = node.getStateSet();
            if ( ss ) rewrite( ss );
            traverse( node );
        }

        virtual void apply( osg::Geode& geode )
        {
            for( unsigned int i=0; i<geode.getNumDrawables(); i++ )
            {
                osg::StateSet* ss = geode.getDrawable( i )->getStateSet();
                if ( ss ) rewrite( ss );
            }
            traverse( geode );
        }

        virtual void apply( osg::ProxyNode& proxy )
        {
            //proxy.setDatabasePath( archive_name );
            std::string name = proxy.getFileName( 0 );
            std::string simple = osgDB::getSimpleFileName( name );
            proxy.setFileName( 0, simple );
            //osg::notify( osg::INFO ) << "Rewrote " << name << " as " << simple << std::endl;
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
                        osg::notify( osg::INFO ) << "ResourcePackager: rewrote " << name << " as " << simple << std::endl;
                    }
                }
            }
        }
    };

    if ( node )
    {
        RewriteImageNamesVisitor v( compress_textures );
        node->accept( v );
    }
}

void
ResourcePackager::packageResources( Session* session, Report* report )
{
    // collect the resources marked as used.
    ResourceList resources_to_localize = session->getResourcesUsed( true );

    osg::ref_ptr<osgDB::ReaderWriter::Options> local_options = new osgDB::ReaderWriter::Options();

    for( ResourceList::const_iterator i = resources_to_localize.begin(); i != resources_to_localize.end(); i++ )
    {
        Resource* resource = i->get();

        // attempt to copy each one to the output location:
        if ( dynamic_cast<SkinResource*>( resource ) )
        {
            SkinResource* skin = static_cast<SkinResource*>( resource );

            osg::ref_ptr<osg::Image> image = skin->getImage();
            if ( image.valid() )
            {
                std::string filename = osgDB::getSimpleFileName( image->getFileName() );

                osg::ref_ptr<osg::Image> output_image = image.get();
                if ( compress_textures )
                {
                    output_image = ImageUtils::convertRGBAtoDDS( image.get() );
                    filename = osgDB::getNameLessExtension( filename ) + ".dds";
                    output_image->setFileName( filename );
                }

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

        else if ( dynamic_cast<ModelResource*>( resource ) )
        {
            ModelResource* model = static_cast<ModelResource*>( resource );

            osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( model->getAbsoluteURI() );
            if ( node.valid() )
            {
                std::string filename = osgDB::getSimpleFileName( model->getAbsoluteURI() );
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

        // now remove any single-use (i.e. non-shared) resources (whether we are localizing them or not)
        if ( resource->isSingleUse() )
        {
            session->getResources()->removeResource( resource );
        }
    }
}