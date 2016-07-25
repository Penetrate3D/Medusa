#ifndef REXHANDLER_H
#define REXHANDLER_H

#include <vector>
#include <map>

using namespace std;

const int EPSILON = 257;

static int State_Num = 0;
static int DState_Num = 0;

enum {
	ERROR = -1,
	KEYWORD = 0,
	OPERATION,
	COMPARASION,
	BRAKET,
	NUM,
	STRING,
	ID,
	COMMENT,
	SPACE,
	NEWL
};

extern string type_prompt[];

class Link;

class state
{
public:
	vector<Link*> edges;
	int vec;

	int links;

	state();
	~state();

	state* AddEdge(Link* l)
	{
		++links;
		edges.push_back(l);
		return this;
	}

};

class Link{
public:
	int value;
	state* end;

	Link(int v,state* e):value(v),end(e){}
};

class Fragment{
public:
	state* start;

	vector<state**> end;//代表状态自动机单元指向的state的指针

	Fragment(state* s,vector<state**> e):start(s),end(e){ }
};

class RexHandler{
private:
	vector<pair<string,int>> rex;
	vector<state*> match;

	state* start;
	vector<state*> nodes;   //所有的状态节点
	map<vector<state*>,int> DState;		//DFA中的状态集合,int代表dstate编号

	map<int,int> DMatch;			//包含接受状态match的dstate,int为dstate编号，string为type

public:
	RexHandler(vector<pair<string, int>>);
	~RexHandler();

	state* CreateNfa();

	bool SimulateNfa(string target,int);

	void Nfa2Dfa();
	void CreateTrans();
	int Scan(string);

	int NodesSize(){return nodes.size();}
	

private:

	void step(vector<state*>*,vector<state*>*,char);
	void Closure(vector<state*>* ,state*);			//求解状态点的闭包

};

#endif