#include "common.h"
#include "resource.h"

#define LU_OK 0
#define LU_ERR_OPEN_PORT 1
#define LU_ERR_REBOOT 2
#define LU_ERR_SYNC 3
#define LU_ERR_PROG_MODE 4
#define LU_ERR_LOAD_ADDR 5
#define LU_ERR_WRITE_PAGE 6
#define LU_ERR_BAD_DEVICE 7
#define LU_DEVICE 0x141E95

HINSTANCE hinst = 0;

char szappname[] = "AtUpdate";

HWND hmain = 0;
HWND hCbLstCom = 0;
HWND hEdLog = 0;
BYTE bHex[32768] = { 255 };

void addToLog(char *msg)
{
  int iLenght;

  iLenght = GetWindowTextLength(hEdLog);
  SendMessage(hEdLog, EM_SETSEL, (WPARAM)iLenght, (LPARAM)iLenght);
  SendMessage(hEdLog, EM_REPLACESEL, 0, (LPARAM)msg);
}

DWORD DlgLoadFile(char *szFile, char *szfilter, char *szname)
{
  OPENFILENAME ofn;
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hInstance = 0;
  ofn.hwndOwner = hmain;
  ofn.lpstrFilter = szfilter;
  ofn.lpstrFile = szFile;
  ofn.lpstrCustomFilter = ofn.lpstrFileTitle = 0;
  ofn.nFileExtension = ofn.nFileOffset = 0;
  ofn.lCustData = ofn.dwReserved = 0;
  ofn.lpTemplateName = ofn.lpstrInitialDir = ofn.lpstrDefExt = 0;
  ofn.lpfnHook = 0; ofn.pvReserved = 0;
  ofn.nFileExtension = ofn.nFileExtension = 0;
  ofn.nMaxCustFilter = ofn.nMaxFileTitle = 0;
  ofn.FlagsEx = OFN_EX_NOPLACESBAR;
  ofn.nFilterIndex = 1; ofn.nMaxFile = 1024;
  ofn.lpstrTitle = szname;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;
  szFile[0] = 0;
  if (!GetOpenFileName(&ofn)) return 0;
  return 1;
}

void serialDrain(HANDLE hCom)
{
	char buf[1];
	DWORD rw;

	while (1) {
		if (ReadFile(hCom, buf, 1, &rw, 0) == 0) break;
		if (rw != 1) break;
	}
}

byte serialRead(HANDLE hCom)
{
  char buf[1] = { 0 };
  DWORD rw;

  while (1) {
    if (ReadFile(hCom, buf, 1, &rw, 0) == 0) break;
    if(rw) break;
  }
  return buf[0];
}

void enumeratePortCom()
{
  char buf[8];
  char *c;
  DWORD dwIdx;
  HANDLE hCom;

  buf[0] = 'C';
  buf[1] = 'O';
  buf[2] = 'M';
  c = buf + 3;

  SendMessage(hCbLstCom, CB_RESETCONTENT, 0, 0);

  dwIdx = 0;
  while (dwIdx < 255) {
    *bnultoa(dwIdx++, c) = 0;
    
    hCom = CreateFile(buf, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hCom == INVALID_HANDLE_VALUE) continue;
    CloseHandle(hCom);
    
    SendMessage(hCbLstCom, CB_ADDSTRING, 0, (LPARAM)buf);
  }

  SendMessage(hCbLstCom, CB_SETCURSEL, 0, 0);
}

/*
  loadHexFile

  szPath        : Path of Hex file
  bHex          : Pointer to buffer (must be allocated)
  dwMaxSize     : Max size for the buffer

  return value  : -1 if error otherwise nb of bytes read
*/
DWORD loadHexFile(char *szPath, BYTE *bHex, DWORD dwMaxSize)
{
	DWORD r = -1; // PRESUME ERREUR
	DWORD rw;
	DWORD nbToRead;
	DWORD idx;
  BYTE bCheckSum;
	HANDLE hfl;
	BYTE *p;
  BYTE b;
	char buf[264]; // 255 DE LECTURE POUR LE FICHIER (VOIR FILE FORMAT) + BUFFER PERSO

	/*
	FILE FORMAT
	[:][Byte Count][Address][Record Type][Data][Checksum]
	So, the following line of the hex file can be split like this:
	[:][10] [0060] [00] [0C944F000C944F0011241FBECFEFD4E0] [2E]

  Ref : https://en.wikipedia.org/wiki/Intel_HEX
	*/

	p = bHex;

	hfl = CreateFile(szPath, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (hfl == INVALID_HANDLE_VALUE) goto onEXIT;

onNEXT:
	ReadFile(hfl, buf, 9, &rw, 0);
  if (rw < 9) goto onCLOSEFL;
	if (buf[0] != ':') goto onCLOSEFL;
  
  b = buf[3];
  buf[3] = 0;
	nbToRead = (DWORD)bnhextouqw((buf + 1));
  if (nbToRead == 0) goto onFINISH;
  bCheckSum = (BYTE)nbToRead;
  buf[3] = b;

  b = buf[5];
  buf[5] = 0;
  bCheckSum += (BYTE)bnhextouqw((buf + 3));
  buf[5] = b;

  buf[7] = 0;
  b = (BYTE)bnhextouqw((buf + 5));
  bCheckSum += b;

  // TYPE D'ENREGISTREMENT, END OF FILE ?
  if (b == 1) goto onFINISH;

  // LECTURE DES BYTES
	nbToRead *= 2;
	ReadFile(hfl, buf, nbToRead, &rw, 0);
  if (rw < nbToRead) goto onCLOSEFL;

	idx = 0;  
	while (1) {
		buf[256] = buf[idx++];
		buf[257] = buf[idx++];
		buf[258] = 0;

    b = (BYTE)bnhextouqw((buf + 256));
    *(p++) = b;
    bCheckSum += b;
		if (idx >= rw) break;

    // DEPASSE T'ON LE BUFFER ?
		if((p - bHex) >= dwMaxSize) goto onCLOSEFL;
	}

  // VERIFICATION DU CHECKSUM
  bCheckSum = ~bCheckSum;
  bCheckSum += 1;
  ReadFile(hfl, buf, 4, &rw, 0); // ASSUME [CR][LF] COMME RETOUR A LA LIGNE
  if (rw < 2) goto onCLOSEFL;
  buf[2] = 0;
  b = (BYTE)bnhextouqw(buf);
  if (bCheckSum != b) goto onCLOSEFL;

  goto onNEXT;

onFINISH:
	r = (DWORD)(p - bHex);
markFF:
	// ON TERMINE DE REMPLIR AVEC FF
	*(p++) = (BYTE)0xFF;	
	if ((p - bHex) >= dwMaxSize) goto onCLOSEFL;
	goto markFF;
onCLOSEFL:
	CloseHandle(hfl);
onEXIT:
	return r;
}

/*
  programMCU

  szCOM         : Path of port com
  bHex          : Pointer to buffer with data to write
  dwSize        : Size of bHex

  return value  : > 0 if error otherwise 0
*/
DWORD programMCU(char *szCOM, BYTE *bHex, DWORD dwSize)
{
  char buf[256];
  DCB dcbSerialParams = { 0 };
  COMMTIMEOUTS timeouts = { 0 };

  DWORD dwError = LU_OK;
  DWORD dwDeviceSignature = 0;
  DWORD idx;
  DWORD i;
  DWORD address;
  DWORD rw;

  HANDLE hCom;

  int iTotalBytes;
  int r;
  
  BYTE rRX;

  iTotalBytes = dwSize;

  hCom = CreateFile(szCOM, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
  if (hCom == INVALID_HANDLE_VALUE) {
    dwError = LU_ERR_OPEN_PORT;
    goto onERROR;
  }

  // Get config
  dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
  GetCommState(hCom, &dcbSerialParams);

  // Set config
  dcbSerialParams.BaudRate = CBR_115200;
  dcbSerialParams.ByteSize = 8;
  dcbSerialParams.StopBits = ONESTOPBIT;
  dcbSerialParams.Parity = NOPARITY;
  SetCommState(hCom, &dcbSerialParams);

  // Configure timeout
  timeouts.ReadIntervalTimeout = 50;
  timeouts.ReadTotalTimeoutConstant = 50;
  timeouts.ReadTotalTimeoutMultiplier = 10;
  timeouts.WriteTotalTimeoutConstant = 50;
  timeouts.WriteTotalTimeoutMultiplier = 10;
  SetCommTimeouts(hCom, &timeouts);

  // Clear DTR et RST
  dwError = LU_ERR_REBOOT;
  if (EscapeCommFunction(hCom, CLRDTR) == 0) goto onERROR;
  if (EscapeCommFunction(hCom, CLRRTS) == 0) goto onERROR;
  Sleep(250);

  // Set DTR et RST
  if (EscapeCommFunction(hCom, SETDTR) == 0) goto onERROR;
  if (EscapeCommFunction(hCom, SETRTS) == 0) goto onERROR;
  Sleep(50);
  
  serialDrain(hCom);

  // Get Sync
  dwError = LU_ERR_SYNC;

  buf[0] = STK_GET_SYNC;
  buf[1] = CRC_EOP;

  WriteFile(hCom, buf, 2, &rw, 0);
  rRX = serialRead(hCom);
  if (rRX != STK_INSYNC) goto onERROR;
  
  serialDrain(hCom);

  // Getting device signature
  buf[0] = STK_READ_SIGN;
  buf[1] = CRC_EOP;

  WriteFile(hCom, buf, 2, &rw, 0);
  rRX = serialRead(hCom);
  dwDeviceSignature |= rRX;
  dwDeviceSignature <<= 8;

  rRX = serialRead(hCom);
  dwDeviceSignature |= rRX;
  dwDeviceSignature <<= 8;

  rRX = serialRead(hCom);
  dwDeviceSignature |= rRX;
  
  dwError = LU_ERR_BAD_DEVICE;
  if (dwDeviceSignature != LU_DEVICE) goto onERROR;
  serialDrain(hCom);

  // Programming
  dwError = LU_ERR_PROG_MODE;
  
  buf[0] = STK_ENTER_PROGMODE;
  buf[1] = CRC_EOP;
  
  WriteFile(hCom, buf, 2, &rw, 0);
  rRX = serialRead(hCom);
  if (rRX != STK_INSYNC) goto onERROR;
  serialDrain(hCom);

  idx = 0;
  address = 0;
  while (1) {
    // Load address
    dwError = LU_ERR_LOAD_ADDR;
    buf[0] = STK_LOAD_ADDRESS;
    buf[1] = address & 0xff;
    buf[2] = (address >> 8) & 0xff;
    buf[3] = CRC_EOP;
    WriteFile(hCom, buf, 4, &rw, 0);
    rRX = serialRead(hCom);
    if (rRX != STK_INSYNC) goto onERROR;
    
    serialDrain(hCom);
    address += 64;

    dwError = LU_ERR_WRITE_PAGE;
    r = 128;
    buf[0] = STK_PROG_PAGE;
    buf[1] = 0;
    buf[2] = r;
    buf[3] = 'F';

    i = 4;
    while (1) {
      buf[i++] = bHex[idx++];
      if (i > 131) break;
    }

    buf[132] = CRC_EOP;
    WriteFile(hCom, buf, 133, &rw, 0);
    rRX = serialRead(hCom);
    if (rRX != STK_INSYNC) goto onERROR;
    serialDrain(hCom);

    iTotalBytes -= 128;
    if (iTotalBytes <= 0) break;
  }

  // Leave prog mode
  dwError = LU_ERR_PROG_MODE;
  buf[0] = STK_LEAVE_PROGMODE;
  buf[1] = CRC_EOP;

  WriteFile(hCom, buf, 2, &rw, 0);
  rRX = serialRead(hCom);
  if (rRX != STK_INSYNC) goto onERROR;

  dwError = LU_OK;
  CloseHandle(hCom);
  return dwError;
onERROR:
  if (hCom) CloseHandle(hCom);
  return dwError;
}


INT_PTR AppDlgProc(HWND hdlg, UINT mssg, WPARAM wParam, LPARAM lParam)
{
  char buf[MAX_PATH];
  int r;
  DWORD dwSizeOfHex;

  switch(mssg) {
    case WM_INITDIALOG:
      //SetClassLongPtr(hdlg, GCLP_HICON, (LONG_PTR) LoadIcon(hinst, MAKEINTRESOURCE(IDI_APP)));
      SetClassLongPtr(hdlg, GCLP_HICON, (LONG_PTR) LoadIcon(0, IDI_APPLICATION));

      hmain = hdlg;

      hCbLstCom = GetDlgItem(hdlg, IDCB_LSTCOM);
      hEdLog = GetDlgItem(hdlg, IDED_LOG);

      enumeratePortCom();

      return 1;
    case WM_COMMAND:
      switch(wParam) {
        case IDOK:
          if (!DlgLoadFile(buf, "HEX files\0*.hex\0\0", szappname)) goto onENABLE_CLOSE;

          EnableWindow(GetDlgItem(hdlg, IDOK), 0);
          EnableWindow(GetDlgItem(hdlg, IDCANCEL), 0);

			    SetWindowText(hEdLog, "");
			    
          addToLog("Check file...\r\n");
          dwSizeOfHex = loadHexFile(buf, bHex, 32728);
			    if (dwSizeOfHex == -1) {
				    addToLog("Error on file format...\r\n");
            goto onENABLE_CLOSE;
			    }

          *bnultoa(dwSizeOfHex, buf) = 0;
          addToLog("File OK: ");
          addToLog(buf);
          addToLog(" octets\r\n");
         
          r = (int)SendMessage(hCbLstCom, CB_GETCURSEL, 0, 0);
          if (r == -1) goto onENABLE_CLOSE;
          SendMessage(hCbLstCom, CB_GETLBTEXT, r, (LPARAM)&buf);
			    

          addToLog("Upload program...\r\n");

          dwSizeOfHex = programMCU(buf, bHex, dwSizeOfHex);
          
          switch (dwSizeOfHex) {
            case LU_OK:
              addToLog("Upload finish...\r\n");
              break;
            case LU_ERR_OPEN_PORT:
              addToLog("Unable to open the port...\r\n");
              break;
            case LU_ERR_SYNC:
              addToLog("Can't get sync...\r\n");
              break;
            case LU_ERR_LOAD_ADDR:
              addToLog("Can't load address...\r\n");
              break;
            case LU_ERR_PROG_MODE:
              addToLog("Can't enter to prog mod...\r\n");
              break;
            case LU_ERR_REBOOT:
              addToLog("Can't reboot device...\r\n");
              break;
            case LU_ERR_WRITE_PAGE:
              addToLog("Can't write page...\r\n");
              break;
            case LU_ERR_BAD_DEVICE:
              addToLog("Bad device signature...\r\n");
              break;
            default:
              break;
          }

          onENABLE_CLOSE:
          EnableWindow(GetDlgItem(hdlg, IDCANCEL), 1);
          SendMessage(hdlg, WM_NEXTDLGCTL, 0, 0);
          return 0;
        case IDCANCEL: EndDialog(hdlg, 0);
      }
      return 0;
    case WM_CLOSE:
      break;
  }
  return 0;
}


#pragma comment(linker, "/entry:myWinMain")
void __fastcall myWinMain()
{
	hinst = GetModuleHandle(0);
  DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_APP), 0, AppDlgProc, 0);
  ExitProcess(0);
}


// Thread to read serial
//DWORD WINAPI readFromSerial(LPVOID lpParameter)
//{
//  char buf[1];
//  DWORD rw;
//  DWORD dwEventMask;
//  
//
//  do {
//    // Create event to wait RX data
//    SetCommMask(hCom, EV_RXCHAR);
//
//    // Wait data
//    WaitCommEvent(hCom, &dwEventMask, 0);
//
//    // Read data
//    ReadFile(hCom, buf, 1, &rw, 0);
//    addToLog(buf);
//  } while (rw > 0);
//
//  addToLog("End readFromSerial()\r\n");
//  return 0;
//}

//DWORD tstBootloader()
//{
//  char buf[256];
//  byte rRX;
//  DCB dcbSerialParams = { 0 };
//  COMMTIMEOUTS timeouts = { 0 };
//  int r;
//  unsigned int address;
//  DWORD rw;
//  char hex[512];
//  DWORD idx;
//  DWORD i;
//  DWORD dwDeviceSignature = 0;
//  DWORD dwError = 1;
//  hCom = 0;
//
//  r = SendMessage(hCbLstCom, CB_GETCURSEL, 0, 0);
//  if (r == -1) goto onERROR;
//  SendMessage(hCbLstCom, CB_GETLBTEXT, r, &buf);
// 
//  hCom = CreateFile(buf, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
//  if (hCom == INVALID_HANDLE_VALUE) goto onERROR;
//
//  // Get config
//  dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
//  GetCommState(hCom, &dcbSerialParams);
//
//  // Set config
//  dcbSerialParams.BaudRate = CBR_115200;
//  dcbSerialParams.ByteSize = 8;
//  dcbSerialParams.StopBits = ONESTOPBIT;
//  dcbSerialParams.Parity = NOPARITY;
//  SetCommState(hCom, &dcbSerialParams);
//
//  // Configure timeout
//  timeouts.ReadIntervalTimeout = 50;
//  timeouts.ReadTotalTimeoutConstant = 50;
//  timeouts.ReadTotalTimeoutMultiplier = 10;
//  timeouts.WriteTotalTimeoutConstant = 50;
//  timeouts.WriteTotalTimeoutMultiplier = 10;
//  SetCommTimeouts(hCom, &timeouts);
//
//  // Clear DTR et RST
//  addToLog("Clear DTR et RTS...\r\n");
//  if (EscapeCommFunction(hCom, CLRDTR) == 0) goto onERROR;
//  if (EscapeCommFunction(hCom, CLRRTS) == 0) goto onERROR;
//  Sleep(250);
//  
//  // Set DTR et RST
//  addToLog("Set DTR et RTS...\r\n");
//  if (EscapeCommFunction(hCom, SETDTR) == 0) goto onERROR;
//  if (EscapeCommFunction(hCom, SETRTS) == 0) goto onERROR;
//  Sleep(50);
//
//  serialDrain();
//
//  // Write Sync
//  buf[0] = STK_GET_SYNC;
//  buf[1] = CRC_EOP;
//
//  addToLog("Getting sync...\r\n");
//  WriteFile(hCom, buf, 2, &rw, 0);
//  rRX = serialRead();
//  if (rRX == STK_INSYNC) addToLog("Sync OK\r\n");
//  else {
//	  addToLog("Error sync\r\n");
//	  goto onERROR;
//  }
//  serialDrain();
//
//  // Getting device signature
//  buf[0] = STK_READ_SIGN;
//  buf[1] = CRC_EOP;
//
//  addToLog("Getting device signature...\r\n");
//  WriteFile(hCom, buf, 2, &rw, 0);
//  rRX = serialRead();
//  dwDeviceSignature |= rRX;
//  dwDeviceSignature <<= 8;
//
//  rRX = serialRead();
//  dwDeviceSignature |= rRX;
//  dwDeviceSignature <<= 8;
//
//  rRX = serialRead();
//  dwDeviceSignature |= rRX;
//  dwDeviceSignature <<= 8;
//
//  *bnqwtohexa(dwDeviceSignature, buf) = 0;
//  addToLog("Device signature : ");
//  addToLog(buf);
//  addToLog("\r\n");
//  serialDrain();
//
//  // Programming
//  buf[0] = STK_ENTER_PROGMODE;
//  buf[1] = CRC_EOP;
//  addToLog("Enter prog mode...\r\n");
//  WriteFile(hCom, buf, 2, &rw, 0);
//  rRX = serialRead();
//  if (rRX == STK_INSYNC) addToLog("Prog mode OK\r\n");
//  else {
//	  addToLog("Error prog mode\r\n");
//	  goto onERROR;
//  }
//  serialDrain();
//
//	idx = 0;
//  address = 0;
//	while (1) {
//		// Load address
//		buf[0] = STK_LOAD_ADDRESS;
//		buf[1] = address & 0xff;
//		buf[2] = (address >> 8) & 0xff;
//		buf[3] = CRC_EOP;
//		WriteFile(hCom, buf, 4, &rw, 0);
//		rRX = serialRead();
//		if (rRX == STK_INSYNC) addToLog("Load address OK\r\n");
//		else {
//			addToLog("Error Load address\r\n");
//			goto onERROR;
//		}
//		serialDrain();
//		address += 64;
//			
//		r = 128;
//		hex[0] = STK_PROG_PAGE;
//		hex[1] = 0;// r & 0xff;
//		hex[2] = r;// (r >> 8) & 0xff;
//		hex[3] = 'F';
//		
//		i = 4;
//		while (1) {
//			hex[i++] = bHex[idx++];
//			if (i > 131) break;
//		}
//
//		hex[132] = CRC_EOP;	  	  
//		WriteFile(hCom, hex, 133, &rw, 0);		  
//		rRX = serialRead();	
//		if (rRX == STK_INSYNC) addToLog("Write OK\r\n");
//		else {
//			addToLog("Error write\r\n");
//			goto onERROR;
//		}
//		serialDrain();
//	
//		dwSizeOfHex -= 128;
//		if (dwSizeOfHex <= 0) break;
//	}
//
//	// Leave prog mode
//	buf[0] = STK_LEAVE_PROGMODE;
//	buf[1] = CRC_EOP;
//
//	addToLog("Leaving prog mode...\r\n");
//	WriteFile(hCom, buf, 2, &rw, 0);
//	rRX = serialRead();
//	if (rRX == STK_INSYNC) addToLog("Leave OK\r\n");
//  else {
//    addToLog("Error leave\r\n");
//    goto onERROR;
//  }
//
//  dwError = 0;
//  CloseHandle(hCom);
//  return dwError;
//onERROR:
//  if (hCom) CloseHandle(hCom);
//  return dwError;
//}
