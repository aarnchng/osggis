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
#include <osg/Notify>
#include <osgUtil/Optimizer>
#include <osgUtil/SmoothingVisitor>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgText/Text>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( SubstituteModelFilter );


#define DEFAULT_CLUSTER true


SubstituteModelFilter::SubstituteModelFilter()
{
    setCluster( DEFAULT_CLUSTER );
}


SubstituteModelFilter::~SubstituteModelFilter()
{
    //NOP
}

void
SubstituteModelFilter::setModelScript( Script* value )
{
    model_script = value;
}

Script*
SubstituteModelFilter::getModelScript() const
{
    return model_script.get();
}

void
SubstituteModelFilter::setModelPathScript( Script* value )
{
    model_path_script = value;
}

Script*
SubstituteModelFilter::getModelPathScript() const
{
    return model_path_script.get();
}

void
SubstituteModelFilter::setCluster( bool value )
{
    cluster = value;
}

bool
SubstituteModelFilter::getCluster() const
{
    return cluster;
}

void
SubstituteModelFilter::setModelScaleScript( Script* value )
{
    model_scale_script = value;
}

Script*
SubstituteModelFilter::getModelScaleScript() const
{
    return model_scale_script.get();
}

void
SubstituteModelFilter::setFeatureNameScript( Script* value )
{
    feature_name_script = value;
}

Script*
SubstituteModelFilter::getFeatureNameScript() const
{
    return feature_name_script.get();
}

void
SubstituteModelFilter::setProperty( const Property& p )
{
    if ( p.getName() == "model" )
        setModelScript( new Script( p.getValue() ) );
    else if ( p.getName() == "model_path" )
        setModelPathScript( new Script( p.getValue() ) );
    else if ( p.getName() == "cluster" )
        setCluster( p.getBoolValue( getCluster() ) );
    else if ( p.getName() == "model_scale" )
        setModelScaleScript( new Script( p.getValue() ) );
    else if ( p.getName() == "feature_name" )
        setFeatureNameScript( new Script( p.getValue() ) );
    NodeFilter::setProperty( p );
}


Properties
SubstituteModelFilter::getProperties() const
{
    Properties p = NodeFilter::getProperties();
    if ( getModelScript() )
        p.push_back( Property( "model", getModelScript()->getCode() ) );
    if ( getModelPathScript() )
        p.push_back( Property( "model_path", getModelPathScript()->getCode() ) );
    if ( getCluster() != DEFAULT_CLUSTER )
        p.push_back( Property( "cluster", getCluster() ) );
    if ( getModelScaleScript() )
        p.push_back( Property( "model_scale", getModelScaleScript()->getCode() ) );
    if ( getFeatureNameScript() )
        p.push_back( Property( "feature_name", getFeatureNameScript()->getCode() ) );
    return p;
}


//optimizer:
//  troll the external model for geodes. for each geode, create a geode in the target
//  model. then, for each geometry in that geode, replicate it once for each instance of
//  the model in the feature batch and transform the actual verts to a local offset
//  relative to the tile centroid. Finally, reassemble all the geodes and optimize. 
//  hopefully stateset sharing etc will work out. we may need to strip out LODs too.
osg::Node*
SubstituteModelFilter::materializeAndClusterFeatures( const FeatureList& features, FilterEnv* env, osg::Node* model )
{
    class ClusterVisitor : public osg::NodeVisitor {
    public:
        ClusterVisitor( SubstituteModelFilter* _filter, const FeatureList& _features, FilterEnv* _env ) 
            : filter( _filter ),
              features( _features ),
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

                            // get the scaler if there is one:
                            osg::Vec3 scaler( 1, 1, 1 );
                            if ( filter->getModelScaleScript() )
                            {
                                ScriptResult r = env->getScriptEngine()->run( filter->getModelScaleScript(), j->get(), env );
                                if ( r.isValid() )
                                    scaler = r.asVec3();
                            }

                            for( osg::Vec3Array::iterator k = verts->begin(); k != verts->end(); k++ )
                            {
                                (*k).set( (*k).x() * scaler.x(), (*k).y() * scaler.y(), (*k).z() * scaler.z() );
                                *k += osg::Vec3( c.x(), c.y(), c.z() );
                            }
                        }

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

            // merge the geometry... probably don't need to do this since the optimizer
            // will get it in BuildNodes
            osgUtil::Optimizer opt;
            opt.optimize( &geode, osgUtil::Optimizer::MERGE_GEOMETRY );

            // automatically generate normals.
            // TODO: maybe this should be an option.
            osgUtil::SmoothingVisitor smoother;
            geode.accept( smoother );
            
            osg::NodeVisitor::apply( geode );
        }

    private:
        SubstituteModelFilter* filter;
        const FeatureList& features;
        FilterEnv* env;
    };


    // make a copy of the model:
	osg::Node* clone = dynamic_cast<osg::Node*>( model->clone( osg::CopyOp::DEEP_COPY_ALL ) );

    // ..and apply the clustering to the copy.
	ClusterVisitor cv( this, features, env );
	clone->accept( cv );

	return clone;
}


void
registerTextures( osg::Node* node, Session* session )
{
    class ImageFinder : public osg::NodeVisitor {
    public:
        ImageFinder( Session* _session )
            : session(_session), osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) { }

        void processStateSet( osg::StateSet* ss )
        {
            if ( ss )
            {
                for( unsigned int i=0; i<ss->getTextureAttributeList().size(); i++ )
                {
                    osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>( ss->getTextureAttribute( i, osg::StateAttribute::TEXTURE ) );
                    if ( tex && tex->getImage() )
                    {
                        std::string abs_path = osgDB::findDataFile( tex->getImage()->getFileName() );
                        if ( abs_path.length() > 0 )
                        {
                            SkinResource* skin = new SkinResource();
                            skin->setURI( abs_path );
                            session->getResources()->addResource( skin );
                            session->markResourceUsed( skin );
                            //osg::notify( osg::DEBUG_INFO ) << "..registered substmodel texture " << abs_path << std::endl;
                        }
                    }
                }
            }
        }

        void apply( osg::Node& node )
        {
            processStateSet( node.getStateSet() );
            osg::NodeVisitor::apply( node ); // up the tree
        }

        void apply( osg::Geode& geode )
        {
            for( unsigned int i=0; i<geode.getNumDrawables(); i++ )
            {
                osg::Drawable* d = geode.getDrawable( i );
                processStateSet( d->getStateSet() );
            }
            osg::NodeVisitor::apply( geode );
        }

        Session* session;
    };

    ImageFinder image_finder( session );
    node->accept( image_finder );
}


osg::NodeList
SubstituteModelFilter::process( FeatureList& input, FilterEnv* env )
{
    osg::NodeList output;

    if ( input.size() > 0 && getCluster() && !getFeatureNameScript() )
    {
        // There is a bug, or an order-of-ops problem, with "FLATTEN" that causes grid
        // cell features to be improperly offset...especially with SubstituteModelFilter
        // TODO: investigate this later.
        env->getOptimizerHints().exclude( osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS );

        if ( getModelScript() )
        {
            ScriptResult r = env->getScriptEngine()->run( getModelScript(), env );
            if ( r.isValid() )
            {
                ModelResource* model = env->getSession()->getResources()->getModel( r.asString() );
                if ( model )
                {
                    osg::Node* node = env->getSession()->getResources()->getNode( model );
                    output.push_back( materializeAndClusterFeatures( input, env, node ) );
                }
            }
        }
        else if ( getModelPathScript() )
        {
            ScriptResult r = env->getScriptEngine()->run( getModelPathScript(), env );
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
    
    // register textures for localization.
    for( osg::NodeList::iterator i = output.begin(); i != output.end(); i++ )
    {
        registerTextures( i->get(), env->getSession() );
    }

    return output;
}

void
SubstituteModelFilter::assignFeatureName( osg::Node* node, Feature* input, FilterEnv* env )
{
    if ( getFeatureNameScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getFeatureNameScript(), input, env );
        if ( r.isValid() )
            node->setName( r.asString() );
    }
}

osg::NodeList
SubstituteModelFilter::process( Feature* input, FilterEnv* env )
{
    osg::NodeList output;

    if ( getModelScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getModelScript(), input, env );
        if ( r.isValid() )
        {
            ModelResource* model = env->getSession()->getResources()->getModel( r.asString() );
            if ( model )
            {
                osg::Node* node = env->getSession()->getResources()->getNode( model );
                if ( node )
                {
                    osg::MatrixTransform* xform = new osg::MatrixTransform(
                        osg::Matrix::translate( input->getExtent().getCentroid() ) );
                    xform->addChild( node );
                    xform->setDataVariance( osg::Object::STATIC );
                    assignFeatureName( xform, input, env );
                    output.push_back( xform );
                    env->getSession()->markResourceUsed( model );
                }
            }
        }
    }
    else if ( getModelPathScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getModelPathScript(), input, env );
        if ( r.isValid() )
        {
            // create a new resource on the fly..
            ModelResource* model = new ModelResource();
            model->setURI( r.asString() );
            model->setName( r.asString() );
            env->getSession()->getResources()->addResource( model );
            osg::Node* node = env->getSession()->getResources()->getNode( model );
//            osg::Node* node = env->getSession()->getResources()->getProxyNode( model );
            if ( node )
            {
                osg::MatrixTransform* xform = new osg::MatrixTransform(
                    osg::Matrix::translate( input->getExtent().getCentroid() ) );
                xform->addChild( node );
                xform->setDataVariance( osg::Object::STATIC );
                assignFeatureName( xform, input, env );
                output.push_back( xform );
            }
            env->getSession()->markResourceUsed( model );
        }
    }

    return output;
}

