#include "common.h"

int _fltused;

__declspec(align(16)) const DWORD BN32ABS[4] = {0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF};
//__declspec(align(16)) const float BNPACK1PZ[4] = {1.0f, 1.0f, 1.0f, 1.0f};
__declspec(align(16)) const DWORD dwFLTNEGATIF[4] = {0x80000000,0x80000000,0x80000000,0x80000000};


const float BNDCMS[6] = {0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f};
const float flt10000PZERO = 10000.0f;

