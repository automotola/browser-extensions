#include "stdafx.h"
#include "Registry.h"
#include <strsafe.h> /* for StringCchCopy */

/**
 * open key
 */
bool Registry::open()
{
  LONG result = ::RegOpenKeyEx(root, subkey.c_str(), 0, KEY_READ | KEY_WRITE, &key);
  if (result != ERROR_SUCCESS) {
    logger->debug(L"Registry::open failed -> " + boost::lexical_cast<wstring>(root)+L" -> " + subkey);
    return false;
  }
  return true;
}


/** 
 * create key if it doesn't exist
 */
bool Registry::create() 
{
  wstring const sroot(boost::lexical_cast<wstring>(root));
  if (open()) {
    logger->debug(L"Registry::create key exists -> " + sroot + L" -> " + subkey);
    return true;
  }

  logger->debug(L"Registry::create -> " + sroot + L" -> " + subkey);

  LONG result = ::RegCreateKeyEx(root, subkey.c_str(), 0, 0, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, 0, &key, 0);
  if (result != ERROR_SUCCESS) {
    logger->debug(L"Registry::create failed -> " + sroot + L" -> " + subkey);
    return false;
  }

  return true;
}


/**
 * set key 
 */
bool Registry::set() 
{
  wstring const sroot(boost::lexical_cast<wstring>(root));
  logger->debug(L"Registry::set"
    L" -> " + sroot +
    L" -> " + subkey +
    L" -> " + name +
    L" -> " + value);

  if (!open()) {
    return false;
  }

  LONG result;
  switch (type) {
  case Registry::string:
    result = ::RegSetValueEx(key, name.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), (DWORD)((value.length() * 2) + 1));
    break;
  case Registry::dword:
    result = ::RegSetValueEx(key, name.c_str(), 0, REG_DWORD, (LPBYTE)&value_dword, sizeof(DWORD));
    break;
  default:
    logger->error(L"Registry::set unknown type -> " + boost::lexical_cast<wstring>(type));
    return false;
    break;
  }

  if (result != ERROR_SUCCESS) {
    logger->debug(L"Registry::set failed"
      L" -> " + sroot +
      L" -> " + subkey +
      L" -> " + name +
      L" -> " + value);
    return false;
  }

  return true;
}


/** 
 * delete key
 */

bool Registry::del() 
{
  logger->debug(L"Registry::del -> " + boost::lexical_cast<wstring>(this->root) + L" -> " + subkey);

  if (!open())
    return false;

  TCHAR buf[MAX_PATH * 2] = { 0 };
  StringCchCopy(buf, MAX_PATH * 2, subkey.c_str());

  return rdel(root, buf) != FALSE;
}


// from: http://msdn.microsoft.com/en-us/library/ms724235(VS.85).aspx
bool Registry::rdel(HKEY hKeyRoot, LPTSTR lpSubKey)
{
  logger->debug(L"Registry::rdel -> " + wstring(lpSubKey));

  LPTSTR lpEnd = nullptr;
  LONG result;
  DWORD size;
  TCHAR szName[MAX_PATH * 2] = { 0 };
  HKEY hKey;
  FILETIME ftWrite;

  // First, see if we can delete the key without having to recurse.
  result = ::RegDeleteKey(hKeyRoot, lpSubKey);
  if (result == ERROR_SUCCESS)
    return true;
  
  result = ::RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);
  if (result == ERROR_FILE_NOT_FOUND) {
    return true;
  } else if (result != ERROR_SUCCESS) {
    logger->debug(L"Registry::rdel error opening key");
    return false;
  }

  // Check for an ending slash and add one if it is missing.
  lpEnd = lpSubKey + lstrlen(lpSubKey);
  if (*(lpEnd - 1) != TEXT('\\')) {
    *lpEnd = TEXT('\\');
    lpEnd++;
    *lpEnd = TEXT('\0');
  }

  // Enumerate the keys
  size = MAX_PATH;
  result = ::RegEnumKeyEx(hKey, 0, szName, &size, NULL, NULL, NULL, &ftWrite);
  if (result == ERROR_SUCCESS) {
    do {
      StringCchCopy(lpEnd, MAX_PATH * 2, szName);
      if (!rdel(hKeyRoot, lpSubKey)) {
        break;
      }
      size = MAX_PATH;
      result = ::RegEnumKeyEx(hKey, 0, szName, &size, NULL,
        NULL, NULL, &ftWrite);
    } while (result == ERROR_SUCCESS);
  }
  lpEnd--;
  *lpEnd = TEXT('\0');
  ::RegCloseKey(hKey);

  // Try again to delete the key.
  result = ::RegDeleteKey(hKeyRoot, lpSubKey);

  return result == ERROR_SUCCESS;
}
