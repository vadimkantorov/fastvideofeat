#include <string>
#include <fstream>
#include <stdexcept>

using namespace std;

#ifndef __IO_UTILS_H__
#define __IO_UTILS_H__

bool FileExists(string file)
{
	ifstream f(file.c_str());
	return f.good();
}

void AssertFileExists(string file, string comment = "")
{
	if(!FileExists(file))
		throw std::runtime_error("File (" + comment + ") doesn't exist: '" + file +"'");
}

static const char * yes = "yes";
static const char * no = "no";

const char* yesno(bool b)
{
	return b ? yes : no;
}

string GetFileExtension(string filePath)
{
	int dotPos = filePath.find_last_of(".");
	return filePath.substr(dotPos);
}

#endif
