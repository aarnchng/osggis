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
#include <osgUtil/Optimizer>
#include <osgDB/ReadFile>
#include <osgText/Text>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( SubstituteModelFilter );


#define DEFAULT_MATERIALIZE false


SubstituteModelFilter::SubstituteModelFilter()
{
    materialize = DEFAULT_MATERIALIZE;
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

void SubstituteModelFilter::setModelPathExpr( const std::string& value )
{
    model_path_expr = value;
}

const std::string&
SubstituteModelFilter::getModelPathExpr() const
{
    return model_path_expr;
}

void
SubstituteModelFilter::setMaterialize( bool value )
{
    materialize = value;
}

bool
SubstituteModelFilter::getMaterialize() const
{
    return materialize;
}

void
SubstituteModelFilter::setProperty( const Property& p )
{
    if ( p.getName() == "model" )
        setModelExpr( p.getValue() );
    else if ( p.getName() == "model_path" )
        setModelPathExpr( p.getValue() );
    else if ( p.getName() == "materialize" )
        setMaterialize( p.getBoolValue( getMaterialize() ) );
    NodeFilter::setProperty( p );
}


Properties
SubstituteModelFilter::getProperties() const
{
    Properties p = NodeFilter::getProperties();
    if ( getModelExpr().length() > 0 )
        p.push_back( Property( "model", getModelExpr() ) );
    if ( getModelPathExpr().length() > 0 )
        p.push_back( Property( "model_path", getModelPathExpr() ) );
    if ( getMaterialize() != DEFAULT_MATERIALIZE )
        p.push_back( Property( "materialize", getMaterialize() ) );
    return p;
}


//optimizer:
//  troll the external model for geodes. for each geode, create a geode in the target
//  model. then, for each geometry in that geode, replicate it once for each instance of
//  the model in the feature batch and transform the actual verts to a local offset
//  relative to the tile centroid. Finally, reassemble all the geodes and optimize. 
//  hopefully stateset sharing etc will work out. we may need to strip out LODs too.
osg::Node*
materializeAndClusterFeatures( const FeatureList& features, FilterEnv* env, osg::Node* model )
{
    class ClusterVisitor : public osg::NodeVisitor {
    public:
        ClusterVisitor( const FeatureList& _features, FilterEnv* _env ) 
            : features( _features ),
              env( _env ),
              osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) {
        }

        void apply( osg::Geode& geode )
        {
            // save the geode's drawables..
            DrawableList old_drawables = geode.getDrawableList();

            // ..and clear out the drawables list.
            geode.removeDrawables( 0, geode.getNumDrawables() );

            // foreach each drawable that was originally in the geode...
            for( DrawableList::iterator i = old_drawables.begin(); i != old_drawables.end(); i++ )
            {
                osg::Drawable* old_d = i->get();

                // go through the list of input features...
                for( FeatureList::const_iterator j = features.begin(); j != features.end(); j++ )
                {
                    // ...and clone the source drawable once for each input feature.
                    osg::ref_ptr<osg::Drawable> new_d = dynamic_cast<osg::Drawable*>( 
                        old_d->clone( osg::CopyOp::DEEP_COPY_ARRAYS | osg::CopyOp::DEEP_COPY_PRIMITIVES ) );

                    if ( dynamic_cast<osg::Geometry*>( new_d.get() ) )
                    {
                        osg::Geometry* geom = static_cast<osg::Geometry*>( new_d.get() );
                        osg::Vec3Array* verts = dynamic_cast<osg::Vec3Array*>( geom->getVertexArray() );
                        if ( verts )
                        {
                            // now, forceably offset the new cloned drawable by the input feature's first point.
                            // grab the centroid just in case:
                            osg::Vec3d c = j->get()->getExtent().getCentroid();

                            if ( j->get()->getShapes().size() > 0 )
                                if ( j->get()->getShapes()[0].getParts().size() > 0 )
                                    if ( j->get()->getShapes()[0].getParts()[0].size() > 0 )
                                        c = j->get()->getShapes()[0].getParts()[0][0];

                            for( osg::Vec3Array::iterator k = verts->begin(); k != verts->end(); k++ )
                            {
                                *k += osg::Vec3( c.x(), c.y(), c.z() );
                            }
                        }

                        // TODO: what's the deal with this:
                        geom->setColorArray( NULL );

                        // TODO: and why do this:
                        geom->setDataVariance( osg::Object::DYNAMIC );

                        // add the new cloned, translated drawable back to the geode.
                        geode.addDrawable( geom );
                    }
                    else if ( dynamic_cast<osgText::Text*>( new_d.get() ) )
                    {
                        osgText::Text* text = static_cast<osgText::Text*>( new_d.get() );
                        //TODO
                    }
                }
            }

            geode.dirtyBound();
            geode.setDataVariance( osg::Object::DYNAMIC );

            osgUtil::Optimizer opt;
            opt.optimize( &geode, osgUtil::Optimizer::MERGE_GEOMETRY );
            
            osg::NodeVisitor::apply( geode );
        }

    private:
        const FeatureList& features;
        FilterEnv* env;
    };


    // make a copy of the model:
	osg::Node* clone = dynamic_cast<osg::Node*>( model->clone( osg::CopyOp::DEEP_COPY_ALL ) );

    // ..and apply the clustering to the copy.
	ClusterVisitor cv( features, env );
	clone->accept( cv );

	return clone;
}


osg::NodeList
SubstituteModelFilter::process( FeatureList& input, FilterEnv* env )
{
    osg::NodeList output;

    if ( input.size() > 0 && getMaterialize() )
    {
        // There is a bug, or an order-of-ops problem, with "FLATTEN" that causes grid
        // cell features to be improperly offset...especially with SubstituteModelFilter
        // TODO: investigate this later.
        env->getOptimizerHints().exclude( osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS );

        if ( getModelExpr().length() > 0 )
        {
            ScriptResult r = env->getScriptEngine()->run( new Script( getModelExpr() ), env );
            if ( r.isValid() )
            {
                ModelResource* model = env->getSession()->getResources().getModel( r.asString() );
                if ( model )
                {
                    osg::Node* node = env->getSession()->getResources().getNode( model );
                    output.push_back( materializeAndClusterFeatures( input, env, node ) );
                }
            }
        }
        else if ( getModelPathExpr().length() > 0 )
        {
            ScriptResult r = env->getScriptEngine()->run( new Script( getModelPathExpr() ), env );
            if ( r.isValid() )
            {
                osg::Node* node = osgDB::readNodeFile( r.asString() );
                if ( node )
                {
                    output.push_back( materializeAndClusterFeatures( input, env, node ) );
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
                    xform->setDataVariance( osg::Object::STATIC );
                    output.push_back( xform );
                    env->getSession()->markResourceUsed( model );
                }
            }
        }
    }
    else if ( getModelPathExpr().length() > 0 )
    {
        ScriptResult r = env->getScriptEngine()->run( new Script( getModelPathExpr() ), input, env );
        if ( r.isValid() )
        {
            // create a new resource on the fly..
            ModelResource* model = new ModelResource();
            model->setModelPath( r.asString() );
            model->setName( r.asString() );
            env->getSession()->getResources().addResource( model );
            osg::Node* node = env->getSession()->getResources().getNode( model );
//            osg::Node* node = env->getSession()->getResources().getProxyNode( model );
            if ( node )
            {
                osg::MatrixTransform* xform = new osg::MatrixTransform(
                    osg::Matrix::translate( input->getExtent().getCentroid() ) );
                xform->addChild( node );
                xform->setDataVariance( osg::Object::STATIC );
                output.push_back( xform );
            }
            env->getSession()->markResourceUsed( model );
        }
    }

    return output;
}

