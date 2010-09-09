#ifndef XDMFSYSTEMUTILS_HPP_
#define XDMFSYSTEMUTILS_HPP_

// Includes
#include <string>

/**
 * @brief System specific functions.
 *
 * Collects all system specific functions needed by Xdmf.
 */
class XdmfSystemUtils {

public:

	/**
	 * Converts a filesystem path to an absolute real path (absolute path with no symlinks)
	 *
	 * @param path a string containing the path to convert.
	 * @return the equivalent real path.
	 */
	static std::string getRealPath(const std::string & path);

protected:

	XdmfSystemUtils();
	~XdmfSystemUtils();

private:

	XdmfSystemUtils(const XdmfSystemUtils & systemUtils);  // Not implemented.
	void operator=(const XdmfSystemUtils & systemUtils);  // Not implemented.

};

#endif /* XDMFSYSTEMUTILS_HPP_ */
