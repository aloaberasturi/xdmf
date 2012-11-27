from Xdmf import *

if __name__ == "__main__":
	pointsXArray = XdmfArray.New()
	pointsXArray.pushBackAsInt32(5)
	pointsXArray.pushBackAsInt32(6)
	pointsXArray.pushBackAsInt32(7)
	pointsXArray.pushBackAsInt32(8)
	pointsXArray.pushBackAsInt32(9)
	pointsXArray.pushBackAsInt32(10)
	pointsYArray = XdmfArray.New()
	pointsYArray.pushBackAsInt32(3)
	pointsYArray.pushBackAsInt32(6)
	pointsYArray.pushBackAsInt32(4)
	pointsYArray.pushBackAsInt32(8)
	pointsYArray.pushBackAsInt32(7)
	pointsYArray.pushBackAsInt32(10)
	exampleGrid = XdmfRectilinearGrid.New(pointsXArray, pointsYArray)
	pointsZArray = XdmfArray.New()
        pointsZArray.pushBackAsInt32(3)
        pointsZArray.pushBackAsInt32(9)
        pointsZArray.pushBackAsInt32(4)
        pointsZArray.pushBackAsInt32(5)
        pointsZArray.pushBackAsInt32(7)
        pointsZArray.pushBackAsInt32(2)
	exampleGrid = XdmfRectilinearGrid.New(pointsXArray, pointsYArray, pointsZArray)

        exampleGrid.setCoordinates(0, pointsXArray)

	pointsCollector = ArrayVector()
	pointsCollector.push_back(pointsXArray)
	pointsCollector.push_back(pointsYArray)
	pointsCollector.push_back(pointsZArray)
	exampleGrid = XdmfRectilinearGrid.New(pointsCollector)

	readPointsX = exampleGrid.getCoordinates(0)
	readPointsY = exampleGrid.getCoordinates(1)

	exampleCoordinates = exampleGrid.getCoordinates()

	exampleDimensions = exampleGrid.getDimensions()

	exampleGrid.setCoordinates(pointsCollector)
