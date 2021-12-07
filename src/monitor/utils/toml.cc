#include <monitor/utils/toml.hh>
#include <fstream>

namespace monitor {

    namespace utils {

	namespace toml {

	    /***
	     * ========================================================================
	     * ========================================================================
	     * =========================         parser           =====================
	     * ========================================================================
	     * ========================================================================
	     */

	
	    void* parse_value (tokenizer & tok, std::string & type);
	
	    config::dict* parse_dict (tokenizer & tok, bool glob = false);

	    config::array* parse_array (tokenizer & tok);

	    long* parse_int (tokenizer & tok);

	    float* parse_float (tokenizer & tok);

	    std::string* parse_string (tokenizer & tok, char beginChar = '"');

	    bool* parse_bool (tokenizer & tok);	


	    void assert_msg (bool test, const std::string & msg, tokenizer & tok) {
		if (!test) {
		    tok.rewind ();
		    throw config::config_error (tok.getLineNumber (), msg + " at line : " + std::to_string (tok.getLineNumber ()));
		}	    
	    }	
	
	    config::dict * parse_dict (tokenizer & tok, bool glob) {
		config::dict* dic = new  config::dict ();
		while (true) {
		    auto name = tok.next ();
		    if (name == "" || name == "[") {
			tok.rewind ();
			break;
		    }

		    assert_msg (tok.next () == "=", "expected = ", tok);
		
		    std::string type;
		    auto value = parse_value (tok, type);
		    dic-> insert (name, value, type);
		
		    if (!glob) {
			auto next = tok.next ();
			assert_msg (next == "}" || next == ",", "expected } or , (not " + next + ")", tok);
			if (next == "}") break;
		    } 
		}
		return dic;
	    }	

	    config::array * parse_array (tokenizer & tok) {
		config::array * arr = new  config::array ();
		while (true) {
		    if (tok.next () != "]") {
			std::string type;
			tok.rewind ();
			auto value = parse_value (tok, type);
			arr-> push (value, type);
			auto t = tok.next ();
			assert_msg (t == "," || t == "]", "expected ,", tok);
			if (t == "]") break;
		    } else break;
		}
	    
		return arr;
	    }
	
	    long * parse_int (tokenizer & tok) {
		auto x = tok.next ();
		auto * i = new  long (std::stoi (x));
		return i;
	    }

	    float* parse_float (tokenizer & tok) {
		auto x = tok.next ();
		auto * i = new  float (std::stof (x));
		return i;
	    }

	    std::string * parse_string (tokenizer & tok, char end) {
		tok.skip (false);
		std::stringstream ss;
		while (true) {
		    auto next = tok.next ();
		    if (next == "") {
			throw config::config_error (tok.getLineNumber (), "Unterminated string literal");
		    } else if (next.length () == 1 && next [0] == end) {
			break;
		    }
		    ss << next;
		}
		tok.skip (true);
	    
		return new  std::string (ss.str ());
	    }
	
	    void * parse_value (tokenizer & tok, std::string & type) {
		auto begin = tok.next ();
		if (begin == "{") {
		    type = typeid (config::dict).name ();
		    return parse_dict (tok, false);
		} else if (begin == "[") {
		    type = typeid (config::array).name ();
		    return parse_array (tok);
		} else if (begin == "'" || begin == "\"") {
		    type = typeid (std::string).name ();
		    return parse_string (tok, begin[0]);
		} else if (begin == "false") {		
		    type = typeid (bool).name ();
		    return new  bool (false);
		} else if (begin == "true") {		
		    type = typeid (bool).name ();
		    return new  bool (true);
		} else {
		    tok.rewind ();
		    if (begin.find (".") != std::string::npos) {
			type = typeid (float).name ();
			return parse_float (tok);
		    } else {
			type = typeid (long).name ();
			return parse_int (tok);
		    }
		}
	    }	

	
	    config::dict parse (const std::string & content) {
		tokenizer tok (content, {"[", "]", "{", "}", "=", ",", "'", "\"", " ", "\n", "\t", "\r", "#"}, {" ", "\t", "\n", "\r"}, {"#"});
	    
		config::dict dic;
		while (true) {
		    auto next = tok.next ();

		    assert_msg (next == "[" || next == "" || next == "#", "expected [ (not " + next + ")", tok);
		    if (next == "[") {
			auto name = tok.next ();
			assert_msg (tok.next () == "]", "expected ]", tok);
			auto content = parse_dict (tok, true);
			dic.insert (name, content, typeid (config::dict).name ());
		    } else break;
		}
	    
		return dic;
	    }


	    config::dict parse_file (const std::string & filePath) {	       
		std::ifstream t(filePath);
		if (!t.good ()) {
		    throw utils::file_error ("File not found : " + filePath);
		}

		std::string str((std::istreambuf_iterator<char>(t)),
				std::istreambuf_iterator<char>());

		return parse (str);
	    }

	    std::string dump (const config::array & cfg) {
		std::stringstream ss;
		ss << "[";
		auto & type = cfg.types ();
		for (auto it : range (0, cfg.size ())) {
		    if (it != 0) ss << ", ";
		    if (type [it] == typeid (float).name ()) ss << cfg.get<float> (it);
		    if (type [it] == typeid (long).name ()) ss << cfg.get<long> (it);
		    if (type [it] == typeid (std::string).name ()) ss << "\"" << cfg.get<std::string> (it) << "\"";
		    if (type [it] == typeid (config::array).name ()) ss << dump (cfg.get<config::array> (it));
		    if (type [it] == typeid (config::dict).name ()) ss << dump (cfg.get<config::dict> (it), false);		   
		}
		ss << "]";
		return ss.str ();
	    }

	    
	    std::string dump (const config::dict & cfg, bool isSuper, bool isGlobal) {		
		std::stringstream ss;
		auto type = cfg.types ();
		int i = 0;
		if (!isGlobal) {
		    ss << "{";
		    for (auto & it : cfg.keys ()) {
			if (i != 0) ss << ", ";
			if (type [it] == typeid (float).name ()) ss << it << " = " << cfg.get<float> (it);
			if (type [it] == typeid (long).name ()) ss << it << " = " << cfg.get<long> (it);
			if (type [it] == typeid (std::string).name ()) ss << it << " = \"" << cfg.get<std::string> (it) << "\"";
			if (type [it] == typeid (config::array).name ()) ss << it << " = " << dump (cfg.get<config::array> (it));
			if (type [it] == typeid (config::dict).name ()) ss << it << " = " << dump (cfg.get<config::dict> (it), false);
			
			i += 1;
		    }
		    ss << "}";
		} else {
		    for (auto & it : cfg.keys ()) {
			if (type [it] == typeid (config::dict).name () && isSuper) {
			    ss << "\n[" << it << "]\n";
			    ss << dump (cfg.get<config::dict> (it), false, true);
			} else {
			    if (type [it] == typeid (float).name ()) ss << it << " = " << cfg.get<float> (it);
			    if (type [it] == typeid (long).name ()) ss << it << " = " << cfg.get<long> (it);
			    if (type [it] == typeid (std::string).name ()) ss << it << " = \"" << cfg.get<std::string> (it) << "\"";
			    if (type [it] == typeid (config::array).name ()) ss << it << " = " << dump (cfg.get<config::array> (it));
			    if (type [it] == typeid (config::dict).name ()) ss << it << " = " << dump (cfg.get<config::dict> (it), false, false);
			    ss << "\n";
			}
			i += 1;
				    }
		}
		return ss.str ();

	    }
	    
	}
	
    }
    
}
