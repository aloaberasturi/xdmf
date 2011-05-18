/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfArray.cpp                                                       */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include <boost/tokenizer.hpp>
#include <sstream>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfHeavyDataController.hpp"
#include "XdmfSystemUtils.hpp"
#include "XdmfVisitor.hpp"
#include "XdmfError.hpp"

class XdmfArray::Clear : public boost::static_visitor<void> {
public:

  Clear()
  {
  }

  template<typename T>
  void
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return array->clear();
  }
};

class XdmfArray::Erase : public boost::static_visitor<void> {
public:

  Erase(const unsigned int index) :
    mIndex(index)
  {
  }

  template<typename T>
  void
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    array->erase(array->begin() + mIndex);
  }

private:

  const unsigned int mIndex;
};

class XdmfArray::GetArrayType :
  public boost::static_visitor<shared_ptr<const XdmfArrayType> > {
public:

  GetArrayType()
  {
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const char * const) const
  {
    return XdmfArrayType::Int8();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const short * const) const
  {
    return XdmfArrayType::Int16();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const int * const) const
  {
    return XdmfArrayType::Int32();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const long * const) const
  {
    return XdmfArrayType::Int64();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const float * const) const
  {
    return XdmfArrayType::Float32();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const double * const) const
  {
    return XdmfArrayType::Float64();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const unsigned char * const) const
  {
    return XdmfArrayType::UInt8();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const unsigned short * const) const
  {
    return XdmfArrayType::UInt16();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const unsigned int * const) const
  {
    return XdmfArrayType::UInt32();
  }

  template<typename T>
  shared_ptr<const XdmfArrayType>
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return this->getArrayType(&(array.get()->operator[](0)));
  }

  template<typename T>
  shared_ptr<const XdmfArrayType>
  operator()(const boost::shared_array<const T> & array) const
  {
    return this->getArrayType(array.get());
  }
};

class XdmfArray::GetCapacity : public boost::static_visitor<unsigned int> {
public:

  GetCapacity()
  {
  }

  template<typename T>
  unsigned int
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return array->capacity();
  }
};

class XdmfArray::GetValuesPointer :
  public boost::static_visitor<const void * const> {
public:

  GetValuesPointer()
  {
  }

  template<typename T>
  const void * const
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return &array->operator[](0);
  }

  template<typename T>
  const void * const
  operator()(const boost::shared_array<const T> & array) const
  {
    return array.get();
  }
};

class XdmfArray::GetValuesString : public boost::static_visitor<std::string> {
public:

  GetValuesString() :
    mArrayPointerNumValues(0)
  {
  }

  GetValuesString(const int arrayPointerNumValues) :
    mArrayPointerNumValues(arrayPointerNumValues)
  {
  }

  template<typename T, typename U>
  std::string
  getValuesString(const T * const array,
                  const int numValues) const
  {
    const unsigned int lastIndex = numValues-1;

    if(lastIndex < 0) {
      return "";
    }

    std::stringstream toReturn;
    for(unsigned int i=0; i<lastIndex; ++i) {
      toReturn << (U)array[i] << " ";
    }
    toReturn << (U)array[lastIndex];
    return toReturn.str();
  }

  std::string
  getValuesString(const char * const array,
                  const int numValues) const
  {
    return getValuesString<char, int>(array, numValues);
  }

  std::string
  getValuesString(const unsigned char * const array,
                  const int numValues) const
  {
    return getValuesString<unsigned char, int>(array, numValues);
  }

  template<typename T>
  std::string
  getValuesString(const T * const array,
                  const int numValues) const
  {
    return getValuesString<T, T>(array, numValues);
  }

  template<typename T>
  std::string
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return getValuesString(&(array->operator[](0)), array->size());
  }

  template<typename T>
  std::string
  operator()(const boost::shared_array<const T> & array) const
  {
    return getValuesString(array.get(), mArrayPointerNumValues);
  }

private:

  const unsigned int mArrayPointerNumValues;
};

class XdmfArray::InsertArray : public boost::static_visitor<void> {
public:

  InsertArray(const unsigned int startIndex,
              const unsigned int valuesStartIndex,
              const unsigned int numValues,
              const unsigned int arrayStride,
              const unsigned int valuesStride,
              std::vector<unsigned int> & dimensions) :
    mStartIndex(startIndex),
    mValuesStartIndex(valuesStartIndex),
    mNumValues(numValues),
    mArrayStride(arrayStride),
    mValuesStride(valuesStride),
    mDimensions(dimensions)
  {
  }

  template<typename T, typename U>
  void
  operator()(const shared_ptr<std::vector<T> > & array,
             const shared_ptr<std::vector<U> > & arrayToCopy) const
  {
    unsigned int size = mStartIndex + mNumValues;
    if(mArrayStride > 1) {
      size = mStartIndex + mNumValues * mArrayStride - 1;
    }
    if(array->size() < size) {
      array->resize(size);
      mDimensions.clear();
    }
    for(unsigned int i=0; i<mNumValues; ++i) {
      array->operator[](mStartIndex + i*mArrayStride) =
        (T)arrayToCopy->operator[](mValuesStartIndex + i*mValuesStride);
    }
  }

private:

  const unsigned int mStartIndex;
  const unsigned int mValuesStartIndex;
  const unsigned int mNumValues;
  const unsigned int mArrayStride;
  const unsigned int mValuesStride;
  std::vector<unsigned int> & mDimensions;
};

class XdmfArray::InternalizeArrayPointer : public boost::static_visitor<void> {
public:

  InternalizeArrayPointer(XdmfArray * const array) :
    mArray(array)
  {
  }

  template<typename T>
  void
  operator()(const boost::shared_array<const T> & array) const
  {
    mArray->mHaveArrayPointer = false;
    mArray->insert(0, array.get(), mArray->mArrayPointerNumValues);
    mArray->mArrayPointer = boost::shared_array<const T>();
    mArray->mArrayPointerNumValues = 0;
  }

private:

  XdmfArray * const mArray;
};

class XdmfArray::NewArray : public boost::static_visitor<void> {
public:

  NewArray()
  {
  }

  template<typename T>
  void
  operator()(shared_ptr<std::vector<T> > & array) const
  {
    shared_ptr<std::vector<T> > newArray(new std::vector<T>());
    array = newArray;
  }
};

class XdmfArray::Reserve : public boost::static_visitor<void> {
public:

  Reserve(const unsigned int size):
    mSize(size)
  {
  }

  template<typename T>
  void
  operator()(shared_ptr<std::vector<T> > & array) const
  {
    array->reserve(mSize);
  }

private:

  const unsigned int mSize;
};

class XdmfArray::Size : public boost::static_visitor<unsigned int> {
public:

  Size()
  {
  }

  template<typename T>
  unsigned int
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return array->size();
  }
};

shared_ptr<XdmfArray>
XdmfArray::New()
{
  shared_ptr<XdmfArray> p(new XdmfArray());
  return p;
}

XdmfArray::XdmfArray() :
  mArrayPointerNumValues(0),
  mHaveArray(false),
  mHaveArrayPointer(false),
  mHeavyDataController(shared_ptr<XdmfHeavyDataController>()),
  mName(""),
  mTmpReserveSize(0)
{
}

XdmfArray::~XdmfArray()
{
}

const std::string XdmfArray::ItemTag = "DataItem";

void
XdmfArray::clear()
{
  if(mHaveArrayPointer) {
    internalizeArrayPointer();
  }
  if(mHaveArray) {
    boost::apply_visitor(Clear(), mArray);
    mDimensions.clear();
  }
}

void
XdmfArray::erase(const unsigned int index)
{
  if(mHaveArrayPointer) {
    internalizeArrayPointer();
  }
  if(mHaveArray) {
    boost::apply_visitor(Erase(index), mArray);
    mDimensions.clear();
  }
}

shared_ptr<const XdmfArrayType>
XdmfArray::getArrayType() const
{
  if(mHaveArray) {
    return boost::apply_visitor(GetArrayType(), mArray);
  }
  else if(mHaveArrayPointer) {
    return boost::apply_visitor(GetArrayType(), mArrayPointer);
  }
  else if(mHeavyDataController) {
    return mHeavyDataController->getType();
  }
  return XdmfArrayType::Uninitialized();
}

unsigned int
XdmfArray::getCapacity() const
{
  if(mHaveArray) {
    return boost::apply_visitor(GetCapacity(), mArray);
  }
  return 0;
}

std::vector<unsigned int>
XdmfArray::getDimensions() const
{
  if(mHaveArray) {
    if(mDimensions.size() == 0) {
      const unsigned int size = boost::apply_visitor(Size(), mArray);
      return std::vector<unsigned int>(1, size);
    }
    return mDimensions;
  }
  else if(mHaveArrayPointer) {
    return std::vector<unsigned int>(1, mArrayPointerNumValues);
  }
  else if(mHeavyDataController) {
    return mHeavyDataController->getDimensions();
  }
  return std::vector<unsigned int>(1, 0);
}

std::string
XdmfArray::getDimensionsString() const
{
  const std::vector<unsigned int> & dimensions = this->getDimensions();
  return GetValuesString().getValuesString(&dimensions[0],
                                           dimensions.size());
}

shared_ptr<XdmfHeavyDataController>
XdmfArray::getHeavyDataController()
{
  return boost::const_pointer_cast<XdmfHeavyDataController>
    (static_cast<const XdmfArray &>(*this).getHeavyDataController());
}

shared_ptr<const XdmfHeavyDataController>
XdmfArray::getHeavyDataController() const
{
  return mHeavyDataController;
}

std::map<std::string, std::string>
XdmfArray::getItemProperties() const
{
  std::map<std::string, std::string> arrayProperties;
  if(mHeavyDataController) {
    arrayProperties["Format"] = mHeavyDataController->getName();
  }
  else {
    arrayProperties["Format"] = "XML";
  }
  arrayProperties["Dimensions"] = this->getDimensionsString();
  if(mName.compare("") != 0) {
    arrayProperties["Name"] = mName;
  }
  shared_ptr<const XdmfArrayType> type = this->getArrayType();
  type->getProperties(arrayProperties);
  return arrayProperties;
}

std::string
XdmfArray::getItemTag() const
{
  return ItemTag;
}

std::string
XdmfArray::getName() const
{
  return mName;
}

unsigned int
XdmfArray::getSize() const
{
  if(mHaveArray) {
    return boost::apply_visitor(Size(), mArray);
  }
  else if(mHaveArrayPointer) {
    return mArrayPointerNumValues;
  }
  else if(mHeavyDataController) {
    return mHeavyDataController->getSize();
  }
  return 0;
}

void *
XdmfArray::getValuesInternal()
{
  return const_cast<void *>
    (static_cast<const XdmfArray &>(*this).getValuesInternal());
}

const void *
XdmfArray::getValuesInternal() const
{
  if(mHaveArray) {
    return boost::apply_visitor(GetValuesPointer(), mArray);
  }
  else if(mHaveArrayPointer) {
    return boost::apply_visitor(GetValuesPointer(), mArrayPointer);
  }
  return NULL;
}

std::string
XdmfArray::getValuesString() const
{
  if(mHaveArray) {
    return boost::apply_visitor(GetValuesString(), mArray);
  }
  else if(mHaveArrayPointer) {
    return boost::apply_visitor(GetValuesString(mArrayPointerNumValues),
                                mArrayPointer);
  }
  return "";
}

void
XdmfArray::initialize(const shared_ptr<const XdmfArrayType> arrayType,
                      const unsigned int size)
{
  if(arrayType == XdmfArrayType::Int8()) {
    this->initialize<char>(size);
  }
  else if(arrayType == XdmfArrayType::Int16()) {
    this->initialize<short>(size);
  }
  else if(arrayType == XdmfArrayType::Int32()) {
    this->initialize<int>(size);
  }
  else if(arrayType == XdmfArrayType::Int64()) {
    this->initialize<long>(size);
  }
  else if(arrayType == XdmfArrayType::Float32()) {
    this->initialize<float>(size);
  }
  else if(arrayType == XdmfArrayType::Float64()) {
    this->initialize<double>(size);
  }
  else if(arrayType == XdmfArrayType::UInt8()) {
    this->initialize<unsigned char>(size);
  }
  else if(arrayType == XdmfArrayType::UInt16()) {
    this->initialize<unsigned short>(size);
  }
  else if(arrayType == XdmfArrayType::UInt32()) {
    this->initialize<unsigned int>(size);
  }
  else if(arrayType == XdmfArrayType::Uninitialized()) {
    this->release();
  }
  else {
    XdmfError::message(XdmfError::FATAL, 
                       "Array of unsupported type in XdmfArray::initialize");
  }
}

void
XdmfArray::initialize(const shared_ptr<const XdmfArrayType> arrayType,
                      const std::vector<unsigned int> & dimensions)
{
  mDimensions = dimensions;
  const unsigned int size = std::accumulate(dimensions.begin(),
                                            dimensions.end(),
                                            1,
                                            std::multiplies<unsigned int>());
  return this->initialize(arrayType, size);
}

void
XdmfArray::insert(const unsigned int startIndex,
                  const shared_ptr<const XdmfArray> values,
                  const unsigned int valuesStartIndex,
                  const unsigned int numValues,
                  const unsigned int arrayStride,
                  const unsigned int valuesStride)
{
  if(mHaveArrayPointer) {
    internalizeArrayPointer();
  }
  if(!mHaveArray) {
    // Copy the values variant in order to get the type
    // Only taking smart pointer so no worries about large copies
    mArray = values->mArray;
    // Reinitialize variant array to contain new array with same type.
    boost::apply_visitor(NewArray(), mArray);
    mHaveArray = true;
  }
  boost::apply_visitor(InsertArray(startIndex,
                                   valuesStartIndex,
                                   numValues,
                                   arrayStride,
                                   valuesStride,
                                   mDimensions),
                       mArray,
                       values->mArray);
}

bool
XdmfArray::isInitialized() const
{
  return mHaveArray || mHaveArrayPointer;
}

void
XdmfArray::internalizeArrayPointer()
{
  if(mHaveArrayPointer) {
    boost::apply_visitor(InternalizeArrayPointer(this), mArrayPointer);
  }
}

void
XdmfArray::populateItem(const std::map<std::string, std::string> & itemProperties,
                        std::vector<shared_ptr<XdmfItem> > & childItems,
                        const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  std::string contentVal;
  unsigned int sizeVal = 1;

  const shared_ptr<const XdmfArrayType> arrayType = 
    XdmfArrayType::New(itemProperties);

  std::map<std::string, std::string>::const_iterator content =
    itemProperties.find("Content");
  if(content != itemProperties.end()) {
    contentVal = content->second;
  }
  else {
    XdmfError::message(XdmfError::FATAL, 
                       "'Content' not found in itemProperties in "
                       "XdmfArray::populateItem");
  }

  std::map<std::string, std::string>::const_iterator dimensions =
    itemProperties.find("Dimensions");
  if(dimensions != itemProperties.end()) {
    boost::tokenizer<> tokens(dimensions->second);
    for(boost::tokenizer<>::const_iterator iter = tokens.begin();
        iter != tokens.end();
        ++iter) {
      mDimensions.push_back(atoi((*iter).c_str()));
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, 
                       "'Dimensions' not found in itemProperties in "
                       "XdmfArray::populateItem");
  }

  std::map<std::string, std::string>::const_iterator format =
    itemProperties.find("Format");
  if(format != itemProperties.end()) {
    if(format->second.compare("HDF") == 0) {
      std::map<std::string, std::string>::const_iterator xmlDir =
        itemProperties.find("XMLDir");
      if(xmlDir == itemProperties.end()) {
        XdmfError::message(XdmfError::FATAL, 
                           "'XMLDir' not found in itemProperties in "
                           "XdmfArray::populateItem");
      }
      size_t colonLocation = contentVal.find(":");
      if(colonLocation != std::string::npos) {
        std::string hdf5Path = contentVal.substr(0, colonLocation);
        std::string dataSetPath =
          contentVal.substr(colonLocation + 1,
                            contentVal.size() - colonLocation - 1);
        if(hdf5Path.compare(XdmfSystemUtils::getRealPath(hdf5Path)) != 0) {
          // Dealing with a relative path for hdf5 location
          std::stringstream newHDF5Path;
          newHDF5Path << xmlDir->second << hdf5Path;
          hdf5Path = newHDF5Path.str();
        }
        mHeavyDataController =
          XdmfHDF5Controller::New(hdf5Path,
                                  dataSetPath,
                                  arrayType,
                                  std::vector<unsigned int>(mDimensions.size(),
                                                            0),
                                  std::vector<unsigned int>(mDimensions.size(),
                                                            1),
                                  mDimensions);
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "':' not found in content in "
                           "XdmfArray::populateItem -- double check an HDF5 "
                           "data set is specified for the file");
      }
    }
    else if(format->second.compare("XML") == 0) {
      this->initialize(arrayType,
                       mDimensions);
      unsigned int index = 0;
      boost::char_separator<char> sep(" \t\n");
      boost::tokenizer<boost::char_separator<char> > tokens(contentVal, sep);
      for(boost::tokenizer<boost::char_separator<char> >::const_iterator
            iter = tokens.begin();
          iter != tokens.end();
          ++iter, ++index) {
        this->insert(index, atof((*iter).c_str()));
      }
    }
    else {
      XdmfError::message(XdmfError::FATAL, 
                         "Neither 'HDF' nor 'XML' specified as 'Format' "
                         "in XdmfArray::populateItem");
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, 
                       "'Format' not found in itemProperties in "
                       "XdmfArray::populateItem");
  }

  std::map<std::string, std::string>::const_iterator name =
    itemProperties.find("Name");
  if(name != itemProperties.end()) {
    mName = name->second;
  }
  else {
    mName = "";
  }
}

void
XdmfArray::read()
{
  if(mHeavyDataController) {
    mHeavyDataController->read(this);
  }
}

void
XdmfArray::release()
{
  releaseArray();
  releaseArrayPointer();
}

void
XdmfArray::releaseArray()
{
  shared_ptr<std::vector<char> > emptyArray;
  mArray = emptyArray;
  mHaveArray = false;
  mDimensions.clear();
}

void
XdmfArray::releaseArrayPointer()
{
  boost::shared_array<const char> emptyArrayPointer;
  mArrayPointer = emptyArrayPointer;
  mHaveArrayPointer = false;
}

void
XdmfArray::reserve(const unsigned int size)
{
  if(mHaveArrayPointer) {
    internalizeArrayPointer();
  }
  if(!mHaveArray) {
    mTmpReserveSize = size;
  }
  else {
    boost::apply_visitor(Reserve(size), mArray);
  }
}

void
XdmfArray::setHeavyDataController(const shared_ptr<XdmfHeavyDataController> heavyDataController)
{
  mHeavyDataController = heavyDataController;
}

void
XdmfArray::setName(const std::string & name)
{
  mName = name;
}

void
XdmfArray::swap(const shared_ptr<XdmfArray> array)
{
  ArrayVariant tmpArray = array->mArray;
  ArrayPointerVariant tmpArrayPointer = array->mArrayPointer;
  int tmpArrayPointerNumValues = array->mArrayPointerNumValues;
  bool tmpHaveArray = array->mHaveArray;
  bool tmpHaveArrayPointer = array->mHaveArrayPointer;
  shared_ptr<XdmfHeavyDataController> tmpHeavyDataController =
    array->mHeavyDataController;
  std::vector<unsigned int> tmpDimensions = array->mDimensions;

  array->mArray = mArray;
  array->mArrayPointer = mArrayPointer;
  array->mArrayPointerNumValues = mArrayPointerNumValues;
  array->mHaveArray = mHaveArray;
  array->mHaveArrayPointer = mHaveArrayPointer;
  array->mHeavyDataController = mHeavyDataController;
  array->mDimensions = mDimensions;

  mArray = tmpArray;
  mArrayPointer = tmpArrayPointer;
  mArrayPointerNumValues = tmpArrayPointerNumValues;
  mHaveArray = tmpHaveArray;
  mHaveArrayPointer = tmpHaveArrayPointer;
  mHeavyDataController = tmpHeavyDataController;
  mDimensions = tmpDimensions;
}
