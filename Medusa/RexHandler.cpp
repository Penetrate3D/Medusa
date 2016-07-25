#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stack>
#include <algorithm>
#include <cassert>


#include "RexHandler.h"
#include "rexchar.h"



string type_prompt[] = {
	"keyword",
	"operator",
	"comparasion",
	"braket",
	"number",
	"string",
	"identifier",
	"comment",
	"space",
	"newline"
};

static int trans[50][256];

static char* re2post(const char *re)
{
	int nalt, natom;
	static char buf[1000];
	char *dst;
	struct {
		int nalt;
		int natom;
	} paren[100], *p;
	
	p = paren;
	dst = buf;
	nalt = 0;
	natom = 0;
	if(strlen(re) >= sizeof buf/2)
		return NULL;
	for(; *re; re++){
		switch(*re){
		case '(':
			if(natom > 1){
				--natom;
				*dst++ = '.';
			}
			if(p >= paren+100)
				return NULL;
			p->nalt = nalt;
			p->natom = natom;
			p++;
			nalt = 0;
			natom = 0;
			break;
		case '|':
			if(natom == 0)
				return NULL;
			while(--natom > 0)
				*dst++ = '.';
			nalt++;
			break;
		case ')':
			if(p == paren)
				return NULL;
			if(natom == 0)
				return NULL;
			while(--natom > 0)
				*dst++ = '.';
			for(; nalt > 0; nalt--)
				*dst++ = '|';
			--p;
			nalt = p->nalt;
			natom = p->natom;
			natom++;
			break;
		case '*':
		case '+':
		case '?':
			if(natom == 0)
				return NULL;
			*dst++ = *re;
			break;
		default:
			if (*re == '\\')
				re++;
			if(natom > 1){
				--natom;
				*dst++ = '.';
			}
			while (*re=='\\')
				*dst++ = *re++;
			*dst++ = *re;
			natom++;
			break;
		}
	}
	if(p != paren)
		return NULL;
	while(--natom > 0)
		*dst++ = '.';
	for(; nalt > 0; nalt--)
		*dst++ = '|';
	*dst = 0;
	return buf;
}


static void pop2(stack<Fragment*>& stk,Fragment** f1,Fragment** f2)//形参是复制的，想要传递出指针，必须传入二级指针
{
	*f2 = stk.top();
	stk.pop();
	*f1 = stk.top();
	stk.pop();
}

static Fragment* connect(Fragment* f1,Fragment* f2)
{
	vector<state**>::iterator iter = f1->end.begin();
	for(;iter<f1->end.end();++iter)
		**iter = f2->start;

	Fragment* newf =  new Fragment(f1->start,f2->end);
	DELETE(f1);
	DELETE(f2);
	return newf;
}


static Fragment* append(Fragment* f1,Fragment* f2)
{
	state* s = f1->start;
	s->AddEdge(new Link(EPSILON, f2->start));
	//隐藏数据很重要，addedge操作应该调用class的成员函数，否则links未自增，导致assert错误

	/*
	state* s = new state();
	Link* l1 = new Link(EPSILON,f1->start);
	Link* l2 = new Link(EPSILON,f2->start);

	s->AddEdge(l1);
	s->AddEdge(l2);
	*/

	vector<state**>::iterator begin,end;					//改进后的‘|’节点构造方法大大减少了自动机的state数量
	begin = f2->end.begin();
	end = f2->end.end();

	f1->end.insert(f1->end.end(),begin,end);
	Fragment* newf = new Fragment(s,f1->end);
	

	DELETE(f1);
	DELETE(f2);
	return newf;
}

static void connect(Fragment* f,state* s)
{
	for (size_t i = 0; i<f->end.size(); ++i)
		*f->end[i] = s;

//	DELETE(f);
}


state::state() :vec(State_Num), links(0){
	++State_Num;
}

state::~state()
{
	assert(links == edges.size());

	for (size_t i = 0; i < edges.size(); ++i)
	{
		delete edges[i];
	}
}

RexHandler::RexHandler(const vector<pair<string, int>>& s) :start(NULL), rex(s){

}

RexHandler::~RexHandler(){
//	cout << "rexhander dtor is runing..." << endl;

	DELETE(start);

	for (size_t i = 0; i < nodes.size(); ++i)
	{
		delete nodes[i];
	}

	for (size_t i = 0; i < match.size(); ++i)
	{
		delete match[i];
	}
}

//start(1) -EPSILON-> state machine -EPSILON-> matchs(n)
state* RexHandler::CreateNfa()
{
	start = new state();

	for (size_t i = 0; i<rex.size(); ++i)
	{
		const char* re = rex[i].first.c_str();

		stack<Fragment*> stk;

		Fragment* f1,*f2,*newf;
		state* news;
		Link* l1,*l2,*newl;

		if(!re) cout<<"regular exp error: no input"<<endl;
		while(*re)
		{
			//case分支不算一个块，之间不能重复定义变量
			switch(*re)
			{
			case '.':
				pop2(stk,&f1,&f2);
				stk.push(connect(f1,f2));
				break;
			case '|':
				pop2(stk,&f1,&f2);
				stk.push(append(f1,f2));	
				break;
			case '*':
				f1 = stk.top();
				stk.pop();
				news = new state();
				nodes.push_back(news);
				l1 = new Link(EPSILON,NULL);
				l2 = new Link(EPSILON,f1->start);
				news->AddEdge(l1);
				news->AddEdge(l2);
				connect(f1,news);												//后续会释放f1，这里不能释放，这个错误检查了好久，fuck you？
				stk.push(new Fragment(news,vector<state**>(1,&l1->end)));		//	->和[] 优先级高于++ & *
				DELETE(f1);								
				break;
			case '?':
				f1 = stk.top();
				stk.pop();
				news = new state();
				nodes.push_back(news);
				l1 = new Link(EPSILON,NULL);
				l2 = new Link(EPSILON,f1->start);
				news->AddEdge(l1);
				news->AddEdge(l2);
				newf = new Fragment(news,vector<state**>(1,&l1->end));
				newf->end.insert(newf->end.end(),f1->end.begin(),f1->end.end());//这里写错过，导致?不能用
				stk.push(newf);
				DELETE(f1);
				break;
			case '+':
				f1 = stk.top();
				stk.pop();
				news = new state();
				nodes.push_back(news);
				l1 = new Link(EPSILON,NULL);
				l2 = new Link(EPSILON,f1->start);
				news->AddEdge(l1);
				news->AddEdge(l2);
				connect(f1,news);
				newf = new Fragment(f1->start,vector<state**>(1,&l1->end));
				stk.push(newf);
				DELETE(f1);
				break;
			default:
				if (*re == '\\')
					re++;
				news = new state();
				nodes.push_back(news);
				newl = new Link(int(*re),NULL);
				news->AddEdge(newl);									
				stk.push(new Fragment(news,vector<state**>(1,&newl->end)));
				break;
			}

			++re;
		}
	

		assert(stk.size() == 1);

		f1 = stk.top();
		stk.pop();

		newl = new Link(EPSILON,f1->start);				//自动机的起始状态
		start->AddEdge(newl);
		news = new state();			//自动机的接受状态
		connect(f1,news);
		DELETE(f1);		//释放栈中的唯一fregment

		match.push_back(news);
	}

	assert(match.size() == rex.size());
	return start;
}

//不动点算法
void RexHandler::Closure(vector<state*>* v,state* s)
{
	if (find(v->begin(), v->end(), s) != v->end())
		return;

	v->push_back(s);

	for (size_t i = 0; i<s->edges.size(); i++)
	{
		if(s->edges[i]->value==EPSILON)
			Closure(v,s->edges[i]->end);
	}
}

void RexHandler::step(vector<state*>* c,vector<state*>* n,char str)
{

	for (size_t i = 0; i<c->size(); i++)
	{
		state* s = (*c)[i];
		for (size_t j = 0; j<s->edges.size(); j++)
		{
			Link* l = s->edges[j];
			if(l->value==str)
				Closure(n,l->end);
		}
	}
}

bool RexHandler::SimulateNfa(const string& t,int type)//type = ID,NUM,COMMENT...判断string是否符合正则规则
{
	const char* cptr = t.c_str();
	vector<state*> clist,nlist,*pc,*pn,*tmp;
	pc = &clist;
	pn = &nlist;

	Closure(pc,start);

	while(*cptr)
	{
		step(pc,pn,*cptr);

		if(pn->size()==0)
			return false;

		tmp = pc;
		pc = pn;
		pn = tmp;

		pn->clear();
		cptr++;
	}

	for (size_t i = 0; i<pc->size(); i++)
		if((*pc)[i] == match[type])			//这里把==写成=,wtf?
			return true;
	return false;
}


void RexHandler::Nfa2Dfa()
{
	int cDState;
	
	vector<state*> s;
	Closure(&s,start);
	sort(s.begin(),s.end());
	++DState_Num;
	DState.insert(pair<vector<state*>,int>(s,DState_Num));

	stack<vector<state*>> stk;
	stk.push(s);

	ofstream out("./data/trans.txt",ios::beg);
	map<vector<state*>,int>::iterator iter;
	while(!stk.empty())
	{
		vector<state*> now = stk.top();
		stk.pop();

		cDState = DState.find(now)->second;
		for(int c=1;c<256; ++c)
		{
			vector<state*> next;
			step(&now,&next,char(c));
			if(next.size()==0)					//has next dstate
				continue;
			sort(next.begin(),next.end());

			iter = DState.find(next);
			if( iter == DState.end())				//dstate doesn't exist in map 
			{
				++DState_Num;
				DState.insert(pair<vector<state*>,int>(next,DState_Num));
				out<<cDState<<'\t'<<c<<'\t'<<DState_Num<<endl;
				stk.push(next);
			}
			else
				out<<cDState<<'\t'<<c<<'\t'<<iter->second<<endl;
		}
	}

	cout<<"dstate has "<<DState_Num<<endl;
	out.close();

	assert(DState_Num == DState.size());

	for(iter=DState.begin();iter!=DState.end();++iter)
	{
		for (size_t i = 0; i<rex.size(); ++i)
		{
			if(find(iter->first.begin(),iter->first.end(),match[i])!=iter->first.end())
			{
				DMatch.insert(pair<int,int>(iter->second,rex[i].second));
				break;
			}
		}
	}
}

void RexHandler::CreateTrans()
{
	ifstream in("./data/trans.txt");
	int i,j,v;
	while(in>>i>>j>>v)
	{
		trans[i][j]=v;
	}

	ofstream out("./data/transmatrix.txt",ios::beg);
	for(i=0;i<=DState_Num;++i)
	{
		for(j=0;j<256;++j)
			out<<trans[i][j]<<'\t';
		out<<endl;
	}


	in.close();
	out.close();

	out.open("./data/dstate.txt");
	map<int,int>::iterator iter = DMatch.begin();
	out<<DState_Num<<endl;
	for(;iter!=DMatch.end();++iter)
		out<<iter->first<<'\t'<<iter->second<<endl;


}

int RexHandler::Scan(string s)
{
	const char* c = s.c_str();
	int now = 1;
	while(*c)
	{
		now = trans[now][*c];
		++c;
	}

	map<int,int>::iterator iter;
	if((iter=DMatch.find(now))!=DMatch.end())
	{
		cout<<"matched, and type is "<<type_prompt[iter->second]<<endl;
		return true;
	}
	else
		cout<<"match failed"<<endl;
	return false;
	
}


//int main(int argc, char* argv[])
//{
//	vector<pair<string,int>> rexs;
////	regexps.push_back(REX_NEWL+REX_NEWL);
//	string s0("def|if|while|return|break|continue|else");
//	string s1("\\\\+|-|\\\\*|/|=");
//	string s2("<|>|==|>=|<=|!=");
//	string s3("\\(|\\)|{|}");
//	string s4("-?"+REX_NUMBER+REX_ADD);
//	string s9("\"" + REX_LPAREN + REX_CHAR + "|" + REX_NUMBER + "|" + REX_WHITE + REX_RPAREN + REX_ADD +"\"");
//	string s5(REX_CHAR+REX_LPAREN+REX_CHAR+REX_OR+REX_NUMBER+REX_RPAREN+REX_MUL);
//	string s6("#"+REX_LPAREN+REX_CHAR+REX_OR+REX_NUMBER+REX_OR+REX_WHITE+REX_RPAREN+REX_MUL+REX_NEWL);
//	string s7(REX_WHITE);
//	string s8(REX_NEWL);
//	
//	char* out = re2post(s0.c_str());
//	rexs.push_back(pair<string, int>(string(out), KEYWORD));
//
//	out = re2post(s1.c_str());
//	rexs.push_back(pair<string, int>(string(out), OPERATION));
//
//	out = re2post(s2.c_str());
//	rexs.push_back(pair<string, int>(string(out), COMPARASION));
//
//	out = re2post(s3.c_str());
//	rexs.push_back(pair<string, int>(string(out), BRAKET));
//
//	out = re2post(s4.c_str());
//	rexs.push_back(pair<string, int>(string(out), NUM));
//
//	out = re2post(s9.c_str());
//	rexs.push_back(pair<string, int>(string(out), STRING));
//
//	out = re2post(s5.c_str());
//	rexs.push_back(pair<string, int>(string(out), ID));
//
//	out = re2post(s6.c_str());
//	rexs.push_back(pair<string, int>(string(out), COMMENT));
//
//	out = re2post(s7.c_str());
//	rexs.push_back(pair<string,int>(string(out),SPACE));
//
//	out = re2post(s8.c_str());
//	rexs.push_back(pair<string, int>(string(out), NEWL));
//
//	for (int i = 0; i < rexs.size(); ++i)
//		cout << rexs[i].first << endl;
//
//	//	cout<<(int)(char('\n'))<<endl;
//	//	RexHandler rh(s);					//写成RexHandler a(string(out)) 被解释成函数定义，WTF?
//	RexHandler rh(rexs);
//	state* f = rh.CreateNfa();
//		
//	rh.Nfa2Dfa();
//	rh.CreateTrans();
//
//	ifstream in("./test/test.c");
//	  string test;
//	while(getline(in,test))
//		rh.Scan(test);
//
//	while (cin>>test)	//cin不能读\t 和 \n 日狗
//		rh.Scan(test);
//
//	return 1;
//}


