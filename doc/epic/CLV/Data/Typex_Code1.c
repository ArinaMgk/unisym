	typedef struct newt {char newt[5];} newt;
	int main()
	{
		newt a = {1,2,3,4,5};
		newt b = {0};
		*(newt*)&b = *(newt*)&a;
	}
