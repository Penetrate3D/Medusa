#ifndef MEM_ALLOCATOR_H
#define MEM_ALLOCATOR_H

#include <cstdlib>
#include <vector>
#include <list>
#include <stack>
#include <set>

using namespace std;

const size_t BLOCK_SIZE = 4096;

typedef unsigned char uchar;

//内存池模型是两层的
//底层的MdsMemPool负责向操作系统申请大块内存，并向上层分发，申请的内存块用vector管理
//MdsAllocator 是解释中的对象申请接口，它会为MdsObject添加meta头，
//set容器alloced保存了解释器所申请的对象地址
class MdsMemPool{
private:
	vector<char*> blocks;
	char* now;
	size_t remain_bytes;

public:
	MdsMemPool();
	~MdsMemPool();

	char* alloc_mem(size_t);
	char* alloc_mem2(size_t, uchar);

	char* alloc_user(size_t);
};


struct MdsBlockMeta{
public:
	uchar size;
	bool marked;

	MdsBlockMeta(int a) :size(a), marked(false){	}
private:
	~MdsBlockMeta(){	}
};

class MdsAllocator{
private:
	MdsMemPool pool;

	size_t cookie;
public:
	MdsAllocator();
	~MdsAllocator();
	char* mds_alloc(size_t t);
	char* mds_free(char*);

	//存储已分配的内存地址，包含StringObject中的字符串区和ListObject中的列表区
	set<char*> alloced;

private:
	void discompose_block(char* begin, size_t s);
};
 


#endif 