extern int default_data;
extern void *default_data_ptr, *external_data_ptr;
extern void *default_function_ptr, *external_function_ptr;
extern void *pointer_block[2];
extern int default_function(void);

int external_data;

int external_function(void)
{
	return 11;
}

int main(void)
{
	return default_data != 7 || default_function() != 9 ||
		default_data_ptr != (void*)((char*)&default_data + 4) ||
		external_data_ptr != &external_data ||
		default_function_ptr != default_function ||
		external_function_ptr != external_function ||
		pointer_block[0] != (void*)((char*)&default_data + 8) ||
		pointer_block[1] != external_function;
}
