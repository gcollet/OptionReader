#include <iostream>
#include "options.h"

using namespace std;

int main (int argc, char ** argv){
	/* 
	 * Parses command line 
	 */
	try {
		Options::Get().Parse(argc, argv);
	}
	catch (exception &e) {
		cerr << e.what() << "\n";
		Options::Get().print_usage();
		exit(0);
	}
	
	cout << "Argument of this program are :\n";
	cout << " string  value = " << Options::Get().string_example << "\n";
	cout << " integer value = " << Options::Get().int_example << "\n";
	cout << " float   value = " << Options::Get().float_example << "\n";
	cout << " double  value = " << Options::Get().double_example<< "\n";
	if (Options::Get().bool_example){
		cout << " boolean value = true\n";
	} else {
		cout << " boolean value = false\n";
	}
	return 0;
}