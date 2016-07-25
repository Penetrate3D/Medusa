#include "mds_buildin_func.h"
#include "mds_object.h"

void mds_buildin_print( MdsEnv* env)
{
	MdsObject* args = mds_stack.front();
	if (args->obj_type != Type_List)
	{
		cout << "args passed in buildin functions is a LIst " << endl;
		exit(1);
	}
	vector<MdsObject*> l = ((MdsListObject*)args)->get_value();
	for (size_t i = 0; i < l.size(); i++)
		test_print(l[i]);
	
	mds_stack.pop_front();
	//print
	mds_stack.push_front(MDS_VNULL);
}