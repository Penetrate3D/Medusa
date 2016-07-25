#include <iostream>

#include "mem_allocator.h"
#include "mds_object.h"

//空闲链表包含meta
static list<char*> free_list[16];
static int area_size[] = { 8, 16, 24, 32, 
			40, 48, 56, 64, 
			72, 80, 88, 96,
			104, 112, 120, 128
			};

MdsMemPool::MdsMemPool() :remain_bytes(0), now(NULL){

}

MdsMemPool::~MdsMemPool(){
	for (size_t i = 0; i < blocks.size(); i++)
		free(blocks[i]);
}


inline uchar power_num(size_t size)
{
	uchar i = (size + 7)>>3;
	return i-1;
}

char* MdsMemPool::alloc_mem2(size_t size, uchar num)
{
	size_t alloc_size = size*num;
	uchar index = power_num(size);

	remain_bytes -= alloc_size;
	char* r = now;
	now += alloc_size;

	uchar i = 1;
	char* p = r + size;
	while (i<num)
	{
		free_list[index].push_front(p);
		p += size;
		i++;
	}

	return r;
}

//size是用户申请的空间大小加上meta大小
char* MdsMemPool::alloc_mem(size_t s)
{
	uchar index = power_num(s);
	size_t size = area_size[index];
	size_t alloc_size = size * 6;

	if (remain_bytes>alloc_size)
	{
		return alloc_mem2(size, 6);
	}

	if (remain_bytes < size)
	{
		char *newblock = (char*)malloc(BLOCK_SIZE);
		blocks.push_back(newblock);
		
		remain_bytes = BLOCK_SIZE;
		now = newblock;
		return alloc_mem2(size,6);
	}

	uchar num = remain_bytes / size;
	return alloc_mem2(size, num);
}

char* MdsMemPool::alloc_user(size_t size)
{
	if (remain_bytes >= size)
	{
		char* p = now;
		now += size;
		remain_bytes -= size;
		return p;
	}
	else
	{
		char* user = (char*)malloc(size);
		blocks.push_back(user);
		return user;
	}
}

MdsAllocator::MdsAllocator():cookie(0){

}

MdsAllocator::~MdsAllocator(){

}

//size不包含meta的大小
char* MdsAllocator::mds_alloc(size_t size)
{
	char* meta;
	size_t size_with_meta = size + sizeof(MdsBlockMeta);
	uchar idx = power_num(size_with_meta);

	if (idx > 15)
	{
		meta = pool.alloc_user(size_with_meta);
		memset(meta, 0, size_with_meta);
		new(meta)MdsBlockMeta(size_with_meta);
		alloced.insert(meta);
		return meta + sizeof(MdsBlockMeta);
	}

	size_t aligned_with_meta = area_size[idx];
	if (!free_list[idx].empty())
	{
		meta = free_list[idx].front();
		free_list[idx].pop_front();

		memset(meta, 0, aligned_with_meta);
		new(meta)MdsBlockMeta(aligned_with_meta);
		alloced.insert(meta);
		return meta + sizeof(MdsBlockMeta);
	}
//	size_t aligned = ALIGNMENT(size);

	meta = pool.alloc_mem(aligned_with_meta);
	memset(meta, 0, aligned_with_meta);
	new(meta)MdsBlockMeta(aligned_with_meta);
	alloced.insert(meta);
	return meta+sizeof(MdsBlockMeta);
}

void MdsAllocator::discompose_block(char* begin, size_t size)
{
	size_t idx = 15;
	size_t area = area_size[idx];

	while (size >= 8)
	{
		if (size >= area)
		{
			free_list[idx].push_front(begin);
			begin += area;
			size -= area;
		}
		else
		{
			--idx;
			area = area_size[idx];
		}
	}
}

char* MdsAllocator::mds_free(char *free)
{
	MdsBlockMeta* meta = (MdsBlockMeta*)(free - sizeof(MdsBlockMeta));
	char* block = (char*)meta;

	set<char*>::iterator it = alloced.find(block);
	if (it == alloced.end())
	{
		cout << "BUG: free a memblock that hasn't been alloced " << endl;
		exit(1);
	}
	else{
		alloced.erase(it);
	}

	size_t size = meta->size;
	uchar idx = power_num(size);

	//是一个big block，需要分解block
	if (idx > 15)
	{
		discompose_block(block, size);
		return block;
	}

	list<char*>::iterator begin = free_list[idx].begin();
	list<char*>::iterator end = free_list[idx].end();
	if (find(begin, end, block) != end)
	{
		cout << "BUG: free a memblock that has been free" << endl;
		exit(1);
	}

	free_list[idx].push_front(block);
	return block;
}



MdsAllocator mds_allocator;
extern list<MdsObject*> mds_stack;
extern list<MdsEnv*> mds_local_list;
extern MdsEnv global_env;

void gc_flush_mark()
{
	set<char*>::iterator it;
	for (it = mds_allocator.alloced.begin(); it != mds_allocator.alloced.end(); ++it)
	{
		MdsBlockMeta* meta = (MdsBlockMeta*)(*it);
		meta->marked = false;
	}
}

void gc_mark_object(MdsObject* obj)
{
	MdsBlockMeta* meta = (MdsBlockMeta*)obj - 1;
	if (meta->marked)
		return;

	if (obj->obj_type == Type_Int)
	{
		meta->marked = true;
		return;
	}

	if (obj->obj_type == Type_String)
	{
		meta->marked = true;
		MdsStringObject* str = (MdsStringObject*)obj;
		char* p = str->ob_val;
		meta = (MdsBlockMeta*)p - 1;
		meta->marked = true;
		return;
	}

	if (obj->obj_type == Type_List)
	{
		MdsListObject* l = (MdsListObject*)obj;
		MdsObject** start = l->list;
		for (size_t i = 0; i < l->size; ++i)
		{
			gc_mark_object(start[i]);
		}
		meta->marked = true;
		return;
	}

	meta->marked = true;
	return;
}

void gc_mark()
{
	list<MdsObject*>::iterator it1 = mds_stack.begin();
	for (; it1 != mds_stack.end(); ++it1)
		gc_mark_object(*it1);

	map<string, MdsObject*>::iterator  it;
	list<MdsEnv*>::iterator it2 = mds_local_list.begin();
	for (; it2 != mds_local_list.end(); ++it2)
	{
		it = (*it2)->var_list.begin();
		for (; it != (*it2)->var_list.end(); ++it)
			gc_mark_object(it->second);
	}

	it = global_env.var_list.begin();
	for (; it != global_env.var_list.end(); ++it)
		gc_mark_object(it->second);

	return;
}

void gc_sweep()
{
	map<size_t, MdsIntObject*>::iterator it_int = mds_int_map.begin();
	for (; it_int != mds_int_map.end(); ++it_int)
	{
		MdsBlockMeta* meta = (MdsBlockMeta*)it_int->second - 1;
		if (!meta->marked)
		{
			destroy_int_object(it_int->second);
		}
	}

	map<size_t, MdsStringObject*>::iterator it_str = mds_str_map.begin();
	for (; it_str != mds_str_map.end(); it_str++)
	{
		MdsBlockMeta* meta = (MdsBlockMeta*)it_str->second - 1;
		if (!meta->marked){
			destroy_string_object(it_str->second);
		}
	}

	map<size_t, MdsListObject*>::iterator it_list = mds_list_map.begin();
	for (; it_list != mds_list_map.end(); ++it_list)
	{
		MdsBlockMeta* meta = (MdsBlockMeta*)it_list->second - 1;
		if (!meta->marked){
			destroy_list_object(it_list->second);
		}
	}
}

void gc()
{
	gc_flush_mark();
	gc_mark();
	gc_sweep();

	return;
}
