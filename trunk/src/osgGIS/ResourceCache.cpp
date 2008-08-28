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

#include <osgGIS/ResourceCache>
#include <osgUtil/Optimizer>

using namespace osgGIS;

ResourceCache::ResourceCache()
{
    //NOP
}

osg::StateSet*
ResourceCache::getStateSet( SkinResource* skin )
{
    osg::StateSet* result = NULL;
    if ( skin )
    {
        SkinStateSets::iterator i = findSkin( skin );
        if ( i == skin_state_sets.end() )
        {
            result = skin->createStateSet();
            skin_state_sets.push_back( SkinStateSet( skin, result ) );
            //skin_state_sets[skin] = result;
        }
        else if ( !i->second.valid() ) // null state set..
        {
            result = skin->createStateSet();
            i->second = result;
        }
        else
        {
            result = i->second.get();
        }
    }
    return result;
}

osg::Node*
ResourceCache::getNode( ModelResource* model, bool optimize )
{
    osg::Node* result = NULL;
    if ( model )
    {
        ModelNodes::iterator i = findModel( model );
        if ( i == model_nodes.end() )
        {
            bool simplify_extrefs = true; //TODO
            result = model->createNode();
            if ( result )
            {
                if ( optimize )
                {
                    //GeomUtils::setDataVarianceRecursively( result, osg::Object::STATIC );
                    osgUtil::Optimizer o;
                    o.optimize( result );
                }
                model_nodes.push_back( ModelNode( model, result ) );
                //model_nodes[model->getAbsoluteURI()] = result;

                // prevent optimization later when the object might be shared!
                //result->setDataVariance( osg::Object::DYNAMIC ); //gw 7/8/08
            }
        }
        else
        {
            result = i->second.get();
        }
    }
    return result;
}


osg::Node*
ResourceCache::getExternalReferenceNode( ModelResource* model )
{
    osg::Node* result = NULL;
    if ( model )
    {
        ModelNodes::iterator i = findProxyModel( model );
        if ( i == model_proxy_nodes.end() )
        {
            result = model->createProxyNode();
            model_proxy_nodes.push_back( ModelNode( model, result ) );
            //model_nodes[model->getAbsoluteURI()] = result;
        }
        else
        {
            result = i->second.get();
        }
    }
    return result;
}

//void
//ResourceCache::addSkin( osg::Image* image )
//{
//    osg::ref_ptr<SkinResource> skin = new SkinResource();
//    skin_state_sets.push_back( SkinStateSet( skin, skin->createStateSet( image ) ) );
//}

SkinResource*
ResourceCache::addSkin( osg::StateSet* state_set )
{
    SkinResource* skin = new SkinResource();
    skin_state_sets.push_back( SkinStateSet( skin, state_set ) );
    return skin;
}

ResourceCache::SkinStateSets::iterator
ResourceCache::findSkin( SkinResource* skin )
{
    for( SkinStateSets::iterator i = skin_state_sets.begin(); i != skin_state_sets.end(); i++ )
    {
        if ( i->first.get() == skin )
        {
            return i;
        }
    }
    return skin_state_sets.end();
}

ResourceCache::ModelNodes::iterator
ResourceCache::findModel( ModelResource* model )
{
    for( ModelNodes::iterator i = model_nodes.begin(); i != model_nodes.end(); i++ )
    {
        if ( i->first.get() == model )
        {
            return i;
        }
    }
    return model_nodes.end();
}

ResourceCache::ModelNodes::iterator 
ResourceCache::findProxyModel( ModelResource* model )
{
    for( ModelNodes::iterator i = model_proxy_nodes.begin(); i != model_proxy_nodes.end(); i++ )
    {
        if ( i->first.get() == model )
        {
            return i;
        }
    }
    return model_proxy_nodes.end();
}


SkinResources 
ResourceCache::getSkins()
{
    SkinResources results;
    for( SkinStateSets::iterator i = skin_state_sets.begin(); i != skin_state_sets.end(); i++ )
    {
        results.push_back( i->first.get() );
    }
    return results;
}

ModelResources 
ResourceCache::getModels()
{
    ModelResources results;
    for( ModelNodes::iterator i = model_nodes.begin(); i != model_nodes.end(); i++ )
    {
        results.push_back( i->first.get() );
    }
    return results;
}

ModelResources
ResourceCache::getExternalReferenceModels()
{
    ModelResources results;
    for( ModelNodes::iterator i = model_proxy_nodes.begin(); i != model_proxy_nodes.end(); i++ )
    {
        results.push_back( i->first.get() );
    }
    return results;
}