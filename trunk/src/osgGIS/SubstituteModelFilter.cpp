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


#define DEFAULT_CLUSTER        false
#define DEFAULT_OPTIMIZE_MODEL false
#define DEFAULT_INLINE_MODEL   true


SubstituteModelFilter::SubstituteModelFilter()
{
    setCluster( DEFAULT_CLUSTER );
    setOptimizeModel( DEFAULT_OPTIMIZE_MODEL );
    setInlineModel( DEFAULT_INLINE_MODEL );
}

SubstituteModelFilter::SubstituteModelFilter( const SubstituteModelFilter& rhs )
: NodeFilter( rhs ),
  cluster( rhs.cluster ),
  optimize_model( rhs.optimize_model ),
  inline_model( rhs.inline_model ),
  model_script( rhs.model_script.get() ),
  model_path_script( rhs.model_path_script.get() ),
  model_scale_script( rhs.model_scale_script.get() ),
  model_heading_script( rhs.model_heading_script.get() ),
  model_max_texture_size_script( rhs.model_max_texture_size_script.get() ),
  feature_name_script( rhs.feature_name_script.get() )
{
    //NOP
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
SubstituteModelFilter::setModelMaxTextureSizeScript( Script* value ) {
    model_max_texture_size_script = value;
}

Script*
SubstituteModelFilter::getModelMaxTextureSizeScript() const {
    return model_max_texture_size_script.get();
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
SubstituteModelFilter::setModelHeadingScript( Script* value )
{
    model_heading_script = value;
}

Script*
SubstituteModelFilter::getModelHeadingScript() const
{
    return model_heading_script.get();
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
SubstituteModelFilter::setOptimizeModel( bool value )
{
    optimize_model = value;
}

bool
SubstituteModelFilter::getOptimizeModel() const
{
    return optimize_model;
}

void
SubstituteModelFilter::setInlineModel( bool value )
{
    inline_model = value;
}

bool
SubstituteModelFilter::getInlineModel() const
{
    return inline_model;
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
    else if ( p.getName() == "model_heading" )
        setModelHeadingScript( new Script( p.getValue() ) );
    else if ( p.getName() == "model_max_texture_size" )
        setModelMaxTextureSizeScript( new Script( p.getValue() ) );
    else if ( p.getName() == "feature_name" )
        setFeatureNameScript( new Script( p.getValue() ) );
    else if ( p.getName() == "optimize_model" )
        setOptimizeModel( p.getBoolValue( getOptimizeModel() ) );
    else if ( p.getName() == "inline_model" )
        setInlineModel( p.getBoolValue( getInlineModel() ) );

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
    if ( getModelHeadingScript() )
        p.push_back( Property( "model_heading", getModelHeadingScript()->getCode() ) );
    if ( getModelMaxTextureSizeScript() )
        p.push_back( Property( "model_max_texture_size", getModelMaxTextureSizeScript()->getCode() ) );
    if ( getFeatureNameScript() )
        p.push_back( Property( "feature_name", getFeatureNameScript()->getCode() ) );
    if ( getOptimizeModel() != DEFAULT_OPTIMIZE_MODEL )
        p.push_back( Property( "optimize_model", getOptimizeModel() ) );
    if ( getInlineModel() != DEFAULT_INLINE_MODEL )
        p.push_back( Property( "inline_model", getInlineModel() ) );

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

                            osg::Matrix translate_mx = osg::Matrix::translate( c );

                            // get the scaler if there is one:
                            //osg::Vec3 scaler( 1, 1, 1 );
                            osg::Matrix scale_mx;
                            if ( filter->getModelScaleScript() )
                            {
                                ScriptResult r = env->getScriptEngine()->run( filter->getModelScaleScript(), j->get(), env );
                                if ( r.isValid() )
                                    scale_mx = osg::Matrix::scale( r.asVec3() );
                                //scaler = r.asVec3();
                            }

                            // and the heading id there is one:
                            osg::Matrix heading_mx;
                            if ( filter->getModelHeadingScript() )
                            {
                                ScriptResult r = env->getScriptEngine()->run( filter->getModelHeadingScript(), j->get(), env );
                                if ( r.isValid() )
                                    heading_mx = osg::Matrix::rotate( osg::DegreesToRadians( r.asDouble(0) ), 0, 0, -1 );
                            }

                            osg::Matrix mx = heading_mx * scale_mx * translate_mx;

                            for( osg::Vec3Array::iterator k = verts->begin(); k != verts->end(); k++ )
                            {
                                (*k).set( *k * mx );
                                //(*k).set( (*k).x() * scaler.x(), (*k).y() * scaler.y(), (*k).z() * scaler.z() );
                                //*k += osg::Vec3( c.x(), c.y(), c.z() );
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


static void
registerTextures( osg::Node* node, ResourceCache* resources, unsigned int max_texture_size )
{
    class ImageFinder : public osg::NodeVisitor {
    public:
        ImageFinder( ResourceCache* _resources, unsigned int _max_texture_size )
            : resources(_resources),
              max_texture_size(_max_texture_size),
              osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) { }

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
                            SkinResource* skin = resources->addSkin( ss );
                            skin->setURI( abs_path );
                            skin->setMaxTextureSize( max_texture_size );
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

        ResourceCache* resources;
        unsigned int max_texture_size;
    };

    ImageFinder image_finder( resources, max_texture_size );
    node->accept( image_finder );
}

void
SubstituteModelFilter::assignFeatureName( osg::Node* node, Feature* input, FilterEnv* env )
{
    if ( getFeatureNameScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getFeatureNameScript(), input, env );
        if ( r.isValid() && r.asString().length() > 0 )
        {
            node->addDescription( r.asString() );

            // there's a bug in the OSG optimizer ... it will still flatten a static xform
            // even if it has descriptions set. This is a workaround for now. TODO: track
            // this down and submit a bugfix to OSG
            env->getOptimizerHints().exclude( osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS );
        }
    }
}


static double 
getLowestZ( Feature* input )
{
    struct LowZFinder : public GeoPartVisitor {
        LowZFinder() : lowest_z(DBL_MAX) { }
        bool visitPart( const GeoPointList& part ) {
            for( GeoPointList::const_iterator i = part.begin(); i != part.end(); i++ ) {
                if ( (*i).z() < lowest_z )
                    lowest_z = (*i).z();
            }
            return true;
        }
        double lowest_z;
    };

    const GeoShapeList& shapes = input->getShapes();
    LowZFinder lzf;
    shapes.accept( lzf );
    return lzf.lowest_z != DBL_MAX? lzf.lowest_z : 0.0;
}


osg::Node*
SubstituteModelFilter::buildOutputNode( osg::Node* model_node, Feature* input, FilterEnv* env )
{
    osg::Matrix heading_mx = osg::Matrix::identity();

    if ( getModelHeadingScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getModelHeadingScript(), input, env );
        if ( r.isValid() ) 
        {
            heading_mx = osg::Matrix::rotate( osg::DegreesToRadians( r.asDouble(0) ), 0, 0, -1 );
        }
    }

    // the sub-point will be the centroid combined with the minimum Z-value in the shape (or 0).
    GeoPoint centroid = input->getExtent().getCentroid();
    centroid.z() = getLowestZ( input );
    centroid.setDim( 3 );

    osg::Matrix translate_mx = osg::Matrix::translate( centroid );

    osg::MatrixTransform* xform = new osg::MatrixTransform( heading_mx * translate_mx );

    xform->addChild( model_node );
    xform->setDataVariance( osg::Object::STATIC );

    // if a feature name was requested, add it now:
    assignFeatureName( xform, input, env );

    return xform;
}

AttributedNodeList
SubstituteModelFilter::process( FeatureList& input, FilterEnv* env )
{
    AttributedNodeList output;

    if ( input.size() > 1 && getCluster() && !getFeatureNameScript() )
    {
        // There is a bug, or an order-of-ops problem, with "FLATTEN" that causes grid
        // cell features to be improperly offset...especially with SubstituteModelFilter
        // TODO: investigate this later.
        env->getOptimizerHints().exclude( osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS );

        bool share_textures = false;

        if ( getModelScript() )
        {
            share_textures = true;

            ScriptResult r = env->getScriptEngine()->run( getModelScript(), env );
            if ( r.isValid() )
            {
                ModelResource* model = env->getSession()->getResources()->getModel( r.asString() );
                if ( model )
                {
                    //osg::Node* node = env->getSession()->getResources()->getNode( model, getOptimizeModel() );
                    osg::Node* node = env->getResourceCache()->getNode( model, getOptimizeModel() );
                    output.push_back( new AttributedNode( materializeAndClusterFeatures( input, env, node ) ) );
                }
            }
        }
        else if ( getModelPathScript() )
        {
            share_textures = false;

            ScriptResult r = env->getScriptEngine()->run( getModelPathScript(), env );
            if ( r.isValid() )
            {
                osg::Node* node = osgDB::readNodeFile( r.asString() );
                if ( node )
                {
                    output.push_back( new AttributedNode( materializeAndClusterFeatures( input, env, node ) ) );
                }
            }
        }

        unsigned int max_texture_size = 0;
        if ( getModelMaxTextureSizeScript() )
        {
            ScriptResult r = env->getScriptEngine()->run( getModelMaxTextureSizeScript(), env );
            if ( r.isValid() )
                max_texture_size = r.asInt( 0 );
        }
            
        // register textures for localization.
        for( AttributedNodeList::iterator i = output.begin(); i != output.end(); i++ )
        {
            registerTextures( (*i)->getNode(), env->getResourceCache(), max_texture_size );
        }
    }
    else
    {
        output = NodeFilter::process( input, env );
    }

    return output;
}


AttributedNodeList
SubstituteModelFilter::process( Feature* input, FilterEnv* env )
{
    AttributedNodeList output;
    
    // disable flattening, since the model will be multi-parented with different xforms.
    env->getOptimizerHints().exclude( osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS );

    // disable tristripping too because it crashes the optimizer for some unknown reason
    env->getOptimizerHints().exclude( osgUtil::Optimizer::TRISTRIP_GEOMETRY );

    // keep the shared nodes shared
    env->getOptimizerHints().exclude( osgUtil::Optimizer::COPY_SHARED_NODES );

    bool share_textures;

    if ( getModelScript() )
    {
        share_textures = true;

        ScriptResult r = env->getScriptEngine()->run( getModelScript(), input, env );
        if ( r.isValid() )
        {
            ModelResource* model = env->getSession()->getResources()->getModel( r.asString() );
            if ( model )
            {
                //osg::Node* node = getNodeFromModelCache( model );
                //if ( !node )
                //    node = cloneAndCacheModelNode( model, env );

                osg::Node* node = getInlineModel()?
                    env->getResourceCache()->getNode( model, getOptimizeModel() ) :
                    env->getResourceCache()->getExternalReferenceNode( model );

                if ( node )
                {
                    osg::Node* output_node = buildOutputNode( node, input, env );
                    output.push_back( new AttributedNode( output_node, input->getAttributes() ) );
                    env->getSession()->markResourceUsed( model ); //dep.
                }
            }
        }
    }
    else if ( getModelPathScript() )
    {
        share_textures = false;

        ScriptResult r = env->getScriptEngine()->run( getModelPathScript(), input, env );
        if ( r.isValid() )
        {
            // make a new one-time model on the fly..
            ModelResource* model = new ModelResource();
            model->setURI( r.asString() );
            model->setName( r.asString() );

            osg::Node* node = getInlineModel()?
                env->getResourceCache()->getNode( model, getOptimizeModel() ) :
                env->getResourceCache()->getExternalReferenceNode( model );

            if ( node )
            {
                osg::Node* output_node = buildOutputNode( node, input, env );
                output.push_back( new AttributedNode( output_node, input->getAttributes() ) );
            }
        }
    }
    
    unsigned int max_texture_size = 0;
    if ( getModelMaxTextureSizeScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getModelMaxTextureSizeScript(), input, env );
        if ( r.isValid() )
            max_texture_size = r.asInt( 0 );
    }
    
    //osgGIS::debug() << "Setting a max texture size hint = " << max_texture_size << std::endl;
                
    // register textures for localization.
    for( AttributedNodeList::iterator i = output.begin(); i != output.end(); i++ )
    {
        registerTextures( (*i)->getNode(), env->getResourceCache(), max_texture_size );
    }

    return output;
}


