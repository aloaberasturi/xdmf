#include "XdmfDomain.hpp"
#include "XdmfWriter.hpp"

#include "XdmfTestDataGenerator.hpp"

int main(int argc, char* argv[])
{
	boost::shared_ptr<XdmfWriter> writer = XdmfWriter::New("output.xmf");
	assert(writer->getFilePath().compare("output.xmf") == 0);
	writer->setLightDataLimit(10);
	assert(writer->getLightDataLimit() == 10);

	boost::shared_ptr<XdmfGrid> grid = XdmfTestDataGenerator::createHexahedron();

	boost::shared_ptr<XdmfDomain> domain = XdmfDomain::New();
	domain->insert(grid);
	domain->accept(writer);

	return 0;
}
