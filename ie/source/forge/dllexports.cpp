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

  HRESULT hr = S_OK;
  static const wchar_t szUserSwitch[] = L"user";

  for (;;) {
    if (pszCmdLine)
      if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0)
        hr = ATL::AtlSetPerUserRegistration(true);
    BreakOnFailed(hr);

    if (bInstall) {
      hr = DllRegisterServer();
      if (FAILED(hr))
        hr = DllUnregisterServer();
    }
    else
      hr = DllUnregisterServer();

    break;
  }  

  return hr;
}
