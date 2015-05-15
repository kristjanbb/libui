// 27 april 2015
#include "uipriv_windows.h"

#define windowClass L"libui_uiWindowClass"

struct window {
	uiWindow w;
	HWND hwnd;
	HMENU menubar;
	uiControl *child;
	int hidden;
	BOOL shownOnce;
	int (*onClosing)(uiWindow *, void *);
	void *onClosingData;
	int margined;
};

static LRESULT CALLBACK windowWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	struct window *w;
	CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
	WINDOWPOS *wp = (WINDOWPOS *) lParam;
	RECT r;

	w = (struct window *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (w == NULL) {
		if (uMsg == WM_CREATE)
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) (cs->lpCreateParams));
		// fall through to DefWindowProc() anyway
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	// TODO container stuff
	switch (uMsg) {
	case WM_COMMAND:
		// not a menu
		if (lParam != 0)
			break;
		if (HIWORD(wParam) != 0)
			break;
		runMenuEvent(LOWORD(wParam), uiWindow(w));
		return 0;
	case WM_WINDOWPOSCHANGED:
		if ((wp->flags & SWP_NOSIZE) != 0)
			break;
		if (w->child != NULL)
			uiControlQueueResize(w->child);
		return 0;
	case WM_CLOSE:
		if ((*(w->onClosing))(uiWindow(w), w->onClosingData))
			uiControlDestroy(uiControl(w));
		return 0;		// we destroyed it already
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

ATOM registerWindowClass(HICON hDefaultIcon, HCURSOR hDefaultCursor)
{
	WNDCLASSW wc;

	ZeroMemory(&wc, sizeof (WNDCLASSW));
	wc.lpszClassName = windowClass;
	wc.lpfnWndProc = windowWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = hDefaultIcon;
	wc.hCursor = hDefaultCursor;
	wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	return RegisterClassW(&wc);
}

void unregisterWindowClass(void)
{
	if (UnregisterClassW(windowClass, hInstance) == 0)
		logLastError("error unregistering uiWindow window class in unregisterWindowClass()");
}

static int defaultOnClosing(uiWindow *w, void *data)
{
	return 0;
}

static void windowDestroy(uiControl *c)
{
	struct window *w = (struct window *) c;

	// first hide ourselves
	ShowWindow(w->hwnd, SW_HIDE);
	// now destroy the child
	// we need to unset its parent first
	if (w->child != NULL) {
		uiControlSetParent(w->child, NULL);
		uiControlDestroy(w->child);
	}
	// now free the menubar, if any
	if (w->menubar != NULL)
		freeMenubar(w->menubar);
	// and finally destroy ourselves
	if (DestroyWindow(w->hwnd) == 0)
		logLastError("error destroying uiWindow in windowDestroy()");
	uiFree(w);
}

static uintptr_t windowHandle(uiControl *c)
{
	struct window *w = (struct window *) c;

	return (uintptr_t) (w->hwnd);
}

static uiControl *windowParent(uiControl *c)
{
	complain("attempt to get the parent of uiWindow %p", c);
	return NULL;		// keep compiler happy
}

static void windowSetParent(uiControl *c, uiControl *parent)
{
	complain("attempt to set the parent of uiWindow %p", c);
}

static void windowPreferredSize(uiControl *c, uiSizing *d, intmax_t *width, intmax_t *height)
{
	complain("attempt to get the preferred size of the uiWindow at %p", c);
}

static void windowResize(uiControl *c, intmax_t x, intmax_t y, intmax_t width, intmax_t height, uiSizing *d)
{
	complain("attempt to resize the uiWindow at %p", c);
}

static void windowQueueResize(uiControl *c)
{
	complain("attempt to queue a resize of the uiWindow at %p", c);
}

static void windowGetSizing(uiControl *c, uiSizing *d)
{
	// TODO
}

// from https://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define windowMargin 7

static void windowComputeChildSize(uiControl *c, intmax_t *x, intmax_t *y, intmax_t *width, intmax_t *height, uiSizing *d)
{
	struct window *w = (struct window *) c;
	RECT r;

	if (GetClientRect(w->hwnd, &r) == 0)
		logLastError("error getting uiWindow client rect in windowComputeChildSize()");
	*x = r.left;
	*y = r.top;
	*width = r.right - r.left;
	*height = r.bottom - r.top;
	if (w->margined) {
		// TODO
	}
}

static int windowVisible(uiControl *c)
{
	struct window *w = (struct window *) c;

	return !w->hidden;
}

static void windowShow(uiControl *c)
{
	struct window *w = (struct window *) c;

	if (w->shownOnce) {
		ShowWindow(w->hwnd, SW_SHOW);
		w->hidden = 0;
		return;
	}
	w->shownOnce = TRUE;
	// make sure the bin is the correct size
	SendMessage(w->hwnd, msgUpdateChild, 0, 0);
	ShowWindow(w->hwnd, nCmdShow);
	if (UpdateWindow(w->hwnd) == 0)
		logLastError("error calling UpdateWindow() after showing uiWindow for the first time in windowShow()");
	w->hidden = 0;
}

static void windowHide(uiControl *c)
{
	struct window *w = (struct window *) c;

	ShowWindow(w->hwnd, SW_HIDE);
	w->hidden = 1;
}

static void windowEnable(uiControl *c)
{
	struct window *w = (struct window *) c;

	EnableWindow(w->hwnd, TRUE);
	if (w->child != NULL)
		uiControlContainerEnable(w->child);
}

static void windowDisable(uiControl *c)
{
	struct window *w = (struct window *) c;

	EnableWindow(w->hwnd, FALSE);
	if (w->child != NULL)
		uiControlContainerDisable(w->child);
}

static void windowContainerEnable(uiControl *c)
{
	complain("attempt to container enable uiWindow %p", c);
}

static void windowContainerDisable(uiControl *c)
{
	complain("attempt to container disable uiWindow %p", c);
}

static void windowSysFunc(uiControl *c, uiControlSysFuncParams *p)
{
	complain("attempt to call system functions on uiWindow %p", c);
}

static void windowStartZOrder(uiControl *c, uiControlSysFuncParams *p)
{
	complain("attempt to start Z-ordering on uiWindow %p", c);
}

static char *windowTitle(uiWindow *ww)
{
	struct window *w = (struct window *) ww;
	WCHAR *wtext;
	char *text;

	wtext = windowText(w->hwnd);
	text = toUTF8(wtext);
	uiFree(wtext);
	return text;
}

static void windowSetTitle(uiWindow *ww, const char *title)
{
	struct window *w = (struct window *) ww;
	WCHAR *wtitle;

	wtitle = toUTF16(title);
	if (SetWindowTextW(w->hwnd, wtitle) == 0)
		logLastError("error setting window title in uiWindowSetTitle()");
	uiFree(wtitle);
}

static void windowOnClosing(uiWindow *ww, int (*f)(uiWindow *, void *), void *data)
{
	struct window *w = (struct window *) ww;

	w->onClosing = f;
	w->onClosingData = data;
}

static void windowSetChild(uiWindow *ww, uiControl *child)
{
	struct window *w = (struct window *) ww;

	if (w->child != NULL)
		uiControlSetParent(w->child, NULL);
	w->child = child;
	uiControlSetParent(w->child, uiControl(w));
	uiControlQueueResize(w->child);
}

static int windowMargined(uiWindow *ww)
{
	struct window *w = (struct window *) ww;

	return w->margined;
}

static void windowSetMargined(uiWindow *ww, int margined)
{
	struct window *w = (struct window *) ww;

	w->margined = margined;
	if (w->child != NULL)
		uiControlQueueResize(w->child);
}

// see http://blogs.msdn.com/b/oldnewthing/archive/2003/09/11/54885.aspx and http://blogs.msdn.com/b/oldnewthing/archive/2003/09/13/54917.aspx
static void setClientSize(struct window *w, int width, int height, BOOL hasMenubar, DWORD style, DWORD exstyle)
{
	RECT window;

	window.left = 0;
	window.top = 0;
	window.right = width;
	window.bottom = height;
	if (AdjustWindowRectEx(&window, style, hasMenubar, exstyle) == 0)
		logLastError("error getting real window coordinates in setClientSize()");
	if (hasMenubar) {
		RECT temp;

		temp = window;
		temp.bottom = 0x7FFF;		// infinite height
		SendMessageW(w->hwnd, WM_NCCALCSIZE, (WPARAM) FALSE, (LPARAM) (&temp));
		window.bottom += temp.top;
	}
	if (SetWindowPos(w->hwnd, NULL, 0, 0, window.right - window.left, window.bottom - window.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		logLastError("error resizing window in setClientSize()");
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar)
{
	struct window *w;
	WCHAR *wtitle;
	BOOL hasMenubarBOOL;

	w = uiNew(struct window);

	hasMenubarBOOL = FALSE;
	if (hasMenubar)
		hasMenubarBOOL = TRUE;

#define style WS_OVERLAPPEDWINDOW
#define exstyle 0

	wtitle = toUTF16(title);
	w->hwnd = CreateWindowExW(exstyle,
		windowClass, wtitle,
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		// use the raw width and height for now
		// this will get CW_USEDEFAULT (hopefully) predicting well
		// even if it doesn't, we're adjusting it later
		width, height,
		NULL, NULL, hInstance, w);
	if (w->hwnd == NULL)
		logLastError("error creating window in uiWindow()");
	uiFree(wtitle);

	if (hasMenubar) {
		w->menubar = makeMenubar();
		if (SetMenu(w->hwnd, w->menubar) == 0)
			logLastError("error giving menu to window in uiNewWindow()");
	}

	// and use the proper size
	setClientSize(w, width, height, hasMenubarBOOL, style, exstyle);

	w->onClosing = defaultOnClosing;

	uiControl(w)->Destroy = windowDestroy;
	uiControl(w)->Handle = windowHandle;
	uiControl(w)->Parent = windowParent;
	uiControl(w)->SetParent = windowSetParent;
	uiControl(w)->PreferredSize = windowPreferredSize;
	uiControl(w)->Resize = windowResize;
	uiControl(w)->QueueResize = windowQueueResize
	uiControl(w)->GetSizing = windowGetSizing;
	uiControl(w)->ComputeChildSize = windowComputeChildSize;
	uiControl(w)->Visible = windowVisible;
	uiControl(w)->Show = windowShow;
	uiControl(w)->Hide = windowHide;
	uiControl(w)->Enable = windowEnable;
	uiControl(w)->Disable = windowDisable;
	uiControl(w)->ContainerEnable = windowContainerEnable;
	uiControl(w)->ContainerDisable = windowContainerDisable;
	uiControl(w)->SysFunc = windowSysFunc;
	uiControl(w)->StartZOrder = windowStartZOrder;

	uiWindow(w)->Title = windowTitle;
	uiWindow(w)->SetTitle = windowSetTitle;
	uiWindow(w)->OnClosing = windowOnClosing;
	uiWindow(w)->SetChild = windowSetChild;
	uiWindow(w)->Margined = windowMargined;
	uiWindow(w)->SetMargined = windowSetMargined;

	return uiWindow(w);
}