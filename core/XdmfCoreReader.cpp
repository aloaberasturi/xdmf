/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCoreReader.cpp                                                  */
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

#include <libxml/uri.h>
#include <libxml/xpointer.h>
#include <libxml/xmlreader.h>
#include "boost/tokenizer.hpp"
#include <map>
#include <sstream>
#include <cstring>
#include <utility>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfCoreItemFactory.hpp"
#include "XdmfCoreReader.hpp"
#include "XdmfError.hpp"
#include "XdmfItem.hpp"
#include "XdmfSystemUtils.hpp"

/**
 * PIMPL
 */
class XdmfCoreReader::XdmfCoreReaderImpl {

public:

  XdmfCoreReaderImpl(const shared_ptr<const XdmfCoreItemFactory> itemFactory,
                     const XdmfCoreReader * const coreReader) :
    mCoreReader(coreReader),
    mItemFactory(itemFactory)
  {
  };

  ~XdmfCoreReaderImpl()
  {
  };

  void
  closeFile()
  {
    mXPathMap.clear();
    xmlXPathFreeContext(mXPathContext);
    for(std::map<std::string, xmlDocPtr>::const_iterator iter = 
	  mDocuments.begin(); iter != mDocuments.end(); ++iter) {
      xmlFreeDoc(iter->second);
    }
    mDocuments.clear();
    
    xmlCleanupParser();
  }

  void
  openFile(const std::string & filePath)
  {
    mXMLDir = XdmfSystemUtils::getRealPath(filePath);
    size_t index = mXMLDir.find_last_of("/\\");
    if(index != std::string::npos) {
      mXMLDir = mXMLDir.substr(0, index + 1);
    }

    mDocument = xmlReadFile(filePath.c_str(), NULL, 0);

    if(mDocument == NULL) {
      XdmfError::message(XdmfError::FATAL,
                         "xmlReadFile could not read " + filePath +
                         " in XdmfCoreReader::XdmfCoreReaderImpl::openFile");
    }

    mDocuments.insert(std::make_pair((char*)mDocument->URL, mDocument));

    mXPathContext = xmlXPtrNewContext(mDocument, NULL, NULL);
    mXPathMap.clear();
  }

  void
  parse(const std::string & lightData) 
  {
    mDocument = xmlParseDoc((const xmlChar*)lightData.c_str());
                               
    if(mDocument == NULL) {
      XdmfError::message(XdmfError::FATAL,
                         "xmlReadFile could not parse passed light data string"
                         " in XdmfCoreReader::XdmfCoreReaderImpl::parse");
    }

    //    mDocuments.insert(std::make_pair((char*)mDocument->URL, mDocument));
    mXPathContext = xmlXPtrNewContext(mDocument, NULL, NULL);
    mXPathMap.clear();
  }

  /**
   * Constructs XdmfItems for all nodes in currNode's tree.
   * XdmfItems are constructed by recursively calling this function for all
   * children of currNode.
   */
  std::vector<shared_ptr<XdmfItem> >
  read(xmlNodePtr currNode)
  {
    std::vector<shared_ptr<XdmfItem> > myItems;

    while(currNode != NULL) {
      if(currNode->type == XML_ELEMENT_NODE) {
        if(xmlStrcmp(currNode->name, (xmlChar*)"include") == 0) {
          // Deal with proper reading of XIncludes

          xmlChar * xpointer = NULL;
          xmlChar * href = NULL;

          xmlAttrPtr currAttribute = currNode->properties;
          while(currAttribute != NULL) {
            if(xmlStrcmp(currAttribute->name, (xmlChar*)"xpointer") == 0) {
              xpointer = currAttribute->children->content;
            }
            if(xmlStrcmp(currAttribute->name, (xmlChar*)"href") == 0) {
              href = currAttribute->children->content;
            }
            currAttribute = currAttribute->next;
          }

          xmlXPathContextPtr context = mXPathContext;
          if(href) {
	    xmlDocPtr document;
            xmlChar * filePath = xmlBuildURI(href, mDocument->URL);
	    std::map<std::string, xmlDocPtr>::const_iterator iter = 
	      mDocuments.find((char*)filePath);
	    if(iter == mDocuments.end()) {
	      document = xmlReadFile((char*)filePath, NULL, 0);
	      mDocuments.insert(std::make_pair((char*)document->URL, document));
	    }
	    else {
	      document = iter->second;
	    }

            context = xmlXPtrNewContext(document, NULL, NULL);           
          }

          if(xpointer) {
            xmlXPathObjectPtr result = xmlXPtrEval(xpointer, context);
            if(result && !xmlXPathNodeSetIsEmpty(result->nodesetval)) {
              for(int i=0; i<result->nodesetval->nodeNr; ++i) {
                this->readSingleNode(result->nodesetval->nodeTab[i],
                                     myItems);
              }
            }
            xmlXPathFreeObject(result);
          }

          if(href) {
            xmlXPathFreeContext(context);
          }

        }
        else if (xmlStrcmp(currNode->name, (xmlChar*)"Function") == 0) {
          //function handling goes here
          xmlNodePtr childNode = currNode->children;

          std::string arraySubType = "";
          //get Array Subtype, if any
          xmlAttrPtr currAttribute = currNode->properties;
          while (currAttribute != NULL) {
            if (xmlStrcmp(currAttribute->name, (xmlChar*)"Type") == 0 ) {
              arraySubType = (char*)currAttribute->children->content;
              break;//uses the first type found
            }
          }

          std::string expressionToParse = "";

          while (childNode != NULL) {
            if (xmlStrcmp(childNode->name, (xmlChar*)"Expression") == 0){
              //store expression
              xmlAttrPtr childAttribute = childNode->properties;
              while (childAttribute != NULL) {
                if(xmlStrcmp(childAttribute->name, (xmlChar*)"Value") == 0) {
                  expressionToParse = (char*)childAttribute->children->content;
                  break;
                }
                childAttribute = childAttribute->next;
              }
              break;
            }
            childNode = childNode->next;
          }

          if (expressionToParse.compare("") == 0) {
            XdmfError::message(XdmfError::FATAL,
              "Error: No Expression in Function");
          }

          //two seperate loops to allow for different orders and multiple variable sets
          childNode = currNode->children;

          std::map<std::string, shared_ptr<XdmfArray> > variableCollection;
          while (childNode != NULL) {
            if (xmlStrcmp(childNode->name, (xmlChar*)"Variable") == 0) {
              //store child variables
              xmlNodePtr childVariable = childNode->children;

              while (childVariable != NULL) {
                if (xmlStrcmp(childVariable->name, (xmlChar*)"DataItem") == 0) {
                  xmlAttrPtr childAttribute = childVariable->properties;
                  std::string childKey = "";
                  shared_ptr<XdmfArray> childArray = XdmfArray::New();
                  std::string dataString = "";
                  shared_ptr<const XdmfArrayType> dataType;
                  xmlChar * childXPointer = NULL;
                  xmlChar * childhref = NULL;
                  std::string childhdf5 = "";
                  std::map<std::string, std::string> typeMap;

                  while (childAttribute != NULL) {
                    //the variable type of the array
                    if (xmlStrcmp(childAttribute->name, (xmlChar*)"DataType") == 0) {
                      typeMap["DataType"] = (char*)childAttribute->children->content;
                    }
                    //the precision of the variable type (only used for long and double)
                    else if (xmlStrcmp(childAttribute->name, (xmlChar*)"Precision") == 0) {
                      typeMap["Precision"] = (char*)childAttribute->children->content;
                    }
                    //the key or mapped string for the variable
                    else if (xmlStrcmp(childAttribute->name, (xmlChar*)"Key") == 0) {
                      childKey = (char*)childAttribute->children->content;
                    }
                    //text based xml data
                    else if (xmlStrcmp(childAttribute->name, (xmlChar*)"Value") == 0) {
                      dataString = (char*)childAttribute->children->content;
                    }
                    //an x pointer to another XdmfArray
                    else if (xmlStrcmp(childAttribute->name, (xmlChar*)"XPointer") == 0) {
                      childXPointer = childAttribute->children->content;
                    }
                    //used in conjunction with Xpointers to reference objects in a different file
                    else if (xmlStrcmp(childAttribute->name, (xmlChar*)"href") == 0) {
                      childhref = childAttribute->children->content;
                    }
                    //path to hdf5 data sets and the dimensions of those sets
                    else if (xmlStrcmp(childAttribute->name, (xmlChar*)"hdf5") == 0) {
                      childhdf5 = (char*)childAttribute->children->content;
                    }
                    childAttribute = childAttribute->next;
                  }

                  if (typeMap["DataType"].compare("Float") == 0) {
                    if (typeMap["Precision"].compare("8") == 0) {
                      dataType = XdmfArrayType::Float64();
                    }
                    else {
                      dataType = XdmfArrayType::Float32();
                    }
                  }
                  else if (typeMap["DataType"].compare("Int") == 0) {
                    if (typeMap["Precision"].compare("8") == 0) {
                      dataType = XdmfArrayType::Int64();
                    }
                    else {
                      dataType = XdmfArrayType::Int32();
                    }
                  }
                  else if (typeMap["DataType"].compare("String") == 0) {
                    dataType = XdmfArrayType::String();
                  }
                  else if (typeMap["DataType"].compare("Char") == 0) {
                    dataType = XdmfArrayType::Int8();
                  }
                  else if (typeMap["DataType"].compare("Short") == 0) {
                    dataType = XdmfArrayType::Int16();
                  }
                  else if (typeMap["DataType"].compare("UChar") == 0) {
                    dataType = XdmfArrayType::UInt8();
                  }
                  else if (typeMap["DataType"].compare("UShort") == 0) {
                    dataType = XdmfArrayType::UInt16();
                  }
                  else if (typeMap["DataType"].compare("UInt") == 0) {
                    dataType = XdmfArrayType::UInt32();
                  }
                  else {
                    dataType = XdmfArrayType::Uninitialized();
                  }

                  //if xpointer grab item at that location
                  if (childXPointer) {
                    xmlXPathContextPtr context = mXPathContext;

                    if(childhref) {
                      xmlDocPtr document;
                      xmlChar * filePath = xmlBuildURI(childhref, mDocument->URL);
                      std::map<std::string, xmlDocPtr>::const_iterator iter =
                        mDocuments.find((char*)filePath);
                      if(iter == mDocuments.end()) {
                        document = xmlReadFile((char*)filePath, NULL, 0);
                        mDocuments.insert(std::make_pair((char*)document->URL, document));
                      }
                      else {
                        document = iter->second;
                      }
                      context = xmlXPtrNewContext(document, NULL, NULL);
                    }

                    if(childXPointer) {
                      xmlXPathObjectPtr result = xmlXPtrEval(childXPointer, context);
                      if(result && !xmlXPathNodeSetIsEmpty(result->nodesetval)) {
                        for(int i=0; i<result->nodesetval->nodeNr; ++i) {
                          //there should only be one item being returned here
                          //place into a new vector
                          std::vector<shared_ptr<XdmfItem> > pointedItems;
                          this->readSingleNode(result->nodesetval->nodeTab[i], pointedItems);
                          try {
                            childArray = shared_dynamic_cast<XdmfArray>(pointedItems[0]);//try to cast it as an array
                          }
                          catch (...) {//if that doesn't work throw an error
                            XdmfError::message(XdmfError::FATAL,
                                               "Error: Variable not Equivalent to an Array");//because we should only be working with arrays
                          }
                        }
                      }
                      xmlXPathFreeObject(result);
                    }

                    if(childhref) {
                      xmlXPathFreeContext(context);
                    }
                  }
                  //if hdf5 create controllers and attach it
                  else if (childhdf5.compare("") != 0) {
                    //parse the hdf5 controllers
                    std::vector<std::string> controllerParts;
                    //split the content based on "|" characters
                    size_t barSplit = 0;
                    std::string splitString(childhdf5);
                    std::string subcontent;

                    while (barSplit != std::string::npos) {
                      barSplit = 0;
                      barSplit = splitString.find_first_of("|", barSplit);
                      if (barSplit == std::string::npos) {
                        subcontent = splitString;
                      }
                      else {
                        subcontent = splitString.substr(0, barSplit);
                        splitString = splitString.substr(barSplit+1);
                        barSplit++;
                      }
                      controllerParts.push_back(subcontent);
                    }

                    //insert those controllers into the childArray
                    int hdf5step = 2;
                    for (int i = 0; i < controllerParts.size(); i = i + hdf5step) {
                      size_t colonLocation = controllerParts[i].find(":");
                      if(colonLocation == std::string::npos) {
                        XdmfError::message(XdmfError::FATAL,
                                           "':' not found in function variable content in "
                                           "read -- double check an HDF5 "
                                           "data set is specified for the file");
                      }

                      std::string hdf5Path = controllerParts[i].substr(0, colonLocation);
                      std::string dataSetPath = controllerParts[i].substr(colonLocation+1);
                      std::vector<unsigned int> contentDims;

			//TODO might need to rewrite this try-catch block
                      try {
                        //this is the string that contains the dimensions
                        boost::tokenizer<> dimtokens(controllerParts[i + 1]);
                        for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
                            iter != dimtokens.end();
                            ++iter) {
                          contentDims.push_back(atoi((*iter).c_str()));
                        }
                        hdf5step = 2;//if this works then the dimension content should be skipped over
                      }
                      catch (...) {//if it fails then it means that the next content is not a dimension string
                        //in this case an error should be thrown, formatting error
                        //because there is no base array to pull dimensions from
                        XdmfError::message(XdmfError::FATAL,
                                           "Error: Improper HDF5 Format");
                      }

                      childArray->insert(XdmfHDF5Controller::New(hdf5Path,
                                                                 dataSetPath,
                                                                 dataType,
                                                                 std::vector<unsigned int>(contentDims.size(), 0),
                                                                 std::vector<unsigned int>(contentDims.size(), 1),
                                                                 contentDims,
                                                                 contentDims));
                    }
                  }
                  //if xml parse strait to insert
                  else if (dataString.compare("") != 0) {
                    //parse the data into tokens
                    childArray->initialize(dataType, 0);
                    unsigned int index = 0;
                    boost::char_separator<char> sep(" \t\n");
                    boost::tokenizer<boost::char_separator<char> > tokens(dataString, sep);
                    if(dataType == XdmfArrayType::String()) {
                      for(boost::tokenizer<boost::char_separator<char> >::const_iterator
                          iter = tokens.begin();
                          iter != tokens.end();
                          ++iter, ++index) {
                        //insert those tokens into the childArray
                        childArray->insert(index, *iter);
                      }
                    }
                    else {
                      for(boost::tokenizer<boost::char_separator<char> >::const_iterator
                          iter = tokens.begin();
                          iter != tokens.end();
                          ++iter, ++index) {
                        //insert those tokens into the childArray
                        childArray->insert(index, atof((*iter).c_str()));
                      }
                    }
                  }

                  //parse the value into the array
                  if (childKey.compare("") != 0){
                    if (variableCollection.find(childKey) != variableCollection.end()) {
                      XdmfError::message(XdmfError::WARNING,
                                         "Warning: Variable Redefined");
                    }
                    childArray->read();
                    variableCollection[childKey] = childArray;
                  }
                  else {
                    XdmfError::message(XdmfError::WARNING,
                                       "Warning: Value Unpaired to Key");
                  }
                }
                childVariable = childVariable->next;
              }
            }
            childNode = childNode->next;
          }

          shared_ptr<XdmfArray> parsedArray = XdmfArray::evaluateExpression(expressionToParse, variableCollection);
          //the properties and children aren't really needed to generate the object, but the factory still requires them.
          std::map<std::string, std::string> newArrayProperties;
          std::vector<shared_ptr<XdmfItem> > newArrayChildren;
          shared_ptr<XdmfArray> returnArray = XdmfArray::New();

          if (arraySubType.compare("") == 0) {//if no type is specified an array is generated
            arraySubType = "DataItem";
          }

          //this should generate an item that corresponds to the tag provided, the casting ensures that it is a subtype of array
          //using a factory to be able to build things outside of core
          returnArray = shared_dynamic_cast<XdmfArray>(mItemFactory->createItem(
                                                       arraySubType,
                                                       newArrayProperties,
                                                       newArrayChildren));

          if (!returnArray) {//if the specified tag fails to generate an item then reclass as an array
            arraySubType = "DataItem";
            returnArray = shared_dynamic_cast<XdmfArray>(mItemFactory->createItem(
                                                         arraySubType,
                                                         newArrayProperties,
                                                         newArrayChildren));
          }

          returnArray->insert(0, parsedArray, 0, parsedArray->getSize());
          myItems.push_back(returnArray);
        }
        else {
          // Normal reading
          this->readSingleNode(currNode, myItems);
        }
      }
      currNode = currNode->next;
    }

    return myItems;
  }

  /**
   * Reads a single xmlNode into an XdmfItem object in memory. The constructed
   * XdmfItem is added to myItems and an entry is added mapping the xmlNodePtr
   * to the new XdmfItem in the mXPathMap.
   */
  void
  readSingleNode(const xmlNodePtr currNode,
                 std::vector<shared_ptr<XdmfItem> > & myItems)
  {

    //check to see if the node is already in the Xpath
    std::map<xmlNodePtr, shared_ptr<XdmfItem> >::const_iterator iter =
      mXPathMap.find(currNode);
    if(iter != mXPathMap.end()) {//if it is grab it from the previously stored items
      myItems.push_back(iter->second);
    }
    else {//otherwise, generate it from the node
      std::map<std::string, std::string> itemProperties;

      xmlNodePtr childNode = currNode->children;
      if (XdmfArray::ItemTag.compare((char *)currNode->name) == 0) {
	unsigned int childContentIndex = 0;
        while(childNode != NULL) {
          if(childNode->type == XML_TEXT_NODE && childNode->content) {
            const char * content = (char*)childNode->content;



            
            // determine if content is whitespace
            bool whitespace = true;
            
            const char * contentPtr = content;
            //step through to end of pointer
            while(contentPtr != NULL) {
              //if not a whitespace character, break
              if(!isspace(*contentPtr++)) {
                whitespace = false;
                break;
              }
            }
            
            if(!whitespace) {
              if (childContentIndex == 0) {
                itemProperties.insert(std::make_pair("XMLDir", mXMLDir));
              }

              //split the content based on "|" characters
              size_t barSplit = 0;
              std::string splitString(content);
              std::string subcontent;
              while (barSplit != std::string::npos) {
		barSplit = 0;
                barSplit = splitString.find_first_of("|", barSplit);
                if (barSplit == std::string::npos) {
                  subcontent = splitString;
                }
                else {
                  subcontent = splitString.substr(0, barSplit);
                  splitString = splitString.substr(barSplit+1);
                  barSplit++;
                }
                std::stringstream contentString;
                contentString << "Content" << childContentIndex;
                itemProperties.insert(std::make_pair(contentString.str(), subcontent));
		childContentIndex++;
              }
            }
          }
          childNode = childNode->next;
        }
      }
    
 
      xmlAttrPtr currAttribute = currNode->properties;
      while(currAttribute != NULL) {
        itemProperties.insert(std::make_pair((char *)currAttribute->name,
                                             (char *)currAttribute->children->content));
        currAttribute = currAttribute->next;
      }

      const std::vector<shared_ptr<XdmfItem> > childItems =
        this->read(currNode->children);
      shared_ptr<XdmfItem> newItem =
        mItemFactory->createItem((const char *)currNode->name,
                                 itemProperties,
                                 childItems);
      if(newItem == NULL) {
        XdmfError::message(XdmfError::FATAL, 
                           "mItemFactory failed to createItem in "
                           "XdmfCoreReader::XdmfCoreReaderImpl::readSingleNode");
      }

      newItem->populateItem(itemProperties, childItems, mCoreReader);
      myItems.push_back(newItem);
      mXPathMap.insert(std::make_pair(currNode, newItem));
    }
  }

  void
  readPathObjects(const std::string & xPath,
                  std::vector<shared_ptr<XdmfItem> > & myItems)
  {
    xmlXPathObjectPtr xPathObject =
      xmlXPathEvalExpression((xmlChar*)xPath.c_str(), mXPathContext);
    if(xPathObject && xPathObject->nodesetval) {
      for(int i=0; i<xPathObject->nodesetval->nodeNr; ++i) {
        this->readSingleNode(xPathObject->nodesetval->nodeTab[i], myItems);
      }
    }
    xmlXPathFreeObject(xPathObject);
  }

  xmlDocPtr mDocument;
  std::map<std::string, xmlDocPtr> mDocuments;
  const XdmfCoreReader * const mCoreReader;
  const shared_ptr<const XdmfCoreItemFactory> mItemFactory;
  std::string mXMLDir;
  xmlXPathContextPtr mXPathContext;
  std::map<xmlNodePtr, shared_ptr<XdmfItem> > mXPathMap;
};

XdmfCoreReader::XdmfCoreReader(const shared_ptr<const XdmfCoreItemFactory> itemFactory) :
  mImpl(new XdmfCoreReaderImpl(itemFactory, this))
{
}

XdmfCoreReader::~XdmfCoreReader()
{
  delete mImpl;
}

shared_ptr<XdmfItem >
XdmfCoreReader::parse(const std::string & lightData) const
{
  mImpl->parse(lightData);
  const xmlNodePtr currNode = xmlDocGetRootElement(mImpl->mDocument);
  std::vector<shared_ptr<XdmfItem> > toReturn;
  if(mImpl->mItemFactory->createItem((const char*)currNode->name,
                                     std::map<std::string, std::string>(),
                                     std::vector<shared_ptr<XdmfItem> >()) == NULL) {
    toReturn = mImpl->read(currNode->children);
  }
  else {
    toReturn = mImpl->read(currNode);
  }
  mImpl->closeFile();
  return(toReturn[0]);
}

std::vector<shared_ptr<XdmfItem> >
XdmfCoreReader::readItems(const std::string & filePath) const
{
  mImpl->openFile(filePath);
  const xmlNodePtr currNode = xmlDocGetRootElement(mImpl->mDocument);
  const std::vector<shared_ptr<XdmfItem> > toReturn =
    mImpl->read(currNode->children);
  mImpl->closeFile();
  return toReturn;
}

shared_ptr<XdmfItem>
XdmfCoreReader::read(const std::string & filePath) const
{
  const std::vector<shared_ptr<XdmfItem> > toReturn = readItems(filePath);
  if (toReturn.size() == 0) {
    return(shared_ptr<XdmfItem>());
  }
  return(toReturn[0]);
}

std::vector<shared_ptr<XdmfItem> >
XdmfCoreReader::read(const std::string & filePath,
                     const std::string & xPath) const
{
  mImpl->openFile(filePath);
  std::vector<shared_ptr<XdmfItem> > toReturn = this->readPathObjects(xPath);
  mImpl->closeFile();
  return toReturn;
}

std::vector<shared_ptr<XdmfItem> >
XdmfCoreReader::readPathObjects(const std::string & xPath) const
{
  std::vector<shared_ptr<XdmfItem> > toReturn;
  mImpl->readPathObjects(xPath, toReturn);
  return toReturn;
}

