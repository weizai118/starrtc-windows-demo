#pragma once
#include "CUserManager.h"

// CMultipleMeetingDialog 对话框

class CMultipleMeetingDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CMultipleMeetingDialog)

public:
	CMultipleMeetingDialog(CUserManager* pUserManager, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CMultipleMeetingDialog();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MULTIPLE_MEETING };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonMeetingListbrush();
	afx_msg void OnBnClickedButtonJoinMeeting();
	CUserManager* m_pUserManager;
	CListCtrl m_MeetingList;
	virtual BOOL OnInitDialog();
	void getMeetingList();
};
