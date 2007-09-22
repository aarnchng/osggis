
SET(TARGET_DEFAULT_PREFIX "app_")
SET(TARGET_DEFAULT_LABEL_PREFIX "App")

#the old construct SUBDIRS( was substituded by ADD_SUBDIRECTORY that is to be preferred according on CMake docs.
FOREACH( mylibfolder 
	osgGIS
	osgGISProjects
	osggis_build
	osggis_makelayer
	osggis_simple
	osggis_viewer
)
	ADD_SUBDIRECTORY(${mylibfolder})
ENDFOREACH( mylibfolder	)
