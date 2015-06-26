#include "stdafx.h"
#include "resource.h"
#include <generated/Forge_i.h>
#include "dllmain.h"
#include "xdlldata.h"


/**
 * Used to determine whether the DLL can be unloaded by OLE.
 */
STDAPI DllCanUnloadNow(void)
{
  logger->debug(L"Forge::dllexports::DllCanUnloadNow");

  // clean up
  HRESULT hr = _AtlModule.DllCanUnloadNow();
  logger->debug(L"Forge::dllexports::DllCanUnloadNow  -> " + logger->parse(hr));

  return hr;
}

/**
 * Returns a class factory to create an object of the requested type.
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#if 0
  LPOLESTR tmp = nullptr;
  ::StringFromCLSID(rclsid, &tmp);
  wstring requested_clsid;
  if (tmp) {
    requested_clsid = wstring(tmp);
    ::CoTaskMemFree(tmp);
  }
#endif

  return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

/**
 * DllRegisterServer - Adds entries to the system registry.
 */
STDAPI DllRegisterServer(void)
{
  logger->debug(L"Forge::dllexports::DllRegisterServer");
  // registers object, typelib and all interfaces in typelib
  return _AtlModule.DllRegisterServer();
}

/**
 * DllUnregisterServer - Removes entries from the system registry.
 */
STDAPI DllUnregisterServer(void)
{
  logger->debug(L"Forge::dllexports::DllUnregisterServer");
	return _AtlModule.DllUnregisterServer();
}

/**
 * DllInstall - Adds/Removes entries to the system registry per user per machine.
 */
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
  logger->debug(L"Forge::dllexports::DllInstall");

  HRESULT hr = E_FAIL;
  static const wchar_t szUserSwitch[] = L"user";

  if (pszCmdLine)
    if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0)
      ATL::AtlSetPerUserRegistration(true);

  if (bInstall) {
    hr = DllRegisterServer();
    if (FAILED(hr))
      DllUnregisterServer();
  } else
    hr = DllUnregisterServer();

  return hr;
}
