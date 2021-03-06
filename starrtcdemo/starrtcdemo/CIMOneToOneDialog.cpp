// CIMOneToOneDialog.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CIMOneToOneDialog.h"
#include "afxdialogex.h"
#include "CCreateNewChatOneToOne.h"
#include <time.h>
#include "CUtil.h"
// CIMOneToOneDialog 对话框

IMPLEMENT_DYNAMIC(CIMOneToOneDialog, CDialogEx)

CIMOneToOneDialog::CIMOneToOneDialog(CUserManager* pUserManager, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ONE_TO_ONE, pParent)
{
	m_pUserManager = pUserManager;
	m_pChatManager = new CChatManager(pUserManager, this);
	m_strDisUserId = "";
	m_pSqliteDB = new CSqliteDB();
	m_pSqliteDB->openDB("chatDB.db");
	m_strUserId = "";
}

CIMOneToOneDialog::~CIMOneToOneDialog()
{
	if (m_pChatManager != NULL)
	{
		delete m_pChatManager;
		m_pChatManager = NULL;
	}
	if (m_pSqliteDB != NULL)
	{
		delete m_pSqliteDB;
		m_pSqliteDB = NULL;
	}
	clearMsgList();
	clearHistoryList();
}

void CIMOneToOneDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_ONE_TO_ONE_LIST, m_OneToOneList);
	DDX_Control(pDX, IDC_EDIT1, m_sendMsgEdit);
	DDX_Control(pDX, IDC_LIST_IM_CONTENT, m_contentCombox);
	DDX_Control(pDX, IDC_STATIC_AIM_ID, m_AimId);
}


BEGIN_MESSAGE_MAP(CIMOneToOneDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_CREATE_NEW_ONE_TO_ONE, &CIMOneToOneDialog::OnBnClickedButtonCreateNewOneToOne)
	ON_BN_CLICKED(IDC_BUTTON_SEND_MESSAGE, &CIMOneToOneDialog::OnBnClickedButtonSendMessage)
	ON_NOTIFY(NM_CLICK, IDC_LIST_ONE_TO_ONE_LIST, &CIMOneToOneDialog::OnNMClickListOneToOneList)
END_MESSAGE_MAP()


// CIMOneToOneDialog 消息处理程序

void CIMOneToOneDialog::OnBnClickedButtonCreateNewOneToOne()
{
	CCreateNewChatOneToOne dlg;
	if (dlg.DoModal() == IDOK)
	{
		m_strUserId = dlg.m_strUserName;
		m_AimId.SetWindowText(m_strUserId.c_str());
	}
}

void CIMOneToOneDialog::addHistoryList(CHistoryBean* pHistoryBean, string strFromId)
{
	CHistoryBean* pFindHistory = NULL;
	for (list<CHistoryBean*>::iterator it = mHistoryDatas.begin(); it != mHistoryDatas.end(); ++it)
	{
		if ((*it)->getConversationId() == pHistoryBean->getConversationId() || (*it)->getConversationId() == strFromId)
		{
			pFindHistory = *it;
			break;
		}
	}

	if (pFindHistory != NULL)
	{
		pFindHistory->setLastMsg(pHistoryBean->getLastMsg());
		pFindHistory->setLastTime(pHistoryBean->getLastTime());
		delete pHistoryBean;
		pHistoryBean = NULL;
	}
	else
	{
		if (m_pSqliteDB != NULL)
		{
			m_pSqliteDB->setHistory(pHistoryBean, true);
		}
		mHistoryDatas.push_front(pHistoryBean);
	}
}

void CIMOneToOneDialog::OnBnClickedButtonSendMessage()
{
	CString strMsg = "";
	m_sendMsgEdit.GetWindowText(strMsg);

	if (strMsg == "")
	{
		return;
	}
	
	CIMMessage* pIMMessage = m_pChatManager->sendMessage((char*)m_strUserId.c_str(), strMsg.GetBuffer(0));
	
	CHistoryBean* pHistoryBean = new CHistoryBean();
	pHistoryBean->setType(HISTORY_TYPE_C2C);
	pHistoryBean->setLastTime(CUtil::getTime());
	pHistoryBean->setLastMsg(pIMMessage->m_strContentData);
	pHistoryBean->setConversationId(pIMMessage->m_strTargetId);
	pHistoryBean->setNewMsgCount(0);

	CMessageBean* pMessageBean = new CMessageBean();
	pMessageBean->setConversationId(pIMMessage->m_strTargetId);
	pMessageBean->setTime(CUtil::getTime());
	pMessageBean->setMsg(pIMMessage->m_strContentData);
	pMessageBean->setFromId(pIMMessage->m_strFromId);

	if (m_pSqliteDB != NULL)
	{
		m_pSqliteDB->setMessage(pMessageBean);
	}
	mDatas.push_front(pMessageBean);
	addHistoryList(pHistoryBean, pIMMessage->m_strFromId);
	resetHistoryList();
	resetMsgList();
	m_sendMsgEdit.SetSel(0, -1); // 选中所有字符
	m_sendMsgEdit.ReplaceSel(_T(""));
}

void CIMOneToOneDialog::onNewMessage(CIMMessage* var1)
{	
	CHistoryBean* pHistoryBean = new CHistoryBean();
	pHistoryBean->setType(HISTORY_TYPE_C2C);
	pHistoryBean->setLastTime(CUtil::getTime());
	pHistoryBean->setLastMsg(var1->m_strContentData);
	pHistoryBean->setConversationId(var1->m_strTargetId);
	pHistoryBean->setNewMsgCount(1);

	CMessageBean* pMessageBean = new CMessageBean();
	pMessageBean->setConversationId(var1->m_strTargetId);
	pMessageBean->setTime(CUtil::getTime());
	pMessageBean->setMsg(var1->m_strContentData);
	pMessageBean->setFromId(var1->m_strFromId);

	if (m_pSqliteDB != NULL)
	{
		m_pSqliteDB->setMessage(pMessageBean);
	}
	addHistoryList(pHistoryBean, var1->m_strFromId);
	resetHistoryList();

	CString strAimId;
	CString strFromId = var1->m_strFromId.c_str();
	m_AimId.GetWindowText(strAimId);
	if (strAimId == strFromId)
	{
		mDatas.push_front(pMessageBean);
		resetMsgList();
	}

}

void CIMOneToOneDialog::onSendMessageSuccess(int msgIndex)
{
}

void CIMOneToOneDialog::onSendMessageFailed(int msgIndex)
{
}
void CIMOneToOneDialog::resetMsgList()
{
	m_contentCombox.ResetContent();
	for (list<CMessageBean*>::iterator it = mDatas.begin(); it != mDatas.end(); ++it)
	{
		if (*it != NULL)
		{
			CString strMsg = "";
			strMsg.Format("%s:%s", (*it)->getFromId().c_str(), (*it)->getMsg().c_str());
			m_contentCombox.InsertString(0, strMsg);
		}
	}
	
}
void CIMOneToOneDialog::resetHistoryList()
{
	m_OneToOneList.DeleteAllItems();
	
	int nRow = 0;
	for (list<CHistoryBean*>::iterator it = mHistoryDatas.begin(); it != mHistoryDatas.end(); ++it)
	{
		LVITEM lvItem = { 0 };                               // 列表视图控 LVITEM用于定义"项"的结构
		//第一行数据
		lvItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_STATE;   // 文字、图片、状态
		lvItem.iItem = nRow;                                // 行号(第一行)
		lvItem.iImage = 0;                               // 图片索引号(第一幅图片 IDB_BITMAP1)
		lvItem.iSubItem = 0;                             // 子列号
		nRow = m_OneToOneList.InsertItem(&lvItem);               // 第一列为图片
		CString strMsg = "";
		strMsg.Format("%s\r\n%s", (*it)->getConversationId().c_str(), (*it)->getLastMsg().c_str());
		m_OneToOneList.SetItemText(nRow, 1, strMsg);            // 第二列为名字
		m_OneToOneList.SetItemText(nRow, 2, (*it)->getLastTime().c_str());     // 第三列为格言
		nRow++;
	}
/*	// 添加数据 InsertItem向列表中插入主项数据 SetItemText向列表中的子项写入数据
	                                       // 记录行号 
	LVITEM lvItem = { 0 };                               // 列表视图控 LVITEM用于定义"项"的结构
	//第一行数据
	lvItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_STATE;   // 文字、图片、状态
	lvItem.iItem = 0;                                // 行号(第一行)
	lvItem.iImage = 0;                               // 图片索引号(第一幅图片 IDB_BITMAP1)
	lvItem.iSubItem = 0;                             // 子列号
	nRow = m_OneToOneList.InsertItem(&lvItem);               // 第一列为图片
	m_OneToOneList.SetItemText(nRow, 1, _T("dog"));            // 第二列为名字
	m_OneToOneList.SetItemText(nRow, 2, _T("人生在于奋斗"));     // 第三列为格言
	//第二行数据
	lvItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_STATE;   // 文字、图片、状态
	lvItem.iItem = 1;                                // 行号(第二行)
	lvItem.iImage = 0;                               // 图片索引号(第二幅图片 IDB_BITMAP2)
	lvItem.iSubItem = 0;                             // 子列号
	nRow = m_OneToOneList.InsertItem(&lvItem);
	m_OneToOneList.SetItemText(nRow, 1, _T("cat"));
	m_OneToOneList.SetItemText(nRow, 2, _T("快乐生活每一天"));*/

}

void CIMOneToOneDialog::clearMsgList()
{
	for (list<CMessageBean*>::iterator it = mDatas.begin(); it != mDatas.end(); ++it)
	{
		if (*it != NULL)
		{
			delete *it;
			*it = NULL;
		}
	}
	mDatas.clear();
}

void CIMOneToOneDialog::getMsgList(string conversationId)
{
	clearMsgList();
	if (m_pSqliteDB != NULL)
	{
		list<CMessageBean*> msgList = m_pSqliteDB->getMessageList(conversationId);
		if (msgList.size() > 0)
		{
			mDatas.splice(mDatas.begin(), msgList);
		}
	}

}

void CIMOneToOneDialog::clearHistoryList()
{
	for (list<CHistoryBean*>::iterator it = mHistoryDatas.begin(); it != mHistoryDatas.end(); ++it)
	{
		if (*it != NULL)
		{
			delete *it;
			*it = NULL;
		}
	}
	mHistoryDatas.clear();
}

void CIMOneToOneDialog::getHistoryList()
{
	clearHistoryList();
	if (m_pSqliteDB != NULL)
	{
		list<CHistoryBean*> historyList = m_pSqliteDB->getHistory(HISTORY_TYPE_C2C);
		if (historyList.size() > 0)
		{
			mHistoryDatas.splice(mHistoryDatas.begin(), historyList);
		}
	}
}

BOOL CIMOneToOneDialog::OnInitDialog()
{
	__super::OnInitDialog();
	/*DWORD dwStyle;
	dwStyle = m_OneToOneList.GetExtendedStyle();
	dwStyle = dwStyle | LVS_REPORT | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_SUBITEMIMAGES;
	m_OneToOneList.SetExtendedStyle(dwStyle);*/

	LONG lStyle;
	lStyle = GetWindowLong(m_OneToOneList.m_hWnd, GWL_STYLE);// 获取当前窗口style 
	lStyle &= ~LVS_TYPEMASK; // 清除显示方式位 
	lStyle |= LVS_REPORT; // 设置style 
	SetWindowLong(m_OneToOneList.m_hWnd, GWL_STYLE, lStyle);// 设置style 
	DWORD dwStyle = m_OneToOneList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;// 选中某行使整行高亮（只适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_SUBITEMIMAGES;
	m_OneToOneList.SetExtendedStyle(dwStyle); // 设置扩展风格 
	// 载入64*64像素 24位真彩(ILC_COLOR24)图片
	
	m_imList.Create(64, 64, ILC_COLOR24, 10, 20);    // 创建图像序列CImageList对象
	CBitmap * pBmp = NULL;
	pBmp = new CBitmap();
	pBmp->LoadBitmap("1.jpg");              // 载入位图IDB_BITMAP1
	//pBmp->LoadBitmap(IDB_BITMAP1);
	m_imList.Add(pBmp, RGB(0, 0, 0));
	delete pBmp;
	

	// 设置CImageList图像列表与CListCtrl控件关联 LVSIL_SMALL小图标列表
	m_OneToOneList.SetImageList(&m_imList, LVSIL_SMALL);
	CRect mRect;
	m_OneToOneList.GetWindowRect(&mRect);                     // 获取控件矩形区域
	int length = mRect.Width();
	m_OneToOneList.InsertColumn(0, _T(""), LVCFMT_CENTER, length / 4, -1);
	m_OneToOneList.InsertColumn(1, _T(""), LVCFMT_LEFT, length / 2, -1);
	m_OneToOneList.InsertColumn(2, _T(""), LVCFMT_RIGHT, length / 4, -1);
	
	getHistoryList();
	resetHistoryList();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CIMOneToOneDialog::OnNMClickListOneToOneList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	*pResult = 0;
	int nItem = -1;
	if (pNMItemActivate != NULL)
	{
		nItem = pNMItemActivate->iItem;
		if (nItem >= 0)
		{
			CString str = m_OneToOneList.GetItemText(nItem, 1);
			int pos = str.Find("\r\n");
			str = str.Left(pos);

			CString strAimId;
			m_AimId.GetWindowText(strAimId);
			if (strAimId == str)
			{
				return;
			}
			
			getMsgList(str.GetBuffer(0));
			//重绘
			resetMsgList();
			m_AimId.SetWindowText(str);
			m_strUserId = str;
		}
	}
}
