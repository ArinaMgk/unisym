#define _COM_NodeInsert(nodtyp,nod,content,next)\
	nodtyp* tmp = zalcof(nodtyp);/* set next null */\
	tmp->content = content;\
	if (!nod) return tmp;/* 0 --> New */\
	AssignParallel(tmp->next, nod->next, tmp);\
	return tmp
