#ifndef MDS_OBJECT_H
#define MDS_OBJECT_H

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <list>

using namespace std;

typedef unsigned char uchar;

//标准的接口类定义，构造函数、析构函数需要给出定义
//防止继承类调用它们而无法找到符号

typedef enum{
	Type_Int = 0,
	Type_String,
	Type_List,
	Type_Null
}ObjectType;

typedef enum{
	ASSIGN = 10,
	ADD,
	MINUS,
	TIMES,
	DIV,
	EQUAL,
	NEQ,
	GE,
	LE,
	GREAT,
	LESS
}OperatorType;

class MdsObject;

extern MdsObject* MDS_VNULL;

class MdsObject{
public:
	ObjectType obj_type;
	MdsObject(ObjectType type) :obj_type(type){}
	MdsObject() :obj_type(Type_Null){}
	virtual ~MdsObject(){}

	friend MdsObject* binary_operator(MdsObject*, MdsObject*, OperatorType);
};


class MdsIntObject :public MdsObject
{
private:
	int ob_val;

public:
	MdsIntObject(int);
	~MdsIntObject();
	void print();

	int get_value(){ return ob_val; }
};


//char字符串存储空间在对象外
class MdsStringObject :public MdsObject
{
public:
	char* ob_val;
	int ob_size;

public:
	MdsStringObject(int, char*);
	~MdsStringObject();

	string get_value(){ return string(ob_val); }
};

//list存储空间在外，可放capacity大小的mdsobject的指针
class MdsListObject :public MdsObject
{
public:
	size_t capacity;
	size_t size;
	MdsObject** list;
public:
	MdsListObject(size_t s,size_t c,MdsObject** l);
	~MdsListObject();

	vector<MdsObject*> get_value(){ 
		MdsObject** start = list;
		vector<MdsObject*> r;
		for (size_t i = 0; i < size; i++)
		{
			r.push_back(*start);
			start++;
		}
		return r;
	}
};


//所有的析构函数都需要放弃map中对象
MdsStringObject* get_string_object(const char *val);
char* destroy_string_object(MdsStringObject* ob);

MdsIntObject* get_int_object(int val);
char* destroy_int_object(MdsIntObject* ob);

MdsListObject* get_list_object(vector<MdsObject*>& list);
char* destroy_list_object(MdsListObject* ob);

MdsObject* int_int_(MdsObject*, MdsObject*, OperatorType);
MdsObject* int_string_(MdsObject*, MdsObject*, OperatorType);
MdsObject* int_list_(MdsObject*, MdsObject*, OperatorType);
MdsObject* string_int_(MdsObject*, MdsObject*, OperatorType);
MdsObject* string_string_(MdsObject*, MdsObject*, OperatorType);
MdsObject* string_list_(MdsObject*, MdsObject*, OperatorType);
MdsObject* list_int_(MdsObject*, MdsObject*, OperatorType);
MdsObject* list_string_(MdsObject*, MdsObject*, OperatorType);
MdsObject* list_list_(MdsObject*, MdsObject*, OperatorType);

typedef MdsObject* (*Obj_Func)(MdsObject*,MdsObject*,OperatorType);

size_t mds_hash(int);
size_t mds_hash(string);
size_t mds_hash(vector<MdsObject*>&);

void test_print(MdsObject* r);

class MdsEnv;
extern list<MdsEnv*> mds_local_list;

class MdsEnv
{
public:
	map<string,MdsObject*> var_list;
	MdsEnv* parent = NULL;

	MdsEnv(){
		mds_local_list.push_back(this);
	}
	~MdsEnv(){
		list<MdsEnv*>::iterator it = find(
			mds_local_list.begin(), mds_local_list.end(), this);
		mds_local_list.erase(it);
	}

	MdsObject** find_var(string name);
	void append_var(string, MdsObject*);
};



typedef void(*buildin_func)( MdsEnv*);

extern map<size_t, MdsIntObject*> mds_int_map;
extern map<size_t, MdsStringObject*> mds_str_map;
extern map<size_t, MdsListObject*> mds_list_map;

#endif