@echo off
set _=%~dp0%
set OSGROOT=G:\devel\osg\osg2.0.0
set GDALROOT=G:\devel\gdal-1.4.2
set EXPATROOT=G:\devel\expat-2.0.1
set PATH=%_%\bin;%OSGROOT%\bin;%GDALROOT%\bin;%EXPATROOT%\bin;%PATH%
osgGIS.sln