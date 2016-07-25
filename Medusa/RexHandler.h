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

	vector<state**> end;//����״̬�Զ�����Ԫָ���state��ָ��

	Fragment(state* s,vector<state**> e):start(s),end(e){ }
};

class RexHandler{
private:
	vector<pair<string,int>> rex;
	vector<state*> match;

	state* start;
	vector<state*> nodes;   //���е�״̬�ڵ�
	map<vector<state*>,int> DState;		//DFA�е�״̬����,int����dstate���

	map<int,int> DMatch;			//��������״̬match��dstate,intΪdstate��ţ�stringΪtype

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
	void Closure(vector<state*>* ,state*);			//���״̬��ıհ�

};

#endif