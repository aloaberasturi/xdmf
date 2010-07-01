#include <math.h>
#include "XdmfArray.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfGrid.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyConverter.hpp"
#include "XdmfTopologyType.hpp"

#include "XdmfArrayType.hpp"
#include "XdmfWriter.hpp"

int main(int argc, char* argv[])
{
	const double epsilon = 1e-6;

	boost::shared_ptr<XdmfTopologyConverter> converter = XdmfTopologyConverter::New();

	// Create Hexahedron Grid
	boost::shared_ptr<XdmfGrid> hexGrid = XdmfGrid::New();
	double hexPoints[24] = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1};
	hexGrid->getGeometry()->setGeometryType(XdmfGeometryType::XYZ());
	hexGrid->getGeometry()->getArray()->resize<double>(24, 0);
	hexGrid->getGeometry()->getArray()->copyValues(0, hexPoints, 24);
	unsigned int hexConn[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	hexGrid->getTopology()->setTopologyType(XdmfTopologyType::Hexahedron());
	hexGrid->getTopology()->getArray()->resize<unsigned int>(8, 0);
	hexGrid->getTopology()->getArray()->copyValues(0, hexConn, 8);

	/*
	 * Hexahedron to Hexahedron_64
	 */
	boost::shared_ptr<XdmfGrid> hex64Grid = converter->convert(hexGrid, XdmfTopologyType::Hexahedron_64());

	assert(hex64Grid->getGeometry()->getGeometryType() == XdmfGeometryType::XYZ());
	assert(hex64Grid->getGeometry()->getNumberPoints() == 64);
	double expectedPoints[192] = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0.333333333333333, 0, 0, 0.666666666666667, 0, 0, 1, 0.333333333333333,
		0, 1, 0.666666666666667, 0, 0.666666666666667, 1, 0, 0.333333333333333, 1, 0, 0, 0.666666666666667, 0, 0, 0.333333333333333, 0,
		0.333333333333333, 0, 1, 0.666666666666667, 0, 1, 1, 0.333333333333333, 1, 1, 0.666666666666667, 1, 0.666666666666667, 1, 1, 0.333333333333333,
		1, 1, 0, 0.666666666666667, 1, 0, 0.333333333333333, 1, 0, 0, 0.333333333333333, 1, 0, 0.333333333333333, 1, 1, 0.333333333333333, 0, 1,
		0.333333333333333, 0, 0, 0.666666666666667, 1, 0, 0.666666666666667, 1, 1, 0.666666666666667, 0, 1, 0.666666666666667, 0, 0.666666666666667,
		0.333333333333333, 0, 0.333333333333333, 0.333333333333333, 1, 0.333333333333333, 0.333333333333333, 1, 0.666666666666667, 0.333333333333333,
		0.333333333333333, 0, 0.333333333333333, 0.666666666666667, 0, 0.333333333333333, 0.666666666666667, 1, 0.333333333333333, 0.333333333333333,
		1, 0.333333333333333, 0, 0.666666666666667, 0.666666666666667, 0, 0.333333333333333, 0.666666666666667, 1, 0.333333333333333,
		0.666666666666667, 1, 0.666666666666667, 0.666666666666667, 0.333333333333333, 0, 0.666666666666667, 0.666666666666667, 0, 0.666666666666667,
		0.666666666666667, 1, 0.666666666666667, 0.333333333333333, 1, 0.666666666666667, 0.333333333333333, 0.333333333333333, 0, 0.666666666666667,
		0.333333333333333, 0, 0.666666666666667, 0.666666666666667, 0, 0.333333333333333, 0.666666666666667, 0, 0.333333333333333, 0.333333333333333,
		1, 0.666666666666667, 0.333333333333333, 1, 0.666666666666667, 0.666666666666667, 1, 0.333333333333333, 0.666666666666667, 1,
		0.333333333333333, 0.333333333333333, 0.333333333333333, 0.666666666666667, 0.333333333333333, 0.333333333333333, 0.666666666666667,
		0.666666666666667, 0.333333333333333, 0.333333333333333, 0.666666666666667, 0.333333333333333, 0.333333333333333, 0.333333333333333,
		0.666666666666667, 0.666666666666667, 0.333333333333333, 0.666666666666667, 0.666666666666667, 0.666666666666667, 0.666666666666667,
		0.333333333333333, 0.666666666666667, 0.666666666666667};

	for(unsigned int i=0; i<192; ++i)
	{
		assert(fabs(expectedPoints[i] - hex64Grid->getGeometry()->getArray()->getValueCopy<double>(i)) < epsilon);
	}
	assert(hex64Grid->getTopology()->getTopologyType() == XdmfTopologyType::Hexahedron_64());
	assert(hex64Grid->getTopology()->getNumberElements() == 1);
	for(unsigned int i=0; i<64; ++i)
	{
		assert(i == hex64Grid->getTopology()->getArray()->getValueCopy<unsigned int>(i));
	}

	return 0;
}
