#include "stdafx.h"
#include <WindowsMessage.h>
#include "Commands.h"


/**
 * Command: button_addCommand::exec
 */
HRESULT button_addCommand::exec(HWND htoolbar, HWND htarget, FrameServer::Button *out_button) 
{
  logger->debug(L"button_addCommand::exec"
    L" -> " + wstring(uuid) +
    L" -> " + wstring(title) +
    L" -> " + wstring(icon) +
    L" -> " + boost::lexical_cast<wstring>(toolbar)+
    L" -> " + boost::lexical_cast<wstring>(htarget));

  HRESULT hr = S_OK;
  HICON   hi = nullptr;

  bfs::wpath path(icon);
  TBBUTTON button = { 0 };

  for (;;) {
    if (!bfs::exists(path)) {
      logger->debug(L"button_addCommand::exec invalid file -> " + path.wstring());
      hr = E_FAIL;
      break;
    }

    hi = (HICON)::LoadImage(NULL, path.wstring().c_str(), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    if (!hi) {
      logger->error(L"button_addCommand::exec failed to load icon -> " + path.wstring());
      hr = E_FAIL;
      break;
    }

    int index = -1;
    if (!WindowsMessage::AddToolbarIcon(htoolbar, hi, &index)) {
      logger->error(L"button_addCommand::exec failed to add icon -> " + path.wstring());
      hr = E_FAIL;
      break;
    }

    button.fsState = TBSTATE_ENABLED;
    button.idCommand = 0xc001; // TODO
    button.iBitmap = index;
    button.fsStyle = 0;
    button.iString = (INT_PTR)title;
    ::SendMessage(htoolbar, TB_INSERTBUTTON, 0, reinterpret_cast<LPARAM>(&button));

    int ie_major = 0, ie_minor = 0;
    if (FAILED(GET_MSIE_VERSION(&ie_major, &ie_minor))) {
      logger->error(L"WindowsMessage::GetToolbar failed to determine IE version");
      hr = E_FAIL;
      break;
    }

    if (ie_major >= 8) {
      RECT rect = {0};
      HWND ieFrame = GetParent(GetParent(GetParent(GetParent(toolbar))));
      // refresh the size of the toolbar
      ::SendMessage(htarget, WM_SIZE, 0, 0);
      // restore the window if it is maximized. Otherwise the resizing (steps below) won't work.
      // ...and yes...this is the only way i found to make it work when the window
      // was maximized before it was last closed
      ShowWindow(ieFrame, SW_RESTORE);
      // force ie frame resize so the new toolbar size kicks in
      GetWindowRect(ieFrame, &rect);
      SetWindowPos(ieFrame, NULL, rect.left, rect.top, rect.right - rect.left + 1,
        rect.bottom - rect.top, SWP_NOZORDER);
      SetWindowPos(ieFrame, NULL, rect.left, rect.top, rect.right - rect.left,
        rect.bottom - rect.top, SWP_NOZORDER);
    }
    else {
      // refresh the size of the toolbar
      ::SendMessage(htoolbar, WM_SIZE, 0, 0);
      ::SendMessage(htarget, WM_SIZE, 0, 0);
    }

    out_button->uuid = uuid;
    out_button->idCommand = 0xc001;     // TODO
    out_button->iBitmap = index;
    out_button->icon = hi;
    out_button->toolbar = htoolbar;
    out_button->target = htarget;

    break;
  }

  return hr;
}


/**
 * Command: button_setIconCommand::exec 
 */
HRESULT button_setIconCommand::exec(HWND htoolbar, HWND htarget, int idCommand, int iBitmap) 
{
  wstring const curl(url);
  logger->debug(L"button_setIconCommand::exec"
    L" -> " + wstring(uuid) +
    L" -> " + curl +
    L" -> " + boost::lexical_cast<wstring>(htoolbar)+
    L" -> " + boost::lexical_cast<wstring>(htarget)+
    L" -> " + boost::lexical_cast<wstring>(idCommand)+
    L" -> " + boost::lexical_cast<wstring>(iBitmap));

  // TODO support network URLs
  bfs::wpath path(curl);

  // load bitmap from URL
  HICON hi = nullptr;
  HRESULT hr = S_OK;

  for (;;) {
    hi = (HICON)::LoadImage(NULL, path.wstring().c_str(), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    if (!hi) {
      logger->error(L"button_setIconCommand::exec failed to load icon -> " + path.wstring());
      hr = E_FAIL;
      break;
    }

    // Replace old icon in image lists
    int index = iBitmap;
    if ( !WindowsMessage::AddToolbarIcon(htoolbar, hi, &index) ) {
      logger->error(L"button_setIconCommand::exec failed to replace icon -> " + path.wstring());
      hr = E_FAIL;
      break;
    }

    // Send bitmap change message to refresh button with updated icon
    if (!WindowsMessage::tb_changebitmap(htoolbar, idCommand, iBitmap)) {
      logger->error(L"button_setIconCommand::exec failed to change bitmap -> " + path.wstring());
      hr = E_FAIL;
    }
    break;
  }

  return hr;
}


/**
 * Command: button_setTitleCommand::exec 
 */
HRESULT button_setTitleCommand::exec(HWND htoolbar, HWND htarget, int idCommand) 
{
    logger->debug(L"button_setTitleCommand::exec"
                  L" -> " + wstring(uuid) +
                  L" -> " + wstring(title) +
                  L" -> " + boost::lexical_cast<wstring>(htoolbar) +
                  L" -> " + boost::lexical_cast<wstring>(htarget));

    // init button info
    TBBUTTONINFO buttoninfo = { 0 };
    wchar_t binfo_textbuff[MAX_PATH] = { 0 };
    buttoninfo.cbSize  = sizeof(TBBUTTONINFO);
    buttoninfo.dwMask  = TBIF_COMMAND | TBIF_TEXT;
    buttoninfo.pszText = binfo_textbuff;
    buttoninfo.cchText = MAX_PATH;
    if (!WindowsMessage::tb_getbuttoninfo(htoolbar, idCommand, &buttoninfo)) {
        logger->error(L"    button"
                      L" -> " + boost::lexical_cast<wstring>(idCommand) +
                      L" -> tb_fetbuttoninfo failed");
        return S_OK;
    }

    CComBSTR btitle(title);
    buttoninfo.pszText = btitle;  // TODO
    buttoninfo.cchText = btitle.Length() * sizeof(TCHAR); // TODO

    // set title
    if (!WindowsMessage::tb_setbuttoninfo(htoolbar, idCommand, &buttoninfo)) {
        logger->error(L"    button"
                      L" -> " + boost::lexical_cast<wstring>(idCommand) +
                      L" -> tb_setbuttoninfo failed");
    }

    return S_OK;
}


/**
 * Command: button_setBadgeCommand::exec 
 */
HRESULT button_setBadgeCommand::exec(HWND htoolbar, HWND htarget, int idCommand) 
{
    logger->debug(L"button_setBadgeCommand::exec"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(number) +
                  L" -> " + boost::lexical_cast<wstring>(htoolbar) +
                  L" -> " + boost::lexical_cast<wstring>(htarget));

    // get button bitmap
    
    // draw badge over button bitmap

    // replace button bitmap
    
    return S_OK;
}


/**
 * Command: button_setBadgeBackgroundColorCommand::exec
 */
HRESULT button_setBadgeBackgroundColorCommand::exec(HWND htoolbar, HWND htarget, int idCommand) 
{
    logger->debug(L"button_setBadgeBackgroundColorCommand::exec"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(r) +
                  L" -> " + boost::lexical_cast<wstring>(g) +
                  L" -> " + boost::lexical_cast<wstring>(b) +
                  L" -> " + boost::lexical_cast<wstring>(a) +
                  L" -> " + boost::lexical_cast<wstring>(htoolbar) +
                  L" -> " + boost::lexical_cast<wstring>(htarget));
    
    return S_OK;
}


#include <generated/Forge_i.h> // For INativeControls

/**
 * Event: button_onClickCommand::exec
 */
HRESULT button_onClickCommand::exec() 
{
  logger->debug(L"button_onClickCommand::exec"
    L" -> " + wstring(uuid) +
    L" -> " + boost::lexical_cast<wstring>(point.x) +
    L" -> " + boost::lexical_cast<wstring>(point.y));

  HRESULT hr = S_OK;  
  CComPtr<INativeControls> nativeControls;

  for (;;) {
    // invoke NativeControls
    logger->debug(L"button_onClickCommand::exec invoking NativeControls");
    hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
      break;

    hr = ::CoCreateInstance(CLSID_NativeControls, NULL, CLSCTX_LOCAL_SERVER, IID_INativeControls, (LPVOID*)&nativeControls);
    if (FAILED(hr) || !nativeControls) {
      logger->error(L"button_onClickCommand::exec failed to create NativeControls instance -> " + logger->parse(hr));
      ::CoUninitialize();
      break;
    }

    hr = nativeControls->button_click(CComBSTR(uuid), point);
    if (FAILED(hr)) {
      logger->error(L"button_onClickCommand::exec failed to invoke NativeControls::button_click -> " + logger->parse(hr));
      break;
    }

    // Force popup to the foreground for IE9
    int major, minor;
    hr = GET_MSIE_VERSION(&major, &minor);
    if (FAILED(hr)) {
      logger->error(L"button_onClickCommand::exec failed to get browser version");
      ::CoUninitialize();
      break;
    }

    if (major >= 8) {
      BOOL visible = FALSE;
      HWND popup = NULL;
      hr = nativeControls->popup_hwnd(CComBSTR(uuid), &visible, (ULONG*)&popup);
      if (FAILED(hr) || !popup) {
        logger->error(L"button_onClickCommand::exec failed to invoke NativeControls::popup_hwnd -> " + logger->parse(hr));
        ::CoUninitialize();
        break;
      }

      if (visible && popup) {
        logger->debug(L"Forcing popup to foreground");
        if (::SetForegroundWindow(popup) == 0)
          logger->error(L"button_onClickCommand::exec failed to set fg win -> " + logger->GetLastError());
      }
    }

    ::CoUninitialize();
    break;
  }

  return hr;
}
