
# FIXME: For OS X, need flag for Framework or dylib
IF   (DYNAMIC_OSGGIS)
    ADD_DEFINITIONS(-DOSGGISPROJECTS_LIBRARY -DOSGGISPROJECTS_EXPORTS)
ELSE (DYNAMIC_OSGGIS)
    ADD_DEFINITIONS(-DOSGGISPROJECTS_LIBRARY_STATIC -DOSGGISPROJECTS_EXPORTS_STATIC)
ENDIF(DYNAMIC_OSGGIS)

SET(LIB_NAME osgGISProjects)

SET(LIB_PUBLIC_HEADERS
    Build
    BuildLayer
    BuildLayerSlice
    Common
    Document
    Export
    Project
    Serializer
    Source
    Terrain
    XmlDocument
    XmlDOM
    XmlSerializer
)

ADD_LIBRARY(${LIB_NAME}
    ${OSGGIS_USER_DEFINED_DYNAMIC_OR_STATIC}
    ${LIB_PUBLIC_HEADERS}
    Build.cpp
    BuildLayer.cpp
    BuildLayerSlice.cpp
    Document.cpp
    Project.cpp
    Serializer.cpp
    Source.cpp
    Terrain.cpp
    XmlDocument.cpp
    XmlDOM.cpp
    XmlSerializer.cpp
)

LINK_INTERNAL(${LIB_NAME}
    osgGIS
)
LINK_EXTERNAL(${LIB_NAME} ${CMAKE_THREAD_LIBS_INIT} ${MATH_LIBRARY} )
LINK_WITH_VARIABLES(${LIB_NAME} EXPAT_LIBRARY)
LINK_CORELIB_DEFAULT(${LIB_NAME} ${CMAKE_THREAD_LIBS_INIT} ${MATH_LIBRARY} )

INCLUDE(ModuleInstall OPTIONAL)
