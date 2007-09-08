@echo off
set _=%~dp0%
set OSGROOT=G:\devel\osg\osg2.0.0
set GDALROOT=G:\devel\gdal-1.4.2
set PATH=%_%\bin;%OSGROOT%\bin;%GDALROOT%\bin;%PATH%
osgGIS.sln