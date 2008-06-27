# Locate gdal
# This module defines
# LUA_LIBRARY
# LUA_FOUND, if false, do not try to link to gdal 
# LUA_INCLUDE_DIR, where to find the headers
#
# Created by Glenn Waldron. 

FIND_PATH(LUA_INCLUDE_DIR NAMES lua.h lauxlib.h lua.hpp 
    PATHS
    $ENV{LUA_DIR}/include
    $ENV{LUA_DIR}/include
    $ENV{LUA_DIR}
    $ENV{OSGDIR}/include
    $ENV{OSGDIR}
    $ENV{OSG_ROOT}/include
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/local/include/lua5.1
    /usr/include
    /usr/include/lua5.1
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]/include
    /usr/freeware/include
    /devel
)

FIND_LIBRARY(LUA_LIBRARY 
    NAMES liblua51 lua5.1
    PATHS
    $ENV{LUA_DIR}/lib
    $ENV{LUA_DIR}/lib
    $ENV{LUA_DIR}
    $ENV{OSGDIR}/lib
    $ENV{OSGDIR}
    $ENV{OSG_ROOT}/lib
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/lib
    /usr/lib
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]/lib
    /usr/freeware/lib64
)

SET(LUA_FOUND "NO")
IF(LUA_LIBRARY AND LUA_INCLUDE_DIR)
    SET(LUA_FOUND "YES")
ENDIF(LUA_LIBRARY AND LUA_INCLUDE_DIR)


