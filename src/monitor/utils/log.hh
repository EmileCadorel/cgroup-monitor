#pragma once

#include <ctime>
#include <iostream>

namespace monitor {

    namespace utils {

	namespace logging {

	    std::string get_time ();
	    void content_print ();
	    
	    template <typename T>
	    void content_print (T a) {
		std::cout << a;
	    }


	    
	    template <typename T, typename ... R>
	    void content_print (T a, R... b) {
		std::cout << a << " ";
		content_print (b...);
	    }
	    

	    const std::string PURPLE = "\e[1;35m";
	    const std::string BLUE = "\e[1;36m";
	    const std::string YELLOW = "\e[1;33m";
	    const std::string RED = "\e[1;31m";
	    const std::string GREEN = "\e[1;32m";
	    const std::string BOLD = "\e[1;50m";
	    const std::string UNDERLINE = "\e[4m";
	    const std::string RESET = "\e[0m"; 

	    template <typename ... T>
	    void info (T... msg) {
		std::cout << "[" << BLUE << "INFO" << RESET << "][" << get_time () << "] ";
		content_print (msg...);
		std::cout << std::endl;
	    }

	    template <typename ... T>
	    void error (T... msg) {
		std::cout << "[" << RED << "ERROR" << RESET << "][" << get_time () << "] ";
		content_print (msg...);
		std::cout << std::endl;
	    }

	    template <typename ... T>
	    void warn (T... msg) {
		std::cout << "[" << YELLOW << "WARNING" << RESET << "][" << get_time () << "] ";
		content_print (msg...);
		std::cout << std::endl;
	    }

	    template <typename ... T>
	    void success (T... msg) {
		std::cout << "[" << GREEN << "SUCCESS" << RESET << "][" << get_time () << "] ";
		content_print (msg...);
		std::cout << std::endl;
	    }

	    template <typename ... T>
	    void strange (T... msg) {
		std::cout << "[" << PURPLE << "STRANGE" << RESET << "][ " << get_time () << "] ";
		content_print (msg...);
		std::cout << std::endl;
	    }

	}
	
    }    

}
