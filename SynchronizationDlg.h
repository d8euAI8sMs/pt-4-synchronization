
// SynchronizationDlg.h : header file
//

#pragma once
#include "afxwin.h"


typedef struct _tagINTERPROCESSDATA
{
    volatile LONG nMasters;
    TCHAR pszMessage[4096];
} INTERPROCESSDATA, *LPINTERPROCESSDATA;


// CSynchronizationDlg dialog
class CSynchronizationDlg : public CDialogEx
{
// Construction
public:
	CSynchronizationDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SYNCHRONIZATION_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    CEdit mTextCtrl;
    HANDLE hMasterOnlineEvent;
    HANDLE hMasterOfflineEvent;
    HANDLE hGoOfflineEvent;
    HANDLE hDataUpdateEvent;
    HANDLE hThread;
    HANDLE hInterProcessData;
    LPINTERPROCESSDATA lpInterProcessData;
    DWORD dwSlaveCount;
    DWORD dwAppId;
    void ShowErrorMessage(LPCTSTR pszMessage, DWORD dwErrorCode);
    BOOL bSlave;
    virtual BOOL DestroyWindow();
    LRESULT OnUpgrade(WPARAM wpD, LPARAM lpD);
    LRESULT OnUpdateData(WPARAM wpD, LPARAM lpD);
    afx_msg void OnEnChangeEdit1();
};
