#include "stdafx.h"
#include "BrowserWindow.h"

#include <gdiplus.h>
using namespace Gdiplus;


/**
 * Construction
 */
BrowserWindow::BrowserWindow(const wstring& uuid, const wstring& url) 
    : hiddenBrowser(this, uuid),
      uuid(uuid), url(url)
{
    logger->debug(L"BrowserWindow::BrowserWindow -> " + uuid);
}


/**
 * Event: BrowserWindow::OnCreate
 */
LRESULT BrowserWindow::OnCreate(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  HRESULT hr = S_OK;
  HWND hwnd;
  CRect dimensions;

  for (;;) {
    if (!m_hWnd) {
      logger->debug(L"BrowserWindow::Oncreate says this does not yet exist");
      break;
    }

    logger->debug(L"BrowserWindow::OnCreate");
    if (!GetClientRect(&dimensions)) {
      hr = E_FAIL;
      break;
    }

    hwnd = hiddenBrowser.Create(m_hWnd, dimensions, CComBSTR(url.c_str()), WS_CHILD | WS_VISIBLE);
    if (!hwnd) {
      logger->debug(L"BrowserWindow::OnCreate failed to create BrowserControl");
      break;
    }

    hr = hiddenBrowser.ShowWindow(SW_SHOW);
    if (FAILED(hr))
      break;

    if(!ShowWindow(SW_HIDE)) 
      hr = E_FAIL;
    
    break;
  }

  return hr; 
}


/**
 * Destruction
 */
BrowserWindow::~BrowserWindow() 
{
  logger->debug(L"BrowserWindow::~BrowserWindow");

  if (!DestroyWindow())
    logger->warn(L"BrowserWindow::~BrowserWindow failed to destroy window");

  m_hWnd = nullptr;

  logger->debug(L"BrowserWindow::~BrowserWindow done");
}


/**
 * Event: BrowserWindow::OnDestroy
 */
LRESULT BrowserWindow::OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  logger->debug(L"BrowserWindow::OnDestroy");
  return 0;
}

/**
 * Event: BrowserWindow::OnSize
 */
LRESULT BrowserWindow::OnSize(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  if (!hiddenBrowser) {
    logger->debug(L"BrowserWindow::OnSize hiddenBrowser not initialized");
    return -1;
  }

  RECT rect;
  WORD width, height;
  GetClientRect(&rect);
  width = LOWORD(lparam);
  height = HIWORD(lparam);
  hiddenBrowser.MoveWindow(rect.left, rect.top, rect.right, rect.bottom);
  handled = TRUE;

  return 0;
}
