#include "stdafx.h"

#include <SDKDDKVer.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <util.h>

/**
 * Helper: error
 */
bool error(wchar_t const* message) 
{
  DWORD error = ::GetLastError();
  logger->error(L"forge.exe error -> " + wstring(message) + L" -> " + boost::lexical_cast<wstring>(error));
  wprintf(L"forge.exe error: %s - %u\n", message, error);
  return false;
}

/**
 * SetProcessPrivilege
 */
bool SetProcessPrivilege(bool enable)
{
  logger->debug(L"forge.exe SetProcessPrivilege -> " + boost::lexical_cast<wstring>(enable));
  
  bool ok = true;
  HANDLE token = nullptr;
  LUID luid;
  TOKEN_PRIVILEGES privileges = { 0 };
  wstring etext;

  for (;;) {
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &token)) {
      etext = L"SetProcessPrivilege ::GetCurrentProcess";
      break;
    }

    if (!::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
      etext = L"SetProcessPrivilege ::LookupPrivilegeValue";
      break;
    }

    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Luid = luid;
    privileges.Privileges[0].Attributes = (enable ? SE_PRIVILEGE_ENABLED : 0);

    if (!::AdjustTokenPrivileges(token, false, &privileges, 0, NULL, NULL))
      etext = error(L"SetProcessPrivilege ::AdjustTokenPrivileges");

    break;
  }

  if (!etext.empty())
    error(etext.c_str());

  if (token)
    ::CloseHandle(token);

  return etext.empty();
}

/**
 * InjectDLL
 */
bool InjectDLL(DWORD processId, wchar_t *dll)
{
  HANDLE process = nullptr;
  LPVOID address = nullptr;
  HMODULE module = nullptr;
  HANDLE thread = nullptr;
  LPTHREAD_START_ROUTINE LoadLibraryW = nullptr;

  wchar_t path[MAX_PATH] = { 0 };
  wstring etext;

  for (;;) {
    if (!dll) {
      etext = L"null dll name";
      break;
    }
    logger->debug(L"forge.exe InjectDLL -> " + boost::lexical_cast<wstring>(processId)+L" -> " + wstring(dll));

    if (!SetProcessPrivilege(true)) {
      etext = L"InjectDLL SetProcessPrivilege";
      break;
    }

    process = ::OpenProcess(PROCESS_ALL_ACCESS, false, processId);
    if (!process) {
      etext = L"InjectDLL ::OpenProcess";
      break;
    }

    if (!::GetModuleFileName(NULL, path, MAX_PATH)) {
      etext = L"InjectDLL ::GetModuleFileName";
      break;
    }

    wchar_t* sp = wcsrchr(path, L'\\') + 1;
    wcscpy_s(sp, path + MAX_PATH - sp, dll);

    address = ::VirtualAllocEx(process, NULL, sizeof(path), MEM_COMMIT, PAGE_READWRITE);
    if (!address) {
      etext = L"InjectDLL ::VirtualAllocEx";
      break;
    }

    if (!::WriteProcessMemory(process, address, path, sizeof(path), NULL)) {
      etext = L"InjectDLL ::WriteProcessMemory";
      break;
    }

    module = ::GetModuleHandle(L"Kernel32");
    if (!module) {
      etext = L"InjectDLL ::GetModuleHandle";
      break;
    }
    
    LoadLibraryW = (LPTHREAD_START_ROUTINE)::GetProcAddress(module, "LoadLibraryW");
    if (LoadLibraryW == NULL) {
      etext = L"InjectDLL ::GetProcAddress";
      break;
    }

    thread = ::CreateRemoteThread(process, NULL, 0, LoadLibraryW, address, 0, NULL);
    if (!thread) {
      etext = L"InjectDLL ::CreateRemoteThread";
      break;
    }

    ::WaitForSingleObject(thread, INFINITE);
    break;
  }

  if (!etext.empty())
    error(etext.c_str());

  if (address)
    ::VirtualFreeEx(process, address, sizeof(path), MEM_RELEASE);
  
  if (process)
    ::CloseHandle(process);

  if (thread)
    ::CloseHandle(thread);
  
  SetProcessPrivilege(false);

  return etext.empty();
}

/**
 * main
 */
int _tmain(int argc, TCHAR* argv[])
{
  // get path
  wchar_t buf[MAX_PATH] = { 0 };
  ::GetModuleFileName(NULL, buf, MAX_PATH);
  boost::filesystem::wpath modulePath = boost::filesystem::wpath(buf).parent_path();

  // initialize logger
  logger->initialize(modulePath);
  logger->debug(L"forge.exe _tmain -> " + modulePath.wstring());

  DWORD exitCode = 1;
  if (argc > 0) {
#ifdef _WIN64
    bool ret = InjectDLL((DWORD)_wtoi(argv[0]), L"frame64.dll");
#else
    bool ret = InjectDLL((DWORD)_wtoi(argv[0]), L"frame32.dll");
#endif
    exitCode = ret ? 0 : 2;
  }

  logger->debug(L"forge.exe _tmain exiting -> " + boost::lexical_cast<wstring>(exitCode));

  /**
   * Possible exit codes:
   * 0 - success
   * 1 - invalid/missing command line arguments
   * 2 - DLL injection failed
   */
  return exitCode;
}
