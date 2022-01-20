#pragma once
#include <monitor/foreign/tinyxml2.h>
#include <vector>
#include <string>

namespace monitor {
    namespace utils {

	/**
	 * Find an element in a XML doc at a given path
	 * @example: 
	 * ====================
	 * auto doc = createDoc ("<root>"
	 *                       "   <A>"
	 *                       "       <B attr="x"></B>"
	 *                       "  </A>"
	 *                       "</root>"
	 * );
	 * auto p = findInXML (doc, {"root", "A", "B"});
	 * std::cout << p-> Attribute ("attr") << std::endl;
	 * delete doc;
	 * ====================
	 * @returns: the element at path, or nullptr if it does not exists
	 */
	tinyxml2::XMLElement * findInXML (tinyxml2::XMLElement * doc, const std::vector <std::string> & path);
	
    }
}
