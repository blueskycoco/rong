#ifndef __SAL_PRES__INC__HH__
#define __SAL_PRES__INC__HH__
#include "sal/sal.h"
//pres request
MS2_PUBLIC int sal_request_presentation(SalOp *op);
//release presentation
MS2_PUBLIC int sal_release_presentation(SalOp *op);
//request pres response
MS2_PUBLIC int sal_presentation_response(SalOp *op,int accepted);
#endif