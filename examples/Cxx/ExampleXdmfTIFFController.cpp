#include "XdmfTIFFController.hpp"

int main(int, char **)
{
        //#initialization begin

        std::string newPath = "File path to TIFF file goes here";
        shared_ptr<const XdmfArrayType> readType = XdmfArrayType::Int32();
        std::vector<unsigned int> readStarts;
        //Three dimensions, all starting at index 0
        readStarts.push_back(0);
        readStarts.push_back(0);
        readStarts.push_back(0);
        std::vector<unsigned int> readStrides;
        //Three dimensions, no skipping between reads
        readStrides.push_back(1);
        readStrides.push_back(1);
        readStrides.push_back(1);
        std::vector<unsigned int> readCounts;
        //Three dimensions, reading 10 values from each
        readCounts.push_back(10);
        readCounts.push_back(10);
        readCounts.push_back(10);
        std::vector<unsigned int> readDataSize;
        //Three dimensions, each with a maximum of 20 values
        readDataSize.push_back(20);
        readDataSize.push_back(20);
        readDataSize.push_back(20);
        shared_ptr<XdmfTIFFController> exampleController = XdmfTIFFController::New(
                newPath,
                readType,
                readStarts,
                readStrides,
                readCounts,
                readDataSize);

        //#initialization end

        //#initializationsimplified begin

        std::string newPath = "File path to TIFF file goes here";
        shared_ptr<const XdmfArrayType> readType = XdmfArrayType::Int32();
        std::vector<unsigned int> readCounts;
        //Three dimensions, reading 10 values from each
        readCounts.push_back(10);
        readCounts.push_back(10);
        readCounts.push_back(10);
        shared_ptr<XdmfTIFFController> exampleController = XdmfTIFFController::New(
                newPath,
                readType,
                readCounts);

        //#initializationsimplified end

        return 0;
}
