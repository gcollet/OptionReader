/* Copyright (c) 2010 Guillaume Collet
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 */

#ifndef __OPTIONS_H_INCLUDED__
#define __OPTIONS_H_INCLUDED__

#include <map>
#include <string>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdexcept>

using namespace std;

class Arg {
protected:
	string	_small_flag;	/**< The small version of the argument flag */
	string	_long_flag;		/**< The long version of the argument flag */
	string	_description;	/**< The description of the argument (for usage printing)*/
	bool		_needValue;		/**< This boolean value is true when a value is needed */
	bool		_isSet;
	/* Protected constructor, only called by inherited classes */
	Arg(const string & small_flag, const string & long_flag, const string & description, const bool & needed, const bool & set):
	_small_flag(small_flag), _long_flag(long_flag),_description(description),	_needValue(needed), _isSet(set)
	{};
	
public:
	
	/**
	 * The destructor is virtual
	 */
	virtual ~Arg(){};
	
	/**
	 * Returns the argument small flag.
	 */
	const string getSmallFlag() const {return _small_flag;};
	
	/**
	 * Returns the argument long flag.
	 */
	const string getLongFlag() const {return _long_flag;};
	
	/**
	 * Returns the argument description.
	 */
	string getDescription() const {return _description;};
	
	/**
	 * Indicates whether the argument needs a value or not.
	 */
	bool isNeeded() const {return _needValue;};
	
	/**
	 * Indicates whether the argument is setted or not.
	 */
	bool isSetted() const {return _isSet;};
	
	/**
	 * Sets the value of the argument.
	 */
	virtual void setValue(string val){};
	
	/**
	 * Try to find the flag in the command line
	 * if needed and not found -> error
	 */
	virtual void find(vector<string> & command_line){};
};

/**
 * ValueArg is an argument with a value of type T
 */
template <typename T>
class ValueArg : public Arg {
private:
	T _value;
	
public:
	ValueArg(const string & small_flag,
					 const string & long_flag,
					 const string & description,
					 const T value
					 ):Arg(small_flag,long_flag,description,false,true), _value(value){};
	
	ValueArg(const string & small_flag,
					 const string & long_flag,
					 const string & description
					 ):Arg(small_flag,long_flag,description,true,false){};
	
	T getValue() const {return _value;};
	
	void setValue(string val) {
		istringstream is(val);
		while (is.good()) {
			is >> _value;
		}
	}
	
	void  find(vector<string> & command_line){
		vector<string>::iterator it = command_line.begin();
		while (it != command_line.end()){
			if (*it == _small_flag || *it == _long_flag){
				it = command_line.erase(it);
				if (it == command_line.end()) {
					throw runtime_error("Value of argument " + _small_flag + ", " + _long_flag + " is missing\n");
				}
				istringstream is(*it);
				while (is.good()) {
					is >> _value;
				}
				command_line.erase(it);
				_isSet = true;
				return;
			}
			it++;
		}
		if (_needValue && !_isSet){
			throw runtime_error("Argument " + _small_flag + ", " + _long_flag + " is needed\n");
		}
	}
};


/**
 * SwitchArg is an argument with a boolean 
 * it is intrinsically not needed -> _needValue = false
 */
class SwitchArg : public Arg {
private:
	bool _value;
public:
	SwitchArg(const string & small_flag,
						const string & long_flag,
						const string & description,
						const bool & value):
	Arg(small_flag,long_flag,description,false,false),
	_value(value)
	{};
	
	bool getValue() const {return _value;};
	void setValue(string val) {_value = true;};
	
	void  find(vector<string> & command_line){
		vector<string>::iterator it = command_line.begin();
		while (it != command_line.end()){
			if (*it == _small_flag || *it == _long_flag){
				if (_small_flag == "-h"){
					throw runtime_error("");
				}
				command_line.erase(it);
				_value = true;
				return;
			}
			it++;
		}
	}
	
};

class Options
{
private:
	string appName;
	vector<string> command_line;
	map<string, Arg *> arg_list;	/**< Map of argument objects by their flag */
	
	
	/*
	 * Constructor is private
	 */
	Options(){};
  
	/* 
	 * Accessor for the uniq Options object
	 */
	static Options & GetNC(){
		static Options opt;
		return opt;
	}
  
	/*
	 * getEnvVar : Get an environment variable of name env
	 */
	string getEnvVar(string env){
		char * env_p;
		string env_s;
		env_p = getenv(env.c_str());
		if (env_p != NULL){
			env_s = env_p;
		}
		return env_s;
	}
    
	/*
	 * Reduce a pathname in a basename
	 */
	string basename(string fname){
		int pos = fname.find_last_of('/');
		return fname.substr(pos+1, fname.size() - pos);
	};
	
	/*
	 * Set the command line from argv
	 */
	void setCommandLine(int argc, char * const argv[]){
		for (int i(1); i < argc; i++){
			command_line.push_back(argv[i]);
		}
	}
	
	/*
   * Initialize the options (This is where you edit the file)
   */
	void Init(int argc, char * const argv[]) {
		try {
			// Set the command_line object
			setCommandLine(argc,argv);
			
			// Set the application name
			appName = basename(argv[0]);
			
			/*
			 * You can add 2 kind of arguments :
			 *   A ValueArg which is for flags with values
			 *   A SwitchArg which is for boolean flags
			 *
			 * Be careful with ValueArg declaration, the 3 first parameters must be there: small_flag, long_flag, description.
			 * The fourth is optional and is a default value for the argument. 
			 * If it is not given, then the flag must be in the command line to set the value (-i option in this example).
			 */
			
			/** Set options
			 *	3 lines are needed to set up an option
			 */
			
			//1 - create the argument as a ValueArg or SwitchArg.
			ValueArg<string>	* sArg = new ValueArg<string>( "-s", "--string",  "Set a string value", "");
			ValueArg<int>			* iArg = new ValueArg<int>(    "-i", "--integer", "Set an integer value");
			ValueArg<float>		* fArg = new ValueArg<float>(  "-f", "--float",   "Set a float value [default=1.25]", 1.25);
			ValueArg<double>	* dArg = new ValueArg<double>( "-d", "--double",  "Set a double value [default=3.0]", 3.0);
			SwitchArg					* bArg = new SwitchArg(        "-b", "--boolean", "Set a boolean value to true", false);
			SwitchArg					* hArg = new SwitchArg(        "-h", "--help",    "Print this help", false);
			
			// 2 -  add the argument to the arg_list for further use (print_usage).
			arg_list[sArg->getSmallFlag()] = sArg;
			arg_list[iArg->getSmallFlag()] = iArg;
			arg_list[fArg->getSmallFlag()] = fArg;
			arg_list[dArg->getSmallFlag()] = dArg;
			arg_list[bArg->getSmallFlag()] = bArg;
			arg_list[hArg->getSmallFlag()] = hArg;
			
			// 3 - try to find the argument in the command line to set up the value.
			hArg->find(command_line);
			sArg->find(command_line);
			iArg->find(command_line);
			fArg->find(command_line);
			dArg->find(command_line);
			bArg->find(command_line);
			
			// If something is left in the command line... It is not an argument of the program -> error
			if (command_line.size() > 0){
				throw runtime_error("Unknown flag: " + command_line[0] + "\n");
			}
			
			// Get arguments
			string_example = sArg->getValue();
			int_example    = iArg->getValue();
			float_example  = fArg->getValue();
			double_example = dArg->getValue();
			bool_example   = bArg->getValue();

		} catch (exception &e) {
        throw;
		}
	}

public:
	/*
	 * List of options (Edit to add options)
	 */
	string string_example;
	int    int_example;
	float  float_example;
  double double_example;
	bool   bool_example;
	
	/* Destructor */
	~Options(){
		Options & opt = GetNC();
		map<string,Arg *>::iterator it = opt.arg_list.begin();
		while (it != opt.arg_list.end()){
			delete it->second;
			it++;
		}
	};
	
	/* Universal accessor */
	static Options const & Get(){
		return GetNC();
	}
  
	/* Initialization */
	static void Parse(int argc_, char * const argv_[]) {
		Options & opt = GetNC();
		try{
			opt.Init(argc_,argv_);
		} catch (exception &e) {
			throw;
		}
	}
	
	/* Print usage */
	static void print_usage(){
		Options & opt = GetNC();
		
		map<string,Arg *>::iterator it = opt.arg_list.begin();
		int sflag_size = 0;
		int lflag_size = 0;
		int desc_size = 0;
		while (it != opt.arg_list.end()){
			if (it->second->getSmallFlag().size() > sflag_size) {
				sflag_size = it->second->getSmallFlag().size();
			}
			if (it->second->getLongFlag().size() > lflag_size) {
				lflag_size = it->second->getLongFlag().size();
			}
			if (it->second->getDescription().size() > desc_size) {
				desc_size = it->second->getDescription().size();
			}
			it++;
		}
		cerr << "Usage: " << opt.appName ;
		it = opt.arg_list.begin();
		int nb = 1;
		while (it != opt.arg_list.end()){
			if (it->second->isNeeded()) {
			  cerr << " " << it->second->getSmallFlag() << " arg_" << nb;
				nb++;
			}
			it++;
		}
		cerr << " [options]\n\nOptions:\n";
		it = opt.arg_list.begin();
		while (it != opt.arg_list.end()){
			string flag = it->second->getSmallFlag() + ",";
			cerr << "   " << setw(sflag_size+1) << left << flag;
			cerr << " " << setw(lflag_size) << left << it->second->getLongFlag()<< " : ";
			cerr << setw(desc_size) << left << it->second->getDescription() << "\n";
			it++;
		}
		cerr << "\n";
	}
};

#endif

