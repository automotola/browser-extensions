#include "stdafx.h"
#include "PopupWindow.h"

#include <gdiplus.h>
using namespace Gdiplus;


/**
 * Construction
 */
PopupWindow::PopupWindow(const wstring& uuid, POINT point, const wstring& url) 
    : hiddenBrowser(this, uuid, true, true), 
      uuid(uuid), url(url)
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
    width  = LOWORD(lparam);
    height = HIWORD(lparam);
    
    logger->debug(L"PopupWindow::OnSize"
                  L" -> " + boost::lexical_cast<wstring>(rect.left) +
                  L" -> " + boost::lexical_cast<wstring>(rect.top) +
                  L" -> " + boost::lexical_cast<wstring>(rect.right) +
                  L" -> " + boost::lexical_cast<wstring>(rect.bottom) +
                  L" -> " + boost::lexical_cast<wstring>(width) +
                  L" -> " + boost::lexical_cast<wstring>(height));

    int inset = 1;
    this->hiddenBrowser.MoveWindow(rect.left + inset, 
                                   rect.top + TAB_SIZE + inset,
                                   width - (inset * 2), 
                                   height - TAB_SIZE - (inset * 2));
    //handled = TRUE;

    return 0;
}


/**
 * Event: OnPaint
 *
 * Render the Popup Window
 */
LRESULT PopupWindow::OnPaint(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
    PAINTSTRUCT ps;
    this->BeginPaint(&ps);  

    // init gdi+
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // get draw area
    RECT client;
    ::GetClientRect(m_hWnd, &client);
    Rect rect(client.left, 
              client.top, 
              client.right - client.left, 
              client.bottom - client.top);
    
    // create Gdiplus Graphics object
    Graphics* graphics = new Graphics(this->m_hWnd, FALSE);
    graphics->SetClip(rect);

    // pens & brushes
    Pen* pen = new Pen(Color(255, 0, 0, 0), 1.0f);
    SolidBrush* brush = 
        new SolidBrush(hiddenBrowser.bgcolor);
    SolidBrush* chromakey = 
        new SolidBrush(Color(255, 77, 77, 77));

    // draw chromakey background
    graphics->FillRectangle(chromakey, 
                            rect.X, rect.Y, rect.Width, rect.Height);

    // draw background
    graphics->FillRectangle(brush, 
                            rect.X, rect.Y + TAB_SIZE, rect.Width, rect.Height);

    // draw border
    if (this->alignment == left) {
        Point border[] = {
            Point(rect.Width - (TAB_SIZE * 3), rect.Y + TAB_SIZE),
            Point(rect.X, rect.Y + TAB_SIZE),
            Point(rect.X, rect.Height - 1),
            Point(rect.Width - 1, rect.Height - 1),
            Point(rect.Width - 1, rect.Y + TAB_SIZE),
            Point(rect.Width - (TAB_SIZE * 1), rect.Y + TAB_SIZE)
        };
        graphics->DrawLines(pen, border, 6);
    } else {
        Point border[] = {
            Point(rect.X + (TAB_SIZE * 1), rect.Y + TAB_SIZE),
            Point(rect.X, rect.Y + TAB_SIZE),
            Point(rect.X, rect.Height - 1),
            Point(rect.Width - 1, rect.Height - 1),
            Point(rect.Width - 1, rect.Y + TAB_SIZE),
            Point(rect.X + (TAB_SIZE * 3), rect.Y + TAB_SIZE)
        };
        graphics->DrawLines(pen, border, 6);
    }

    // draw tab
    if (this->alignment == left) {
        Point tab[] = {
            Point(rect.Width - (TAB_SIZE * 1), rect.Y + TAB_SIZE + 1),
            Point(rect.Width - (TAB_SIZE * 2), rect.Y),
            Point(rect.Width - (TAB_SIZE * 3), rect.Y + TAB_SIZE + 1)
        };
        graphics->FillPolygon(brush, tab, 3);
        Point tab_border[] = {
            Point(rect.Width - (TAB_SIZE * 1), rect.Y + TAB_SIZE),
            Point(rect.Width - (TAB_SIZE * 2), rect.Y),
            Point(rect.Width - (TAB_SIZE * 3), rect.Y + TAB_SIZE)
        };
        graphics->DrawLines(pen, tab_border, 3);
    } else {
        Point tab[] = {
            Point((TAB_SIZE * 1), rect.Y + TAB_SIZE + 1),
            Point((TAB_SIZE * 2), rect.Y),
            Point((TAB_SIZE * 3), rect.Y + TAB_SIZE + 1)
        };
        graphics->FillPolygon(brush, tab, 3);
        Point tab_border[] = {
            Point((TAB_SIZE * 1), rect.Y + TAB_SIZE),
            Point((TAB_SIZE * 2), rect.Y),
            Point((TAB_SIZE * 3), rect.Y + TAB_SIZE)
        };
        graphics->DrawLines(pen, tab_border, 3);
    }

    // clean up, need to explicitly delete stuff as we are shutting down gdi+ within the scope of the function
    // and thus cannot rely on stuff going out of scope first
    delete chromakey;
    delete brush;
    delete pen;
    delete graphics;

    // shutdown gdi+
    Gdiplus::GdiplusShutdown(gdiplusToken);

    this->EndPaint(&ps);
    handled = TRUE;

    return 0;
}


/**
 * Event: OnKillFocus
 */
LRESULT PopupWindow::OnKillFocus(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
    HWND new_hwnd = reinterpret_cast<HWND>(wparam);

    logger->debug(L"PopupWindow::OnKillFocus"
                  L" -> " + boost::lexical_cast<wstring>(new_hwnd));

    if (!::IsChild(this->m_hWnd, new_hwnd)) {
        this->ShowWindow(SW_HIDE);
    }

    return this->DefWindowProc(msg, wparam, lparam); 
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


