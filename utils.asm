EXTRN BN32ABS : XMMWORD
EXTRN flt10000PZERO : DWORD
EXTRN BNDCMS : DWORD
EXTRN dwFLTNEGATIF : XMMWORD

.code

; UINT64 bnhextouqw(RCX = char *psz)
ALIGN 16
bnhextouqw PROC
  xor       edx, edx
  xor       eax, eax
  mov       r8d, 16
nbrLoop:
  mov       dl, [rcx]
  sub       dl, 48
  add       rcx, 1
  cmp       dl, 9 ; LE PLUS PROBABLE
  jbe       short go09
  cmp       dl, 54  ; 'f'
  ja        short nbrStop
  cmp       dl, 49  ; 'a'
  jae       short goMIN
  cmp       dl, 22  ; 'F'
  ja        short nbrStop
  cmp       dl, 17  ; 'A'
  jb        short nbrStop
  sub       dl, 7
  jmp       short go09
goMIN:
  sub       dl, 39
go09:
  add       rax, rax
  sub       r8d, 1
  lea       rax, [rax * 8 + rdx]
  jne       short nbrLoop
nbrStop:
  ret       0
bnhextouqw ENDP

ALIGN 16
bnitoa PROC ; ECX = DWORD dwnum, RDX = char* szdst
  test    ecx, ecx
  jns     short bnultoa
  mov     byte ptr[rdx], 45
  neg     ecx
  add     rdx, 1
bnitoa ENDP
bnultoa PROC ; ECX = DWORD dwnum, RDX = char* szdst
  cmp     ecx, 10
  jae     short sup9
  add     cl, 48
  lea     rax, [rdx+1]
  mov     byte ptr[rdx], cl
  ret     0
sup9:
  lea     r9, [rdx + 2] ; R9 pointeur final SANS LE ZERO
  cmp     ecx, 100
  jb      short lenOK
  add     r9, 1
  cmp     ecx, 1000
  jb      short lenOK
  add     r9, 1
  cmp     ecx, 10000
  jb      short lenOK
  add     r9, 1
  cmp     ecx, 100000
  jb      short lenOK
  add     r9, 1
  cmp     ecx, 1000000
  jb      short lenOK
  add     r9, 1
  cmp     ecx, 10000000
  jb      short lenOK
  add     r9, 1
  cmp     ecx, 100000000
  jb      short lenOK
  add     r9, 1
  cmp     ecx, 1000000000
  jb      short lenOK
  add     r9, 1
lenOK:
  mov     r8, r9
toASC:
  mov     eax, -858993459
  mul     ecx
  sub     r8, 1
  shr     edx, 3
  mov     eax, ecx
  lea     ecx, [edx+edx*8]
  sub     eax, edx
  sub     eax, ecx
  add     al, 48
  test    edx, edx
  mov     [r8], al
  mov     ecx, edx
  jnz     short toASC
  mov     rax, r9
  ret     0
bnultoa ENDP

bnuqwtoa PROC ; RCX = uqw, RDX = *psz
  mov       r8, rsp
  mov       r9, rdx
L1:
  mov       rax, -3689348814741910323
  mul       rcx
  sub       r8, 1
  shr       rdx, 3
  mov       rax, rcx
  lea       rcx, [rdx+rdx*8]
  sub       rax, rdx
  sub       rax, rcx
  add       al, 48
  test      rdx, rdx
  mov       [r8], al
  mov       rcx, rdx
  jnz       short L1
  mov       rax, rsp
  lddqu     xmm0, xmmword ptr[r8]
  mov       ecx, [r8 + 16]
  sub       rax, r8
  movdqu    xmmword ptr[r9], xmm0
  mov       [r9 + 16], ecx
  add       rax, r9
  ret       0
bnuqwtoa ENDP

bnFloatToa PROC ; XMM0 = float flt, RDX = char *pdst
  xorps     xmm2, xmm2
  xor       eax, eax              ; EAX = sign
  ucomiss   xmm0, xmm2
  je        short toZERO
  ja        short supZERO
  andps     xmm0, BN32ABS         ; flt = -flt
  mov       eax, 1                ; sign = 1
supZERO:
  cvttss2si rcx, xmm0             ; RCX = PARTIE ENTIERE
  cvtsi2ss  xmm2, rcx
  subss     xmm0, xmm2
  mov       r8, rcx
  mulss     xmm0, dword ptr flt10000PZERO
  cvtss2si  r10d, xmm0              ; R10d = PARTIE FRACTIONNAIRE
  cmp       r10d, 9999
  jb        short okFRACT
  add       rcx, 1
  xor       r10d, r10d
  jmp       short noZERO
okFRACT:
  or        r8, r10
  jne       short noZERO
toZERO:
  lea       rax, [rdx + 1]
  mov       byte ptr[rdx], 48
  ret       0
noZERO:
  test      eax, eax
  je        short noSIGN
  mov       byte ptr[rdx], '-'
  add       rdx, 1
noSIGN: ; RCX = PARTIE ENTIERE, R10d = PARTIE FRACTIONNAIRE, RDX = psz
  mov       r8, rsp
  mov       r9, rdx
L1:
  mov       rax, -3689348814741910323
  mul       rcx
  sub       r8, 1
  shr       rdx, 3
  mov       rax, rcx
  lea       rcx, [rdx+rdx*8]
  sub       rax, rdx
  sub       rax, rcx
  add       al, 48
  test      rdx, rdx
  mov       [r8], al
  mov       rcx, rdx
  jnz       short L1
  lea       rax, [r9 + rsp]
  lddqu     xmm0, xmmword ptr[r8]
  mov       ecx, [r8 + 16]
  sub       rax, r8
  movdqu    xmmword ptr[r9], xmm0
  mov       [r9 + 16], ecx
  lea       r8, [rax + 5]      ; R8 = EVENTUEL POINTEUR ECRITURE
  test      r10d, r10d
  mov       r9, rax
  je        ftoaEXIT
  mov       r9, r8            ; R9 = POINTEUR SORTIE
  mov       ecx, r10d
  mov       byte ptr[r8 - 5], '.'
  mov       byte ptr[r8 - 4], 48
  mov       byte ptr[r8 - 3], 48
  mov       byte ptr[r8 - 2], 48
L2:
  mov       eax, -858993459
  mul       ecx
  sub       r8, 1
  shr       edx, 3
  mov       eax, ecx
  lea       ecx, [edx+edx*8]
  sub       eax, edx
  sub       eax, ecx
  add       al, 48
  test      edx, edx
  mov       [r8], al
  mov       ecx, edx
  jnz       short L2
ftoaEXIT:
  mov       rax, r9
  ret       0
bnFloatToa ENDP

; '-' ET 19 CHIFFRES MAXI EN PARTIE ENTIERE
; 6 CHIFFRES MAXI EN PARTIE FRACTIONNAIRE
bnatof PROC ; RCX = *psrc
  movzx     edx, byte ptr[rcx]
  xorps     xmm0, xmm0
  xor       r8d, r8d
  xor       eax, eax
  mov       r9d, 19
  movaps    xmm2, xmm0
  cmp       edx, '-'
  jb        short badNBR
  jne       short noSIGN
  add       rcx, 1
  movaps    xmm2, dwFLTNEGATIF
intLOOP:
  movzx     edx, byte ptr[rcx]
  mov       r8, rax       ; R8 RETIENT LA PARTIE ENTIERE
  cmp       edx, '.'
  je        short goPOINT
noSIGN:
  sub       edx, 48
  cmp       edx, 9
  ja        short nbrINTSTOP
  sub       r9d, 1
  jz        short fractSTOP
  lea       rax, [rax+rax*4]
  add       rcx, 1
  lea       rax, [rax*2+rdx]
  jmp       short intLOOP
nbrINTSTOP:
  cvtsi2ss  xmm0, rax
  test      rax, rax
  jne       short verifNEG      ; PAS DE NEGATIF SI 0
badNBR:
  ret       0
goPOINT: ;   R8 = PARTIE ENTIERE
  add       rcx, 1     ; ON PASSE LE POINT
  mov       r9d, 6
  xor       eax, eax
  mov       r10, offset BNDCMS - 4
fractLOOP:
  movzx     edx, byte ptr[rcx]
  sub       edx, 48
  cmp       edx, 9
  ja        short fractSTOP
  add       r10, 4
  lea       rax, [rax+rax*4]
  add       rcx, 1
  lea       rax, [rax*2+rdx]
  sub       r9d, 1  ; AUTRES SI Y SONT
  jnz       short fractLOOP
fractSTOP:
  cvtsi2ss  xmm1, rax
  cvtsi2ss  xmm0, r8
  or        rax, r8
  je        short badNBR         ; PAS DE NEGATIF SI 0
  mulss     xmm1, dword ptr[r10]
  addss     xmm0, xmm1
verifNEG:
  orps      xmm0, xmm2
  ret       0
bnatof ENDP


; char* bnbytetohexa(CL = BYTE btnum, RDX = char* szdst
ALIGN 16
bnbytetohexa PROC
  test      cl, cl
  jne       short noZERO
  lea       rax, [rdx + 1]
  mov       byte ptr[rdx], 48
  ret       0
noZERO:
  mov       al, cl
  and       al, 15
  shr       cl, 4
  add       al, 48
  add       cl, 48
  cmp       al, 57
  jbe       short loINF58
  add       al, 7
loINF58:
  cmp       cl, 57
  jbe       short hiINF58
  add       cl, 7
hiINF58:
  mov       byte ptr[rdx + 1], al
  mov       byte ptr[rdx], cl
  lea       rax, [rdx + 2]
  ret       0
bnbytetohexa ENDP


; char* bnqwtohexa(RCX = UINT64 qwnum, RDX = char* szdst
ALIGN 16
bnqwtohexa PROC
  test      rcx, rcx
  jne       short noZERO
  lea       rax, [rdx + 2]
  mov       byte ptr[rdx], 48
  mov       byte ptr[rdx + 1], 48
  ret       0
noZERO:
  bswap     rcx
  mov       r8d, 8
delZEROLEFT:
  test      cl, cl
  jne       short goHEXA
  sub       r8d, 1
  shr       rcx, 8
  jmp       short delZEROLEFT
goHEXA:
  mov       al, cl
  and       al, 15
  shr       cl, 4
  add       al, 48
  add       cl, 48
  cmp       al, 57
  jbe       short loINF58
  add       al, 7
loINF58:
  cmp       cl, 57
  jbe       short hiINF58
  add       cl, 7
hiINF58:
  mov       byte ptr[rdx + 1], al
  mov       byte ptr[rdx], cl
  shr       rcx, 8
  add       rdx, 2
  sub       r8d, 1
  jne       short goHEXA
  mov       rax, rdx
  ret       0
bnqwtohexa ENDP




; DWORD bnatoi(RCX = char *szsrc)
ALIGN 16
bnatoi PROC
  movzx     edx, byte ptr[rcx]
  xor       eax, eax
  xor       r8d, r8d
  cmp       edx, '-'
  jb        short nbrEXIT
  jne       short noSIGN
  add       rcx, 1
  sub       r8d, 1
nbrLoop:
  movzx     edx, byte ptr[rcx]
noSIGN:
  sub       edx, 48
  cmp       edx, 9
  ja        short nbrStop
  lea       eax, [eax+eax*4]
  lea       eax, [eax*2+edx]
  movzx     edx, byte ptr[rcx + 1]
  sub       edx, 48
  cmp       edx, 9
  ja        short nbrStop
  lea       eax, [eax+eax*4]
  lea       eax, [eax*2+edx]
  movzx     edx, byte ptr[rcx + 2]
  sub       edx, 48
  cmp       edx, 9
  ja        short nbrStop
  lea       eax, [eax+eax*4]
  lea       eax, [eax*2+edx]
  add       rcx, 3
  jmp       short nbrLoop
 nbrStop:
  add       eax, r8d
  xor       eax, r8d
 nbrEXIT:
  ret       0
bnatoi ENDP







END
