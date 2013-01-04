from Xdmf import *
from numpy import *

if __name__ == "__main__":
	exampleArray = XdmfArray.New()

	#Assuming that exampleArray is a shared pointer to an XdmfArray object

	newName = "New Name"
	exampleArray.setName(newName)

	newSize = 10
	exampleArray.reserve(newSize)
	exampleArray.initialize(XdmfArrayType.Int32(), newSize)
	newSizeVector = UInt32Vector()
	newSizeVector.push_back(5)
	newSizeVector.push_back(5)
	newSizeVector.push_back(5)
	exampleArray.initialize(XdmfArrayType.Int32(), newSizeVector)

	exampleType = exampleArray.getArrayType()
	exampleCapacity = exampleArray.getCapacity()
	exampleDimensions = exampleArray.getDimensions()
	exampleDimensionString = exampleArray.getDimensionsString()
	exampleName = exampleArray.getName()
	exampleSize = exampleArray.getSize()

	initArray = [0,1,2,3,4,5,6,7,8,9]
	storeArray = XdmfArray.New()
	exampleArray.insertAsInt32(0, initArray)
	storeArray.insert(0, exampleArray, 0, 10, 1, 1)
	#storeArray now contains {0,1,2,3,4,5,6,7,8,9}
	storeArray.insert(0, exampleArray, 0, 5, 2, 1)
	#storeArray now contains {0,1,1,3,2,5,3,7,4,9}
	storeArray.insert(0, exampleArray, 0, 5, 1, 2)
	#storeArray now contains {0,2,4,6,8,5,3,7,4,9}
	exampleArray.insertAsInt32(0, initArray)
	#exampleArray now contains {0,1,2,3,4,5,6,7,8,9}
	exampleArray.insertAsInt32(0, initArray[0:5:2])
	#exampleArray now contains {0,2,4,6,8,5,3,7,4,9}
	exampleArray.insertAsInt32(0, initArray[::-1])
	#exampleArray now contains {9,8,7,6,5,4,3,2,1,0}

	outputArray = exampleArray.getNumpyArray()

	#Python uses a different function for each data type
	#This example uses insertAsInt32 to insert ints
	#insertAsFloat64 can also be used to insert doubles
	#This function takes a start index and a list
	#Sublists are inserted using Python's sublist notation

	newSize = 20
	baseValue = 1
	exampleArray.resizeAsInt32(newSize, baseValue)
	#exampleArray now contains {0,1,2,3,4,5,6,7,8,9,1,1,1,1,1,1,1,1,1,1}
	newSize = 5
	exampleArray.resizeAsInt32(newSize, baseValue)
	#exampleArray now contains {0,1,2,3,4}
	#This example uses resizeAsInt32 because the baseValue inserted is to be an integer
	#All other supported data types have similarly named function calls
	#For example to insert a double resizeAsFloat64 is called

        newSizeArray = UInt32Vector()
	newSizeArray.push_back(4)
	newSizeArray.push_back(5)
        baseValue = 1
        exampleArray.resizeAsInt32(newSizeArray, baseValue)
        #exampleArray now contains {0,1,2,3,4,5,6,7,8,9,1,1,1,1,1,1,1,1,1,1}
	newSizeArray[0] = 1
        exampleArray.resizeAsInt32(newSizeArray, baseValue)
	#exampleArray now contains {0,1,2,3,4}
	#This example uses resizeAsInt32 because the baseValue inserted is to be an integer
	#All other supported data types have similarly named function calls
	#For example to insert a double resizeAsFloat64 is called

	#If exampleArray contains  [0, 1, 2, 3, 4, 5, 6, 7]
	exampleValue = exampleArray.getValueAsInt32(4)
	#exampleValue now equals 4
	#The data type of the returned value can be changed by changing the function name
	#getValueAsInt32 returns an int value while getValueAsFloat64 returns a double value
	#Variations of this function exist for all supported data types

	exampleValueString = exampleArray.getValuesString()
	valueArray = [float(piece) for piece in exampleArray.getValuesString().split()]
	#This is one method of getting the contained values of the array

	if not(exampleArray.isInitialized()):
		exampleArray.read()

	exampleValues = exampleArray.getValuesInternal()
	#alternatively getBuffer gives a buffer object
	exampleValues = exampleArray.getBuffer()
	#due to the way python handles void pointers, this function is only useful for getting a pointer to pass
	#if the retrieval of the internal values of the array is required, another function should be used

	swapArray = XdmfArray.New()
	initArray2 = [1,2,3,4,5]
	swapArray.insertAsInt32(0, initArray2)
	#exampleArray contains {0,1,2,3,4,5,6,7,8,9} and swapArray contains {1,2,3,4,5}
	exampleArray.swap(swapArray)
	#Now exampleArray contains {1,2,3,4,5} and swapArray contains {0,1,2,3,4,5,6,7,8,9}

	newValue = 5
	exampleArray.pushBackAsInt32(newValue)
	#For Python pushBack has multiple functions to cover different data types
	#This case used an int so the function was pushBackAsInt32
	#Another example would be to use pushBackAsFloat64 for double values

	newIndex = 0
	newValue = 3.5
	exampleArray.insertAsFloat64(newIndex, newValue)
	#this example uses insertAsFloat64 to insert a double value
	#versions for all other data types exist
	#for example insertAsInt32 inserts as an int

	#If exampleArray contains  [0, 1, 2, 3, 4, 5, 6, 7]
	erasedIndex = 4
	exampleArray.erase(erasedIndex)
	#exampleArray now contains the following
	#[0, 1, 2, 3, 5, 6, 7]

	exampleArray.clear()




	exampleController = exampleArray.getHeavyDataController()

	newArray = XdmfArray.New()
	newArray.setHeavyDataController(exampleController)

	exampleArray.release()
