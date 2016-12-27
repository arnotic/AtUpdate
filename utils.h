#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif


char* __fastcall bnitoa(int num, char* szdst);
char* __fastcall bnultoa(DWORD unum, char* szdst);

char* __fastcall bnuqwtoa(unsigned __int64 qwnum, char* szdst);

char* __fastcall bnFloatToa(float fval, char* szdst);
float __fastcall bnatof(char *szsrc);

char* __fastcall bnbytetohexa(BYTE btnum, char* szdst);

char* __fastcall bnqwtohexa(UINT64 qwnum, char* szdst);
int __fastcall bnatoi(char *szsrc);
UINT64 __fastcall bnhextouqw(char *psz); // 0 to 9, A to F, a to f



#ifdef __cplusplus
}
#endif

#endif
