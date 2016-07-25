#include <iostream>
#include "paser.h"

LiteralNodeAST::~LiteralNodeAST()
{
	if (type == LITERAL_STRING)
		free(value.c);
}

LiteralNodeAST::LiteralNodeAST(int x, token* t) 
{
	line = MDSInterpretor::_CurLine->l;
	type = LITERAL_INT;
	value.i = x;
	contents.push_back(t);
}

LiteralNodeAST::LiteralNodeAST(char* x, token* t) 
{
	line = MDSInterpretor::_CurLine->l;
	type = LITERAL_STRING;
	value.c = x;
	contents.push_back(t);
}

void LiteralNodeAST::eval(MdsEnv* e)
{
	MdsObject* res = NULL;
	switch (type)
	{
	case LITERAL_INT:
		res = get_int_object(value.i); break;
	case LITERAL_FLOAT:
		break;
	case LITERAL_STRING:
		res = get_string_object(value.c); break;
	}
	mds_stack.push_front(res);
}

void LiteralNodeAST::print(int n)
{
	while (n--)
		cout << ' ';
	switch (type)
	{
	case LITERAL_INT:
		cout << "INT: " << value.i << endl; break;
	case LITERAL_FLOAT:
		break;
	case LITERAL_STRING:
		cout << "STRING: " << value.c << endl; break;
	}
}

LiteralNodeAST* LiteralNodeAST::operator+(LiteralNodeAST& r)
{
	LiteralNodeAST* newl = new LiteralNodeAST(0, NULL);
	newl->contents.insert(newl->contents.end(), this->contents.begin(), this->contents.end());
	newl->contents.insert(newl->contents.end(), r.contents.begin(), r.contents.end());

	if (this->type != r.type)
		throw PaserException("type match false in binary operator");

	if (this->type == LITERAL_STRING)
	{
		size_t s1 = strlen(this->value.c);
		size_t s2 = strlen(r.value.c);
		char *res = (char*)malloc(s1 + s2 + 1);
		strncpy(res, this->value.c, s1);
		strncpy(res + s1, r.value.c, s2);
		*(res + s1 + s2) = 0;
		return new LiteralNodeAST(res, NULL);
	}
	
	if (this->type == LITERAL_INT)
	{
		int res = this->value.i + r.value.i;
		return new LiteralNodeAST(res,NULL);
	}

	return NULL;
}

LiteralNodeAST* LiteralNodeAST::operator-(LiteralNodeAST& r)
{
	if (this->type != r.type)
		throw PaserException("type match false in binary operator");

	if (this->type == LITERAL_STRING)
	{
		throw PaserException("type match false in binary operator");
	}

	if (this->type == LITERAL_INT)
	{
		int res = this->value.i - r.value.i;
		return new LiteralNodeAST(res,NULL);
	}

	return NULL;
}

LiteralNodeAST* LiteralNodeAST::operator*(LiteralNodeAST& r)
{
	if (this->type != r.type)
		throw PaserException("type match false in binary operator");

	if (this->type == LITERAL_STRING)
	{
		throw PaserException("type match false in binary operator");
	}

	if (this->type == LITERAL_INT)
	{
		int res = this->value.i * r.value.i;
		return new LiteralNodeAST(res,NULL);
	}

	return NULL;
}

LiteralNodeAST* LiteralNodeAST::operator/(LiteralNodeAST& r)
{
	if (this->type != r.type)
		throw PaserException("type match false in binary operator");

	if (this->type == LITERAL_STRING)
	{
		throw PaserException("type match false in binary operator");
	}

	if (this->type == LITERAL_INT)
	{
		int res = this->value.i / r.value.i;
		return new LiteralNodeAST(res,NULL);
	}

	return NULL;
}

VariableNodeAST::VariableNodeAST(token* t) 
{
	line = MDSInterpretor::_CurLine->l;
	contents.push_back(t);
	VariableName = t->value;
}

void VariableNodeAST::print(int n)
{
	while (n--)
		cout << ' ';
	cout << "VARIABLE: " << VariableName << endl;
}

BinaryNodeAST::BinaryNodeAST(token* t, NodeAST* lhs, NodeAST* rhs) :
_lhs(lhs), _rhs(rhs){
	line = MDSInterpretor::_CurLine->l;

	BinaryOp = opstring_2_type(t->value);

	contents.insert(
		contents.end(), lhs->contents.begin(), lhs->contents.end());

	contents.push_back(t);

	contents.insert(
		contents.end(), rhs->contents.begin(), rhs->contents.end());

	constant_folding();
}

void BinaryNodeAST::constant_folding()
{

}

BinaryNodeAST::~BinaryNodeAST(){
	delete _lhs;
	delete _rhs;
}

OperatorType BinaryNodeAST::opstring_2_type(string op)
{
	if (op == "=") return ASSIGN;
	if (op == "==") return EQUAL;
	if (op == "!=") return NEQ;
	if (op == ">") return GREAT;
	if (op == "<") return LESS;
	if (op == ">=") return GE;
	if (op == "<=") return LE;
	if (op == "+") return ADD;
	if (op == "-") return MINUS;
	if (op == "*") return TIMES;
	if (op == "/") return DIV;
}

void BinaryNodeAST::print(int n)
{
	int a = n;
	while (a--)
		cout << ' ';
	cout << "EXPR: " << endl;
	_lhs->print(n + 6);
	a = n + 6;
	while (a--)
		cout << ' ';
	cout << BinaryOp << endl;
	_rhs->print(n + 6);
}

CallNodeAST::CallNodeAST(token* t, vector<NodeAST*>& list) :
Callee(t->value), args(list)
{
	line = MDSInterpretor::_CurLine->l;
	contents.push_back(t);
	for (size_t i = 0; i < args.size(); i++)
	{
		contents.insert(
			contents.end(), args[i]->contents.begin(), args[i]->contents.end());
	}
}

CallNodeAST::~CallNodeAST(){
	free_vector<NodeAST>(args);
}

void CallNodeAST::print(int n){
	int a = n;
	while (a--)
		cout << ' ';
	cout << "CALLFUNC:" << Callee << endl;
	for (size_t i = 0; i < args.size(); ++i)
	{
		args[i]->print(n + 6);
	}
}


MDSInterpretor::MDSInterpretor(vector<line*>& code) :_linestream(code){
	if (!_linestream.empty())
		_CurLine = _linestream[0];
	if (!(_CurLine->tokens.empty()))
		_CurTok = _linestream[0]->tokens[0];
}

MDSInterpretor::~MDSInterpretor()
{
	for (size_t i = 0; i < _linestream.size(); ++i)
	{
		delete _linestream[i];
	}

	for (size_t i = 0; i < _main.size(); ++i)
	{
		delete _main[i];
	}

	map<string, MdsFunc*>::iterator iter;

	for (iter = funcs.begin(); iter != funcs.end(); ++iter)
	{
		delete(iter->second);
	}
}

MdsFunc* MDSInterpretor::find_func(string name){
	map<string, MdsFunc*>::iterator it = funcs.find(name);
	if (it != funcs.end())
		return it->second;

	return NULL;
}

token* MDSInterpretor::_CurTok = NULL;
line* MDSInterpretor::_CurLine = NULL;
map<string, MdsFunc*> MDSInterpretor::funcs;


int MDSInterpretor::NextToken()
{
	if (_index == _linestream[_lineIndex]->tokens.size()-1)
	{
		++_lineIndex;
		if (_lineIndex == _linestream.size())
		{
			eof = true;
			return -1;
		}
		_CurLine = _linestream[_lineIndex];
		_index = 0;
	}
	else{
		++_index;
	}
	_CurTok = _CurLine->tokens[_index];

	if (_CurTok->type == SPACE)
		NextToken();

	return 0;
}



NodeAST* MDSInterpretor::paser_literal()
{
	if (_CurTok->type == NUM)
	{
		NodeAST* newn = new LiteralNodeAST(atoi(_CurTok->value.c_str()),_CurTok);
		NextToken();
		return newn;
	}

	if (_CurTok->type == STRING)
	{
		const char* str = _CurTok->value.data();
		str++;

		char* res = (char*)malloc(strlen(str));
		strncpy(res, str,strlen(str));
		*(res + strlen(str)-1) = 0;

		NodeAST* newn = new LiteralNodeAST(res,_CurTok);
		NextToken();
		return newn;
	}
	else
		return NULL;
}

NodeAST* MDSInterpretor::paser_identifier()
{
	if (_CurTok->type != ID)
		throw PaserException("expect a identifier");

	token* name = _CurTok;
	NextToken();
	if (_CurTok->value == "(")
	{
		NextToken();
		vector<NodeAST*> args;
		while(_CurTok->value!=")"){
			args.push_back(paser_condition_expr());
		}
		NextToken();
		CallNodeAST* newc = new CallNodeAST(name, args);
		return newc;
	}

	return new VariableNodeAST(name);
}

NodeAST* MDSInterpretor::paser_primary()
{
	if (_CurTok->type == NUM || _CurTok->type == STRING)
	{
		NodeAST* newn = paser_literal();
		return newn;
	}
	if (_CurTok->type == ID)
	{
		NodeAST* newi = paser_identifier();
		return newi;
	}
	//表达式
	if (_CurTok->value == "(")
	{
		NextToken();
		NodeAST* newe = paser_expr();
		if (_CurTok->value != ")")
			throw PaserException("expect a \')\' in expression");
		NextToken();
		return newe;
	}
	else
	{
		throw PaserException("unknow characters");
		return NULL;
	}
}

NodeAST* MDSInterpretor::paser_factor()
{
	NodeAST* lhs = paser_primary();
	NodeAST* newb = NULL;
	
	while (_CurTok->value == "*" || _CurTok->value == "/")
	{
		token* op = _CurTok;
		NextToken();
		NodeAST* rhs = paser_primary();
		newb = new BinaryNodeAST(op, lhs, rhs);
		lhs = newb;
	}
	return lhs;
}

NodeAST* MDSInterpretor::paser_expr()
{
	NodeAST* lhs = paser_factor(),*rhs,*newb;
	
	while (_CurTok->value == "+" || _CurTok->value == "-")
	{
		token* op = _CurTok;
		NextToken();
		rhs = paser_factor();
		newb = new BinaryNodeAST(op, lhs, rhs);
		lhs = newb;
	}
	return lhs;
	
}

NodeAST* MDSInterpretor::paser_condition_expr()
{
	NodeAST* lhs = paser_expr(),*rhs;

	NodeAST* newc;
	while (_CurTok->type == COMPARASION)
	{
		token* op = _CurTok;
		NextToken();
		rhs = paser_expr();
		newc = new BinaryNodeAST(op, lhs, rhs);
		lhs = newc;
	}
	return lhs;
}

NodeAST* MDSInterpretor::paser_assign_expr()
{
	//exp = exp;
	NodeAST* lhs = paser_condition_expr(),*rhs;

	while (_CurTok->value == "=")
	{
		token* op = _CurTok;
		NextToken();
		rhs = paser_condition_expr();
		NodeAST* newb = new BinaryNodeAST(op,lhs,rhs);
		lhs = newb;
	}

	return lhs;
}

vector<Statement*> MDSInterpretor::paser_qexpr()
{
	if (_CurTok->value != "{")
		throw PaserException("expect a \'{\' after function defination");
	vector<Statement*> body;
	NextToken();

	while (_CurTok->value != "}")
	{
		body.push_back(paser_statement());
	}
	NextToken();

	return body;
}

int MDSInterpretor::paser_args(vector<string> &args)
{
	if (_CurTok->value != "(")
		throw PaserException("expect a \'(\' after function calling");
	NextToken();

	int i = 0;
	while (_CurTok->type == ID)
	{
		++i;
		args.push_back(_CurTok->value);
		NextToken();
	}

	if (_CurTok->value != ")")
		throw PaserException("lack of \')\' to match the )");
	NextToken();

	return i;
}


Statement* MDSInterpretor::paser_statement()//处理顶层结构，paser_ast + 函数定义 + expr
{	
	if (_CurTok->value == "return")
	{
		NextToken();
		NodeAST* ret = paser_assign_expr();
		return new ReturnStatement(ret);
	}
	if (_CurTok->value == "while")
	{
		NextToken();
		NodeAST* cond = paser_assign_expr();

		vector<Statement*> block = paser_qexpr();
		return new LoopStatement(cond, block);
	}

	if (_CurTok->value == "if")
	{
		NextToken();
		NodeAST* cond = paser_assign_expr();
		vector<Statement*> b1 = paser_qexpr();
		if (_CurTok->value == "else")
		{
			NextToken();
			vector<Statement*> b2 = paser_qexpr();
			return new CondStatement(cond, b1, b2);
		}

		return new CondStatement(cond, b1, vector<Statement*>());
	}


	NodeAST* lhs = paser_assign_expr();
	return new ExpressionStatement(lhs);	
}


void MDSInterpretor::start()
{
	// 定义函数
	if (_CurTok->value == "def")
	{
		NextToken();
		if (_CurTok->type != ID)
			throw PaserException(
			"expect a identifier after keyword \'def\'");
		string& name = _CurTok->value;
		NextToken();
		vector<string> args;
		paser_args(args);
		vector<Statement*> body = paser_qexpr();
		MdsFunc* newf = new MdsDefFunction(args, body);

		funcs.insert(pair<string, MdsFunc*>(name, newf));
		return;
	}

	_main.push_back(paser_statement());
}

bool MDSInterpretor::interpret()
{
	//语法分析
	while (!eof)
	{
		try{
			start();
		}
		catch (PaserException e)
		{
			e.error_print();
			return false;
		}
	}

	//执行
	for (size_t i = 0; i < _main.size(); ++i)
	{
		try{
			if (_main[i])
				StatementResult r = _main[i]->excute(&global_env);
		}
		catch (RunException e)
		{
			e.error_print();
			return false;
		}
	}

	return true;
}

void MDSInterpretor::init()
{
	funcs.insert(pair<string, MdsFunc*>(
		"print", new MdsBuildinFunction(mds_buildin_print)));

	char* vnull = mds_allocator.mds_alloc(sizeof(MdsObject));
	MDS_VNULL = new(vnull)MdsObject();
}

void MDSInterpretor::printAST()
{
	if (eof)
	{
		vector<Statement*>::iterator iter = _main.begin();
		for (; iter < _main.end(); ++iter);
//			(*iter)->print(0);
	}
}