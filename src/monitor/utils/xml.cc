#include <monitor/utils/xml.hh>

using namespace tinyxml2;

namespace monitor {

    namespace utils {

	XMLElement * findInXML (XMLElement * doc, const std::vector <std::string> & inners) {
	    if (doc == nullptr) return nullptr;

	    auto current = doc;
	    for (int i = 0 ; i < inners.size () ; i++) {
		current = current-> FirstChildElement (inners [i].c_str ());
		if (current == nullptr) return nullptr;
	    }

	    return current;
	}

	
    }
    
}
