
# FIXME: For OS X, need flag for Framework or dylib
IF (DYNAMIC_OSGGIS)
    ADD_DEFINITIONS(-DOSGGIS_LIBRARY -DOSGGIS_EXPORTS)
ELSE (DYNAMIC_OSGGIS)
    ADD_DEFINITIONS(-DOSGGIS_LIBRARY_STATIC -DOSGGIS_EXPORTS_STATIC)
ENDIF(DYNAMIC_OSGGIS)

SET(LIB_NAME osgGIS)

SET(LIB_PUBLIC_HEADERS
    Attribute
    BuildGeomFilter
    BuildNodesFilter
    ChangeShapeTypeFilter
    ClampFilter
    CollectionFilter
    Common
    ConvexHullFilter
    CombineLinesFilter
    Compiler
    CropFilter
    DecimateFilter
    DefaultFeatureStoreFactory
    DensifyFilter
    DrawableFilter
    Ellipsoid
    Export
    ExpressionEvaluator
    ExtrudeGeomFilter
    FadeHelper
    Feature
    FeatureCursorImpl
    FeatureFilter
    FeatureLayer
    FeatureStore
    FeatureStoreFactory
    Filter
    FilterEnv
    GeocentricSpatialReference
    GeoExtent
    GeoPoint
    GeoShape
    GriddedLayerCompiler
    LayerCompiler
    LineSegmentIntersector2
    LocalizeNodesFilter
    NodeFilter
    OGR_Feature
    OGR_FeatureStore
    OGR_SpatialReference
    OGR_SpatialReferenceFactory
    OGR_Utils
    PagedLayerCompiler
    Property
    Registry
    RemoveHolesFilter
    RTree
    RTreeSpatialIndex
    Script
    ScriptContext
    SelectFilter
    SimpleFeature
    SimpleLayerCompiler
    SimpleSpatialIndex
    SmartReadCallback
    SpatialIndex
    SpatialReference
    SpatialReferenceBase
    SpatialReferenceFactory
    StubSpatialIndex
    SuperFeature
    TerrainUtils
    TransformFilter
)

ADD_LIBRARY(${LIB_NAME}
    ${OSGGIS_USER_DEFINED_DYNAMIC_OR_STATIC}
    ${LIB_PUBLIC_HEADERS}
    Attribute.cpp
    BuildGeomFilter.cpp
    BuildNodesFilter.cpp
    ChangeShapeTypeFilter.cpp
    ClampFilter.cpp
    CollectionFilter.cpp
    CombineLinesFilter.cpp
    Compiler.cpp
    ConvexHullFilter.cpp
    CropFilter.cpp
    DecimateFilter.cpp
    DefaultFeatureStoreFactory.cpp
    DensifyFilter.cpp
    DrawableFilter.cpp
    Ellipsoid.cpp
    ExpressionEvaluator.cpp
    ExtrudeGeomFilter.cpp
    FadeHelper.cpp
    FeatureCursorImpl.cpp
    FeatureFilter.cpp
    FeatureLayer.cpp
    Filter.cpp
    FilterEnv.cpp
    GeocentricSpatialReference.cpp
    GeoExtent.cpp
    GeoPoint.cpp
    GeoShape.cpp
    GriddedLayerCompiler.cpp
    LayerCompiler.cpp
    LineSegmentIntersector2.cpp
    LocalizeNodesFilter.cpp
    NodeFilter.cpp
    OGR_Feature.cpp
    OGR_FeatureStore.cpp
    OGR_SpatialReference.cpp
    OGR_SpatialReferenceFactory.cpp
    OGR_Utils.cpp
    PagedLayerCompiler.cpp
    Property.cpp
    Registry.cpp
    RemoveHolesFilter.cpp
    RTreeSpatialIndex.cpp
    Script.cpp
    ScriptContext.cpp
    SelectFilter.cpp
    SimpleFeature.cpp
    SimpleLayerCompiler.cpp
    SimpleSpatialIndex.cpp
    SmartReadCallback.cpp
    SpatialReference.cpp
    TerrainUtils.cpp
    TransformFilter.cpp
)

LINK_EXTERNAL(${LIB_NAME} ${CMAKE_THREAD_LIBS_INIT}    ${MATH_LIBRARY})
LINK_WITH_VARIABLES(${LIB_NAME} OSG_LIBRARY OSGUTIL_LIBRARY OSGDB_LIBRARY OSGFX_LIBRARY OSGTEXT_LIBRARY OPENTHREADS_LIBRARY GDAL_LIBRARY)
LINK_CORELIB_DEFAULT(${LIB_NAME} ${CMAKE_THREAD_LIBS_INIT} ${MATH_LIBRARY} )

INCLUDE(ModuleInstall OPTIONAL)
