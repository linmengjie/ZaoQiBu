#include "stdafx.h"
#include "VideoPlayerDlg.h"
#include "CourseUtil.h"
#include "AboutDlg.h"
#include "SettingDlg.h"

CVideoPlayerDlg::CVideoPlayerDlg()
: m_isFullScreen(false)
{
	LoadConfig();
}

void CVideoPlayerDlg::LoadConfig()
{
	m_config.Load();
	m_playlist.SetCourses(m_config.GetCourses());
}

CVideoPlayerDlg::~CVideoPlayerDlg()
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
}

BOOL CVideoPlayerDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			FullScreen(false);
		}
		else if (pMsg->wParam == VK_F1)
		{
			SendMessage(WM_COMMAND, MAKEWPARAM(ID_APP_ABOUT, BN_CLICKED), 0);
		}
		else if (pMsg->wParam == VK_F11)
		{
			FullScreen(!m_isFullScreen);
		}
		else if (pMsg->wParam == VK_F12)
		{
			SendMessage(WM_COMMAND, MAKEWPARAM(IDC_SNAPSHOT, BN_CLICKED), 0);
		}
		else if (pMsg->wParam == VK_SPACE)
		{
			SendMessage(WM_COMMAND, MAKEWPARAM(IDC_COURSE_PLAY, BN_CLICKED), 0);
			m_bmpBtnPlay.Invalidate();
		}
		else if (pMsg->wParam == VK_PRIOR)
		{
			SendMessage(WM_COMMAND, MAKEWPARAM(IDC_COURSE_PREV_CHAPTER, BN_CLICKED), 0);
			return TRUE;
		}
		else if (pMsg->wParam == VK_NEXT)
		{
			SendMessage(WM_COMMAND, MAKEWPARAM(IDC_COURSE_NEXT_CHAPTER, BN_CLICKED), 0);
			return TRUE;
		}
		else if (pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT || pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN)
		{
			return TRUE; //不转发给子控件
		}
	}
	else if (pMsg->message == UM_COURSE_PLAYER_TIME_CHANGED)
	{
		BOOL bHandled = 0;
		OnCoursePlayerTimeChanged(0, 0, 0, bHandled);
	}

	return IsDialogMessage(pMsg);
}

BOOL CVideoPlayerDlg::OnIdle()
{
	return FALSE;
}

class Background
{
public:
	Background()
	{
		m_hInstBkgRes = LoadLibrary(_T("BkgRes.dll"));
	}

	HBITMAP GetBitmap()
	{
		if (m_hInstBkgRes)
		{
			HBITMAP hBitmap = LoadBitmap(m_hInstBkgRes, MAKEINTRESOURCE(101));
			if (hBitmap)
				return hBitmap;
		}

		return GetDefaultBackground();
	}

	~Background()
	{
		FreeLibrary(m_hInstBkgRes);
	}

private:
	HBITMAP GetDefaultBackground() const
	{
		return AtlLoadGdiplusImage(IDB_BACKGROUND, _T("JPG"));
	}

private:
	HINSTANCE m_hInstBkgRes = NULL;

};
LRESULT CVideoPlayerDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	m_bmpBtnTitle.SubclassWindow(GetDlgItem(IDC_TITLE));
	CImageList imageList;
	imageList.Create(173, 72, TRUE | ILC_COLOR32, 1, 1);
	imageList.Add(LoadBitmapWithPNG(IDB_TITLE));
	m_bmpBtnTitle.SetImageList(imageList);
	m_bmpBtnTitle.SetImages(0);

	Background background;
	m_coursePlayer.SetBitmap(background.GetBitmap());

	m_courseList.SubclassWindow(GetDlgItem(IDC_COURSE_LIST));
	m_courseList.SetItemHeight(0, 70);

	m_courseImageList.Create(80, 64, TRUE | ILC_COLOR32, 1, 1);
	m_courseList.SetImageList(m_courseImageList.m_hImageList, ILSIL_NORMAL);
	m_courseList.SetImageList(m_courseImageList.m_hImageList, ILSIL_SELECTED);

	m_courseChapterList.SubclassWindow(GetDlgItem(IDC_COURSE_ITEM_LIST));
	m_courseChapterList.SetItemHeight(0, 30);

	//ILBSETTINGS cfg;
	//m_courseList.GetPreferences(&cfg);
	//cfg.clrHighlite = ::GetSysColor(COLOR_HIGHLIGHT);	//RGB(247, 247, 247);
	//cfg.clrHighliteText = RGB(0, 0, 0);
	//cfg.clrHighliteBorder = RGB(148, 150, 148);
	//cfg.clrText = RGB(0, 0, 0);
	//cfg.clrBackground = RGB(255, 255, 255);
	//cfg.sizeMargin.cx = cfg.sizeMargin.cy = 1;
	//cfg.sizeSubIndent.cx = 10;
	//cfg.sizeSubIndent.cy = 4;
	//m_courseList.SetPreferences(cfg);

	InsertCourses();

	m_playlist.SetCourseByIndex(m_playlist.GetLastPlayCourseIndex());
	SetCurrentCourseIndex(m_playlist.GetLastPlayCourseIndex());

	m_coursePlayer.SubclassWindow(GetDlgItem(IDC_COURSE_PLAYER));

	m_linkSourceCode.SubclassWindow(GetDlgItem(IDC_SOURCE_CODE_LINK));
	m_linkSourceCode.SetHyperLink(_T("https://github.com/vwarship/ZaoQiBu"));

	CreateBitmapButton(IDC_ADD_COURSE, { IDB_ADD_COURSE }, _T("增加课程"), m_bmpBtnAddCourse, 32);
	CreateBitmapButton(IDC_DELETE_COURSE, { IDB_DELETE_COURSE }, _T("删除课程"), m_bmpBtnDeleteCourse, 32);

	CreateBitmapButton(ID_APP_ABOUT, { IDB_ABOUT }, _T("帮助 热键:F1"), m_bmpBtnHelp);
	CreateBitmapButton(ID_APP_EXIT, { IDB_APP_EXIT }, _T("退出"), m_bmpBtnAppExit);
	CreateBitmapButton(IDC_COURSE_PREV_CHAPTER, { IDB_COURSE_PREV_CHAPTER }, _T("上一章 热键:Page Up"), m_bmpBtnCoursePrevChapter);
	CreateBitmapButton(IDC_COURSE_PLAY, { IDB_COURSE_PLAY, IDB_COURSE_PAUSE }, _T("播放 热键:空格"), m_bmpBtnPlay);
	CreateBitmapButton(IDC_COURSE_NEXT_CHAPTER, { IDB_COURSE_NEXT_CHAPTER }, _T("下一章 热键:Page Down"), m_bmpBtnCourseNextChapter);
	CreateBitmapButton(IDC_FULL_SCREEN, { IDB_FULL_SCREEN }, _T("全屏 热键:F11"), m_bmpBtnFullScreen);
	CreateBitmapButton(IDC_SNAPSHOT, { IDB_SNAPSHOT }, _T("快照 热键:F12"), m_bmpBtnSnapshot);
	CreateBitmapButton(IDC_MUTE, { IDB_SOUND_ON, IDB_SOUND_OFF }, _T("音量"), m_bmpBtnMute);

	m_hWndMediaTime = GetDlgItem(IDC_MEDIA_TIME);
	m_sMediaCurrentTime = m_sMediaLength = _T("00:00:00");

	m_hWndVolume = GetDlgItem(IDC_VOLUME);
	InitVolume();

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	DoDataExchange(false);
	DlgResize_Init(false, false, 0);

	return TRUE;
}

void CVideoPlayerDlg::InsertCourses()
{
	const shared_ptr<Courses> courses = m_playlist.GetCourses();
	for (size_t i = 0; i < courses->Count(); ++i)
	{
		const shared_ptr<Course> course = courses->GetCourse(i);
		InsertCourse(course, i);
	}
}

void CVideoPlayerDlg::InsertCourse(const shared_ptr<Course> course, int iImage)
{
	AddCourseImage(course->GetIcon().data());

	ILBITEM item = { 0 };
	item.mask = ILBIF_TEXT | ILBIF_IMAGE | ILBIF_SELIMAGE | ILBIF_STYLE | ILBIF_FORMAT;
	item.iItem = iImage;
	item.iImage = iImage;
	item.iSelImage = iImage;
	item.pszText = const_cast<LPTSTR>(course->GetTitle().data());
	item.style = ILBS_IMGLEFT | ILBS_SELROUND;
	item.format = DT_LEFT;
	m_courseList.InsertItem(&item);
}

void CVideoPlayerDlg::AddCourseImage(LPCTSTR pszImageFilename)
{
	CBitmap bitmap = AtlLoadGdiplusImage(pszImageFilename);
	if (bitmap.IsNull())
		bitmap = LoadBitmapWithPNG(IDB_COURSE_ICON_CARROT);

	SIZE thumbnailSize = { 0 };
	m_courseImageList.GetIconSize(thumbnailSize);
	HBITMAP hBitmapThumbnail = CreateThumbnail(GetDC(), bitmap, thumbnailSize);
	m_courseImageList.Add(hBitmapThumbnail);
}

HBITMAP CVideoPlayerDlg::LoadBitmapWithPNG(int nID)
{
	return AtlLoadGdiplusImage(nID, _T("PNG"));
}

int CVideoPlayerDlg::GetCurrentCourseIndex() const
{
	return m_courseList.GetCurSel();
}

void CVideoPlayerDlg::SetCurrentCourseIndex(int courseIndex)
{
	if (m_courseList.GetCount() <= 0)
		return;

	if (courseIndex >= m_courseList.GetCount())
		courseIndex = 0;

	SelectListBox(m_courseList, courseIndex);
}

void CVideoPlayerDlg::SelectListBox(CImageListBoxCtrl &listbox, int index)
{
	if (listbox.GetCount() == 0)
		return;

	listbox.SetCurSel(index);
	SendMessage(WM_COMMAND,
		MAKEWPARAM(listbox.GetDlgCtrlID(), LBN_SELCHANGE),
		(LPARAM)listbox.m_hWnd);
}

void CVideoPlayerDlg::InitVolume()
{
	SetSliderRange(m_hWndVolume, 0, 100);
	SetSliderPos(m_hWndVolume, m_playlist.GetVolume());
}

int CVideoPlayerDlg::GetCurrentChapterIndex() const
{
	return m_courseChapterList.GetCurSel();
}

void CVideoPlayerDlg::SetCurrentChapterIndex(int chapterIndex)
{
	SelectListBox(m_courseChapterList, chapterIndex);
}

void CVideoPlayerDlg::DeleteAllCourseChapters()
{
	for (int i = m_courseChapterList.GetCount() - 1; i >= 0; --i)
	{
		//PILBITEM pItem = (PILBITEM)m_courseChapterList.GetItemDataPtr(i);

		//DELETEITEMSTRUCT dis = { 0 };
		//dis.itemData = reinterpret_cast<ULONG_PTR>(pItem);
		//m_courseChapterList.DeleteItem(&dis);
		m_courseChapterList.DeleteString(i);
	}
}

void CVideoPlayerDlg::AddCourseChapters(const shared_ptr<Course> course)
{
	for (size_t i = 0; i < course->GetChapterCount(); ++i)
	{
		const Chapter &chapter = course->GetChapter(i);
		tstring itemName = chapter.GetTitle();

		ILBITEM item = { 0 };
		item.mask = ILBIF_TEXT | ILBIF_STYLE | ILBIF_FORMAT;
		item.iItem = i;
		item.pszText = const_cast<LPTSTR>(itemName.data());
		item.style = ILBS_SELROUND;
		item.format = DT_LEFT | DT_VCENTER;
		m_courseChapterList.InsertItem(&item);
	}
}

void CVideoPlayerDlg::CreateBitmapButton(int nButtonID, const std::vector<int> &imageIDs, PCTSTR toolTipText, CBitmapButton &bitmapButton, int size/* = 48*/)
{
	bitmapButton.SubclassWindow(GetDlgItem(nButtonID));
	bitmapButton.SetToolTipText(toolTipText);
	bitmapButton.SetBitmapButtonExtendedStyle(BMPBTN_HOVER);

	size_t imageIDCount = imageIDs.size();

	CImageList imageList;
	imageList.Create(size, size, TRUE | ILC_COLOR32, 2 * imageIDCount, 2 * imageIDCount);

	for (int imageID : imageIDs)
	{
		imageList.Add(LoadBitmapWithPNG(imageID));
		imageList.Add(CreateHoverBitmapWithPNG(GetDC(), imageID));
	}

	bitmapButton.SetImageList(imageList);
	bitmapButton.SetImages(0, -1, 1);
}

void CVideoPlayerDlg::UpdatePlayTime(int64_t currentPlayTime)
{
	int64_t curPos = currentPlayTime / 1000;
	int64_t mediaLength = m_coursePlayer.GetLength() / 1000;

	CTimeSpan curPosTime(static_cast<time_t>(curPos));
	CTimeSpan mediaLengthTime(static_cast<time_t>(mediaLength));

	static PCTSTR TIME_FORMAT = _T("%02d:%02d:%02d");
	m_sMediaCurrentTime.Format(TIME_FORMAT,
		curPosTime.GetHours(),
		curPosTime.GetMinutes(),
		curPosTime.GetSeconds());

	m_sMediaLength.Format(TIME_FORMAT,
		mediaLengthTime.GetHours(),
		mediaLengthTime.GetMinutes(),
		mediaLengthTime.GetSeconds());

	SetSliderPos(m_hWndMediaTime, (LPARAM)curPos);

	DoDataExchange(false);
}

void CVideoPlayerDlg::SetSliderPos(HWND hWnd, LPARAM pos)
{
	::SendMessage(hWnd, (UINT)TBM_SETPOS, (WPARAM)TRUE, pos);
}

void CVideoPlayerDlg::SetSliderRange(HWND hWnd, DWORD start, DWORD end)
{
	::SendMessage(hWnd, (UINT)TBM_SETRANGE, (WPARAM)0, 
		(LPARAM)MAKELONG(start, end));
}

void CVideoPlayerDlg::FullScreen(bool isFullScreen)
{
	if (isFullScreen)
	{
		ShowChildWindows(!isFullScreen);
	}

	m_coursePlayer.ShowWindow(isFullScreen ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);

	if (!isFullScreen)
	{
		ShowChildWindows(!isFullScreen);
	}

	m_isFullScreen = isFullScreen;
}

void CVideoPlayerDlg::ShowChildWindows(bool isShow)
{
	static const int windowIDs[] = {
		IDC_LOGO,
		ID_APP_ABOUT,
		ID_APP_EXIT,
		IDC_TITLE,
		IDC_COURSE_LIST,
		IDC_ADD_COURSE,
		IDC_DELETE_COURSE,
		IDC_COURSE_ITEM_LIST,
		IDC_COURSE_PREV_CHAPTER,
		IDC_COURSE_PLAY,
		IDC_COURSE_NEXT_CHAPTER,
		IDC_SNAPSHOT,
		IDC_FULL_SCREEN,
		IDC_MUTE, IDC_VOLUME,
		IDC_MEDIA_CURRENT_TIME,
		IDC_MEDIA_TIME,
		IDC_MEDIA_LENGTH,
		IDC_SOURCE_CODE_LINK
	};

	for (int nID : windowIDs)
	{
		::ShowWindow(GetDlgItem(nID), isShow ? SW_SHOW : SW_HIDE);
	}
}

LRESULT CVideoPlayerDlg::OnAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	UINT cmd = GET_APPCOMMAND_LPARAM(lParam);
	if (APPCOMMAND_MEDIA_PLAY_PAUSE == cmd)
	{
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_COURSE_PLAY, BN_CLICKED), 0);
		m_bmpBtnPlay.Invalidate();
	}

	// Return TRUE from a WM_APPCOMMAND handler, even if you process the message.
	return TRUE;
}

LRESULT CVideoPlayerDlg::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	int iScrollCode = (int)LOWORD(wParam);
	HWND hWndScrollBar = (HWND)lParam;

	if (m_hWndMediaTime == hWndScrollBar)	//播放时间
	{
		int nPos = SendMessage(hWndScrollBar, TBM_GETPOS, 0, 0);

		switch (iScrollCode)
		{
		case SB_THUMBTRACK://用户拖动滚动框
		case SB_PAGEUP:
		case SB_PAGEDOWN:
			m_coursePlayer.SetTime(nPos * 1000);
			break;
		}
	}
	else if (m_hWndVolume == hWndScrollBar)	//音量
	{
		int nPos = SendMessage(hWndScrollBar, TBM_GETPOS, 0, 0);

		switch (iScrollCode)
		{
		case SB_THUMBTRACK:
		case SB_PAGEUP:
		case SB_PAGEDOWN:
			m_coursePlayer.SetVolume(nPos);
			m_playlist.SetVolume(nPos);
			if (nPos == 0)
				m_bmpBtnMute.SetImages(2, -1, 3);
			else
				m_bmpBtnMute.SetImages(0, -1, 1);
			m_bmpBtnMute.Invalidate();
			break;
		}
	}

	return 0;
}

LRESULT CVideoPlayerDlg::OnCtrlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (!m_hBrushBackgroundColor)
		m_hBrushBackgroundColor = ::CreateSolidBrush(RGB(235, 248, 164));	//RGB(255, 229, 126)

	SetBkMode((HDC)wParam, TRANSPARENT);

	return (LRESULT)m_hBrushBackgroundColor;
}

LRESULT CVideoPlayerDlg::OnCourseListSelChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	//避免对当前已经选中的课程重复响应
	//if (IsCourseSelected())
	//	return TRUE;

	m_selectedCourseIndex = m_courseList.GetCurSel();

	DeleteAllCourseChapters();
	AddCourseChapters(m_playlist.GetCourseByIndex(m_selectedCourseIndex));

	SetCurrentChapterIndex(m_playlist.GetSelectedCourseLastPlayChapterIndex());

	return 0;
}

static bool isFirstSelected = false;
bool CVideoPlayerDlg::IsCourseSelected() const
{
	if (!isFirstSelected)
	{
		isFirstSelected = true;
		return false;
	}

	return m_playlist.GetCurrentCourseIndex() == m_courseList.GetCurSel();
}

LRESULT CVideoPlayerDlg::OnCourseChapterListSelChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_selectedChapterIndex = m_courseChapterList.GetCurSel();
	return 0;
}

LRESULT CVideoPlayerDlg::OnCourseChapterListDoubleClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_startTime = time(NULL);

	Play();
	return 0;
}

LRESULT CVideoPlayerDlg::OnCoursePrevChapter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(IDC_COURSE_PREV_CHAPTER, !m_bmpBtnCoursePrevChapter.IsWindowEnabled());
	PlayCoursePrevChapter();
	return 0;
}

LRESULT CVideoPlayerDlg::OnCoursePlay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(IDC_COURSE_PLAY, !m_bmpBtnPlay.IsWindowEnabled());

	m_startTime = time(NULL);

	if (m_coursePlayer.IsNotOpen())
	{
		Play();
	}
	else if (m_coursePlayer.IsPlaying())
	{
		m_coursePlayer.Pause();
		m_bmpBtnPlay.SetImages(0, -1, 1);
	}
	else if (m_coursePlayer.IsPaused())
	{
		m_coursePlayer.Play();
		m_bmpBtnPlay.SetImages(2, -1, 3);
	}

	return 0;
}

void CVideoPlayerDlg::Play()
{
	//判断是否还是从上一次播放的章节开始，不是就应该从头开始播放。
	int lastPlayChapterIndex = m_playlist.GetLastPlayChapterIndex();
	if (lastPlayChapterIndex != m_selectedChapterIndex)
		m_playlist.SetLastPlayChapterTime(0);

	m_playlist.SetCourseByIndex(m_selectedCourseIndex);
	m_playlist.SetCurrentCourseIndex(m_selectedCourseIndex);
	m_playlist.SetCurrentChapterIndex(m_selectedChapterIndex);
	m_playlist.SetPlayRecord(m_selectedCourseIndex, m_selectedChapterIndex);

	const Chapter& currentChapter = m_playlist.GetCurrentChapter();

	tstring selectedFilename = currentChapter.GetFilePath();
	if (FileUtil::IsExistWithFile(selectedFilename))
	{
		m_coursePlayer.OpenMedia(selectedFilename);
		m_coursePlayer.Play();

		m_coursePlayer.SetTime(m_playlist.GetLastPlayChapterTime());
		m_coursePlayer.SetVolume(m_playlist.GetVolume());

		m_bmpBtnPlay.SetImages(2, -1, 3);
		m_bmpBtnPlay.Invalidate();

		Sleep(500);

		InitMediaTimeControl();
	}
	else
	{
		//更新画面
	}
}

void CVideoPlayerDlg::InitMediaTimeControl()
{
	SetSliderRange(m_hWndMediaTime, 0, static_cast<DWORD>(m_coursePlayer.GetLength() / 1000));
	SetSliderPos(m_hWndMediaTime, 0);
}

LRESULT CVideoPlayerDlg::OnCourseNextChapter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(IDC_COURSE_NEXT_CHAPTER, !m_bmpBtnCourseNextChapter.IsWindowEnabled());
	PlayCourseNextChapter();
	return 0;
}

LRESULT CVideoPlayerDlg::OnSnapshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(IDC_SNAPSHOT, !m_bmpBtnSnapshot.IsWindowEnabled());
	m_coursePlayer.Snapshot();
	return 0;
}

LRESULT CVideoPlayerDlg::OnMute(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(IDC_MUTE, !m_bmpBtnMute.IsWindowEnabled());

	int currentVolume = m_coursePlayer.GetVolume();
	if (currentVolume == -1)
		return 0;

	if (m_coursePlayer.GetMute())
	{
		m_coursePlayer.SetMute(false);
		SetSliderPos(m_hWndVolume, m_coursePlayer.GetVolume());
		m_bmpBtnMute.SetImages(0, -1, 1);
	}
	else
	{
		m_coursePlayer.SetMute(true);
		SetSliderPos(m_hWndVolume, 0);
		m_bmpBtnMute.SetImages(2, -1, 3);
	}

	return 0;
}

LRESULT CVideoPlayerDlg::OnFullScreen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(IDC_FULL_SCREEN, !m_bmpBtnFullScreen.IsWindowEnabled());
	FullScreen(true);
	return 0;
}

LRESULT CVideoPlayerDlg::OnAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(ID_APP_ABOUT, !m_bmpBtnHelp.IsWindowEnabled());
	CAboutDlg dlg;
	dlg.DoModal();

	return 0;
}

LRESULT CVideoPlayerDlg::OnExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_config.Save();

	UIEnable(ID_APP_EXIT, !m_bmpBtnAppExit.IsWindowEnabled());
	this->GetParent().PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CVideoPlayerDlg::OnCoursePlayerTimeChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	int64_t currentPlayTime = m_coursePlayer.GetTime();
	m_playlist.SetLastPlayChapterTime(currentPlayTime);

	UpdatePlayTime(currentPlayTime);

	const Chapter& currentChapter = m_playlist.GetCurrentChapter();
	const int64_t currentChapterEndTime = currentChapter.GetEndTime();
	if ( m_coursePlayer.IsEnded() ||
		(currentChapterEndTime>0 && currentPlayTime >= currentChapterEndTime))	//如果设置了结束标记
	{
		m_coursePlayer.Stop();
		PlayCourseNextChapter();
	}

	//int64_t courseLength = m_coursePlayer.GetLength();
	//int64_t courseEndTime = courseLength;
	//if (currentChapterEndTime > 0 && currentChapterEndTime < courseLength)
	//	courseEndTime = currentChapterEndTime;
	//int64_t currentChapterRemainTime = courseEndTime - currentPlayTime;

	time_t currentTime = time(NULL);
	int timeLimit = m_playlist.GetTimeLimit() * 60;
	if ( timeLimit > 0 && 
		(currentTime - m_startTime) > timeLimit)
	{
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_COURSE_PLAY, BN_CLICKED), 0);
		m_bmpBtnPlay.Invalidate();
	}

	return TRUE;
}

void CVideoPlayerDlg::PlayCoursePrevChapter()
{
	int index = m_playlist.GetPrevChapter();
	if (index != -1)
	{
		SetCurrentChapterIndex(index);
		Play();
	}
}

void CVideoPlayerDlg::PlayCourseNextChapter()
{
	int index = m_playlist.GetNextChapter();
	if (index != -1)
	{
		SetCurrentChapterIndex(index);
		Play();
	}
}

LRESULT CVideoPlayerDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_coursePlayer.Stop();

	EndDialog(wID);
	return 0;
}

LRESULT CVideoPlayerDlg::OnAddCourse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(IDC_ADD_COURSE, !m_bmpBtnAddCourse.IsWindowEnabled());

	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("早起步课程 (*.zqb)\0*.zqb\0"));
	if (dlg.DoModal() == IDOK)
	{
		shared_ptr<Course> course = CourseUtil::Create(dlg.m_szFileName);
		if (course)
		{
			course->SetPath(dlg.m_szFileName);
			m_playlist.AddCourse(course);
			InsertCourse(course, m_courseList.GetCount());
		}
	}

	return 0;
}

LRESULT CVideoPlayerDlg::OnDeleteCourse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UIEnable(IDC_DELETE_COURSE, !m_bmpBtnDeleteCourse.IsWindowEnabled());

	size_t curIndex = m_courseList.GetCurSel();
	m_playlist.RemoveCourse(curIndex);

	//PILBITEM pItem = (PILBITEM)m_courseList.GetItemDataPtr(curIndex);
	//DELETEITEMSTRUCT dis = { 0 };
	//dis.itemData = reinterpret_cast<ULONG_PTR>(pItem);
	//m_courseList.DeleteItem(&dis);

	m_courseList.DeleteString(curIndex);
	m_courseImageList.Remove(curIndex);

	int courseCount = m_courseList.GetCount();
	for (int i = curIndex; i < courseCount; ++i)
	{
		ILBITEM item = { 0 };
		item.mask = ILBIF_IMAGE | ILBIF_SELIMAGE;
		item.iItem = i;
		m_courseList.GetItem(&item);
		item.iImage = i;
		item.iSelImage = i;

		m_courseList.SetItem(&item);
	}

	if (curIndex < m_courseList.GetCount())
		SetCurrentCourseIndex(curIndex);
	else
		SetCurrentCourseIndex(curIndex-1);

	return 0;
}
