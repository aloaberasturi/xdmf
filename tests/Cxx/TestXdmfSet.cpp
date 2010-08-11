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
	assert(set->getType() == XdmfSetType::NoSetType());
	set->setType(XdmfSetType::Node());
	assert(set->getType() == XdmfSetType::Node());
	set->setName("TEST");
	assert(set->getName().compare("TEST") == 0);

	assert(set->isInitialized() == false);
	assert(set->find(0) == set->end());
	set->insert(0);
	assert(set->isInitialized() == true);
	assert(set->find(0) != set->end());

	return 0;
}
