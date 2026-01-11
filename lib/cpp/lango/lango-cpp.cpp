#include "../../../inc/cpp/unisym"

_ESYM_C Handler_t __init_array_start[], __init_array_end[];
_ESYM_C void _preprocess() {
	for (Handler_t* func = __init_array_start; func < __init_array_end; func++) (*func)();
}
