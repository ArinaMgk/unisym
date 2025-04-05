// UTF-8 
// @ArinaMgk
// OutFileFormat

#include "aasm.h"

outffmt* OutfileFormat::crt_format = NULL;

_ESYM_C struct outffmt* ofmt_register();

// register_output_formats = ofmt_register
void OutfileFormat::setMode() {
	crt_format = ofmt_register();
}

_ESYM_C struct outffmt** ofmt_f() {
	return &OutfileFormat::crt_format;
}
