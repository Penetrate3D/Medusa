#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <vector>
#include <fstream>
#include <map>

#include "RexHandler.h"
using namespace std;


//between [start,end)
class token{
public:
	size_t start;
	size_t end;

	string value;
	char type;

	token(string v, size_t l, size_t t) :value(v), start(l), type(t)
	{
		end = start+value.size();
	}
};

class line 
{
public:
	size_t l;
	vector<token*> tokens;

	line(int i):l(i){ }
	~line()
	{
		for (size_t i = 0; i < tokens.size(); ++i)
			delete tokens[i];
	}
};


class CLex
{
private:
	char** trans;
	map<int,int> match;

	int state;

public:
	CLex();
	~CLex();
	bool run(ifstream& source,string& outfile);
};

extern vector<line*> lines;

#endif 