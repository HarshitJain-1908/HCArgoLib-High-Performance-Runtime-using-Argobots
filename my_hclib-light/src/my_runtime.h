#include "hclib.h"

void HCArgoLib_init(int argc, char *argv[]);
void HCArgoLib_finalize();
void HCArgoLib_kernel(generic_frame_ptr fct_ptr, void * arg);
void HCArgoLib_finish(generic_frame_ptr fct_ptr, void * arg);
void HCArgoLib_async(generic_frame_ptr fct_ptr, void * arg);