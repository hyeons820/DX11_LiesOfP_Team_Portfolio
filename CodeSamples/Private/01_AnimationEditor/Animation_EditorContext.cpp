// 포트폴리오 코드 발췌: 클립 구간·복제 → Notify/Window 설정 → 시퀀스 연결 → 타임라인 동기화
// 원본: Animation_Editor/Private/Animation_EditorContext.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 793~820행: Set_SelectedClipFrameRange
void CAnimation_EditorContext::Set_SelectedClipFrameRange(_uint iStartFrame, _uint iEndFrame)
{
	ANIM_CLIP_SETTING* pClip = Get_SelectedClipSetting();

	if (pClip == nullptr)
		return;

	if (pClip->iSourceFrameCount == 0)
		return;

	const _uint iMaxFrame = pClip->iSourceFrameCount - 1;

	if (iStartFrame > iMaxFrame)
		iStartFrame = iMaxFrame;

	if (iEndFrame > iMaxFrame)
		iEndFrame = iMaxFrame;

	if (iStartFrame > iEndFrame)
		swap(iStartFrame, iEndFrame);

	pClip->iStartFrame = iStartFrame;
	pClip->iEndFrame = iEndFrame;

	Sync_PreviewTimeline();

	Set_ClipRangeDirty(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 822~891행: Duplicate_SelectedClipSetting
void CAnimation_EditorContext::Duplicate_SelectedClipSetting()
{
	const _int iSelectedClipIndex = m_tSelection.iSelectedClipIndex;

	if (iSelectedClipIndex < 0 || iSelectedClipIndex >= static_cast<_int>(m_tLoadedData.ClipSettings.size()))
		return;

	ANIM_CLIP_SETTING NewClip = m_tLoadedData.ClipSettings[iSelectedClipIndex];

	const string strBaseName = NewClip.strExportName.empty() ? NewClip.strSourceName : NewClip.strExportName;

	string strNewExportName;
	_uint64 iNewClipHash = 0;

	for (_int iSuffix = 1; iSuffix < 10000; ++iSuffix)
	{
		strNewExportName = strBaseName + "_Variant_" + to_string(iSuffix);
		iNewClipHash = Hash_String(StringToWString(strNewExportName).c_str());

		_bool bDuplicated = false;

		for (const auto& Clip : m_tLoadedData.ClipSettings)
		{
			if (Clip.strExportName == strNewExportName || Clip.iClipHash == iNewClipHash)
			{
				bDuplicated = true;
				break;
			}
		}

		if (!bDuplicated)
			break;

		strNewExportName.clear();
		iNewClipHash = 0;
	}

	if (strNewExportName.empty() || iNewClipHash == 0)
		return;

	NewClip.strExportName = strNewExportName;
	NewClip.iClipHash = iNewClipHash;

	Register_RemovableClipHash(NewClip.iClipHash);

	// Raw Animation 참조는 원본과 동일하게 유지
	// strSourceName / iRawAnimationHash / iRawAnimationIndex / source duration / frame count  변경 X

	m_tLoadedData.ClipSettings.push_back(NewClip);

	m_tSelection.iSelectedClipIndex = static_cast<_int>(m_tLoadedData.ClipSettings.size()) - 1;
	m_tSelection.iSelectedNotifyIndex = -1;
	m_tSelection.iSelectedWindowIndex = -1;
	m_tSelection.iSelectedSpeedSectionIndex = -1;

	m_tTimeline.eMode = EDITOR_TIMELINE_MODE::CLIP;
	m_tTimeline.iCurFrame = 0;
	m_tTimeline.fCurTime = 0.f;

	Sync_PreviewTimeline();

	m_tPreview.bPreviewAvailable = true;
	m_tPreview.bIsPlaying = true;
	m_tPreview.bIsLoop = NewClip.bLoop;
	m_tPreview.bUseRootMotionPreview = NewClip.bUseRootMotion;
	m_tPreview.fPlayRate = NewClip.fPlayRate;

	Set_ClipListDirty(true);
	Set_ClipDataDirty(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1079~1125행: Add_Notify
void CAnimation_EditorContext::Add_Notify()
{
	ANIM_CLIP_SETTING* pClip = Get_SelectedClipSetting();

	if (pClip == nullptr)
		return;

	_uint iMaxFrame = 0;

	if (pClip->iEndFrame >= pClip->iStartFrame)
		iMaxFrame = pClip->iEndFrame - pClip->iStartFrame;
	else if (pClip->iSourceFrameCount > 0)
		iMaxFrame = pClip->iSourceFrameCount - 1;

	_uint iFrame = m_tTimeline.iCurFrame;

	if (iFrame > iMaxFrame)
		iFrame = iMaxFrame;

	ANIM_NOTIFY Notify = {};
	Notify.strName = "New Notify";
	Notify.iNameHash = Hash_String(StringToWString(Notify.strName).c_str());

	Notify.eType = ANIM_NOTIFY_TYPE::CUSTOM;

	Notify.iFrame = iFrame;

	if (m_tTimeline.iFrameCount > 1)
		Notify.fTime = m_tTimeline.fDuration * static_cast<_float>(iFrame) / static_cast<_float>(m_tTimeline.iFrameCount - 1);
	else
		Notify.fTime = 0.f;

	Notify.strPayload.clear();
	Notify.iPayloadHash = 0;

	Notify.bUseSlot = false;
	Notify.Slot = {};
	Notify.Slot.iBoneIndex = -1;
	Notify.Slot.vOffsetScale = { 1.f, 1.f, 1.f };

	pClip->Notifies.push_back(Notify);

	m_tSelection.iSelectedNotifyIndex = pClip->Notifies.size() - 1;
	m_tSelection.iSelectedWindowIndex = -1;

	Set_ClipDataDirty(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2200~2229행: Set_SelectedNotifyFrame
void CAnimation_EditorContext::Set_SelectedNotifyFrame(_uint iFrame)
{
	ANIM_CLIP_SETTING* pClip = Get_SelectedClipSetting();
	ANIM_NOTIFY* pNotify = Get_SelectedNotify();

	if (pClip == nullptr || pNotify == nullptr)
		return;

	if (pClip->iSourceFrameCount == 0)
		return;

	_uint iMaxFrame = 0;

	if (pClip->iEndFrame >= pClip->iStartFrame)
		iMaxFrame = pClip->iEndFrame - pClip->iStartFrame;
	else if (pClip->iSourceFrameCount > 0)
		iMaxFrame = pClip->iSourceFrameCount - 1;

	if (iFrame > iMaxFrame)
		iFrame = iMaxFrame;

	pNotify->iFrame = iFrame;

	if (m_tTimeline.iFrameCount > 1)
		pNotify->fTime = m_tTimeline.fDuration * static_cast<_float>(iFrame) / static_cast<_float>(m_tTimeline.iFrameCount - 1);
	else
		pNotify->fTime = 0.f;

	Set_ClipDataDirty(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1189~1228행: Add_Window
void CAnimation_EditorContext::Add_Window()
{
	ANIM_CLIP_SETTING* pClip = Get_SelectedClipSetting();

	if (pClip == nullptr)
		return;

	_uint iMaxFrame = 0;

	if (pClip->iEndFrame >= pClip->iStartFrame)
		iMaxFrame = pClip->iEndFrame - pClip->iStartFrame;
	else if (pClip->iSourceFrameCount > 0)
		iMaxFrame = pClip->iSourceFrameCount - 1;

	_uint iStartFrame = m_tTimeline.iCurFrame;

	if (iStartFrame > iMaxFrame)
		iStartFrame = iMaxFrame;

	_uint iEndFrame = iStartFrame + 5;

	if (iEndFrame > iMaxFrame)
		iEndFrame = iMaxFrame;

	ANIM_WINDOW Window = {};
	Window.strName = "New Window";
	Window.iNameHash = Hash_String(StringToWString(Window.strName).c_str());
	Window.eType = ANIM_WINDOW_TYPE::CUSTOM;
	Window.iStartFrame = iStartFrame;
	Window.iEndFrame = iEndFrame;
	Window.fValue = 0.f;
	Window.iPayloadHash = 0;
	Window.strPayload.clear();

	pClip->Windows.push_back(Window);

	m_tSelection.iSelectedWindowIndex = pClip->Windows.size() - 1;

	Set_ClipDataDirty(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2354~2385행: Set_SelectedWindowFrameRange
void CAnimation_EditorContext::Set_SelectedWindowFrameRange(_uint iStartFrame, _uint iEndFrame)
{
	ANIM_CLIP_SETTING* pClip = Get_SelectedClipSetting();
	ANIM_WINDOW* pWindow = Get_SelectedWindow();

	if (pClip == nullptr || pWindow == nullptr)
		return;

	if (pClip->iSourceFrameCount == 0)
		return;

	_uint iMaxFrame = 0;

	if (pClip->iEndFrame >= pClip->iStartFrame)
		iMaxFrame = pClip->iEndFrame - pClip->iStartFrame;
	else
		iMaxFrame = pClip->iSourceFrameCount - 1;

	if (iStartFrame > iMaxFrame)
		iStartFrame = iMaxFrame;

	if (iEndFrame > iMaxFrame)
		iEndFrame = iMaxFrame;

	if (iStartFrame > iEndFrame)
		swap(iStartFrame, iEndFrame);

	pWindow->iStartFrame = iStartFrame;
	pWindow->iEndFrame = iEndFrame;

	Set_ClipDataDirty(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2387~2420행: Set_SelectedWindowFrameRange
void CAnimation_EditorContext::Set_SelectedWindowFrameRange(_int iWindowIndex, _uint iStartFrame, _uint iEndFrame)
{
	ANIM_CLIP_SETTING* pClip = Get_SelectedClipSetting();

	if (pClip == nullptr)
		return;

	if (pClip->iSourceFrameCount == 0)
		return;

	if (iWindowIndex < 0 || iWindowIndex >= pClip->Windows.size())
		return;

	_uint iMaxFrame = 0;

	if (pClip->iEndFrame >= pClip->iStartFrame)
		iMaxFrame = pClip->iEndFrame - pClip->iStartFrame;
	else
		iMaxFrame = pClip->iSourceFrameCount - 1;

	if (iStartFrame > iMaxFrame)
		iStartFrame = iMaxFrame;

	if (iEndFrame > iMaxFrame)
		iEndFrame = iMaxFrame;

	if (iStartFrame > iEndFrame)
		swap(iStartFrame, iEndFrame);

	pClip->Windows[iWindowIndex].iStartFrame = iStartFrame;
	pClip->Windows[iWindowIndex].iEndFrame = iEndFrame;

	Set_ClipDataDirty(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1334~1359행: Add_Sequence
void CAnimation_EditorContext::Add_Sequence()
{
	ANIM_SEQUENCE Sequence = {};

	Sequence.strName = "New Sequence";
	Sequence.iNameHash = Hash_String(StringToWString(Sequence.strName).c_str());

	Sequence.eFlow = ANIM_SEQUENCE_FLOW::ONE_SHOT;
	Sequence.eStepBlendMode = ANIM_SEQUENCE_STEP_BLEND_MODE::NONE;
	Sequence.eRestartRule = ANIM_SEQUENCE_RESTART_RULE::AFTER_INACTIVE;
	Sequence.bStartCancelByWindow = true;
	Sequence.bAllowEndToLoopReentry = false;
	Sequence.bUseCommitTime = false;
	Sequence.fCommitTime = 0.f;
	Sequence.fLoopToEndBlendTime = 0.f;
	Sequence.fEndBlendOutTime = 0.f;
	Sequence.fReentryBlendTime = 0.f;
	Sequence.fReentryStartOffsetTime = 0.f;

	m_tLoadedData.Sequences.push_back(Sequence);

	m_tSelection.iSelectedSequenceIndex = static_cast<_int>(m_tLoadedData.Sequences.size()) - 1;
	m_tSelection.iSelectedSequenceStepIndex = -1;

	Set_ControllerDirty(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1436~1462행: Add_SelectedSequenceStep
void CAnimation_EditorContext::Add_SelectedSequenceStep()
{
	ANIM_SEQUENCE* pSequence = Get_SelectedSequence();

	if (pSequence == nullptr)
		return;

	ANIM_SEQUENCE_STEP Step = {};
	Step.eStepType = ANIM_SEQUENCE_STEP_TYPE::ACTIVE;
	Step.iClipIndex = -1;
	Step.iClipHash = 0;

	if (m_tSelection.iSelectedClipIndex >= 0 && m_tSelection.iSelectedClipIndex < m_tLoadedData.ClipSettings.size())
	{
		const ANIM_CLIP_SETTING& Clip = m_tLoadedData.ClipSettings[m_tSelection.iSelectedClipIndex];

		Step.iClipIndex = m_tSelection.iSelectedClipIndex;
		Step.iClipHash = Clip.iClipHash;
	}

	pSequence->Steps.push_back(Step);

	m_tSelection.iSelectedSequenceStepIndex = static_cast<_int>(pSequence->Steps.size()) - 1;

	Set_ControllerDirty(true);
	Sync_SequenceTimeline();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2723~2745행: Set_SelectedSequenceStepClip
void CAnimation_EditorContext::Set_SelectedSequenceStepClip(_int iClipIndex)
{
	ANIM_SEQUENCE_STEP* pStep = Get_SelectedSequenceStep();

	if (pStep == nullptr)
		return;

	if (iClipIndex < 0 || iClipIndex >= m_tLoadedData.ClipSettings.size())
	{
		pStep->iClipIndex = -1;
		pStep->iClipHash = 0;
	}
	else
	{
		const ANIM_CLIP_SETTING& Clip = m_tLoadedData.ClipSettings[iClipIndex];

		pStep->iClipIndex = iClipIndex;
		pStep->iClipHash = Clip.iClipHash;
	}

	Set_ControllerDirty(true);
	Sync_SequenceTimeline();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 3745~3795행: Sync_PreviewTimeline
void CAnimation_EditorContext::Sync_PreviewTimeline()
{
	const ANIM_CLIP_SETTING* pClip = Get_SelectedClipSetting();

	if (pClip == nullptr)
		return;

	const _uint iFrameCount =
		pClip->iEndFrame >= pClip->iStartFrame ?
		pClip->iEndFrame - pClip->iStartFrame + 1 : pClip->iSourceFrameCount;

	_float fDuration = pClip->fSourceDuration;

	if (pClip->iSourceFrameCount > 1 && pClip->iEndFrame > pClip->iStartFrame)
	{
		const _float fFrameTime = pClip->fSourceDuration / static_cast<_float>(pClip->iSourceFrameCount - 1);

		fDuration = static_cast<_float>(pClip->iEndFrame - pClip->iStartFrame) * fFrameTime;
	}

	m_tTimeline.iFrameCount = iFrameCount;
	m_tTimeline.fDuration = fDuration;

	if (m_tTimeline.fCurTime > fDuration)
		m_tTimeline.fCurTime = fDuration;

	if (m_tTimeline.fCurTime < 0.f)
		m_tTimeline.fCurTime = 0.f;

	if (fDuration > 0.f && iFrameCount > 1)
	{
		_float fRatio = m_tTimeline.fCurTime / fDuration;

		if (fRatio < 0.f)
			fRatio = 0.f;

		if (fRatio > 1.f)
			fRatio = 1.f;

		m_tTimeline.iCurFrame = static_cast<_uint>(fRatio * static_cast<_float>(iFrameCount - 1));
	}
	else
	{
		m_tTimeline.iCurFrame = 0;
		m_tTimeline.fCurTime = 0.f;
	}

	m_tPreview.bIsLoop = pClip->bLoop;
	m_tPreview.bUseRootMotionPreview = pClip->bUseRootMotion;
	m_tPreview.fPlayRate = pClip->fPlayRate;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 3799~3842행: Sync_SequenceTimeline
void CAnimation_EditorContext::Sync_SequenceTimeline()
{
	const ANIM_SEQUENCE* pSequence = Get_SelectedSequence();

	m_tTimeline.iFrameCount = 0;
	m_tTimeline.fDuration = 0.f;

	if (pSequence == nullptr)
		return;

	for (const auto& Step : pSequence->Steps)
	{
		if (Step.iClipIndex < 0 || Step.iClipIndex >= m_tLoadedData.ClipSettings.size())
			continue;

		const ANIM_CLIP_SETTING& Clip = m_tLoadedData.ClipSettings[Step.iClipIndex];

		if (Clip.iEndFrame < Clip.iStartFrame)
			continue;

		const _uint iClipFrameCount = Clip.iEndFrame - Clip.iStartFrame + 1;
		m_tTimeline.iFrameCount += iClipFrameCount;

		if (Clip.iSourceFrameCount > 1)
		{
			const _float fFrameTime = Clip.fSourceDuration / static_cast<_float>(Clip.iSourceFrameCount - 1);

			m_tTimeline.fDuration += static_cast<_float>(iClipFrameCount - 1) * fFrameTime;
		}
	}

	if (m_tTimeline.iFrameCount == 0)
	{
		m_tTimeline.iCurFrame = 0;
		m_tTimeline.fCurTime = 0.f;
		return;
	}

	if (m_tTimeline.iCurFrame >= m_tTimeline.iFrameCount)
		m_tTimeline.iCurFrame = m_tTimeline.iFrameCount - 1;

	if (m_tTimeline.fCurTime > m_tTimeline.fDuration)
		m_tTimeline.fCurTime = m_tTimeline.fDuration;
}
