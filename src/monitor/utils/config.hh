#pragma once

#include <map>
#include <vector>
#include <typeinfo>
#include <gc/gc.h>
#include <cassert>
#include <iostream>
#include <sstream>

#include <monitor/utils/tokenizer.hh>
#include <monitor/utils/range.hh>
#include <monitor/utils/exception.hh>

namespace monitor {

    namespace utils {

	namespace config {

	    struct config_error : utils::exception {

		int line;
		
		config_error (int line, const std::string & msg) :
		    exception (msg),
		    line (line)
		    {}
		
	    };

	    class array {

	    private :

		std::vector <std::string> _types;
		std::vector <void*> _content;

	    public :

		array ();
		
		/**
		 * Deep copy
		 */
		array (const array& other);

		/**
		 * Deep copy
		 */
		const array& operator= (const array & other);
		
		template <typename T>
		void push (const T * value) {
		    this-> _types.push_back (typeid (T).name ());
		    this-> _content.push_back (value);
		}

		void push (void * value, const std::string & type);
		
		template <typename T> 
		T get (unsigned int i) const {
		    if constexpr (std::is_floating_point <T> ()) return get_float (i);
		    if constexpr (std::is_signed<T> ()) return get_int (i);
		    if (i >= this-> _content.size ()) {
			std::stringstream ss;
			ss << "Out of array : " << i << " > " << this-> _content.size ();
			throw config_error (0, ss.str ());
		    } else if (this-> _types [i] != typeid (T).name ()) {			
			throw config_error (0, "Incompatible types : " + this-> _types [i] + "  and " + typeid (T).name ());
		    }
		    return * (reinterpret_cast <T*> (this-> _content [i]));
		}

		int get_int (unsigned int i) const;
		float get_float (unsigned int i) const;
		
				
		int size () const;
		
		void print (std::ostream & stream) const;

		const std::vector <std::string> & types () const;
		
		/**
		 * clear the array
		 */
		void clear ();
		
		~array ();
		
	    private :		
		/**
		 * Deep copy of the content inside types, and content
		 */
		void clone_content (std::vector <std::string> & types, std::vector <void*> & content) const;		
		
	    };

	    struct dict {

		std::map <std::string, std::string> _types;
		std::map <std::string, void*> _content;

	    public :

		dict ();

		/**
		 * Deep copy
		 */
		dict (const dict & other);

		/**
		 * Deep copy
		 */
		const dict & operator= (const dict & other);
		
		
		template <typename T>
		void insert (const std::string & name, T* value) {
		    this-> _types.emplace (name, typeid (T).name ());
		    this-> _content.emplace (name, value);
		}

		void insert (const std::string & name, void* value, const std::string & type);

		std::vector <std::string> keys () const;
		
		template <typename T>
		const T& getRef (const std::string & name) const {		    
		    auto value = this-> _content.find (name);
		    if (value == this-> _content.end ()) {
			std::stringstream ss;
			ss << "Name : [" << name << "] not found in dictionnary ";
			this-> print (ss);
			throw config_error (0, ss.str ());
		    } else if (this-> _types.find (name)-> second != std::string (typeid (T).name ())) {
			throw config_error (0, "Incompatible types : " + this-> _types.find (name)-> second + " and " + std::string (typeid (T).name ()) + " for index [" + name + "]");
		    }
		    return * (reinterpret_cast <T*> (value-> second));
		}

		template <typename T>
		T get (const std::string & name) const {
		    if constexpr (std::is_floating_point<T> ()) return get_float (name);
		    if constexpr (std::is_signed<T> ()) return get_int (name);
		    
		    auto value = this-> _content.find (name);
		    if (value == this-> _content.end ()) {
			std::stringstream ss;
			ss << "Name : [" << name << "] not found in dictionnary ";
			this-> print (ss);
			throw config_error (0, ss.str ());
		    } else if (this-> _types.find (name)-> second != std::string (typeid (T).name ())) {
			throw config_error (0, "Incompatible types : " + this-> _types.find (name)-> second + " and " + std::string (typeid (T).name ()) + " for index [" + name + "]");
		    }
		    return * (reinterpret_cast <T*> (value-> second));
		}

		
		int get_int (const std::string & name) const;
		float get_float (const std::string & name) const;
		
		template <typename T>
		bool has (const std::string & name) const {
		    if constexpr (std::is_floating_point <T> ()) return has_float (name);
		    if constexpr (std::is_signed <T> ()) return has_int (name);
		    auto value = this-> _content.find (name);
		    if (value == this-> _content.end ()) return false;
		    auto ty = this-> _types.find (name);
		    return (ty-> second == typeid (T).name ());
		}

		bool has_int (const std::string & name) const;
		
		bool has_float (const std::string & name) const;
		
		template <typename T>
		T getOr (const std::string & name, T orVal) const {
		    if constexpr (std::is_floating_point <T> ()) return getOr_float (name, orVal);
		    if constexpr (std::is_signed <T> ()) return getOr_int (name, orVal);
		    auto value = this-> _content.find (name);
		    if (value == this-> _content.end ()) return orVal;
		    else if (this-> _types.find (name)-> second == typeid (T).name ()) {
			return * (reinterpret_cast<T*> (value-> second));
		    } else {
			throw config_error (0, "Incompatible types : " + this-> _types.find (name)-> second + " and " + std::string (typeid (T).name ()) + " for index [" + name + "]");
		    }
		}

		float getOr_float (const std::string & name, float orVal) const;
		int getOr_int (const std::string & name, int orVal) const;
		
		void print (std::ostream & stream) const;

		void clear ();


		const std::map <std::string, std::string> & types () const;

		const std::map <std::string, void*> & content () const;
		
		~dict ();

	    private :


		/**
		 * Deep copy of the content inside types, and content
		 */
		void clone_content (std::map <std::string, std::string> & types, std::map <std::string, void*> & content) const;
		
	    };
	           
	    	    
	}
		
    }    

}


std::ostream & operator<< (std::ostream & stream, const monitor::utils::config::array&);
std::ostream & operator<< (std::ostream & stream, const monitor::utils::config::dict&);
