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

#include <osgGIS/SubstituteModelFilter>
#include <osg/MatrixTransform>
#include <osg/NodeVisitor>
#include <osg/Drawable>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgText/Text>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( SubstituteModelFilter );



SubstituteModelFilter::SubstituteModelFilter()
{
    //NOP
}


SubstituteModelFilter::~SubstituteModelFilter()
{
    //NOP
}

void
SubstituteModelFilter::setModelExpr( const std::string& value )
{
    model_expr = value;
}

const std::string&
SubstituteModelFilter::getModelExpr() const
{
    return model_expr;
}

void
SubstituteModelFilter::setProperty( const Property& p )
{
    if ( p.getName() == "model" )
        setModelExpr( p.getValue() );
    NodeFilter::setProperty( p );
}


Properties
SubstituteModelFilter::getProperties() const
{
    Properties p = NodeFilter::getProperties();
    if ( getModelExpr().length() > 0 )
        p.push_back( Property( "model", getModelExpr() ) );
    return p;
}


//optimizer:
//  troll the external model for geodes. for each geode, create a geode in the target
//  model. then, for each geometry in that geode, replicate it once for each instance of
//  the model in the feature batch and transform the actual verts to a local offset
//  relative to the tile centroid. Finally, reassemble all the geodes and optimize. 
//  hopefully stateset sharing etc will work out. we may need to strip out LODs too.
osg::Node*
optimizeCluster( const FeatureList& features, osg::Node* model )
{
    class ClusterVisitor : public osg::NodeVisitor {
    public:
        ClusterVisitor( const FeatureList& _features ) 
            : features( _features ),
              osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) {
        }

        void apply( osg::Geode& geode )
        {
            DrawableList old_drawables = geode.getDrawableList();
            geode.removeDrawables( 0, geode.getNumDrawables() );

			for( DrawableList::iterator i = old_drawables.begin(); i != old_drawables.end(); i++ )
			{
				osg::Drawable* old_d = i->get();
				for( FeatureList::const_iterator j = features.begin(); j != features.end(); j++ )
				{
					osg::Vec3d c = j->get()->getExtent().getCentroid();
					osg::Drawable* new_d = dynamic_cast<osg::Drawable*>( old_d->clone( osg::CopyOp::DEEP_COPY_ALL ) );

					if ( dynamic_cast<osg::Geometry*>( new_d ) )
					{
						osg::Geometry* geom = static_cast<osg::Geometry*>( new_d );
						osg::Vec3Array* verts = dynamic_cast<osg::Vec3Array*>( geom->getVertexArray() );
						if ( verts )
						{
							for( osg::Vec3Array::iterator k = verts->begin(); k != verts->end(); k++ )
							{
								//todo
							}
						}
					}
					else if ( dynamic_cast<osgText::Text*>( new_d ) )
					{
						osgText::Text* text = static_cast<osgText::Text*>( new_d );
					}
				}
			}

            geode.dirtyBound();
        }

    private:
        const FeatureList& features;
    };

	osg::Node* clone = dynamic_cast<osg::Node*>( model->clone( osg::CopyOp::DEEP_COPY_ALL ) );
	ClusterVisitor cv( features );
	clone->accept( cv );
	return clone;
}


osg::NodeList
SubstituteModelFilter::process( FeatureList& input, FilterEnv* env )
{
    osg::NodeList output;

    if ( input.size() > 0 )
    {
        if ( getModelExpr().length() > 0 )
        {
            ScriptResult r = env->getScriptEngine()->run( new Script( getModelExpr() ), env );
            if ( r.isValid() )
            {
                ModelResource* model = env->getSession()->getResources().getModel( r.asString() );
                if ( model )
                {
                    osg::Node* node = env->getSession()->getResources().getNode( model );
                    output.push_back( optimizeCluster( input, node ) );
                }
            }
        }
    }
    else
    {
        output = NodeFilter::process( input, env );
    }
    
    return output;
}


osg::NodeList
SubstituteModelFilter::process( Feature* input, FilterEnv* env )
{
    osg::NodeList output;

    if ( getModelExpr().length() > 0 )
    {
        ScriptResult r = env->getScriptEngine()->run( new Script( getModelExpr() ), input, env );
        if ( r.isValid() )
        {
            ModelResource* model = env->getSession()->getResources().getModel( r.asString() );
            if ( model )
            {
                osg::Node* node = env->getSession()->getResources().getNode( model );
                if ( node )
                {
                    osg::MatrixTransform* xform = new osg::MatrixTransform(
                        osg::Matrix::translate( input->getExtent().getCentroid() ) );
                    xform->addChild( node );
                    output.push_back( xform );
                    env->getSession()->markResourceUsed( model );
                }
            }
        }
    }

    return output;
}

