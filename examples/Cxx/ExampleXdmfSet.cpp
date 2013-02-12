#include "XdmfSet.hpp"
#include "XdmfSetType.hpp"

int main(int, char **)
{
	shared_ptr<XdmfSet> exampleSet = XdmfSet::New();

	//Assuming that exampleSet is a shared pointer to an XdmfSet object

	std::string newName = "New Name";
	exampleSet->setName(newName);

	exampleSet->setType(XdmfSetType::Node());

	std::string exampleName = exampleSet->getName();

	shared_ptr<const XdmfSetType> exampleType = exampleSet->getType();

	if (exampleType == XdmfSetType::Node())
	{
		//Do whatever is to be done if the set is a node
	}

	return 0;
}
