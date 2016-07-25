#include <iostream>

#include "token.h"
#include "paser.h"

#include "vld.h"

#define TRANS(x,y) trans[x*256+y]

vector<line*> lines;

//读取词法分析的结果 状态转移矩阵 和 状态表
CLex CScan;

class FileLex{
private:
	CLex* scan;
	string source;
	string target;

	ifstream in;

public:
	FileLex(string& s, string& t) :source(s), target(t), scan(&CScan)
	{
		in.open(source);
	}

	~FileLex()
	{
		in.close();
	}

	bool ScanSource()
	{
		return scan->run(in, target);
	}

	void printfObj();
};


CLex::CLex()
{
	ifstream in("./data/dstate.txt");
	in>>state;
	int match_id;
	int type;
	while(in>>match_id>>type)
		match.insert(pair<int,int>(match_id,type));
	in.close();

	++state;
	trans = new char*[state];

	char* area = new char[state*256];
	for(int i=0;i<state;++i)
		trans[i] = area+i*256;


	in.open("./data/transmatrix.txt");
	int w;
	for(int i=0;i<state;i++)
		for(int j=0;j<256;j++)
		{
			in>>w;
			trans[i][j]=w;
		}

	in.close();
}


CLex::~CLex()
{
	delete[] trans[0];

	delete[] trans;
}

bool CLex::run(ifstream& source,string& target)
{
	string cline;
	map<int,int>::iterator iter;
	int l = 0;


//	ofstream t(target,ios::beg);

	while(getline(source,cline))
	{
//		cline.append("\n");			//不需要加入newline
		++l;

		if (cline.empty())			
			continue;
		size_t start,end;
		size_t lastmatchpos = 0;
		size_t lastmatchtype = ERROR;
		size_t now = 1;
		start = end =0;

		line *newl = new line(l);
		char c;
		while(end<cline.size())
		{
			c = cline[end++]; 
			now = trans[now][c];

			if((iter = match.find(now))!=match.end())
			{
				lastmatchpos = end;
				lastmatchtype = iter->second;
			}

			if(now == 0||end==cline.size())
			{		
				if(lastmatchpos==start)
				{
					cout<<l<<":"<<start+1
						<<" error: the word "<<"\""
						<<cline.substr(start,end-start)<<"\""
						<<" undefined!\n";
					return false;
				}
				else
				{
					string tkn = cline.substr(start,lastmatchpos-start);
					if (lastmatchtype != SPACE)
						newl->tokens.push_back(new token(tkn,start+1,lastmatchtype));
//					t<<tkn<<"("<<lastmatchtype<<")"<<";";
					start = end = lastmatchpos;
					now = 1;
				}
			}
		}
		lines.push_back(newl);
//		t<<endl;
	}

	return true;
}

void FileLex::printfObj()
{
	ofstream out(target);
	for (size_t i = 0; i<lines.size(); ++i)
	{
		out<<"line "<<lines[i]->l<<"	: ";
		line * newl = lines[i];
		for (size_t j = 0; j<newl->tokens.size(); ++j)
		{
			token* newt = newl->tokens[j];
			out<<newt->value<<"("<<type_prompt[newt->type]<<")"<<' ';
		}
		out << endl;
	}

	out.close();
}


int main()
{
	string source("./test/test.c");
	string target("./test/test.o");
	FileLex *fScan = new FileLex(source,target);

	if (!fScan->ScanSource())
		return -1;
	fScan->printfObj();

	MDSInterpretor CPaser(lines);
	CPaser.init();
	CPaser.interpret();

	delete fScan;
	return 0;
}

