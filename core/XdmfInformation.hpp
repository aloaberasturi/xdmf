/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfInformation.hpp                                                 */
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

#ifndef XDMFINFORMATION_HPP_
#define XDMFINFORMATION_HPP_

// Forward declarations
class XdmfArray;

// Includes
#include "XdmfCore.hpp"
#include "XdmfItem.hpp"

/**
 * @brief Holds a key/value pair that can be attached to an Xdmf
 * structure.
 *
 * XdmfInformation stores two strings as a key value pair. These can
 * be used to store input parameters to a code or for simple result
 * data like wall time.
 */
class XDMFCORE_EXPORT XdmfInformation : public XdmfItem {

public:

  /**
   * Create a new XdmfInformation.
   *
   * Example of use:
   *
   * C++
   *
   * @code {.cpp}
   * shared_ptr<XdmfInformation> infoExample = XdmfInformation::New();
   * //Then the key and value must be set seperately
   * infoExample->setKey("Your Key String");
   * infoExample->setValue("Your Value String");
   * @endcode
   *
   * Python
   *
   * @code {.py}
   * infoExample = XdmfInformation.New()
   * '''
   * Then the key and value must be set seperately
   * '''
   * infoExample.setKey("Your Key String")
   * infoExample.setValue("Your Value String")
   * @endcode
   *
   * @return constructed XdmfInformation.
   */
  static shared_ptr<XdmfInformation> New();

  /**
   * Create a new XdmfInformation.
   *
   * Example of use:
   *
   * C++
   *
   * @code {.cpp}
   * shared_ptr<XdmfInformation> infoExample = XdmfInformation::New("Your Key String", "Your Value String");
   * //This code creates an information with the key "Your Key String" and the value "Your Value String"
   * @endcode
   *
   * Python
   *
   * @code {.py}
   * infoExample = XdmfInformation.New("Your Key String", "Your Value String")
   * '''
   * This code creates an information with the key "Your Key String" and the value "Your Value String"
   * '''
   * @endcode
   *
   * @param key a string containing the key of the XdmfInformation to create.
   * @param value a string containing the value of the XdmfInformation to
   * create.
   *
   * @return constructed XdmfInformation
   */
  static shared_ptr<XdmfInformation> New(const std::string & key,
                                         const std::string & value);

  virtual ~XdmfInformation();

  LOKI_DEFINE_VISITABLE(XdmfInformation, XdmfItem);
  XDMF_CHILDREN(XdmfInformation, XdmfArray, Array, Name);
  static const std::string ItemTag;

  std::map<std::string, std::string> getItemProperties() const;

  virtual std::string getItemTag() const;

  /**
   * Get the key for this information item.
   *
   * Example of use:
   *
   * C++
   *
   * @code {.cpp}
   * shared_ptr<XdmfInformation> infoExample = XdmfInformation::New("Your Key String", "Your Value String");
   * //This code creates an information with the key "Your Key String" and the value "Your Value String"
   * std::string storedKey = infoExample->getKey();
   * //"Your Key String" is now stored in the variable storedKey 
   * @endcode
   *
   * Python
   *
   * @code {.py}
   * infoExample = XdmfInformation.New("Your Key String", "Your Value String")
   * '''
   * This code creates an information with the key "Your Key String" and the value "Your Value String"
   * '''
   * storedKey = infoExample.getKey()
   * '''
   * "Your Key String" is now stored in the variable storedKey 
   * '''
   * @endcode
   *
   * @return string containing the key.
   */
  std::string getKey() const;

  /**
   * Get the value for this information item.
   *
   * Example of use:
   *
   * C++
   *
   * @code {.cpp}
   * shared_ptr<XdmfInformation> infoExample = XdmfInformation::New("Your Key String", "Your Value String");
   * //This code creates an information with the key "Your Key String" and the value "Your Value String"
   * std::string storedValue = infoExample->getValue();
   * //"Your Value String" is now stored in the variable storedValue 
   * @endcode
   *
   * Python
   *
   * @code {.py}
   * infoExample = XdmfInformation.New("Your Key String", "Your Value String")
   * '''
   * This code creates an information with the key "Your Key String" and the value "Your Value String"
   * '''
   * storedValue = infoExample.getValue()
   * '''
   * "Your Value String" is now stored in the variable storedValue 
   * '''
   * @endcode
   *
   * @return string containing the value.
   */
  std::string getValue() const;

  using XdmfItem::insert;

  /**
   * Set the key for this information item.
   *
   * Example of use:
   *
   * C++
   *
   * @code {.cpp}
   * shared_ptr<XdmfInformation> infoExample = XdmfInformation::New("Your Key String", "Your Value String");
   * //This code creates an information with the key "Your Key String" and the value "Your Value String"
   * infoExample->setKey("Your New Key");
   * //"Your New Key" is now the key for infoExample 
   * @endcode
   *
   * Python
   *
   * @code {.py}
   * infoExample = XdmfInformation.New("Your Key String", "Your Value String")
   * '''
   * This code creates an information with the key "Your Key String" and the value "Your Value String"
   * '''
   * infoExample.setKey("Your New Key")
   * '''
   * "Your New Key" is now the key for infoExample 
   * '''
   * @endcode
   *
   * @param key a string containing the key to set.
   */
  void setKey(const std::string & key);

  /**
   * Set the value for this information item.
   *
   * Example of use:
   *
   * C++
   *
   * @code {.cpp}
   * shared_ptr<XdmfInformation> infoExample = XdmfInformation::New("Your Key String", "Your Value String");
   * //This code creates an information with the key "Your Key String" and the value "Your Value String"
   * infoExample->setValue("Your New Value");
   * //"Your New Value" is now the value for infoExample 
   * @endcode
   *
   * Python
   *
   * @code {.py}
   * infoExample = XdmfInformation.New("Your Key String", "Your Value String")
   * '''
   * This code creates an information with the key "Your Key String" and the value "Your Value String"
   * '''
   * infoExample.setValue("Your New Value")
   * '''
   * "Your New Value" is now the value for infoExample 
   * '''
   * @endcode
   *
   * @param value a string containing the value to set.
   */
  void setValue(const std::string & value);

  virtual void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfInformation(const std::string & key = "",
                  const std::string & value = "");

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfInformation(const XdmfInformation &);  // Not implemented.
  void operator=(const XdmfInformation &);  // Not implemented.

  std::string mKey;
  std::string mValue;
};

#ifdef _WIN32
XDMFCORE_TEMPLATE template class XDMFCORE_EXPORT
shared_ptr<Loki::BaseVisitor>;
XDMFCORE_TEMPLATE template class XDMFCORE_EXPORT
Loki::Visitor<shared_ptr<XdmfInformation>,
              shared_ptr<XdmfItem> >;
#endif

#endif /* XDMFINFORMATION_HPP_ */
