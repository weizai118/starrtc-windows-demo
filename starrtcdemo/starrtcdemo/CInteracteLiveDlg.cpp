// CInteracteLiveDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CInteracteLiveDlg.h"
#include "afxdialogex.h"
#include "HttpClient.h"
#include "json.h"

#include "CCreateLiveDialog.h"

#include <opencv2/core/core.hpp>  

#include <opencv2/highgui/highgui.hpp>  

#include <opencv2/imgproc/imgproc.hpp>  
using namespace cv;

enum LIVE_VIDEO_LIST_REPORT_NAME
{
	LIVE_VIDEO_NAME = 0,
	LIVE_VIDEO_STATUS,
	LIVE_VIDEO_ID,
	LIVE_VIDEO_CREATER
};


void MatToCImage(Mat &mat, CImage &cImage)
{
	//create new CImage  
	int width = mat.cols;
	int height = mat.rows;
	int channels = mat.channels();

	cImage.Destroy(); //clear  
	cImage.Create(width,
		height, //positive: left-bottom-up   or negative: left-top-down  
		8 * channels); //numbers of bits per pixel  

	//copy values  
	uchar* ps;
	uchar* pimg = (uchar*)cImage.GetBits(); //A pointer to the bitmap buffer  

	//The pitch is the distance, in bytes. represent the beginning of   
	// one bitmap line and the beginning of the next bitmap line  
	int step = cImage.GetPitch();

	for (int i = 0; i < height; ++i)
	{
		ps = (mat.ptr<uchar>(i));
		for (int j = 0; j < width; ++j)
		{
			if (channels == 1) //gray  
			{
				*(pimg + i * step + j) = ps[j];
			}
			else if (channels == 3) //color  
			{
				for (int k = 0; k < 3; ++k)
				{
					*(pimg + i * step + j * 3 + k) = ps[(width-j) * 3 + k];
				}
			}
		}
	}

}

DWORD WINAPI GetCameraDataThread(LPVOID p)
{
	CInteracteLiveDlg* pInteracteLiveDlg = (CInteracteLiveDlg*)p;

	while (WaitForSingleObject(pInteracteLiveDlg->m_hGetDataEvent, INFINITE) == WAIT_OBJECT_0)
	{
		if (pInteracteLiveDlg->m_bExit)
		{
			break;
		}

		VideoCapture cap;
		cap.open(0);

		Mat frame;

		CImage image;
		pInteracteLiveDlg->m_pDataShowView->addUpId(pInteracteLiveDlg->m_nUpId);
		while (pInteracteLiveDlg->m_bExit == false && pInteracteLiveDlg->m_bStop == false)
		{
			cap >> frame;
			//imshow("img", frame);
			//裁剪 

			//pInteracteLiveDlg->putShowPicData(image.GetWidth(), image.GetHeight(), NULL, 0);
			//绘制 传递给编码器
			MatToCImage(frame, image);
			if (pInteracteLiveDlg->m_pDataShowView != NULL)
			{
				pInteracteLiveDlg->m_pDataShowView->drawPic(pInteracteLiveDlg->m_nUpId, image.GetWidth(), image.GetHeight(), image);
			}

			Sleep(10);
		}

	}
	SetEvent(pInteracteLiveDlg->m_hExitEvent);
	return 0;
}


DWORD WINAPI SetShowPicThread(LPVOID p)
{
	CInteracteLiveDlg* pInteracteLiveDlg = (CInteracteLiveDlg*)p;

	while (WaitForSingleObject(pInteracteLiveDlg->m_hSetShowPicEvent, INFINITE) == WAIT_OBJECT_0)
	{
		if (pInteracteLiveDlg->m_bExit)
		{
			break;
		}
		if (pInteracteLiveDlg->m_pDataShowView != NULL)
		{
			pInteracteLiveDlg->m_pDataShowView->setShowPictures();
		}
	}
	return 0;
}

DWORD WINAPI ShowPicThread(LPVOID p)
{
	CInteracteLiveDlg* pInteracteLiveDlg = (CInteracteLiveDlg*)p;

	while (true)
	{
		if (pInteracteLiveDlg->m_bExit)
		{
			break;
		}
		CShowPicData* pShowPicData = NULL;
		EnterCriticalSection(&pInteracteLiveDlg->m_csShowPicDataQueue);//进入临界区	
		int nSize = pInteracteLiveDlg->m_ShowPicDataQueue.size();
		if (nSize > 0)
		{
			pShowPicData = pInteracteLiveDlg->m_ShowPicDataQueue.front();
			pInteracteLiveDlg->m_ShowPicDataQueue.pop();
		}	
		LeaveCriticalSection(&pInteracteLiveDlg->m_csShowPicDataQueue);//离开临界区
		if (pShowPicData != NULL)
		{
			if (pInteracteLiveDlg->m_pDataShowView != NULL)
			{
				pInteracteLiveDlg->m_pDataShowView->drawPic(FMT_NV12, pInteracteLiveDlg->m_nUpId, pShowPicData->m_nWidth, pShowPicData->m_nHeight, pShowPicData->m_pVideoData, pShowPicData->m_nDataLength);
			}
			delete pShowPicData;
			pShowPicData = NULL;
		}
		else
		{
			Sleep(10);
		}
	}
	return 0;
}

// CInteracteLiveDlg 对话框

IMPLEMENT_DYNAMIC(CInteracteLiveDlg, CDialogEx)

CInteracteLiveDlg::CInteracteLiveDlg(CUserManager* pUserManager, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LIVE, pParent)
{
	m_pUserManager = pUserManager;
	m_pLiveManager = new CLiveManager(m_pUserManager, this, this, this);
	m_pDataShowView = NULL;
	m_hGetDataEvent = NULL;
	m_hGetCameraDataThread = NULL;
	m_hExitEvent = NULL;

	m_hSetShowPicThread = NULL;
	m_hSetShowPicEvent = NULL;

	m_hShowPicThread = NULL;

	m_bExit = false;
	m_nUpId = 0;

	InitializeCriticalSection(&m_csShowPicDataQueue);//初始化临界区
}

CInteracteLiveDlg::~CInteracteLiveDlg()
{
	m_bStop = true;
	m_bExit = true;
	
	if (m_pLiveManager != NULL)
	{
		delete m_pLiveManager;
		m_pLiveManager = NULL;
	}

	if (m_pDataShowView != NULL)
	{
		delete m_pDataShowView;
		m_pDataShowView = NULL;
	}

	if (m_hSetShowPicEvent != NULL)
	{
		SetEvent(m_hSetShowPicEvent);
		m_hSetShowPicEvent = NULL;
	}

	if (m_hGetDataEvent != NULL)
	{
		SetEvent(m_hGetDataEvent);
		m_hGetDataEvent = NULL;
	}

	if (m_hExitEvent != NULL)
	{
		WaitForSingleObject(m_hExitEvent, INFINITE);
		CloseHandle(m_hExitEvent);
		m_hExitEvent = NULL;
	}

	if (m_hGetCameraDataThread != NULL)
	{
		CloseHandle(m_hGetCameraDataThread);
		m_hGetCameraDataThread = NULL;
	}
	clearShowPicDataQueue();
	DeleteCriticalSection(&m_csShowPicDataQueue);//删除临界区

	mVLivePrograms.clear();
}

void CInteracteLiveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_liveList);
	DDX_Control(pDX, IDC_STATIC_LIVE_SHOW_AREA, m_ShowArea);
	DDX_Control(pDX, IDC_LIST2, m_liveListListControl);
}


BEGIN_MESSAGE_MAP(CInteracteLiveDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_LIVE_BRUSH_LIST, &CInteracteLiveDlg::OnBnClickedButtonLiveBrushList)
	ON_WM_PAINT()
	ON_LBN_SELCHANGE(IDC_LIST1, &CInteracteLiveDlg::OnLbnSelchangeList1)
	ON_MESSAGE(UPDATE_SHOW_PIC_MESSAGE, setShowPictures)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_CREATE_NEW_LIVE, &CInteracteLiveDlg::OnBnClickedButtonCreateNewLive)
	ON_NOTIFY(NM_CLICK, IDC_LIST2, &CInteracteLiveDlg::OnNMClickList2)
END_MESSAGE_MAP()


// CInteracteLiveDlg 消息处理程序
void CInteracteLiveDlg::getLiveList()
{
	mVLivePrograms.clear();

	m_liveListListControl.DeleteAllItems();

	CString stUrl = "";
	stUrl.Format(_T("https://api.starrtc.com/public/live/list?appid=%s"), m_pUserManager->m_ServiceParam.m_strAgentId.c_str());
	int port = 9904;
	char* data = "";

	CString strPara = _T("");
	CString strContent;

	CHttpClient httpClient;
	int nRet = httpClient.HttpPost(stUrl, strPara, strContent);

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
					CLiveProgram livePro;

					if (data[i].isMember("ID"))
					{
						livePro.m_strId = data[i]["ID"].asCString();
					}

					if (data[i].isMember("Name"))
					{
						livePro.m_strName = data[i]["Name"].asCString();
					}

					if (data[i].isMember("Creator"))
					{
						livePro.m_strCreator = data[i]["Creator"].asCString();
					}
					livePro.m_liveState = FALSE;
					if (data[i].isMember("liveState"))
					{
						CString status = data[i]["liveState"].asCString();
						if (status == "1")
						{
							livePro.m_liveState = TRUE;
						}
					}
					mVLivePrograms.push_back(livePro);
				}
			}
		}
	}


	//m_liveList.ResetContent();
	int nRowIndex = 0;
	CString strStatus = "";
	for (int i = 0; i < (int)mVLivePrograms.size(); i++)
	{
		//m_liveList.AddString(mVLivePrograms[i].m_strName);
		m_liveListListControl.InsertItem(i, mVLivePrograms[i].m_strName);
		
		if (mVLivePrograms[i].m_liveState)
		{
			strStatus = "正在直播";
		}
		else
		{
			strStatus = "直播未开始";
		}
		m_liveListListControl.SetItemText(i, LIVE_VIDEO_STATUS, strStatus);
	}
}

void CInteracteLiveDlg::OnBnClickedButtonLiveBrushList()
{
	getLiveList();
}
LRESULT CInteracteLiveDlg::setShowPictures(WPARAM wParam, LPARAM lParam)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->setShowPictures();
	}
	return 1L;
}

CLiveProgram* CInteracteLiveDlg::getLiveProgram(CString strName)
{
	CLiveProgram* pRet = NULL;
	for (int i = 0; i < (int)mVLivePrograms.size(); i++)
	{
		if (mVLivePrograms[i].m_strName == strName)
		{
			pRet = new CLiveProgram();
			pRet->m_strId = mVLivePrograms[i].m_strId;
			pRet->m_strName = mVLivePrograms[i].m_strName;
			pRet->m_strCreator = mVLivePrograms[i].m_strCreator;
			pRet->m_liveState = mVLivePrograms[i].m_liveState;
			break;
		}
	}
	return pRet;
}

/*
 * 将显示图片放入队列
 */
void CInteracteLiveDlg::putShowPicData(int width, int height, uint8_t* videoData, int dataLength)
{
	EnterCriticalSection(&m_csShowPicDataQueue);//进入临界区
	CShowPicData* pShowPicData = new CShowPicData(width, height, videoData, dataLength);
	m_ShowPicDataQueue.push(pShowPicData);
	LeaveCriticalSection(&m_csShowPicDataQueue);//离开临界区
}

/*
 * 清空显示图片队列
 */
void CInteracteLiveDlg::clearShowPicDataQueue()
{
	EnterCriticalSection(&m_csShowPicDataQueue);//进入临界区
	int nSize = m_ShowPicDataQueue.size();
	
	for (int i = 0; i < nSize; i++)
	{
		CShowPicData* pTemp = m_ShowPicDataQueue.front();
		if (pTemp != NULL)
		{
			delete pTemp;
			pTemp = NULL;
		}
		m_ShowPicDataQueue.pop();
	}
	LeaveCriticalSection(&m_csShowPicDataQueue);//离开临界区
	
}

BOOL CInteracteLiveDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	LONG lStyle;
	lStyle = GetWindowLong(m_liveListListControl.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK;
	lStyle |= LVS_REPORT;
	SetWindowLong(m_liveListListControl.m_hWnd, GWL_STYLE, lStyle);

	DWORD dwStyleLiveList = m_liveListListControl.GetExtendedStyle();
	dwStyleLiveList |= LVS_EX_FULLROWSELECT;                                        //选中某行使整行高亮(LVS_REPORT)
	dwStyleLiveList |= LVS_EX_GRIDLINES;                                            //网格线(LVS_REPORT)
	//dwStyle |= LVS_EX_CHECKBOXES;                                            //CheckBox
	m_liveListListControl.SetExtendedStyle(dwStyleLiveList);

	m_liveListListControl.InsertColumn(LIVE_VIDEO_NAME, _T("ID"), LVCFMT_LEFT, 110);
	//m_liveListListControl.InsertColumn(MEETING_NAME, _T("Name"), LVCFMT_LEFT, 120);
	//m_liveListListControl.InsertColumn(MEETING_CREATER, _T("Creator"), LVCFMT_LEFT, 80);
	m_liveListListControl.InsertColumn(LIVE_VIDEO_STATUS, _T("liveState"), LVCFMT_LEFT, 100);

	
	getLiveList();


	CRect rect;
	::GetWindowRect(m_ShowArea, rect);
	CRect dlgRect;
	::GetWindowRect(this->m_hWnd, dlgRect);
	int left = rect.left - dlgRect.left - 7;
	int top = rect.top - dlgRect.top - 20;

	CRect showRect(left, top, left + rect.Width() - 5, top + rect.Height() - 15);

	m_hSetShowPicEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pDataShowView = new CDataShowView();
	m_pDataShowView->setDrawRect(showRect);

	CPicControl *pPicControl = new CPicControl();
	CRect rect1(left, top, left + showRect.Width() / 2, top + showRect.Height() / 2);
	pPicControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, showRect, this, WM_USER + 100);
	//mShowPicControlVector[i] = pPicControl;
	pPicControl->ShowWindow(SW_SHOW);
	DWORD dwStyle = ::GetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE);
	::SetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);


	m_pDataShowView->m_pPictureControlArr[0] = pPicControl;
	pPicControl->setInfo(this, m_pDataShowView);

	CRect rectClient = showRect;
	CRect rectChild(rectClient.right - rectClient.Width()*0.25, rectClient.top, rectClient.right, rectClient.bottom);

	for (int n = 0; n < 6; n++)
	{
		CPicControl *pPictureControl = new CPicControl();
		pPictureControl->setInfo(this, m_pDataShowView);
		rectChild.top = rectClient.top + n * rectClient.Height()*0.25;
		rectChild.bottom = rectClient.top + (n + 1) * rectClient.Height()*0.25;
		pPictureControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, rectChild, this, WM_USER + 100 + n + 1);
		m_pDataShowView->m_pPictureControlArr[n + 1] = pPictureControl;
		DWORD dwStyle1 = ::GetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE);
		::SetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE, dwStyle1 | SS_NOTIFY);
	}

	m_hGetDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hGetCameraDataThread = CreateThread(NULL, 0, GetCameraDataThread, (void*)this, 0, 0); // 创建线程

	m_hSetShowPicThread = CreateThread(NULL, 0, SetShowPicThread, (void*)this, 0, 0); // 创建线程
	m_hShowPicThread = CreateThread(NULL, 0, ShowPicThread, (void*)this, 0, 0); // 创建线程

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CInteracteLiveDlg::liveExit(void* pParam)
{
	CDataShowView* pDataShowView = (CDataShowView*)pParam;

	if (pDataShowView != NULL)
	{
		if (m_pLiveManager != NULL)
		{
			delete m_pLiveManager;
			m_pLiveManager = NULL;
		}
	}
}
void CInteracteLiveDlg::changeStreamConfig(void* pParam, int upid)
{
	CDataShowView* pDataShowView = (CDataShowView*)pParam;

	if (pDataShowView != NULL && upid != -1 && upid < UPID_MAX_SIZE)
	{
		if (pDataShowView->m_configArr[upid] != 2)
		{
			for (int i = 0; i < UPID_MAX_SIZE; i++)
			{
				if (pDataShowView->m_configArr[i] == 2)
				{
					pDataShowView->m_configArr[i] = 1;
					pDataShowView->m_configArr[upid] = 2;
					m_pLiveManager->setStreamConfig(pDataShowView->m_configArr, UPID_MAX_SIZE);
					SetEvent(m_hSetShowPicEvent);
					break;
				}
			}
		}
	}
}

void CInteracteLiveDlg::closeCurrentLive(void* pParam)
{
	m_bStop = true;
}

void CInteracteLiveDlg::startFaceFeature(void* pParam)
{
}

void CInteracteLiveDlg::stopFaceFeature(void* pParam)
{
}

int CInteracteLiveDlg::downloadChannelClosed()
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUpUser();
		SetEvent(m_hSetShowPicEvent);
	}
	return 0;
}
int CInteracteLiveDlg::downloadChannelLeave()
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUpUser();
		SetEvent(m_hSetShowPicEvent);
	}
	return 0;
}
int CInteracteLiveDlg::downloadNetworkUnnormal()
{
	return 0;
}
int CInteracteLiveDlg::queryVDNChannelOnlineNumberFin(char* channelId, int totalOnlineNum)
{
	return 0;
}
int CInteracteLiveDlg::uploaderAdd(char* upUserId, int upId)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->addUpId(upId);
		SetEvent(m_hSetShowPicEvent);
	}
	return 0;
}
int CInteracteLiveDlg::uploaderRemove(char* upUserId, int upId)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeUpUser(upId);
		SetEvent(m_hSetShowPicEvent);
	}
	return 0;
}
int CInteracteLiveDlg::getRealtimeData(int upId, uint8_t* data, int len)
{
	return 0;
}
int CInteracteLiveDlg::getVideoRaw(int upId, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(FMT_YUV420P, upId, w, h, videoData, videoDataLen);
	}
	return 0;
}
int CInteracteLiveDlg::deleteChannel(char* channelId)
{
	return 0;
}
int CInteracteLiveDlg::stopOK()
{
	return 0;
}
int CInteracteLiveDlg::srcError(char* errString)
{
	return 0;
}

/**
 * 聊天室成员数变化
 * @param number
 */
void CInteracteLiveDlg::onMembersUpdated(int number)
{
}

/**
 * 自己被踢出聊天室
 */
void CInteracteLiveDlg::onSelfKicked()
{
}

/**
 * 自己被踢出聊天室
 */
void CInteracteLiveDlg::onSelfMuted(int seconds)
{
}

/**
 * 聊天室已关闭
 */
void CInteracteLiveDlg::onClosed()
{
}

/**
 * 收到消息
 * @param message
 */
void CInteracteLiveDlg::onReceivedMessage(CIMMessage* pMessage)
{
}

/**
 * 收到私信消息
 * @param message
 */
void CInteracteLiveDlg::onReceivePrivateMessage(CIMMessage* pMessage)
{
}

void CInteracteLiveDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 __super::OnPaint()
	if (m_pDataShowView != NULL)
	{
		dc.SetStretchBltMode(COLORONCOLOR);
		FillRect(dc.m_hDC, m_pDataShowView->m_DrawRect, CBrush(RGB(0, 0, 0)));
	}
}


void CInteracteLiveDlg::OnLbnSelchangeList1()
{
	int nCurSel;

	nCurSel = m_liveList.GetCurSel();    // 获取当前选中列表项

	if (nCurSel >= 0)
	{
		//CString str = m_liveList.GetItemText(nIndex, LIVE_NAME);
		CString str = "";
		m_liveList.GetText(nCurSel, str);

		CLiveProgram* pLiveProgram = getLiveProgram(str);
		if (pLiveProgram != NULL)
		{
			string strId = pLiveProgram->m_strId.GetBuffer(0);
			if (strId.length() == 32)
			{
				string strChannelId = strId.substr(0, 16);
				string strChatRoomId = strId.substr(16, 16);
				CString strUserId = m_pUserManager->m_ServiceParam.m_strUserId.c_str();

				if (pLiveProgram->m_strCreator == strUserId)
				{
					bool bRet = m_pLiveManager->joinLive(strChatRoomId, strChannelId, true);
					if (bRet)
					{
						//AfxMessageBox("success");
					}
					else
					{
						AfxMessageBox("join live failed");
					}
					/*m_pLiveVideoInterface = new CLiveVideoSrc(this, m_pUserManager);
					CLiveVideoSrc* pLiveVideoSrc = (CLiveVideoSrc*)m_pLiveVideoInterface;
					pLiveVideoSrc->setInfo(strChannelId, strChatRoomId);
					bool bRet = pLiveVideoSrc->getServerAddr();
					if (bRet)
					{
						pLiveVideoSrc->globalSetting(m_pUserManager->m_ServiceParam.m_BigPic.m_nWidth, m_pUserManager->m_ServiceParam.m_BigPic.m_nHeight, 1024, 15);

						if (pLiveVideoSrc->startLiveVideo())
						{
							if (pLiveVideoSrc->startLiveSrcEncoder())
							{
								m_bStop = false;
								SetEvent(m_hGetDataEvent);
							}
						}
						else
						{
							AfxMessageBox("连接错误");
						}
					}
					else
					{
						AfxMessageBox("获取地址错误");
					}*/
				}
				else
				{
					bool bRet = m_pLiveManager->joinLive(strChatRoomId, strChannelId, false);
					if (bRet)
					{
						//AfxMessageBox("success");
					}
					else
					{
						AfxMessageBox("join live failed");
					}
				}
			}
			else
			{
				CString strMessage = "";
				strMessage.Format("err id %s", strId.c_str());
				AfxMessageBox(strMessage);
			}
			delete pLiveProgram;
			pLiveProgram = NULL;
		}
	}
}


void CInteracteLiveDlg::OnDestroy()
{
	__super::OnDestroy();
}


void CInteracteLiveDlg::OnBnClickedButtonCreateNewLive()
{
	CString strName = "";
	bool bPublic = false;
	CHATROOM_TYPE chatRoomType = CHATROOM_TYPE::CHATROOM_TYPE_PUBLIC;
	LIVE_VIDEO_TYPE channelType = LIVE_VIDEO_TYPE::LIVE_VIDEO_TYPE_LOGIN_SPECIFY;
	CCreateLiveDialog dlg(m_pUserManager);
	if (dlg.DoModal() == IDOK)
	{
		strName = dlg.m_strLiveName;
		bPublic = dlg.m_bPublic;
	}
	else
	{
		return;
	}
	if (m_pLiveManager != NULL)
	{
		bool bRet = m_pLiveManager->createLiveAndJoin(strName.GetBuffer(0), chatRoomType, channelType);
		if (bRet)
		{
			m_bStop = false;
			SetEvent(m_hGetDataEvent);
		}
		else
		{
			AfxMessageBox("createLiveAndJoin failed !");
		}
	}
}


void CInteracteLiveDlg::OnNMClickList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	int nItem = -1;
	if (pNMItemActivate != NULL)
	{
		nItem = pNMItemActivate->iItem;
		if (nItem >= 0)
		{
			CString str = m_liveListListControl.GetItemText(nItem, LIVE_VIDEO_NAME);
			CLiveProgram* pLiveProgram = getLiveProgram(str);
			CString strUserId = m_pUserManager->m_ServiceParam.m_strUserId.c_str();
			if (pLiveProgram != NULL)
			{
				if (pLiveProgram->m_liveState == false && pLiveProgram->m_strCreator != strUserId)
				{
					AfxMessageBox("直播尚未开始");
					return;
				}
				string strId = pLiveProgram->m_strId.GetBuffer(0);
				if (strId.length() == 32)
				{
					string strChannelId = strId.substr(0, 16);
					string strChatRoomId = strId.substr(16, 16);

					if (pLiveProgram->m_strCreator == strUserId)
					{
						bool bRet = m_pLiveManager->joinLive(strChatRoomId, strChannelId, true);
						if (bRet)
						{
							//AfxMessageBox("success");
							m_bStop = false;
							SetEvent(m_hGetDataEvent);
						}
						else
						{
							AfxMessageBox("join live failed");
						}
					}
					else
					{
						bool bRet = m_pLiveManager->joinLive(strChatRoomId, strChannelId, false);
						if (bRet)
						{
							//AfxMessageBox("success");
							m_bStop = false;
							SetEvent(m_hGetDataEvent);
						}
						else
						{
							AfxMessageBox("join live failed");
						}
					}
				}
				else
				{
					CString strMessage = "";
					strMessage.Format("err id %s", strId.c_str());
					AfxMessageBox(strMessage);
				}
				delete pLiveProgram;
				pLiveProgram = NULL;
			}

		}
	}
}
