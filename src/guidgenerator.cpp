// guidgenerator.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "guidgenerator.h"
#include <Windows.h>

#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE GAppInstance;                 // current instance
TCHAR GWindowTitle[MAX_LOADSTRING];		// The title bar text
TCHAR GWindowClass[MAX_LOADSTRING];     // the main window class name
HFONT GAppFont;

// Global holder of control handles.
struct
{
    HWND GuidTextBox;
    HWND GuidButton;
    HWND CopyButton;
    HWND UseBraces;
} GHandles;

// Global state.
bool GUseBraces = false;
std::wstring GCurrentGuid;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void CreateControls( HWND hwnd );
bool ProcessCommand( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam  );
void CopyGuidToClipboard();
void CopyToClipboard( const std::wstring& text );
std::wstring GenerateGUID();
void ShowCurrentGuid();


/**
 * Application entry point.
 */
int APIENTRY _tWinMain( _In_ HINSTANCE instance,
                        _In_opt_ HINSTANCE previousInstance,
                        _In_ LPTSTR    pCommandLine,
                        _In_ int       commandShow )
{
	UNREFERENCED_PARAMETER( previousInstance );
	UNREFERENCED_PARAMETER( pCommandLine );

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(instance, IDS_APP_TITLE, GWindowTitle, MAX_LOADSTRING);
	LoadString(instance, IDC_GUIDGENERATOR, GWindowClass, MAX_LOADSTRING);
	MyRegisterClass(instance);

	// Perform application initialization:
	if (!InitInstance (instance, commandShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(instance, MAKEINTRESOURCE(IDC_GUIDGENERATOR));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

/**
 * Create and register the main window.
 */
ATOM MyRegisterClass( HINSTANCE instance )
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= instance;
	wcex.hIcon			= LoadIcon( instance, MAKEINTRESOURCE( IDI_GUIDGENERATOR ) );
	wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground	= (HBRUSH) COLOR_WINDOW;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_GUIDGENERATOR);
	wcex.lpszClassName	= GWindowClass;
	wcex.hIconSm		= LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_SMALL ) );

	return RegisterClassEx(&wcex);
}

/**
 * Create the main window and save the instance handle.
 */
BOOL InitInstance( HINSTANCE instance, int commandShow )
{
	HWND hWnd;
	GAppInstance = instance; // Store instance handle in our global variable

    // Create window.
	hWnd = CreateWindow(
		GWindowClass,
		GWindowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		445,
		140,
		NULL,
		NULL,
		instance,
		NULL
	);

	// Did it work???
	if ( !hWnd )
	{
		return FALSE;
	}

	// Set a system appropriate font.
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);

	::SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0 );
	GAppFont = ::CreateFontIndirect(&ncm.lfMessageFont);

	::SendMessage( hWnd, WM_SETFONT, (WPARAM) GAppFont, MAKELPARAM( TRUE, 0 ) );
	
	ShowWindow(hWnd, commandShow);
	UpdateWindow(hWnd);

	return TRUE;
}

/**
 * Process main window event loop.
 */
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		case WM_CREATE:
			CreateControls( hWnd );
			break;

		case WM_COMMAND:
            if ( !ProcessCommand( hWnd, message, wParam, lParam ) )
            {
                return DefWindowProc( hWnd, message, wParam, lParam );
            }

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/**
 * Process main window commands.
 */
bool ProcessCommand(  HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam  )
{
    int wmId    = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    // Which command was emitted?
    switch (wmId)
    {
        case IDM_ABOUT:
            DialogBox(GAppInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;

        case IDC_GUID_BUTTON:
            // Generate new guid.
            GCurrentGuid = GenerateGUID();

            // Update visual state.
            SendMessage( GHandles.GuidButton, BM_CLICK, 0, 0 );
            ShowCurrentGuid();
            break;

        case IDC_BRACES_CHECKBOX:
            GUseBraces = !GUseBraces;

            // Update visual state.
            ShowCurrentGuid();
            SendMessage( GHandles.UseBraces,
                         BM_SETCHECK,
                         ( GUseBraces ? BST_CHECKED : BST_UNCHECKED ),
                         0
            );
            
            break;

        case IDC_COPY_BUTTON:
            CopyGuidToClipboard();
            break;

        default:
            // Let the caller know that the message was not handled.
            return false;
    }

    return true;
}

void ShowCurrentGuid()
{
    // Are we supposed to append curly braces to the guid?
    std::wstring guid = GCurrentGuid;

    if ( GUseBraces && !guid.empty() )
    {
        guid = std::wstring( L"{" ) + guid + std::wstring( L"}" );
    }

    // Instruct the textbox to show the guid.
    SendMessage( GHandles.GuidTextBox,
        WM_SETTEXT,
        0,
        (LPARAM)
        guid.c_str() );
}

void CopyGuidToClipboard()
{
    // Are we supposed to append curly braces to the guid?
    std::wstring guid = GCurrentGuid;

    if ( GUseBraces && !guid.empty() )
    {
        guid = std::wstring( L"{" ) + guid + std::wstring( L"}" );
    }

    // Copy it to the clipboard.
    if ( !guid.empty() )
    {
        CopyToClipboard( guid );
    }
}

void CopyToClipboard( const std::wstring& text )
{
    // We need a staging buffer so we can properly null termiante the string before passing it to
    // the clipboard.
    size_t textBufferLength = ( text.size() + 1 ) * sizeof( wchar_t );
    wchar_t * pTextBuffer   = new wchar_t[ textBufferLength ];
    pTextBuffer[textBufferLength] = L'\0';

    // Allocate memory for the clipboard.
    HGLOBAL bufferHandle = GlobalAlloc( GMEM_MOVEABLE, textBufferLength );
    memcpy( GlobalLock( bufferHandle ), text.c_str(), textBufferLength );
    GlobalUnlock( bufferHandle );

    // Open the clipboard and copy the text.
    OpenClipboard( NULL );
    EmptyClipboard();

    SetClipboardData( CF_UNICODETEXT, bufferHandle );

    CloseClipboard();
}

void CreateControls( HWND hwnd )
{
	// GUID text box
	GHandles.GuidTextBox =
        CreateWindowEx(
		          WS_EX_CLIENTEDGE,
		          TEXT("EDIT"),
				  TEXT(""),
				  WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
				  15,
				  15,
				  310,
			      25,
				  hwnd,
				  (HMENU) IDC_GUID_TEXTBOX,
				  GetModuleHandle( NULL ),
				  NULL
    );

	// Create GUID button
    GHandles.GuidButton =
	    CreateWindow(
            TEXT("BUTTON"),
            TEXT("Generate"),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            340,
            15,
            75,
            25,
            hwnd,
            (HMENU) IDC_GUID_BUTTON,
            GetModuleHandle( NULL ),
            NULL
    );

    // Copy button
    GHandles.CopyButton =
        CreateWindow(
            TEXT("BUTTON"),
            TEXT("Copy"),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            340,
            45,
            75,
            25,
            hwnd,
            (HMENU) IDC_COPY_BUTTON,
            GetModuleHandle( NULL ),
            NULL
    );

	// Create GUID button
	GHandles.UseBraces =
        CreateWindow(
            TEXT("BUTTON"),
            TEXT("Use {} braces"),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX,
            25,
            45,
            150,
            25,
            hwnd,
            (HMENU) IDC_BRACES_CHECKBOX,
            GetModuleHandle( NULL ),
            NULL
    );

    // Set the textbox as readonly.
    SendMessage( GHandles.GuidTextBox, EM_SETREADONLY, TRUE, 0 );
}

std::wstring GenerateGUID()
{
    std::wstring result;

    // Generate a new UUID.
    UUID uuid;
    ::ZeroMemory( &uuid, sizeof(UUID) );

    ::UuidCreate( &uuid );

    // Kindly request windows to convert the UUID into a string.
    WCHAR * pUuidString = NULL;
    ::UuidToStringW( &uuid, (RPC_WSTR*) &pUuidString );

    // If we get a result from windows convert it to a wstring.
    if ( pUuidString != NULL )
    {
        result = pUuidString;
    }

    // Clean up before returning to the caller.
    ::RpcStringFreeW( (RPC_WSTR*) &pUuidString );
    return result;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
