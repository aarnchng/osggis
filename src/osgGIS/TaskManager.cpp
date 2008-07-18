#include <osgGIS/TaskManager>
#include <OpenThreads/ScopedLock>
#include <osg/Notify>
#include <osg/Timer>

using namespace osgGIS;

TaskThread::TaskThread( int _id, AutoResetBlock& _ab )
  : id( _id ), activity_block( _ab )
{
    state = STATE_READY;
    startThread();
}

int
TaskThread::getID()
{
    return id;
}

void
TaskThread::dispose()
{
    setState( STATE_EXIT );
    run_block.signal();
}

void
TaskThread::setState( State _state )
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> sl(state_mutex);
    state = _state;
}

TaskThread::State
TaskThread::getState()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> sl(state_mutex);
    return state;
}

void
TaskThread::run()
{
    while( true )
    {
        // wait until the next task arrives:
        run_block.block();

        // if the last task was cleared, start this new one
        if ( getState() == STATE_READY && task.valid() )
        {
            setState( STATE_RUNNING );
            start = osg::Timer::instance()->tick();
            task->run();
            end = osg::Timer::instance()->tick();
            setState( STATE_RESULT_READY );
        }
        
        activity_block.signal();

        // quit the thread if necessary:
        if ( getState() == STATE_EXIT )
            break;
    }
}

void
TaskThread::runTask( Task* _task )
{
    if ( getState() == STATE_READY )
    {
        task = _task;
        run_block.signal();
    }
    else
    {
        osg::notify(osg::FATAL) << "ILLEGAL STATE" << std::endl;
    }
}

double
TaskThread::getResultDuration()
{
    return osg::Timer::instance()->delta_s( start, end );
}

osg::ref_ptr<Task>
TaskThread::getResult()
{
    osg::ref_ptr<Task> result = NULL;
    if ( getState() == STATE_RESULT_READY )
    {
        result = task.get();
        task = NULL;
        setState( STATE_READY );
    }
    else
    {
        osg::notify(osg::FATAL) << "ILLEGAL STATE" << std::endl;
    }
    return result;
}

/* ========================================================================= */

TaskManager::TaskManager()
{
    init( OpenThreads::GetNumberOfProcessors() );
}

TaskManager::TaskManager( int _max_running_tasks )
{
    init( std::max( 0, _max_running_tasks ) );
}

TaskManager::~TaskManager()
{
    for( TaskThreadList::iterator i = threads.begin(); i != threads.end(); i++ )
    {
        (*i)->dispose();
        (*i)->join();
    }
}

void
TaskManager::init( int num_threads )
{   
    multi_threaded = num_threads > 0;

    if ( multi_threaded && osg::Referenced::getThreadSafeReferenceCounting() == false )
    {
        osg::notify(osg::FATAL) 
            << "ERROR: use of the osgGIS Task Manager REQUIRES thread-safe reference counting be enabled"
            << std::endl;

        // throw an exception?
    }

    num_running_tasks = 0;

    for( int i=0; i<num_threads; i++ )
    {
        threads.push_back( new TaskThread( i, activity_block ) );
    }

    if ( multi_threaded )
        osg::notify( osg::NOTICE ) << "Task manager started; threads = " << num_threads << std::endl;
    else        
        osg::notify( osg::NOTICE ) << "Task manager started (single-threaded)" << std::endl;
}

void
TaskManager::queueTask( Task* task )
{
    pending_tasks.push( task );
}

bool
TaskManager::wait( unsigned long timeout_ms )
{
    update();

    if ( completed_tasks.size() > 0 )
        return true;

    if ( !hasMoreTasks() )
        return false;

    if ( multi_threaded )
    {
        if ( timeout_ms > 0L )
            activity_block.block( timeout_ms );
        else
            activity_block.block();

        update();
    }

    return true;
}

bool
TaskManager::hasMoreTasks()
{
    return
        pending_tasks.size()   > 0 ||
        num_running_tasks      > 0 ||
        completed_tasks.size() > 0;
}

osg::ref_ptr<Task>
TaskManager::getNextCompletedTask()
{
    osg::ref_ptr<Task> result;
    if ( completed_tasks.size() > 0 )
    {
        result = completed_tasks.front().get();
        completed_tasks.pop();
    }

    return result;
}

void
TaskManager::update()
{
    if ( multi_threaded )
    {
        for( TaskThreadList::iterator i = threads.begin(); i != threads.end(); i++ )
        {
            TaskThread* thread = *i;

            //osg::notify(osg::ALWAYS) <<
            //    "UPDATE: pending=" << pending_tasks.size() << ", running=" << num_running_tasks << ", completed=" << completed_tasks.size() 
            //    << std::endl;

            // handle any completed tasks:
            if ( thread->getState() == TaskThread::STATE_RESULT_READY )
            {
                double seconds = thread->getResultDuration();
                osg::ref_ptr<Task> task = thread->getResult();
                completed_tasks.push( task.get() );
                num_running_tasks--;
                osg::notify(osg::NOTICE) << thread->getID() << "> " << task->getName() << ": completed, time = " << seconds << "s" << std::endl;
            }

            // dispatch any pending tasks:
            if ( thread->getState() == TaskThread::STATE_READY && pending_tasks.size() > 0 )
            {
                osg::ref_ptr<Task> task = pending_tasks.front().get();
                pending_tasks.pop();
                num_running_tasks++;
                thread->runTask( task.get() );
                osg::notify(osg::NOTICE) << thread->getID() << "> " << task->getName() << ": started" << std::endl;
            }
        }
    }
    else // single-threaded mode:
    {
        // pop and run the next task:
        if ( pending_tasks.size() > 0 )
        {
            osg::ref_ptr<Task> task = pending_tasks.front().get();
            pending_tasks.pop();

            if ( task.valid() )
            {
                osg::notify(osg::NOTICE) << "0> " << task->getName() << ": started" << std::endl;

                osg::Timer_t t0 = osg::Timer::instance()->tick();
                task->run();
                completed_tasks.push( task.get() );
                osg::Timer_t t1 = osg::Timer::instance()->tick();

                double seconds = osg::Timer::instance()->delta_s( t0, t1 );

                osg::notify(osg::NOTICE) << "0> " << task->getName() << ": completed, time = " << seconds << "s" << std::endl;
            }
        }
    }
}
