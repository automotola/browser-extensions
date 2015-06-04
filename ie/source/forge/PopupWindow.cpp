#include "stdafx.h"
#include "PopupWindow.h"

#include <gdiplus.h>

/**
 * Construction
 */
PopupWindow::PopupWindow(const wstring& uuid, POINT point, const wstring& url)
  : hiddenBrowser(this, uuid, true, true), uuid(uuid), url(url)
{
  logger->debug(L"PopupWindow::PopupWindow -> " + uuid);

  // get screen metrics
  DWORD screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
  alignment = (point.x > (int)(screenWidth / 2))
    ? PopupWindow::left
    : PopupWindow::right;
}


/**
 * Event: PopupWindow::OnCreate
 */
LRESULT PopupWindow::OnCreate(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  logger->debug(L"PopupWindow::OnCreate");

  HRESULT hr = E_FAIL;
  COLORREF chromakey = RGB(77, 77, 77);
  HWND hwnd;
  CRect rect;

  for (;;) {
    // set transparency
    ::SetLayeredWindowAttributes(m_hWnd, chromakey, 255, LWA_COLORKEY);

    // create browser control  
    if (!GetClientRect(&rect))
      break;

    hwnd = hiddenBrowser.Create(m_hWnd, rect, CComBSTR(url.c_str()), WS_CHILD | WS_VISIBLE);
    if (!hwnd) {
      logger->error(L"PopupWindow::OnCreate failed to create BrowserControl");
      break;
    }
  
    hr = S_OK;
    break;
  }

  return hr;
}


/**
 * Destruction
 */
PopupWindow::~PopupWindow() 
{
  logger->debug(L"PopupWindow::~PopupWindow");

  if (!DestroyWindow())
    logger->warn(L"PopupWindow::~PopupWindow failed to destroy window");

  m_hWnd = nullptr;

  logger->debug(L"PopupWindow::~PopupWindow done");
}


/**
 * Event: PopupWindow::OnDestroy
 */
LRESULT PopupWindow::OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  logger->debug(L"PopupWindow::OnDestroy");
  return 0;
}


/**
 * Event: PopupWindow::OnSize
 */
LRESULT PopupWindow::OnSize(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  if (!hiddenBrowser) {
    logger->debug(L"PopupWindow::OnSize hiddenBrowser not initialized");
    return -1;
  }

  RECT rect;
  WORD width, height;
  GetClientRect(&rect);
  width = LOWORD(lparam);
  height = HIWORD(lparam);

  logger->debug(L"PopupWindow::OnSize"
    L" -> " + boost::lexical_cast<wstring>(rect.left) +
    L" -> " + boost::lexical_cast<wstring>(rect.top) +
    L" -> " + boost::lexical_cast<wstring>(rect.right) +
    L" -> " + boost::lexical_cast<wstring>(rect.bottom) +
    L" -> " + boost::lexical_cast<wstring>(width)+
    L" -> " + boost::lexical_cast<wstring>(height));

  int inset = 1;
  hiddenBrowser.MoveWindow(rect.left + inset, rect.top + TAB_SIZE + inset, width - (inset * 2), height - TAB_SIZE - (inset * 2));
  //handled = TRUE;

  return 0;
}

/**
 * Event: OnPaint
 *
 * Render the Popup Window
 */

class MyGdiplusHelper
{
  ULONG_PTR m_token;
  Gdiplus::GdiplusStartupInput m_startup_input;
public:
  MyGdiplusHelper()
  {
    Gdiplus::GdiplusStartup(&m_token, &m_startup_input, NULL);
  }
  ~MyGdiplusHelper()
  {
    Gdiplus::GdiplusShutdown(m_token);
  }
};

LRESULT PopupWindow::OnPaint(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  PAINTSTRUCT ps;
  BeginPaint(&ps);
  {
    MyGdiplusHelper gdi_plus_autostart;
    { /// Gdiplus objects are used locally, because they need to be destroyed before 
      // get draw area
      RECT client;
      ::GetClientRect(m_hWnd, &client);
      Gdiplus::Rect rect(client.left, client.top, client.right - client.left, client.bottom - client.top);
      // create Gdiplus Graphics object
      Gdiplus::Graphics graphics(m_hWnd, FALSE);
      graphics.SetClip(rect);

      // pens & brushes
      Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 0), 1.0f);
      Gdiplus::SolidBrush brush(hiddenBrowser.bgcolor);
      Gdiplus::SolidBrush chromakey(Gdiplus::Color(255, 77, 77, 77));

      // draw chromakey background
      graphics.FillRectangle(&chromakey, rect.X, rect.Y, rect.Width, rect.Height);

      // draw background
      graphics.FillRectangle(&brush, rect.X, rect.Y + TAB_SIZE, rect.Width, rect.Height);

      Gdiplus::Point vtx_data [12] = {
        /// border
        Gdiplus::Point(rect.X + (TAB_SIZE * 1), rect.Y + TAB_SIZE),
        Gdiplus::Point(rect.X, rect.Y + TAB_SIZE),
        Gdiplus::Point(rect.X, rect.Height - 1),
        Gdiplus::Point(rect.Width - 1, rect.Height - 1),
        Gdiplus::Point(rect.Width - 1, rect.Y + TAB_SIZE),
        Gdiplus::Point(rect.X + (TAB_SIZE * 3), rect.Y + TAB_SIZE),
        /// tab
        Gdiplus::Point((TAB_SIZE * 1), rect.Y + TAB_SIZE + 1),
        Gdiplus::Point((TAB_SIZE * 2), rect.Y),
        Gdiplus::Point((TAB_SIZE * 3), rect.Y + TAB_SIZE + 1),
        /// tab_border
        Gdiplus::Point((TAB_SIZE * 1), rect.Y + TAB_SIZE),
        Gdiplus::Point((TAB_SIZE * 2), rect.Y),
        Gdiplus::Point((TAB_SIZE * 3), rect.Y + TAB_SIZE)
      };

      if (alignment == left) {
        vtx_data[0] = Gdiplus::Point(rect.Width - (TAB_SIZE * 3), rect.Y + TAB_SIZE);
        vtx_data[1] = Gdiplus::Point(rect.X, rect.Y + TAB_SIZE);
        vtx_data[2] = Gdiplus::Point(rect.X, rect.Height - 1);
        vtx_data[3] = Gdiplus::Point(rect.Width - 1, rect.Height - 1);
        vtx_data[4] = Gdiplus::Point(rect.Width - 1, rect.Y + TAB_SIZE);
        vtx_data[5] = Gdiplus::Point(rect.Width - (TAB_SIZE * 1), rect.Y + TAB_SIZE);
        vtx_data[6] = Gdiplus::Point(rect.Width - (TAB_SIZE * 1), rect.Y + TAB_SIZE + 1);
        vtx_data[7] = Gdiplus::Point(rect.Width - (TAB_SIZE * 2), rect.Y);
        vtx_data[8] = Gdiplus::Point(rect.Width - (TAB_SIZE * 3), rect.Y + TAB_SIZE + 1);
        vtx_data[9] = Gdiplus::Point(rect.Width - (TAB_SIZE * 1), rect.Y + TAB_SIZE);
        vtx_data[10] = Gdiplus::Point(rect.Width - (TAB_SIZE * 2), rect.Y);
        vtx_data[11] = Gdiplus::Point(rect.Width - (TAB_SIZE * 3), rect.Y + TAB_SIZE);
      }

      // draw border
      graphics.DrawLines(&pen, vtx_data, 6);
      // draw tab
      graphics.FillPolygon(&brush, vtx_data + 6, 3);
      graphics.DrawLines(&pen, vtx_data + 9, 3);
      /// Gdiplus objects are destroyed here
    } 
    /// Gdiplus::GdiplusShutdown happens here
  } 

  EndPaint(&ps);
  handled = TRUE;

  return 0;
}

/**
 * Event: OnKillFocus
 */
LRESULT PopupWindow::OnKillFocus(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  HWND new_hwnd = reinterpret_cast<HWND>(wparam);

  logger->debug(L"PopupWindow::OnKillFocus -> " + boost::lexical_cast<wstring>(new_hwnd));

  if (!::IsChild(m_hWnd, new_hwnd))
    ShowWindow(SW_HIDE);
  
  return DefWindowProc(msg, wparam, lparam);
}

/**
 * Event: OnSetFocus
 */
LRESULT PopupWindow::OnSetFocus(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
  HWND new_hwnd = reinterpret_cast<HWND>(wparam);
  LRESULT lr = S_OK;
  HRESULT hr = S_OK;
  CComPtr<IDispatch> disp = nullptr;
  CComPtr<IHTMLElement> body = nullptr;
  for (;;) {
    if (!new_hwnd) {
      logger->debug(L"PopupWindow::OnSetFocus Ignoring event");
      break;
    }

    logger->debug(L"PopupWindow::OnSetFocus -> " + boost::lexical_cast<wstring>(new_hwnd));
    
    hiddenBrowser->get_Document(&disp);
    BreakOnNull(disp, lr);

    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
    BreakOnNull(htmlDocument2, lr);

    hr = htmlDocument2->get_body(&body);
    BreakOnFailed(hr);
    BreakOnNull(body, hr);
    //if (FAILED(hr) || !body) return S_OK; // don't send focus if we don't have a body

    CComQIPtr<IHTMLDocument4, &IID_IHTMLDocument4> htmlDocument4(htmlDocument2);
    BreakOnNull(htmlDocument4, lr);
    logger->debug(L"PopupWindow::OnSetFocus htmlDocument4->focus()");

    hr = htmlDocument4->focus();
    if (FAILED(hr)) 
      logger->error(L"PopupWindow::OnSetFocus failed to set focus -> " + logger->parse(hr));
    
    lr = DefWindowProc(msg, wparam, lparam);
    break;
  }

  return lr;
}


