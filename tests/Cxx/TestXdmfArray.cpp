#include "XdmfArray.hpp"

int main(int argc, char* argv[])
{

	int values[] = {1, 2, 3, 4};

	//
	// COPIES
	//

	/**
	 * Array stride = 1, Values stride = 1
	 */
	boost::shared_ptr<XdmfArray> array = XdmfArray::New();
	assert(array->getSize() == 0);
	assert(array->getType() == "");
	assert(array->getPrecision() == 0);
	assert(array->getValuesString() == "");
	assert(array->getHDF5Type() == -1);
	assert(array->getValuesPointer() == NULL);
	array->copyValues(0, &values[0], 4, 1, 1);
	assert(array->getSize() == 4);
	assert(array->getType().compare("Int") == 0);
	assert(array->getPrecision() == 4);
	assert(array->getValuesString().compare("1 2 3 4 ") == 0);
	assert(array->getHDF5Type() == H5T_NATIVE_INT);
	const int * const arrayPointer = (const int * const)array->getValuesPointer();
	assert(arrayPointer[0] == 1);
	assert(arrayPointer[1] == 2);
	assert(arrayPointer[2] == 3);
	assert(arrayPointer[3] == 4);
	// Assert we copied values correctly
	boost::shared_ptr<std::vector<int> > storedValues = array->getValues<int>();
	assert((*storedValues)[0] == 1);
	assert((*storedValues)[1] == 2);
	assert((*storedValues)[2] == 3);
	assert((*storedValues)[3] == 4);

	/**
	 * Array stride = 2, Values stride = 1
	 */
	boost::shared_ptr<XdmfArray> array2 = XdmfArray::New();
	array2->copyValues(0, &values[0], 2, 2, 1);
	assert(array2->getSize() == 3);
	assert(array2->getType().compare("Int") == 0);
	assert(array2->getPrecision() == 4);
	assert(array2->getValuesString().compare("1 0 2 ") == 0);
	storedValues = array2->getValues<int>();
	assert((*storedValues)[0] == 1);
	assert((*storedValues)[1] == 0);
	assert((*storedValues)[2] == 2);

	/**
	 * Array stride = 1, Values stride = 2
	 */
	boost::shared_ptr<XdmfArray> array3 = XdmfArray::New();
	array3->copyValues(0, &values[0], 2, 1, 2);
	assert(array3->getSize() == 2);
	assert(array3->getType().compare("Int") == 0);
	assert(array3->getPrecision() == 4);
	assert(array3->getValuesString().compare("1 3 ") == 0);
	storedValues = array3->getValues<int>();
	assert((*storedValues)[0] == 1);
	assert((*storedValues)[1] == 3);

	/**
	 * Array stride = 2, Values stride = 2
	 */
	boost::shared_ptr<XdmfArray> array4 = XdmfArray::New();
	array4->copyValues(0, &values[0], 2, 2, 2);
	assert(array4->getSize() == 3);
	assert(array4->getType().compare("Int") == 0);
	assert(array4->getPrecision() == 4);
	assert(array4->getValuesString().compare("1 0 3 ") == 0);
	storedValues = array4->getValues<int>();
	assert((*storedValues)[0] == 1);
	assert((*storedValues)[1] == 0);
	assert((*storedValues)[2] == 3);

	/**
	 * Copy values from another XdmfArray
	 * Array stride = 1, Values stride = 1
	 */
	boost::shared_ptr<XdmfArray> array5 = XdmfArray::New();
	array5->copyValues(0, array, 1, 3);
	assert(array5->getSize() == 3);
	assert(array5->getType().compare("Int") == 0);
	assert(array5->getPrecision() == 4);
	storedValues = array5->getValues<int>();
	assert(array5->getSize() == 3);
	assert(array->getSize() == 4);

	//
	// SETS
	//

	/**
	 * Simple Set
	 */
	array5->setValues(values, 2, 0);
	assert(array5->getSize() == 2);
	assert(array5->getType().compare("Int") == 0);
	assert(array5->getPrecision() == 4);
	assert(array5->getValuesString().compare("1 2 ") == 0);
	assert(array5->getHDF5Type() == H5T_NATIVE_INT);
	const int * const array5Pointer = (const int * const)array5->getValuesPointer();
	assert(array5Pointer[0] == 1);
	assert(array5Pointer[1] == 2);

	/**
	 * Copy after Set
	 */
	array5->setValues(&values[1], 3, 0);
	assert(array5->getSize() == 3);
	assert(array5->getValuesString().compare("2 3 4 ") == 0);
	int zero = 0;
	array5->copyValues(3, &zero, 1, 1, 0);
	assert(array5->getSize() == 4);
	assert(array5->getValuesString().compare("2 3 4 0 ") == 0);

	/**
	 * Set and Ownership transfer
	 */
	double * doubleValues = new double[3];
	doubleValues[0] = 0;
	doubleValues[1] = 1.1;
	doubleValues[2] = 10.1;
	array5->setValues(doubleValues, 3, 1);
	assert(array5->getSize() == 3);
	assert(array5->getType().compare("Float") == 0);
	assert(array5->getPrecision() == 8);
	assert(array5->getValuesString().compare("0 1.1 10.1 ") == 0);

	//
	// SHARED ASSIGNMENTS
	//

	/**
	 * Shared vector assignment
	 */
	boost::shared_ptr<std::vector<char> > values2(new std::vector<char>());
	values2->push_back(-2);
	values2->push_back(-1);
	values2->push_back(0);
	values2->push_back(1);
	values2->push_back(2);
	boost::shared_ptr<XdmfArray> array6 = XdmfArray::New();
	array6->setValues(values2);
	assert(array6->getSize() == 5);
	assert(array6->getType().compare("Char") == 0);
	assert(array6->getPrecision() == 1);
	assert(array6->getValuesString().compare("-2 -1 0 1 2 ") == 0);
	// Assert we have the same values!
	boost::shared_ptr<std::vector<char> > storedValues2 = array6->getValues<char>();
	int i = 0;
	for(std::vector<char>::const_iterator iter = storedValues2->begin(); iter != storedValues2->end(); ++iter, ++i)
	{
		assert(*iter == values2->operator[](i));
	}
	// Now modify original array
	values2->push_back(8);
	assert(array6->getSize() == 6);
	assert(array6->getValuesString().compare("-2 -1 0 1 2 8 ") == 0);
	// Assert we have the same values!
	i = 0;
	for(std::vector<char>::const_iterator iter = storedValues2->begin(); iter != storedValues2->end(); ++iter, ++i)
	{
		assert(*iter == values2->operator[](i));
	}
	// Assert we can't get an int vector out of our array.
	boost::shared_ptr<std::vector<int> > storedValues2Int = array6->getValues<int>();
	assert(storedValues2Int == NULL);

	//
	// SWAPS
	//

	/**
	 * Swap values from a vector
	 */
	std::vector<short> values3;
	values3.push_back(-1);
	values3.push_back(0);
	values3.push_back(1);
	boost::shared_ptr<XdmfArray> array7 = XdmfArray::New();
	array7->swap(values3);
	assert(values3.size() == 0);
	assert(array7->getSize() == 3);
	assert(array7->getType().compare("Short") == 0);
	assert(array7->getPrecision() == 2);
	boost::shared_ptr<std::vector<short> > storedValues3 = array7->getValues<short>();
	assert((*storedValues3)[0] == -1);
	assert((*storedValues3)[1] == 0);
	assert((*storedValues3)[2] == 1);

	/**
	 * Swap values from a shared vector
	 */
	array7->releaseData();
	array7->swap(values2);
	assert(storedValues2->size() == 0);
	assert(array7->getSize() == 6);
	assert(array7->getType().compare("Char") == 0);
	assert(array7->getPrecision() == 1);

	/**
	 * Swap values from an XdmfArray (with copy)
	 */
	array7->releaseData();
	array7->swap(array4);
	assert(array4->getSize() == 0);
	assert(array7->getSize() == 3);

	/**
	 * Swap values from an XdmfArray (with pointer)
	 */
	array5->setValues(&values[1], 3, 0);
	assert(array5->getSize() == 3);
	array7->releaseData();
	array7->swap(array5);
	assert(array5->getSize() == 0);
	assert(array7->getSize() == 3);

	//
	// Various STL like functions
	//

	/**
	 * Resize
	 */
	boost::shared_ptr<XdmfArray> array8 = XdmfArray::New();
	array8->copyValues(0, &values[0], 4, 1, 1);
	array8->resize(5, 0);
	assert(array8->getValuesString().compare("1 2 3 4 0 ") == 0);
	array8->resize(3, 0);
	assert(array8->getValuesString().compare("1 2 3 ") == 0);
	array8->resize(8, 1.1);
	assert(array8->getValuesString().compare("1 2 3 1 1 1 1 1 ") == 0);

	/**
	 * Reserve / Capacity
	 */
	array8->reserve(50);
	assert(array8->getCapacity() >= 50);

	return 0;
}
