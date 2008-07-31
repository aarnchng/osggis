#include <osgGIS/SmartReadCallback>
#include <osgDB/ReadFile>
#include <osg/BoundingSphere>
#include <osg/MatrixTransform>
#include <osg/Notify>
#include <stdlib.h>

using namespace osgGIS;

SmartReadCallback::SmartReadCallback( int max_lru )
{
    unsigned int size = 500;
    const char* sstr = getenv( "OSGGIS_CACHE_SIZE" );
    if ( sstr ) {
        sscanf( sstr, "%ld", &size );
        if ( size > 2000 )
            size = 2000;
    }
    max_cache_size = size; //max_lru > 0? max_lru : 100;
    mru_tries = 0;
    mru_hits = 0;
}

osg::Node*
SmartReadCallback::readNodeFile( const std::string& filename )
{
    osg::Timer_t now = osg::Timer::instance()->tick();

    NodeRef* n = NULL;
    NodeMap::iterator i = node_map.find( filename );
    if ( i != node_map.end() )
    {
        n = i->second.get();

        // update the timestamp and re-insert
        mru.erase( mru.find( n ) );
        n->timestamp = now;
        mru.insert( n );
    }
    else
    {
        osg::Node* node = osgDB::readNodeFile( filename );
        if ( node )
        {
            n = new NodeRef( filename, node );
            n->timestamp = now;
            node_map[filename] = n;
            mru.insert( n );
        }
    }

    if ( mru.size() >= max_cache_size )
    {
        // remove the oldest item when the cache overflows
        NodeRef* oldest = *(mru.begin());
        node_map.erase( node_map.find( n->name ) );
        mru.erase( mru.begin() );
    }

    return n? n->node.get() : NULL;
}

/*
osg::Node*
SmartReadCallback::readNodeFile( const std::string& filename )
{
    // first check to see if file is already loaded.
    {
        //OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);

        FileNameSceneMap::iterator i = cache.find( filename );
        if ( i != cache.end() )
        {
            //setMruNode( i->second.get() );
            //osg::notify(osg::WARN)<<"Read from cache: " << filename << ", radius=" << mru_world_bs.radius() << std::endl;
            return i->second.get();
        }
    }

    // now load the file.
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( filename );

    // insert into the cache.
    if (node.valid())
    {
        //OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);

        if ( cache.size() < max_cache_size )
        {
            //osg::notify(osg::INFO)<<"Inserting into cache "<<filename<<std::endl;
            cache[filename] = node;
        }
        else
        {
            // for time being implement a crude search for a candidate to chuck out from the cache.
            for( FileNameSceneMap::iterator i = cache.begin(); i != cache.end(); i++ )
            {
                if ( i->second->referenceCount() == 1 )
                {
                    //osg::notify(osg::NOTICE)<<"Erasing "<<i->first<<std::endl;
                    // found a node which is only referenced in the cache so we can disgard it
                    // and know that the actual memory will be released.
                    cache.erase( i );
                    break;
                }
            }
            //osg::notify(osg::INFO)<<"And the replacing with "<<filename<<std::endl;
            cache[filename] = node;
        }

        //setMruNode( node.get() );
        //osg::notify(osg::WARN)<<"Read from disk:  " << filename << ", radius=" << mru_world_bs.radius() << std::endl;
    }

    else
    {
        //osg::notify( osg::WARN ) << "SMART READ CALLBACK: Read(" << filename << ") FAILED!" << std::endl;
    }

    return node.release();
}
*/


osg::Node*
SmartReadCallback::getMruNode()
{
    return mru_node.get();
}


void
SmartReadCallback::setMruNode( osg::Node* node )
{
    if ( node )
    {
        osg::MatrixList mats = node->getWorldMatrices();
        if ( mats.size() > 0 ) {
            osg::MatrixTransform* xform = new osg::MatrixTransform( mats[0] );
            xform->addChild( node );
            mru_node = xform;
        }
        else {
            mru_node = node;
        }
        mru_world_bs = mru_node->getBound();
    }
    else
    {
        mru_node = node;
    }
}


osg::Node*
SmartReadCallback::getMruNodeIfContains( const osg::Vec3d& p, osg::Node* fallback )
{
    osg::Node* result = fallback;
    mru_tries++;
    if ( mru_node.valid() )
    {
        //osg::BoundingSphere bs = mru_world_bs;
        //osg::notify(osg::WARN)
        //    << "Bound = " << mru_world_bs.center().x() << "," 
        //    << mru_world_bs.center().y() << ","
        //    << mru_world_bs.center().z() << "; "
        //    << "rad = " << mru_world_bs.radius() << "; "
        //    << "pt = "
        //    << p.x() << "," << p.y() << "," << p.z() 
        //    << std::endl;

        if ( mru_world_bs.contains( p ) )
        {
            mru_hits++;
            result = mru_node.get();
        }
    }
    //osg::notify(osg::WARN) << "Hitrate: " << getMruHitRatio() << std::endl;
    return result;
}

osg::Node*
SmartReadCallback::getMruNodeIfContains( const osg::Vec3d& p1, const osg::Vec3d& p2, osg::Node* fallback )
{
    osg::Node* result = fallback;
    mru_tries++;
    if ( mru_node.valid() && mru_world_bs.contains( p1 ) && mru_world_bs.contains( p2 ) )
    {
        mru_hits++;
        result = mru_node.get();
    }
    //osg::notify(osg::WARN) << "Hitrate: " << getMruHitRatio() << std::endl;
    return result;
}


float
SmartReadCallback::getMruHitRatio() const
{
    return mru_tries > 0? (float)mru_hits/(float)mru_tries : 0.0f;
}

