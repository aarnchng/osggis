/**
/* osgGIS - GIS Library for OpenSceneGraph
 * Copyright 2007-2008 Glenn Waldron and Pelican Ventures, Inc.
 * http://osggis.org
 *
 * osgGIS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * osggis_simple - Simple vector layer compiler
 *
 * This simple command-line utility compiles a GIS vector layer into geometry.
 */

#include <osgGISProjects/Project>
#include <osgGISProjects/XmlSerializer>

#include <iostream>

int
main(int argc, char* argv[])
{
//	for (int i =0 ; i < argc ; ++i )
//		std::cout << "argv["<<i<<"] = " << argv[i] << std::endl;

	if(argc !=3)
	{
		std::cout << "usage " << argv[0] << " input-project.xml output-project.xml" << std::endl;
		return 1;
	}

	osg::ref_ptr<osgGISProjects::Project> project = osgGISProjects::XmlSerializer::loadProject( argv[1]  );
	if ( !project.valid() )
	{
		std::cout << "Cannot load project file " << argv[1] << std::endl;
		return 2;
	}

	return osgGISProjects::XmlSerializer::writeProject(project.get(),argv[2]) ? 0 : 3;
}

