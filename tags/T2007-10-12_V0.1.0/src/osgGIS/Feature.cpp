#include <osgGIS/Feature>

using namespace osgGIS;

Feature::Feature( FeatureOID _oid, GeoShape* _shape )
: oid( _oid ),
  shape( _shape )
{
}


Feature::~Feature()
{
	//NOP
}


FeatureOID
Feature::getOID() const
{
	return oid;
}


GeoShape*
Feature::getShape() const
{
	return shape;
}