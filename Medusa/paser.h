#ifndef PASER_H
#define PASER_H

#pragma warning(disable:4996)

#include <vector>
#include <string>
#include <iostream>
#include <list>

#include "token.h"
#include "mds_object.h"
#include "mem_allocator.h"
#include "mds_buildin_func.h"

using namespace std;

extern MdsAllocator mds_allocator;
extern list<MdsObject*> mds_stack;
extern list<MdsEnv*> mdslocal_list;
extern MdsEnv global_env;

class MdsFunc;

template<class T>
void free_vector(vector<T*> f)
{
	for (size_t i = 0; i < f.size(); ++i)
	{
		delete f[i];
	}
}

typedef enum{
	NULL_NODE = 0,
	LITERAL_NODE,
	VARIABLE_NODE,
	BINARY_NODE,
	CALLFUNC_NODE
}AstNodeType;


typedef enum{
	LITERAL_STRING = 30,
	LITERAL_INT,
	LITERAL_FLOAT
}LiteralType;


class NodeAST{
public:
	NodeAST(){
	}
	virtual ~NodeAST(){  }
	virtual void print(int n) { return; }//每一个派生类必须定义一份纯虚函数的实现版本

	//因为是动态类型的语言，所以静态语言中的类型检查不用做，仅检查符号表相关项
	virtual void eval(MdsEnv* e) { }

	virtual int NodeType(){ return NULL_NODE; }

	size_t line = 0;
	vector<token*> contents;
};

class LiteralNodeAST :public NodeAST{
	union{
		int i;
		char* c;
	}value;
	
	LiteralType type;
public:
	~LiteralNodeAST();

	LiteralNodeAST(int x, token* t);

	LiteralNodeAST(float f, token* t);

	LiteralNodeAST(char* x, token* t);

	void eval(MdsEnv* e);

	void print(int n);

	int NodeType(){ return LITERAL_NODE; }

	LiteralNodeAST* operator+(LiteralNodeAST &r);
	LiteralNodeAST* operator-(LiteralNodeAST &r);
	LiteralNodeAST* operator*(LiteralNodeAST &r);
	LiteralNodeAST* operator/(LiteralNodeAST &r);
};

//变量的eval求解过程可能使用户程序出现未知错误
//因为每个block将会定义有一个环境量，如果没有找到该变量则在新环境量中定义，只为MD_NULL
//将MD_null返回继续用户程序的运行，可能造成未知的后果
//

class VariableNodeAST :public NodeAST
{
public:
	string VariableName;

	VariableNodeAST(token* t);

	void print(int n);

	void eval(MdsEnv* e);

	~VariableNodeAST(){  }

	int NodeType(){ return VARIABLE_NODE; }
};

class BinaryNodeAST :public NodeAST
{
	//只保存符号的token位置，打印时递归查看左右节点的token
	OperatorType BinaryOp;
	NodeAST* _lhs, *_rhs;

	void constant_folding();

public:
	BinaryNodeAST(token* op, NodeAST* lhs, NodeAST* rhs);

	~BinaryNodeAST();

	void print(int n);

	void eval(MdsEnv* e);

	int NodeType(){ return BINARY_NODE; }

	OperatorType opstring_2_type(string op);
};


class CallNodeAST :public NodeAST{
	string Callee;
	vector<NodeAST*> args;

public:
	CallNodeAST(token* t, vector<NodeAST*>& list);

	~CallNodeAST();

	void print(int n);

	void eval(MdsEnv* e);

	int NodeType(){ return CALLFUNC_NODE; }
};


typedef enum{
	CONTINUE = 0,
	BREAK,
	RETURN,
	NORMAL,
	STATEMENT_NO_RET
}StatementResultType;

class StatementResult{
public:
	StatementResultType type = STATEMENT_NO_RET;
	MdsObject* res = NULL;

	StatementResult(){};
	StatementResult(StatementResultType t, MdsObject* o)
		:type(t), res(o){}
};

class Statement{
public:
	Statement(){};
	virtual ~Statement() { };
	virtual StatementResult excute(MdsEnv* e) { 
		return StatementResult(); 
	}
};

class ReturnStatement :public Statement{
	NodeAST* _ret_value;
public:
	ReturnStatement(NodeAST* ret) :
		_ret_value(ret){
	}

	~ReturnStatement(){
		delete _ret_value;
	}

	StatementResult excute(MdsEnv* e)
	{
		_ret_value->eval(e);
		MdsObject* r = mds_stack.front();
		mds_stack.pop_front();
		return StatementResult(RETURN, r);
	}
};

class ExpressionStatement :public Statement
{
	NodeAST* expression;
public:
	ExpressionStatement(NodeAST* a) : expression(a){
	}

	~ExpressionStatement(){
		delete expression;
	}

	StatementResult excute(MdsEnv* e){
		expression->eval(e);
		MdsObject* r = mds_stack.front();
		mds_stack.pop_front();
		return StatementResult(NORMAL, r);
	}

};

class CondStatement :public Statement
{
	NodeAST* cond;
	vector<Statement*> block1;
	vector<Statement*> block2;

public:
	CondStatement(NodeAST* c, vector<Statement*>& b1, vector<Statement*>& b2) :
		cond(c), block1(b1),block2(b2){ }

	~CondStatement(){
		delete cond;
		free_vector(block1);
		free_vector(block2);
	}

	StatementResult excute(MdsEnv* e);
};

class LoopStatement :public Statement
{
	NodeAST* cond;
	vector<Statement*> block;

public:
	LoopStatement(NodeAST* c, vector<Statement*>& b) :
		cond(c), block(b){};

	~LoopStatement(){
		delete cond;
		free_vector(block);
	}

	StatementResult excute(MdsEnv* e);
};

class MdsFunc{
public:
	bool is_buildin_func;

	MdsFunc(bool t) :is_buildin_func(t){ }
	virtual ~MdsFunc(){}
};

class MdsDefFunction:public MdsFunc
{//自定义函数的对象
public:
	vector<string> _args;
	vector<Statement*> _body;

	~MdsDefFunction(){
		free_vector(_body);
	}

	MdsDefFunction(vector<string>& args, vector<Statement*>& body)
		:MdsFunc(false),  _args(args), _body(body){ }
};

class MdsBuildinFunction :public MdsFunc
{
public:
	buildin_func func;

	MdsBuildinFunction(buildin_func f) :MdsFunc(true), func(f){ }
};


class MDSInterpretor{
private:
	int _lineIndex = 0;
	int _index = 0;

	vector<line*> _linestream;
	vector<Statement*> _main;

	bool eof = false;

public:
	static token* _CurTok;
	static line* _CurLine;
	static map<string, MdsFunc*> funcs;

	MDSInterpretor(vector<line*>& code);
	~MDSInterpretor();

	void init();
/*
primary = number | '(' expr ')'
factor = primary ( (*|/) primary )*
expr = factor ( (+|-) factor )* 
*/
	int NextToken();

	NodeAST* paser_literal();
	NodeAST* paser_variable();
	NodeAST* paser_primary();
	NodeAST* paser_identifier();
	NodeAST* paser_factor();
	NodeAST* paser_expr();
	NodeAST* paser_condition_expr();
	NodeAST* paser_assign_expr();
	int paser_args(vector<string>&);

	void start();
	Statement* paser_statement();
	vector<Statement*> paser_qexpr();

	bool interpret();
	void printAST();

	static MdsFunc* find_func(string name);
};

//上下文无关文法的错误
class PaserException{
public:
	token* error_token;
	size_t line;
	string prompt;

	PaserException(char* c) :
		line(MDSInterpretor::_CurLine->l),
		error_token(MDSInterpretor::_CurTok),
		prompt(c){};

	void error_print()
	{
		cout << "Paser Error:" 
			<< line << ":" << error_token->start << '\t'
			<< '\"' << error_token->value << '\"' << '\t'
			<< prompt
			<< endl;
	}
};

class RunException{
public:
	vector<token*> error;
	size_t line;
	string prompt;

	RunException(size_t l, vector<token*>& t, char* p) :
		line(l), error(t), prompt(p){ }

	void error_print()
	{
		cout << "Excute Error:"
			<< line << ":" << (*error.begin())->start
			<< '\t' << '\"';
		for (size_t i = 0; i < error.size(); i++)
		{
			cout << error[i]->value << ' ';
		}

		cout << '\"' << prompt << endl;
	}
};

#endif