#include <fstream>
#include <sstream>
#include "XdmfDomain.hpp"
#include "XdmfReader.hpp"
#include "XdmfWriter.hpp"

#include "XdmfTestDataGenerator.hpp"

int main(int argc, char* argv[])
{
	boost::shared_ptr<XdmfWriter> writer = XdmfWriter::New("xpath1.xmf");
	boost::shared_ptr<XdmfGrid> grid = XdmfTestDataGenerator::createHexahedron();

	boost::shared_ptr<XdmfGrid> newGrid = XdmfGrid::New();
	newGrid->setName("NoAttributes");
	newGrid->setGeometry(grid->getGeometry());
	newGrid->setTopology(grid->getTopology());

	boost::shared_ptr<XdmfDomain> domain = XdmfDomain::New();
	domain->insert(grid);
	domain->insert(grid);
	domain->insert(newGrid);
	domain->accept(writer);

	// Try to find xpaths written to file
	std::ifstream file("xpath1.xmf");
	std::stringstream fileBuffer;
	fileBuffer << file.rdbuf();
	std::string fileContents(fileBuffer.str());

	assert(fileContents.find("xpointer=\"element(/1/1/1)\"") != std::string::npos);
	assert(fileContents.find("xpointer=\"element(/1/1/1/2)\"") != std::string::npos);
	assert(fileContents.find("xpointer=\"element(/1/1/1/3)\"") != std::string::npos);

	// Make sure when we read it in we get the same structure as when we wrote it out (multiple items holding the same shared pointers)
	boost::shared_ptr<XdmfReader> reader = XdmfReader::New();
	boost::shared_ptr<XdmfDomain> domain2 = boost::shared_dynamic_cast<XdmfDomain>(reader->read("xpath1.xmf"));
	boost::shared_ptr<XdmfWriter> writer2 = XdmfWriter::New("xpath2.xmf");
	domain2->accept(writer2);

	// Compare two files for equality
	std::ifstream firstFile("xpath1.xmf");
	std::ifstream secondFile("xpath2.xmf");

	std::stringstream firstBuffer;
	std::stringstream secondBuffer;

	firstBuffer << firstFile.rdbuf();
	secondBuffer << secondFile.rdbuf();

	std::string firstContents(firstBuffer.str());
	std::string secondContents(secondBuffer.str());

	assert(firstContents.compare(secondContents) == 0);

	return 0;
}
