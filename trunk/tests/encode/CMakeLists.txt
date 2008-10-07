SET(TARGET_SRC main.cpp )
SET(TARGET_ADDED_LIBRARIES osgGIS osgGISProjects)
#### end var setup  ###
SETUP_TEST_APPLICATION(osggis_test_encode ${CMAKE_HOME_DIRECTORY}/data/project.xml out.xml)
