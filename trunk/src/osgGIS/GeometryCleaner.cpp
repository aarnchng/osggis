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

#include <osgGIS/GeometryCleaner>
#include <osg/NodeVisitor>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/TexEnv>

using namespace osgGIS;

GeometryCleaner::GeometryCleaner()
{
    //NOP
}

struct CleaningVisitor : public osg::NodeVisitor 
{
    CleaningVisitor() : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) { }

    void apply( osg::Geode& node )
    {
        for( unsigned int i=0; i<node.getNumDrawables(); i++ )
        {
            osg::Geometry* geom = dynamic_cast<osg::Geometry*>( node.getDrawable( i ) );
            if ( geom )
            {
                apply( *geom );
            }
        }
    }

    void apply( osg::Geometry& geom )
    {
        removeUnusedColors( geom );
        consolidateColors( geom );
        consolidateNormals( geom );
    }

    void removeUnusedColors( osg::Geometry& geom )
    {
        // if the geometry has a DECAL texture, get rid of colors.
        bool has_decal = false;
        osg::StateSet* ss = geom.getStateSet();
        if ( ss )
        {
            for( unsigned int i=0; i<ss->getTextureAttributeList().size(); i++ )
            {
                osg::TexEnv* env = dynamic_cast<osg::TexEnv*>( ss->getTextureAttribute( i, osg::StateAttribute::TEXENV ) );
                if ( env && env->getMode() == osg::TexEnv::DECAL )
                {
                    has_decal = true;
                    break;
                }
            }
        }
        if ( has_decal )
        {
            geom.setColorArray( 0L );
        }
    }

    void consolidateColors( osg::Geometry& geom )
    {
        // If all the colors are the same, convert to BIND_OVERALL.
        if ( geom.getColorArray() && geom.getColorBinding() != osg::Geometry::BIND_OVERALL )
        {
            bool consolidate = true;
            osg::Vec4 c0, c1(1,1,1,1);

            osg::Vec4Array* colors = dynamic_cast<osg::Vec4Array*>( geom.getColorArray() );
            if ( colors )
            {
                for( unsigned int i=0; i<colors->size(); i++ )
                {
                    c1 = (*colors)[i];
                    if ( i > 0 && c1 != c0 )
                    {
                        consolidate = false;
                        break;
                    }
                    c0 = c1;
                }
            }

            if ( consolidate )
            {
                osg::Vec4Array* new_colors = new osg::Vec4Array(1);
                (*new_colors)[0] = c1;
                geom.setColorArray( new_colors );
                geom.setColorBinding( osg::Geometry::BIND_OVERALL );
            }
        }
    }

    void consolidateNormals( osg::Geometry& geom )
    {
        // If all the colors are the same, convert to BIND_OVERALL.
        if ( geom.getNormalArray() && geom.getNormalBinding() != osg::Geometry::BIND_OVERALL )
        {
            bool consolidate = true;
            osg::Vec3 v0, v1(0,0,1);

            osg::Vec3Array* colors = dynamic_cast<osg::Vec3Array*>( geom.getNormalArray() );
            if ( colors )
            {
                for( unsigned int i=0; i<colors->size(); i++ )
                {
                    v1 = (*colors)[i];
                    if ( i > 0 && v1 != v0 )
                    {
                        consolidate = false;
                        break;
                    }
                    v0 = v1;
                }
            }

            if ( consolidate )
            {
                osg::Vec3Array* new_vector = new osg::Vec3Array(1);
                (*new_vector)[0] = v1;
                geom.setNormalArray( new_vector );
                geom.setNormalBinding( osg::Geometry::BIND_OVERALL );
            }
        }
    }
};

void
GeometryCleaner::clean( osg::Node* node )
{
    if ( node )
    {
        CleaningVisitor cv;
        node->accept( cv );
    }
}