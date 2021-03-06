// CMultipleMeetingDialog.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CMultipleMeetingDialog.h"
#include "afxdialogex.h"
#include "HttpClient.h"
#include "json.h"
enum MEETING_LIST_REPORT_NAME
{
	MEETING_NAME = 0,
	MEETING_ID,
	//MEETING_STATUS,
	MEETING_CREATER
};

// CMultipleMeetingDialog 对话框

IMPLEMENT_DYNAMIC(CMultipleMeetingDialog, CDialogEx)

CMultipleMeetingDialog::CMultipleMeetingDialog(CUserManager* pUserManager, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MULTIPLE_MEETING, pParent)
{
	m_pUserManager = pUserManager;
}

CMultipleMeetingDialog::~CMultipleMeetingDialog()
{
}

void CMultipleMeetingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTCONTROL_MEETING_LIST, m_MeetingList);
}


BEGIN_MESSAGE_MAP(CMultipleMeetingDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_MEETING_LISTBRUSH, &CMultipleMeetingDialog::OnBnClickedButtonMeetingListbrush)
	ON_BN_CLICKED(IDC_BUTTON_JOIN_MEETING, &CMultipleMeetingDialog::OnBnClickedButtonJoinMeeting)
END_MESSAGE_MAP()


// CMultipleMeetingDialog 消息处理程序


void CMultipleMeetingDialog::OnBnClickedButtonMeetingListbrush()
{
	getMeetingList();
}


void CMultipleMeetingDialog::OnBnClickedButtonJoinMeeting()
{
	POSITION pos = m_MeetingList.GetFirstSelectedItemPosition();
	int nIndex = m_MeetingList.GetSelectionMark();
	if (nIndex >= 0)
	{
		CString str = m_MeetingList.GetItemText(nIndex, MEETING_ID);
		string strId = str.GetBuffer(0);
		if (strId.length() == 32)
		{
			/*CLiveDialog dlg(m_pUserManager, this);
			string strChannelId = strId.substr(0, 16);
			string strChatRoomId = strId.substr(16, 16);
			dlg.m_pStartRtcLive->setInfo(strChannelId, strChatRoomId);
			dlg.DoModal();*/
		}
		else
		{
			CString strMessage = "";
			strMessage.Format("err id %s", strId.c_str());
			AfxMessageBox(strMessage);
		}
	}
}


BOOL CMultipleMeetingDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_MeetingList.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK;
	lStyle |= LVS_REPORT;
	SetWindowLong(m_MeetingList.m_hWnd, GWL_STYLE, lStyle);

	DWORD dwStyle = m_MeetingList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;                                        //选中某行使整行高亮(LVS_REPORT)
	dwStyle |= LVS_EX_GRIDLINES;                                            //网格线(LVS_REPORT)
	//dwStyle |= LVS_EX_CHECKBOXES;                                            //CheckBox
	m_MeetingList.SetExtendedStyle(dwStyle);

	m_MeetingList.InsertColumn(MEETING_ID, _T("ID"), LVCFMT_LEFT, 210);
	m_MeetingList.InsertColumn(MEETING_NAME, _T("Name"), LVCFMT_LEFT, 120);
	m_MeetingList.InsertColumn(MEETING_CREATER, _T("Creator"), LVCFMT_LEFT, 80);
	//m_MeetingList.InsertColumn(MEETING_STATUS, _T("liveState"), LVCFMT_LEFT, 80);
	getMeetingList();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CMultipleMeetingDialog::getMeetingList()
{
	m_MeetingList.DeleteAllItems();

	CString stUrl = "";
	stUrl.Format(_T("https://api.starrtc.com/public/meeting/list?appid=%s"), m_pUserManager->m_ServiceParam.m_strAgentId.c_str());

	int port = 9904;
	char* data = "";
	//std::string strVal = "";
	//std::string strErrInfo = "";
	CString strPara = _T("");
	CString strContent;

	CHttpClient httpClient;
	int nRet = httpClient.HttpPost(stUrl, strPara, strContent);

	//CString str;
	//str.Format(_T("%d  "), (int)nRet);
	//str += strContent;
	//AfxMessageBox(str);

	string str_json = strContent.GetBuffer(0);
	Json::Reader reader;
	Json::Value root;
	if (nRet == 0 && str_json != "" && reader.parse(str_json, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素   
	{
		std::cout << "========================[Dispatch]========================" << std::endl;
		if (root.isMember("status") && root["status"].asInt() == 1)
		{
			if (root.isMember("data"))
			{
				Json::Value data = root.get("data", "");
				int nSize = data.size();
				for (int i = 0; i < nSize; i++)
				{
					if (data[i].isMember("Name"))
					{
						m_MeetingList.InsertItem(i, data[i]["Name"].asCString());
					}

					if (data[i].isMember("ID"))
					{
						m_MeetingList.SetItemText(i, MEETING_ID, data[i]["ID"].asCString());
					}

					if (data[i].isMember("Creator"))
					{
						m_MeetingList.SetItemText(i, MEETING_CREATER, data[i]["Creator"].asCString());
					}
					/*if (data[i].isMember("liveState"))
					{
						m_MeetingList.SetItemText(i, MEETING_STATUS, data[i]["liveState"].asCString());
					}*/
				}
			}

		}

	}
}
