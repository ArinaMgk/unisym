#define _COM_NnodeInsert(insnod, onleft, parent, tmp)\
	tmp->col = insnod ? insnod->col : 0;\
	tmp->row = insnod ? insnod->row : 0;\
	tmp->pare = stepval(insnod)->pare;


