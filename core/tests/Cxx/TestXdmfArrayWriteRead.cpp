#include "XdmfArray.hpp"
#include "XdmfHDF5Writer.hpp"

int main(int argc, char* argv[])
{
	int values[] = {1, 2, 3, 4};

	boost::shared_ptr<XdmfArray> array = XdmfArray::New();
	array->copyValues(0, &values[0], 4, 1, 1);
	assert(array->size() == 4);
	assert(array->getValuesString().compare("1 2 3 4 ") == 0);

	boost::shared_ptr<XdmfHDF5Writer> writer = XdmfHDF5Writer::New("test.h5");
	array->accept(writer);

	assert(array->size() == 4);
	assert(array->getValuesString().compare("1 2 3 4 ") == 0);

	array->release();
	assert(array->getValuesString() == "");
	assert(array->size() == 4);

	array->read();
	assert(array->getValuesString().compare("1 2 3 4 ") == 0);
}
