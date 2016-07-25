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

//�ڴ��ģ���������
//�ײ��MdsMemPool���������ϵͳ�������ڴ棬�����ϲ�ַ���������ڴ����vector����
//MdsAllocator �ǽ����еĶ�������ӿڣ�����ΪMdsObject���metaͷ��
//set����alloced�����˽�����������Ķ����ַ
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

	//�洢�ѷ�����ڴ��ַ������StringObject�е��ַ�������ListObject�е��б���
	set<char*> alloced;

private:
	void discompose_block(char* begin, size_t s);
};
 


#endif 