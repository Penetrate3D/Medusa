#include <cstdlib>
#include <cstring>
#include <cassert>

#include "mds_object.h"
#include "mem_allocator.h"

extern list<char*> free_list[];
extern  int area_size[];

map<size_t, MdsIntObject*> mds_int_map;
map<size_t, MdsStringObject*> mds_str_map;
map<size_t, MdsListObject*> mds_list_map;
MdsObject* MDS_VNULL;

extern MdsAllocator mds_allocator;

static hash<int> int_hash;
static hash<string> char_hash;
static hash<MdsObject*> ptr_hash;

size_t mds_hash(int i){
	return int_hash(i);
}

size_t mds_hash(string c){
	return char_hash(c);
}

size_t mds_hash(vector<MdsObject*>& v){
	long long res = 0;

	for (size_t i = 0; i < v.size(); i++)
	{
		res = res + i*ptr_hash(v[i]);
	}
	return res / v.size();
}

static Obj_Func obj_math_funcs[9] = {
	&int_int_,
	NULL,// int_string_add,
	NULL,//int_list_add,
	NULL,//string_int_add,
	&string_string_,
	NULL,//string_list_add,
	NULL,//list_int_add,
	NULL,//list_string_add,
	&list_list_
};

MdsObject* binary_operator(MdsObject* l, MdsObject* r, OperatorType op)
{
	uchar type1 = l->obj_type,
		type2 = r->obj_type;
	uchar idx = type1*3+type2;

	if (!obj_math_funcs[idx])
	{
		cout << "undefined binary operator " << endl;
		exit(0);
	}

	return (obj_math_funcs[idx])(l, r, op);
}


MdsIntObject::MdsIntObject(int v) :ob_val(v),MdsObject(Type_Int) {

}

MdsIntObject::~MdsIntObject(){

}

MdsIntObject* get_int_object(int val){
	size_t hash = mds_hash(val);

	map<size_t, MdsIntObject*>::iterator it = mds_int_map.find(hash);
	if (it != mds_int_map.end())
		return it->second;

	char* area = mds_allocator.mds_alloc(sizeof(MdsIntObject));
	MdsIntObject* res = new(area)MdsIntObject(val);

	mds_int_map.insert(pair<size_t, MdsIntObject*>(hash, res));

	return res;
}

char* destroy_int_object(MdsIntObject* ob){

	size_t hash = mds_hash(ob->get_value());

	map<size_t,MdsIntObject*>::iterator it = mds_int_map.find(hash);
	assert(it != mds_int_map.end());

	mds_int_map.erase(hash);

	char* area = (char*)(ob);
	mds_allocator.mds_free(area);

	return area;
}

MdsStringObject::MdsStringObject(int s, char* v)
:ob_size(s), ob_val(v), MdsObject(Type_String) {

}

MdsStringObject::~MdsStringObject(){
	memset(ob_val, 0, ob_size);
}

MdsStringObject* get_string_object(const char *val)
{
	string index(val);
	size_t hash = mds_hash(index);

	map<size_t, MdsStringObject*>::iterator it = mds_str_map.find(hash);
	if (it != mds_str_map.end())
		return it->second;

	int size = strlen(val) + 1;
	char* strobj = mds_allocator.mds_alloc(sizeof(MdsStringObject));
	char* str = mds_allocator.mds_alloc(size);

	MdsStringObject* ret = new(strobj)MdsStringObject(size, str);

	strncpy(str, val, size);
	mds_str_map.insert(pair<size_t, MdsStringObject*>(hash, ret));

	return ret;
}

char* destroy_string_object(MdsStringObject* ob)
{
	//后面的字符串未作处理，空间是随机的
	char* v = (char*)(ob + 1);
	string index(v);
	size_t hash = mds_hash(index);

	map<size_t, MdsStringObject*>::iterator it = mds_str_map.find(hash);
	assert(it != mds_str_map.end());


	mds_allocator.mds_free(ob->ob_val);

	mds_str_map.erase(hash);
	mds_allocator.mds_free((char*)ob);
	return (char*)ob;
}


MdsListObject::MdsListObject(size_t s,size_t c,MdsObject** l)
:MdsObject(Type_List),size(s),capacity(c),list(l){
}

MdsListObject::~MdsListObject(){

}

MdsListObject* get_list_object(vector<MdsObject*>& list)
{
	size_t hash = mds_hash(list);
	//list的有待考虑 到底是完全引用 还是 imutable。list可以修改么
	map<size_t, MdsListObject*>::iterator it = mds_list_map.find(hash);
	if (it != mds_list_map.end())
		return it->second;

	size_t size = list.size();
	size_t capacity = (size+7)&(~7);

	char* listobj = mds_allocator.mds_alloc(sizeof(MdsListObject));

	char *area = mds_allocator.mds_alloc(sizeof(void*)*capacity);

	MdsListObject* r = new(listobj)MdsListObject(size, capacity, (MdsObject**)area);
	MdsObject** elements = (MdsObject**)area;

	for (size_t i = 0; i < size; i++)
	{
		*elements++ = list[i];
	}

	mds_list_map.insert(pair<size_t, MdsListObject*>(hash,r));

	return r;
}

//需要防止重复destroy，所有的对象都放在map中，所以destroy前检查map中是否有此对象
char* destroy_list_object(MdsListObject* ob){
	size_t hash = mds_hash(ob->get_value());

	map<size_t, MdsListObject*>::iterator it = mds_list_map.find(hash);
	assert(it != mds_list_map.end());

	mds_allocator.mds_free((char*)(ob->list));
	mds_list_map.erase(hash);
	char* area = (char*)ob;
	mds_allocator.mds_free(area);

	return area;
}


MdsObject* int_int_(MdsObject* l, MdsObject* r, OperatorType op){
	MdsIntObject* lint = (MdsIntObject*)l;
	MdsIntObject* rint = (MdsIntObject*)r;

	int a = lint->get_value(),
		b = rint->get_value();

	int res;
	if (op == ADD)
		res = a + b;
	else
		if (op == MINUS)
			res = a - b;
		else
			if (op == TIMES)
				res = a * b;
			else
				if (op == DIV)
					res = a / b;
				else
					if (op == EQUAL)
						res = a == b ? 1 : 0;
					else
						if (op == NEQ)
							res = a != b ? 1 : 0;
						else
							if (op == GREAT)
								res = a > b ? 1 : 0;
							else
								if (op == LESS)
									res = a < b ? 1 : 0;
								else
									if (op == GE)
										res = a >= b ? 1 : 0;
									else
										if (op == LE)
											res = a <= b ? 1 : 0;
										else
											return NULL;

	MdsIntObject* newi = get_int_object(res);
	return newi;
}
MdsObject* int_string_(MdsObject* l, MdsObject* r, OperatorType op){
	return NULL;
}
MdsObject* int_list_(MdsObject* l, MdsObject* r, OperatorType op){
	return NULL;
}
MdsObject* string_int_(MdsObject* l, MdsObject* r, OperatorType op){
	return NULL;
}
MdsObject* string_string_(MdsObject* l, MdsObject* r, OperatorType op){

	MdsStringObject* lstr = (MdsStringObject*)l;
	MdsStringObject* rstr = (MdsStringObject*)r;

	string a = lstr->get_value();
	string b = rstr->get_value();

	if (op == ADD)
	{
		string res = a + b;
		MdsStringObject* news = get_string_object(res.data());
		return news;
	}

	int res;
	if (op == EQUAL)
		res = a == b ? 1 : 0;
	else
		if (op == NEQ)
			res = a != b ? 1 : 0;
		else
			if (op == GREAT)
				res = a > b ? 1 : 0;
			else
				if (op == LESS)
					res = a < b ? 1 : 0;
				else
					if (op == GE)
						res = a >= b ? 1 : 0;
					else
						if (op == LE)
							res = a <= b ? 1 : 0;
						else
							return NULL;

	MdsIntObject* newi = get_int_object(res);
	return newi;
}
MdsObject* string_list_(MdsObject* l, MdsObject* r, OperatorType op){
	return NULL;
}
MdsObject* list_int_(MdsObject* l, MdsObject* r, OperatorType op){
	return NULL;
}
MdsObject* list_string_(MdsObject* l, MdsObject* r, OperatorType op){
	return NULL;
}
MdsObject* list_list_(MdsObject* l, MdsObject* r, OperatorType op){
	vector<MdsObject*> l1 = ((MdsListObject*)l)->get_value();
	vector<MdsObject*> l2 = ((MdsListObject*)r)->get_value();

	if (op != ADD)
		return NULL;
	vector<MdsObject*> list;
	list.insert(list.end(),l1.begin(),l1.end());
	list.insert(list.end(), l2.begin(), l2.end());

	MdsListObject* res = get_list_object(list);
	return res;

}

void test_print(MdsObject* r)
{
	if (!r)
	{
		cout << "something error" << endl;
		return;
	}
	switch (r->obj_type)
	{
		case Type_Int:{
					  MdsIntObject* res1 = (MdsIntObject*)r;
					  cout << res1->get_value() << endl;
					  break;
		}
		case Type_String:{
						 MdsStringObject* res2 = (MdsStringObject*)r;
						 cout << res2->get_value() << endl;
						 break;
		}
		case Type_List:{
					   MdsListObject* res3 = (MdsListObject*)r;
					   vector<MdsObject*> vec = res3->get_value();
					   cout << "[" << endl;
					   for (size_t i = 0; i < vec.size(); i++)
					   {
						   cout << '\t';
						   test_print(vec[i]);
					   }
					   cout << "]" << endl;
					   break;
		}
	}
}
/*
int main()
{
	cout << sizeof(MdsIntObject) << endl;
	for (int i = 0; i < 20; ++i)
	{
		char c[10];
		MdsStringObject* l = get_string_object(_itoa(i,c,10));
		MdsStringObject* r = get_string_object(_itoa(i,c,10));

		MdsObject* res = operatorfunc(l, r, "+");
		test_print(res);

		cout << l << '\t' << r << '\t' << res << '\t' << endl;

		destroy_string_object(l);
		destroy_string_object(r);
	}

	return 0;
}
*/