#include "stdafx.h"
#include "resource.h"
#include <generated/BHO_h.h>
#include "dllmain.h"
#include "vendor.h"

#include <bho/Registry.h>


// Used to determine whether the DLL can be unloaded by OLE.
STDAPI DllCanUnloadNow(void)
{
    logger->debug(L"BHO::dllexports::DllCanUnloadNow");

    // clean up

    return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type.
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr = S_OK; 

    LPOLESTR tmp;
    ::StringFromCLSID(rclsid, &tmp);
    wstring requested_clsid = wstring(tmp);
    ::CoTaskMemFree(tmp);

    // BrowserHelperObject
    wstring bho = VENDOR_UUID_STR(VENDOR_UUID_BrowserHelperObject);

    // get classobject
    if (requested_clsid.find(_AtlModule.moduleCLSID) == 0) {
        GUID guid;
        hr = ::CLSIDFromString(bho.c_str(), &guid);
        if (FAILED(hr)) {
            logger->error(L"BHO::dllexports::DllGetClassObject failed to get CLSIDFromString"
                          L" -> " + bho +
                          L" -> " + logger->parse(hr));
            return hr;
        }
        hr = _AtlModule.DllGetClassObject(guid, riid, ppv);
    } else {
        hr = _AtlModule.DllGetClassObject(rclsid, riid, ppv);
    }

    if (FAILED(hr)) {
        logger->error(L"BHO::dllexports::DllGetClassObject failed to get ClassObject"
                      L" -> " + logger->parse(hr));
    }

    return hr;
}

/**
 * DllRegisterServer - Adds entries to the system registry.
 */
STDAPI DllRegisterServer(void)
{
  logger->debug(L"BHO::dllexports::DllRegisterServer");

  // CLSID's
  wstring clsid = _AtlModule.moduleCLSID;
  wstring typelib = VENDOR_UUID_STR(VENDOR_UUID_ForgeBHOLib); // only changes between SDK versions

  wstring elevation32 = VENDOR_UUID_STR(VENDOR_UUID_ELEVATION32);
  wstring elevation64 = VENDOR_UUID_STR(VENDOR_UUID_ELEVATION64);

  wstring uuid = _AtlModule.moduleManifest->uuid;

  wstring hkcr, hklm_bho, hklm_button, hklm_elevation;

  /** CLSID registration
    %MODULE% = Z:\platform.master.hg\ie\build\Win32\Debug\forge32.dll
    HKEY_CLASSES_ROOT\CLSID\clsid = ${name}
    HKEY_CLASSES_ROOT\CLSID\clsid\TypeLib = typelib
    HKEY_CLASSES_ROOT\CLSID\clsid\Version = ${version}
    HKEY_CLASSES_ROOT\CLSID\clsid\InprocServer32 = %MODULE%
    HKEY_CLASSES_ROOT\CLSID\clsid\InProcServer32\ThreadingModel = Apartment
    */
  logger->debug(L"BHO::dllexports::DllRegisterServer registering CLSID");
  hkcr = L"CLSID\\" + clsid;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr).create()) goto error;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr, L"", _AtlModule.moduleManifest->name).set()) goto error;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr + L"\\TypeLib").create()) goto error;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr + L"\\TypeLib", L"", typelib).set()) goto error;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr + L"\\Version").create()) goto error;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr + L"\\Version", L"", _AtlModule.moduleManifest->version).set()) goto error;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr + L"\\InProcServer32").create()) goto error;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr + L"\\InProcServer32", L"", _AtlModule.moduleExec.wstring()).set()) goto error;
  if (!Registry(HKEY_CLASSES_ROOT, hkcr + L"\\InProcServer32", L"ThreadingModel", L"Apartment").set()) goto error;

  /** BHO registration
    HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Explorer\
    Browser Helper Objects\clsid = '${name} BHO'
    */
  logger->debug(L"BHO::dllexports::DllRegisterServer registering BHO");
  hklm_bho =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
    L"Browser Helper Objects\\" + clsid;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_bho).create()) goto error;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_bho, L"", _AtlModule.moduleManifest->name + L" BHO").set()) goto error;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_bho, L"NoExplorer", L"1").set()) goto error; // TODO support ints -> d '1'

  /** elevation policy for Frame-Injection
    HKLM\Software\Microsoft\Internet Explorer\Low Rights\ElevationPolicy = elevation[32|64]
    HKLM\Software\Microsoft\Internet Explorer\Low Rights\ElevationPolicy\elevation\AppPath = %MODULEPATH%
    HKLM\Software\Microsoft\Internet Explorer\Low Rights\ElevationPolicy\elevation\AppName = forge64.exe
    HKLM\Software\Microsoft\Internet Explorer\Low Rights\ElevationPolicy\elevation\Policy  = d '3'
    */
  hklm_elevation =
    L"Software\\Microsoft\\Internet Explorer\\Low Rights\\ElevationPolicy\\" + elevation32;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_elevation).create()) goto error;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_elevation, L"AppPath", _AtlModule.modulePath.wstring()).set()) goto error;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_elevation, L"AppName", L"forge32.exe").set()) goto error;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_elevation, L"Policy", (DWORD)3).set()) goto error;
  hklm_elevation =
    L"Software\\Microsoft\\Internet Explorer\\Low Rights\\ElevationPolicy\\" + elevation64;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_elevation).create()) goto error;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_elevation, L"AppPath", _AtlModule.modulePath.wstring()).set()) goto error;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_elevation, L"AppName", L"forge64.exe").set()) goto error;
  if (!Registry(HKEY_LOCAL_MACHINE, hklm_elevation, L"Policy", (DWORD)3).set()) goto error;

  // default initilization
  logger->debug(L"BHO::dllexports::DllRegisterServer default initialization");
  HRESULT hr = _AtlModule.DllRegisterServer();
  if (FAILED(hr)) {
    logger->debug(L"BHO::dllexports::DllRegisterServer "
      L" -> Warning: _AtlModule.DllRegisterServer failed");
  }

  logger->debug(L"BHO::dllexports::DllRegisterServer complete");

  return S_OK;

error:
  logger->debug(L"BHO::dllexports::DllRegisterServer failed to register addon");
  return E_FAIL;
}


/**
 * DllUnregisterServer - Removes entries from the system registry.
 */
STDAPI DllUnregisterServer(void)
{
    logger->debug(L"BHO::dllexports::DllUnregisterServer");

    // unregister addon
    if (!Registry(HKEY_CLASSES_ROOT, L"CLSID\\" + _AtlModule.moduleCLSID).del()) {
        logger->error(L"BHO::dllexports::DllUnregisterServer failed to delete registry entry");
    }
    if (!Registry(HKEY_LOCAL_MACHINE, 
                  L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                  L"Browser Helper Objects\\" + _AtlModule.moduleCLSID).del()) {
        logger->error(L"BHO::dllexports::DllUnregisterServer failed to delete registry entry");
    }

    // elevation policy for Frame-Injection
    if (!Registry(HKEY_LOCAL_MACHINE, 
                  L"Software\\Microsoft\\Internet Explorer\\Low Rights\\ElevationPolicy\\" + 
                  _AtlModule.moduleCLSID).del()) {
        logger->error(L"BHO::dllexports::DllUnregisterServer failed to delete registry entry: "
                      L"elevation policy");
    }


    HRESULT hr = _AtlModule.DllUnregisterServer();
    if (FAILED(hr)) {
        logger->error(L"BHO::dllexports::DllUnregisterServer "
                      L" -> Warning: _AtlModule.DllUnregisterServer failed");
    }

    return hr;
}


/**
 * DllInstall - Adds/Removes entries to the system registry per user per machine.
 */
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    logger->debug(L"BHO::dllexports::DllInstall");
    HRESULT hr = E_FAIL;
    static const wchar_t szUserSwitch[] = L"user";

    if (pszCmdLine != NULL) {
        if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0) {
            ATL::AtlSetPerUserRegistration(true);
        }
    }

    if (bInstall) {	
        hr = DllRegisterServer();
        if (FAILED(hr)) {
            DllUnregisterServer();
        }
    } else {
        hr = DllUnregisterServer();
    }

    return hr;
}

