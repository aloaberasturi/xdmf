#include "XdmfSet.hpp"
#include "XdmfSetType.hpp"

int main(int argc, char* argv[])
{
	// Test != and == operators
	assert(XdmfSetType::Node() == XdmfSetType::Node());
	assert(XdmfSetType::Node() == XdmfSetType::Cell() == false);
	assert(XdmfSetType::Node() != XdmfSetType::Node() == false);
	assert(XdmfSetType::Node() != XdmfSetType::Cell());

	boost::shared_ptr<XdmfSet> set = XdmfSet::New();
	assert(set->getSetType() == XdmfSetType::NoSetType());
	set->setSetType(XdmfSetType::Node());
	assert(set->getSetType() == XdmfSetType::Node());
	set->setName("TEST");
	assert(set->getName().compare("TEST") == 0);

	return 0;
}
