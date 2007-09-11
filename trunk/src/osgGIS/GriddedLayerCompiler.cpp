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
#include <sstream>

using namespace osgGIS;


GriddedLayerCompiler::GriddedLayerCompiler(FeatureLayer* _layer,
                                           Script*       _script,
                                           unsigned int  _num_cols,
                                           unsigned int  _num_rows )
: Compiler( _layer, _script )                                           
{
    num_cols = _num_cols > 1? _num_cols : 1;
    num_rows = _num_rows > 1? _num_rows : 1;
}


GriddedLayerCompiler::~GriddedLayerCompiler()
{
    //NOP
}


osg::Group*
GriddedLayerCompiler::compile( FilterEnv* _env )
{
    osg::Group* output = new osg::Group();

    osg::ref_ptr<FilterEnv> env = _env? _env->clone() : new FilterEnv();

    GeoExtent extent = env->getExtent();
    if ( extent.isInfinite() || !extent.isValid() )
    {
        extent = getFeatureLayer()->getExtent();
        if ( extent.isInfinite() || !extent.isValid() )
        {
            const SpatialReference* srs = getFeatureLayer()->getSRS()->getBasisSRS();

            extent = GeoExtent(
                GeoPoint( -180.0, -90.0, srs ),
                GeoPoint(  180.0,  90.0, srs ) );
        }
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
                GeoExtent sub_extent(
                    GeoPoint( sw.x() + (double)col*dx, sw.y() + (double)row*dy, srs.get() ),
                    GeoPoint( sw.x() + (double)(col+1)*dx, sw.y() + (double)(row+1)*dy, srs.get() ) );
                    
                env->setExtent( sub_extent );
                
                osg::Node* sub_node = Compiler::compile( env.get() );
                if ( sub_node )
                {
                    std::stringstream s;
                    s << "Tile " << row << "x" << col;
                    sub_node->setName( s.str() );
                    output->addChild( sub_node );

                    //osg::notify( osg::ALWAYS )
                    //    << s.str() << sub_extent.toString() << std::endl;
                }
            }
        }
    }

    return output;
}
