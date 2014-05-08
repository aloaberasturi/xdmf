#include <fstream>
#include <vector>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfBinaryController.hpp"
#include "XdmfReader.hpp"
#include "XdmfWriter.hpp"

int main(int, char **)
{
  
  //
  // write binary file
  //

  std::vector<int> outputData;
  outputData.push_back(1);
  outputData.push_back(0);
  outputData.push_back(-1);  
  outputData.push_back(100);
                       
  std::ofstream output("binary.bin",
                       std::ofstream::binary);
  output.write(reinterpret_cast<char *>(&(outputData[0])),
               sizeof(int) * outputData.size());
  output.close();

  //
  // read binary file using XdmfBinaryController
  //
  shared_ptr<XdmfBinaryController> binaryController = 
    XdmfBinaryController::New("binary.bin",
                              XdmfArrayType::Int32(),
                              XdmfBinaryController::NATIVE,
                              0,
                              std::vector<unsigned int>(1, 4));
  
  shared_ptr<XdmfArray> testArray = XdmfArray::New();
  testArray->setHeavyDataController(binaryController);
  testArray->read();
  
  assert(testArray->getSize() == 4);
  assert(testArray->getValue<int>(0) == outputData[0]);
  assert(testArray->getValue<int>(1) == outputData[1]);
  assert(testArray->getValue<int>(2) == outputData[2]);
  assert(testArray->getValue<int>(3) == outputData[3]);
  
  testArray->release();

  //
  // output array to disk
  //
  shared_ptr<XdmfWriter> writer = XdmfWriter::New("TestXdmfBinary.xmf");
  writer->setMode(XdmfWriter::DistributedHeavyData);
  testArray->accept(writer);

  //
  // read array in
  //
  shared_ptr<XdmfReader> reader = XdmfReader::New();
  shared_ptr<XdmfArray> array = 
    shared_dynamic_cast<XdmfArray>(reader->read("TestXdmfBinary.xmf"));
  assert(array != NULL);

  return 0;
}
