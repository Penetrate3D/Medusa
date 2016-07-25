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

MdsObject** MdsEnv::find_var(const string& name)//����ָ����Ϊ���жϴ�������㣬��������Ӧ���и��Ʋ���
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
	//���ú�����ջӦ����һ���Եģ����ܷ���callnodeast�ṹ�У���Ϊfunction
	//�����е�callnodeast�ڵ���Ψһ�ģ����ʹ��ջ���ۼ���ʷֵ
	//���bug����˺þã�debugˮƽ���ĵ�
	_local.parent = e;

	MdsFunc* func = MDSInterpretor::find_func(Callee);
	if (func == NULL)
		throw RunException(
		line,
		contents,
		"undefined function"
		);

	//ԭ������ִ��
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


	//�Զ��庯�� ִ�й���
	//��ʱ�����Ѵ洢��mdsenv�У�gcʱ���ῼ��
	//ֻ���㺯�������еĲ����������������������������
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
