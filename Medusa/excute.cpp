#include "paser.h"
list<MdsObject*> mds_stack;
list<MdsEnv*> mds_local_list;
MdsEnv global_env;

StatementResult excute_block(vector<Statement*>& block, MdsEnv* e)
{
	for (size_t i = 0; i < block.size(); i++)
	{
		StatementResult r = block[i]->excute(e);
		if (r.type == CONTINUE || r.type == BREAK)
			return r;
	}
	return StatementResult();
}

MdsObject** MdsEnv::find_var(const string& name)//返回指针是为了判断错误更方便，后续处理应该有复制操作
{
	map<string, MdsObject*>::iterator it; 
	it = var_list.find(name);
	if (it != var_list.end()) return &it->second;

	if (!parent) return NULL;

	return parent->find_var(name);
}

void MdsEnv::append_var(const string& name, MdsObject* value)
{
	map<string, MdsObject*>::iterator it;
	it = var_list.find(name);
	if (it != var_list.end()) 
		it->second = value;
	else
	{
		var_list.insert(pair<string, MdsObject*>(name, value));
	} 
}

void VariableNodeAST::eval(MdsEnv* e)
{
	MdsObject** var_ptr = e->find_var(VariableName);
	if (!var_ptr)
	{
		throw RunException(
			line,
			this->contents,
			"undefined variable");
	}
	mds_stack.push_front(*var_ptr);
}


void BinaryNodeAST::eval(MdsEnv* e)
{
	if (BinaryOp == ASSIGN)
	{
		if (_lhs->NodeType() != VARIABLE_NODE)
			throw RunException(
			line,
			contents,
			"expect variable in operator = left");

		VariableNodeAST* name = (VariableNodeAST*)_lhs;
		_rhs->eval(e);

		MdsObject* res = mds_stack.front();
		MdsObject** value = e->find_var(name->VariableName);
		if (value == NULL)
			e->append_var(name->VariableName, res);
		else
			*value = res;
		return;
	}

	_lhs->eval(e);
	_rhs->eval(e);

	list<MdsObject*>::iterator it = mds_stack.begin();
	MdsObject* rv = *it;
	MdsObject* lv = *(++it);

	MdsObject* res = binary_operator(lv, rv, BinaryOp);
	mds_stack.pop_front();
	mds_stack.pop_front();
	mds_stack.push_front(res);
}


void CallNodeAST::eval(MdsEnv* e)
{	
	MdsEnv _local;					
	//调用函数的栈应该是一次性的，不能放在callnodeast结构中，因为function
	//定义中的callnodeast节点是唯一的，多次使用栈会累计历史值
	//这个bug检查了好久，debug水平真心低
	_local.parent = e;

	MdsFunc* func = MDSInterpretor::find_func(Callee);
	if (func == NULL)
		throw RunException(
		line,
		contents,
		"undefined function"
		);

	//原生函数执行
	if (func->is_buildin_func == true)
	{
		MdsBuildinFunction* f = (MdsBuildinFunction*)func;
		vector<MdsObject*> list;
		for (int i = args.size()-1; i >= 0; --i)
		{
			args[i]->eval(&_local);
		}
		for (size_t i = 0; i < args.size(); ++i)
		{
			list.push_back(mds_stack.front());
			mds_stack.pop_front();
		}
		MdsListObject* para = get_list_object(list);
		mds_stack.push_front(para);
		f->func(&_local);
		return;
	}


	//自定义函数 执行过程
	//临时对象已存储到mdsenv中，gc时将会考虑
	//只计算函数定义中的参数个数，多余参数不报错，将忽视
	MdsDefFunction* f = (MdsDefFunction*)func;
	for (size_t i = 0; i < f->_args.size(); ++i)
	{
		args[i]->eval(&_local);
		_local.var_list.insert(
			pair<string, MdsObject*>(f->_args[i], mds_stack.front())
			);
		mds_stack.pop_front();
	}

	for (size_t i = 0; i < f->_body.size(); ++i)
	{
		StatementResult r =
			f->_body[i]->excute(&_local);
		if (r.type == RETURN)
		{
			mds_stack.push_front(r.res);
			return;
		}
	}
	mds_stack.push_front(MDS_VNULL);
}

StatementResult CondStatement::excute(MdsEnv* e)
{
	MdsEnv local;
	local.parent = e;

	cond->eval(&local);
	MdsObject* r = mds_stack.front();

	if (r->obj_type == Type_Int)
	{
		MdsIntObject* i = (MdsIntObject*)r;
		if (i->get_value())
		{
			excute_block(block1, &local);
		}
		else
		{
			excute_block(block2, &local);
		}
	}
	else
	{
		throw RunException(
			cond->line,
			cond->contents,
			"non boolean object in IF statement");
	}

	mds_stack.pop_front();
	return StatementResult();
}

StatementResult LoopStatement::excute(MdsEnv* e)
{
	MdsEnv local;
	local.parent = e;

	while (1)
	{
		cond->eval(&local);
		MdsObject* r = mds_stack.front();

		if (r->obj_type != Type_Int)
		{
			throw RunException(
				cond->line,
				cond->contents,
				"non boolean object in WHILE statement");
		}

		MdsIntObject* i = (MdsIntObject*)r;
		if (i->get_value())
		{
			StatementResult r = excute_block(block,&local);
			if (r.type == CONTINUE) continue;
			if (r.type == BREAK) break;
			mds_stack.pop_front();
		}
		else
		{
			mds_stack.pop_front();
			break;
		}
	}
	return StatementResult();
}
