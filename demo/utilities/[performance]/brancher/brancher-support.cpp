
#include "brancher.hpp"
#include <stdio.h>
#include <unistd.h>


bool exist_intel_pt() {
    return (access("/sys/bus/event_source/devices/intel_pt", F_OK) != -1);
}

void hard_list() {
	if (exist_intel_pt())
		ploginfo("Intel PT OK."); 
	else 
		plogerro("Intel PT not exist.");

}

