// SynchronizationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Synchronization.h"
#include "SynchronizationDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_UPGRADE (WM_USER + 1000)
#define WM_UPDATE_DATA (WM_USER + 1001)


// CSynchronizationDlg dialog

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    CSynchronizationDlg *dlg = (CSynchronizationDlg *)lpParameter;

    HANDLE handles[] = { dlg->hMasterOfflineEvent, dlg->hDataUpdateEvent, dlg->hGoOfflineEvent };

    for (;;)
    {
        DWORD result = WaitForMultipleObjects(3, handles, false, INFINITE);
        if (WAIT_FAILED == result)
        {
            dlg->ShowErrorMessage(_T("Cannot wait, terminate"), GetLastError());
            return 0;
        }
        if (WAIT_OBJECT_0 <= result && result < WAIT_OBJECT_0 + 3)
        {
            switch (result)
            {
            case WAIT_OBJECT_0 + 0:
                dlg->SendMessage(WM_UPGRADE);
                break;
            case WAIT_OBJECT_0 + 1:
                dlg->SendMessage(WM_UPDATE_DATA);
                break;
            case WAIT_OBJECT_0 + 2:
                return 0;
            default:
                // no default
                ASSERT(FALSE);
                break;
            }
        }
    }

    return 0;
}

CSynchronizationDlg::CSynchronizationDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSynchronizationDlg::IDD, pParent)
    , bSlave(TRUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSynchronizationDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, mTextCtrl);
}

BEGIN_MESSAGE_MAP(CSynchronizationDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_UPGRADE, &CSynchronizationDlg::OnUpgrade)
    ON_MESSAGE(WM_UPDATE_DATA, &CSynchronizationDlg::OnUpdateData)
    ON_EN_CHANGE(IDC_EDIT1, &CSynchronizationDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()


// CSynchronizationDlg message handlers

BOOL CSynchronizationDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    hDataUpdateEvent = CreateEvent(NULL, TRUE, FALSE, _T("DataUpdateEvt"));
    if (!hDataUpdateEvent)
    {
        ShowErrorMessage(_T("Cannot create event"), GetLastError());
        DestroyWindow();
    }

    hMasterOnlineEvent = CreateEvent(NULL, TRUE, FALSE, _T("MasterOnlineEvt"));
    if (!hMasterOnlineEvent)
    {
        ShowErrorMessage(_T("Cannot create event"), GetLastError());
        DestroyWindow();
    }

    hMasterOfflineEvent = CreateEvent(NULL, TRUE, FALSE, _T("MasterOfflineEvt"));
    if (!hMasterOfflineEvent)
    {
        ShowErrorMessage(_T("Cannot create event"), GetLastError());
        DestroyWindow();
    }

    bSlave = (WaitForSingleObject(hMasterOnlineEvent, 0) == WAIT_OBJECT_0);

    hGoOfflineEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hGoOfflineEvent)
    {
        ShowErrorMessage(_T("Cannot create event"), GetLastError());
        DestroyWindow();
    }

    hInterProcessData = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                          0, sizeof (INTERPROCESSDATA), _T("InterProcessFM"));
    if (!hInterProcessData)
    {
        ShowErrorMessage(_T("Cannot create file mapping"), GetLastError());
        DestroyWindow();
    }

    lpInterProcessData = (LPINTERPROCESSDATA)MapViewOfFile(hInterProcessData, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(INTERPROCESSDATA));
    if (!lpInterProcessData)
    {
        ShowErrorMessage(_T("Cannot create file mapping view"), GetLastError());
        DestroyWindow();
    }

    if (!bSlave)
    {
        if (0 == InterlockedCompareExchange(&lpInterProcessData->nMasters, 1, 0))
        {
            SetEvent(hMasterOnlineEvent);
        }
    }
    else
    {
        mTextCtrl.SetReadOnly(TRUE);
    }

    hThread = CreateThread(NULL, 0, &ThreadProc, this, NULL, NULL);
    if (!hThread)
    {
        ShowErrorMessage(_T("Cannot create thread"), GetLastError());
        DestroyWindow();
    }

	return TRUE;  // return TRUE  unless you set the focus to a control
}
 
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSynchronizationDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSynchronizationDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CSynchronizationDlg::ShowErrorMessage(LPCTSTR pszMessage, DWORD dwErrorCode)
{
    LPTSTR messageBuffer = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&messageBuffer, 0, NULL);
    CString str; str.Format(_T("%s\r\n(%d) %s"), pszMessage, dwErrorCode, messageBuffer);
    AfxMessageBox(str);
	LocalFree(messageBuffer);
}


BOOL CSynchronizationDlg::DestroyWindow()
{
    SetEvent(hGoOfflineEvent);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hGoOfflineEvent);

    if (!bSlave)
    {
        InterlockedCompareExchange(&lpInterProcessData->nMasters, 0, 1);
        ResetEvent(hMasterOnlineEvent);
        SetEvent(hMasterOfflineEvent);
    }
    CloseHandle(hMasterOfflineEvent);
    CloseHandle(hMasterOnlineEvent);
    UnmapViewOfFile(lpInterProcessData);
    CloseHandle(hInterProcessData);

    return CDialogEx::DestroyWindow();
}


LRESULT CSynchronizationDlg::OnUpgrade(WPARAM wpD, LPARAM lpD)
{
    if (!bSlave) return 0;

    if (0 == InterlockedCompareExchange(&lpInterProcessData->nMasters, 1, 0))
    {
        ResetEvent(hMasterOfflineEvent);
        SetEvent(hMasterOnlineEvent);
        bSlave = FALSE;
        mTextCtrl.SetReadOnly(FALSE);
    }

    return 0;
}


LRESULT CSynchronizationDlg::OnUpdateData(WPARAM wpD, LPARAM lpD)
{
    if (!bSlave)
    {
        // do nothing
    }
    else
    {
        mTextCtrl.SetWindowText(lpInterProcessData->pszMessage);
    }

    return 0;
}

void CSynchronizationDlg::OnEnChangeEdit1()
{
    mTextCtrl.GetWindowText(lpInterProcessData->pszMessage, sizeof (lpInterProcessData->pszMessage) / sizeof (TCHAR));
    PulseEvent(hDataUpdateEvent);
}
