#include "stdafx.h"

#include <gdiplus.h>
#include "NotificationWindow.h"


/**
 * Static initialization
 */
NotificationWindow* NotificationWindow::theNotification;


/**
 * Lifecycle: Create
 */
NotificationWindow::NotificationWindow()
    : m_isInitialized(false)
{
}


/**
 * Lifecycle: Destroy
 */
NotificationWindow::~NotificationWindow()
{
    this->DestroyWindow();
}


/**
 * Method: Show
 *
 * @param message
 */
void NotificationWindow::Show(const wstring& icon, const wstring& title, 
                              const wstring& message)
{
    // make sure we hide any previous instance
    if (m_isInitialized) { 
        ShowWindow(SW_HIDE);  
    }

    bfs::wpath path = icon;
    if (!bfs::exists(path)) {
        logger->error(L"NotificationWindow::Show icon not found: " + 
                      path.wstring()); 
        m_icon = L"";
    } else {
        m_icon = icon;
    }

    m_icon    = icon;
    m_title   = title;
    m_message = message;
    
    this->Show();
}


/**
 * Method: Show
 */
void NotificationWindow::Show()
{
  if (!m_isInitialized)
    Initialize();

  UpdateWindow();
  ShowWindow(SW_SHOW);

  ::SetTimer(m_hWnd, IDT_TIMER, 5000, NULL);
}


/**
 * Method: Hide
 */
void NotificationWindow::Hide()
{
  if (!m_isInitialized)
    return;

  ShowWindow(SW_HIDE);
}


/**
 * Method: Initialize
 */
void NotificationWindow::Initialize()
{
  if (m_isInitialized)
    return;

  // size and position bottom right hand corner of desktop
  HWND desktopHwnd = ::GetDesktopWindow();
  RECT rect;
  ::GetClientRect(desktopHwnd, &rect);
  rect.left = rect.right - DEFAULT_INSET - DEFAULT_WIDTH;
  rect.right = rect.right - DEFAULT_INSET;
  rect.top = rect.top + DEFAULT_INSET;
  rect.bottom = rect.top + DEFAULT_INSET + DEFAULT_HEIGHT;

  Create(NULL, rect, _T("forge notification window"));
  ShowWindow(SW_HIDE);
  UpdateWindow();
  ::SetLayeredWindowAttributes(m_hWnd, 0, DEFAULT_ALPHA, LWA_ALPHA); // 64 = alpha 

  m_isInitialized = true;
}


/**
 * Event: OnPaint
 *
 * Render the Notification Window
 */
LRESULT NotificationWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  PAINTSTRUCT ps = { 0 };
  BeginPaint(&ps);

  // init gdi+
  ULONG_PTR gdiplusToken;
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

  // get draw area
  RECT clientRect = { 0 };
  ::GetClientRect(m_hWnd, &clientRect);

  // create Gdiplus Graphics object
  Gdiplus::Graphics graphics(m_hWnd, FALSE);
  graphics.SetClip(Gdiplus::Rect(clientRect.left,
    clientRect.top,
    clientRect.right - clientRect.left,
    clientRect.bottom - clientRect.top));

  // draw a background
  Gdiplus::SolidBrush backgroundBrush(Gdiplus::Color(DEFAULT_ALPHA, 255, 255, 255));
  graphics.FillRectangle(&backgroundBrush,
    clientRect.left,
    clientRect.top,
    clientRect.right - clientRect.left,
    clientRect.bottom - clientRect.top);

  // shrink draw area 
  int inset = 4;
  clientRect.left += inset;
  clientRect.top += inset;
  clientRect.right -= inset;
  clientRect.bottom -= inset;

  // whack a logo TODO
  //Bitmap* bitmap = new Bitmap(m_icon.c_str(), FALSE);
  int bitmapWidth = 0;//bitmap->GetWidth(); 
  int bitmapHeight = 15;//bitmap->GetHeight(); 
  //graphics->DrawImage(bitmap, clientRect.left, clientRect.top, 
  //bitmapWidth, bitmapHeight); 

  // draw a separator
  Gdiplus::Pen blackPen(Gdiplus::Color(0, 0, 0), 1.0f);
  graphics.DrawLine(&blackPen,
    clientRect.left,
    clientRect.top + bitmapHeight + inset,
    clientRect.right,
    clientRect.top + bitmapHeight + inset);

  // setup text properties
  Gdiplus::Font titleFont(L"Verdana", 10, Gdiplus::FontStyleBold);
  Gdiplus::Font textFont(L"Verdana", 10, Gdiplus::FontStyleRegular);
  Gdiplus::RectF titleRect((float)clientRect.left + inset + bitmapWidth,
    (float)clientRect.top,
    (float)clientRect.right,
    (float)20);
  Gdiplus::RectF textRect((float)clientRect.left,
    (float)clientRect.top + bitmapHeight + (inset * 2),
    (float)clientRect.right,
    (float)clientRect.bottom - bitmapHeight - (inset * 2));
  Gdiplus::StringFormat format;
  format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
  format.SetFormatFlags(Gdiplus::StringFormatFlagsLineLimit);
  Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 0, 0, 0));

  // draw the message
  graphics.DrawString(m_title.c_str(), (int)m_title.length(), &titleFont, titleRect, &format, &blackBrush);
  graphics.DrawString(m_message.c_str(), (int)m_message.length(), &textFont, textRect, &format, &blackBrush);

  // shutdown gdi+
  Gdiplus::GdiplusShutdown(gdiplusToken);

  EndPaint(&ps);
  bHandled = TRUE;

  return 0;
}


/**
 * Event: OnClose
 */
LRESULT NotificationWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return 0;
}


/**
 * Event: OnDestroy
 */
LRESULT NotificationWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return 0;
}


/**
 * Event: OnTimer
 */
LRESULT NotificationWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  ::KillTimer(m_hWnd, IDT_TIMER);
  Hide();

  return 0;
}
