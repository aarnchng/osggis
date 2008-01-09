# Locate TOLUA
# This module defines
# TOLUA_LIBRARY
# TOLUA_FOUND, if false, do not try to link to gdal 
# TOLUA_INCLUDE_DIR, where to find the headers
#
# Created by Glenn Waldron. 

FIND_PATH(TOLUA_INCLUDE_DIR tolua.h
    $ENV{TOLUA_DIR}/include
    $ENV{TOLUA_DIR}/include
    $ENV{TOLUA_DIR}
    $ENV{OSGDIR}/include
    $ENV{OSGDIR}
    $ENV{OSG_ROOT}/include
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]/include
    /usr/freeware/include
    /devel
)

FIND_LIBRARY(TOLUA_LIBRARY 
    NAMES libtolua51
    PATHS
    $ENV{TOLUA_DIR}/lib
    $ENV{TOLUA_DIR}/lib
    $ENV{TOLUA_DIR}
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

SET(TOLUA_FOUND "NO")
IF(TOLUA_LIBRARY AND TOLUA_INCLUDE_DIR)
    SET(TOLUA_FOUND "YES")
ENDIF(TOLUA_LIBRARY AND TOLUA_INCLUDE_DIR)


