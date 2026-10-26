// 포트폴리오 코드 발췌: Update → 편집 변경 반영 → 시퀀스 재생 → 런타임 시간과 에디터 타임라인 동기화
// 원본: Animation_Editor/Private/Preview_Manager.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 282~483행: Update
// 공동 수정: 본인 구현·연동 외 팀원의 수정 이력이 포함된 함수입니다.
void CPreview_Manager::Update(_float fTimeDelta)
{    
    if (m_pGameInstance->Key_Down(DIK_TAB))
        m_bTab = !m_bTab;
    
	Update_Camera(fTimeDelta);

    if (m_pPreview_Level == nullptr || m_pEditorContext == nullptr)
        return;

    const LOADED_DATA_CONTEXT& tLoadedData = m_pEditorContext->Get_LoadedData();
    const SELECTION_CONTEXT& tSelection = m_pEditorContext->Get_Selection();
    const PREVIEW_CONTEXT& tPreview = m_pEditorContext->Get_Preview();
    const TIMELINE_CONTEXT& tTimeline = m_pEditorContext->Get_Timeline();
    const DIRTY_CONTEXT& tDirty = m_pEditorContext->Get_Dirty();


    if (m_iAppliedRootMotionBoneIndex != tPreview.iRootMotionBoneIndex)
    {
        m_iAppliedRootMotionBoneIndex = tPreview.iRootMotionBoneIndex;

        m_pPreview_Level->Set_RootMotionBoneIndex(tPreview.iRootMotionBoneIndex);
        m_pPreview_Level->Reset_RootMotionPreviewTransform();

        m_bSequenceRuntimeStarted = false;
        m_bSequenceWasPlaying = false;
    }

    if (m_pGameInstance->Key_Down(DIK_SPACE))
    {
        const bool bHasSelectedClip = 
            tTimeline.eMode == EDITOR_TIMELINE_MODE::CLIP && 
            tSelection.iSelectedClipIndex >= 0 && 
            tSelection.iSelectedClipIndex < tLoadedData.ClipSettings.size();

        const bool bHasSelectedSequence = 
            tTimeline.eMode == EDITOR_TIMELINE_MODE::SEQUENCE && 
            tSelection.iSelectedSequenceIndex >= 0 && 
            tSelection.iSelectedSequenceIndex < tLoadedData.Sequences.size() &&
            tTimeline.iFrameCount > 0;

        if (bHasSelectedSequence)
        {
            const bool bAtEnd = tTimeline.iFrameCount > 1 && tTimeline.iCurFrame >= tTimeline.iFrameCount - 1;

            if (tPreview.bIsPlaying)
            {
                m_pEditorContext->Set_Playing(false);
                m_pPreview_Level->Set_Playing(false);
                m_bSequenceWasPlaying = false;
            }
            else
            {
                if (bAtEnd || m_bSequencePreviewEnded)
                {
                    m_pEditorContext->Set_CurTime(0.f);
                    m_pEditorContext->Set_CurFrame(0);
                    Reset_SequencePreviewRuntime();
                }

                Start_SelectedSequencePreview();
            }
        }
        else if (bHasSelectedClip)
        {
            const bool bAtEnd = tTimeline.iFrameCount > 1 && tTimeline.iCurFrame >= tTimeline.iFrameCount - 1;

            if (bAtEnd)
            {
                m_pPreview_Level->Set_TrackPosition(0.f);
                m_pEditorContext->Set_CurTime(0.f);
                m_pEditorContext->Set_CurFrame(0);
                m_pEditorContext->Set_Playing(true);
                m_fLastAppliedTime = 0.f;
            }
            else
            {
                m_pEditorContext->Set_Playing(!tPreview.bIsPlaying);
            }
        }
    }

    if (m_pGameInstance->Key_Down(DIK_8))
        Reset_ModelTransform();
    if (m_pGameInstance->Key_Down(DIK_9))
        Reset_Camera();
    if (m_pGameInstance->Key_Down(DIK_0))
        Reset_CameraAndModel();

    const _bool bNeedsReapply =
        tDirty.bControllerDirty ||
        tDirty.bClipListDirty ||
        tDirty.bClipRangeDirty ||
        tDirty.bClipDataDirty;

    if (bNeedsReapply)
    {
        Reapply_EditData_AfterDirty(tLoadedData, tSelection, tTimeline);

        // 이번 프레임에 확인한 Dirty Flag를 한 번에 정리한다.
        m_pEditorContext->Clear_DirtyFlags();
    }

    if (tTimeline.eMode == EDITOR_TIMELINE_MODE::SEQUENCE)
    {
        Update_SequencePreview(fTimeDelta);

        m_pPreview_Level->Update(fTimeDelta);
        return;
    }

    if (m_iPoseLayerPreviewBaseClipIndex < 0 && tSelection.iSelectedClipIndex >= 0 && tSelection.iSelectedClipIndex != m_iLastClipIndex)
    {
        if (SUCCEEDED(m_pPreview_Level->Set_AnimationIndex(tSelection.iSelectedClipIndex)))
        {
            m_iLastClipIndex = tSelection.iSelectedClipIndex;
            m_fLastAppliedTime = 0.f;
            m_fLastClipDuration = tTimeline.fDuration;

            m_pPreview_Level->Set_TrackPosition(0.f);
            m_pEditorContext->Set_CurTime(0.f);
            m_pEditorContext->Set_CurFrame(0);
        }
    }

    const ANIM_CLIP_SETTING* pClip = m_pEditorContext->Get_SelectedClipSetting();
    const bool bClipRangeChanged = pClip != nullptr && fabsf(tTimeline.fDuration - m_fLastClipDuration) > 0.001f;

    if (bClipRangeChanged)
    {
        _float fKeepTime = tTimeline.fCurTime;

        if (fKeepTime > tTimeline.fDuration)
            fKeepTime = tTimeline.fDuration;

        if (fKeepTime < 0.f)
            fKeepTime = 0.f;

        m_fLastClipDuration = tTimeline.fDuration;
        m_fLastAppliedTime = fKeepTime;

        m_pPreview_Level->Set_TrackPosition(fKeepTime);
        m_pEditorContext->Set_CurTime(fKeepTime);

        if (tTimeline.fDuration > 0.f && tTimeline.iFrameCount > 1)
        {
            _float fRatio = fKeepTime / tTimeline.fDuration;
            m_pEditorContext->Set_CurFrame(static_cast<_uint>(fRatio * (tTimeline.iFrameCount - 1)));
        }
        else
        {
            m_pEditorContext->Set_CurFrame(0);
        }
    }

    m_pPreview_Level->Set_Loop(tPreview.bIsLoop);
    m_pPreview_Level->Set_PlayRate(tPreview.fPlayRate);
    m_pPreview_Level->Set_Playing(tPreview.bIsPlaying);
    m_pPreview_Level->Set_UseRootMotion(tPreview.bUseRootMotionPreview);

    const bool bUserSeek = fabsf(tTimeline.fCurTime - m_fLastAppliedTime) > 0.001f;

    if (!tPreview.bIsPlaying && bUserSeek)
    {
        m_pPreview_Level->Set_TrackPosition(tTimeline.fCurTime);
        m_fLastAppliedTime = tTimeline.fCurTime;
    }

    if (tPreview.bIsPlaying)
    {
        m_pPreview_Level->Update_Animation(fTimeDelta);

        _float fLocalTime = m_pPreview_Level->Get_TrackPosition();

        if (fLocalTime < 0.f)
            fLocalTime = 0.f;

        m_fLastAppliedTime = fLocalTime;

        m_pEditorContext->Set_CurTime(fLocalTime);

        if (tTimeline.fDuration > 0.f && tTimeline.iFrameCount > 1)
        {
            _float fRatio = fLocalTime / tTimeline.fDuration;

            if (fRatio < 0.f)
                fRatio = 0.f;

            if (fRatio > 1.f)
                fRatio = 1.f;

            const _uint iFrame = static_cast<_uint>(fRatio * (tTimeline.iFrameCount - 1));

            m_pEditorContext->Set_CurFrame(iFrame);
        }

        if (m_pPreview_Level->Is_AnimationEnded())
            m_pEditorContext->Set_Playing(false);
    }

	m_pPreview_Level->Update(fTimeDelta);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1026~1045행: Apply_EditData_ToPreview
void CPreview_Manager::Apply_EditData_ToPreview()
{
    if (m_pPreview_Level == nullptr || m_pEditorContext == nullptr)
        return;

    const LOADED_DATA_CONTEXT& tLoadedData = m_pEditorContext->Get_LoadedData();

    ANIMATION_EDIT_DATA EditData = {};
    EditData.ClipSettings = tLoadedData.ClipSettings;
    EditData.Sequences = tLoadedData.Sequences;
    EditData.Controller = tLoadedData.Controller;

    if (m_iPoseLayerPreviewBaseClipIndex >= 0)
        m_pPreview_Level->Set_ControllerEnabled(false);

    m_pPreview_Level->Set_EditData(EditData);

    const PREVIEW_CONTEXT& tPreview = m_pEditorContext->Get_Preview();
    m_pPreview_Level->Set_RootMotionBoneIndex(tPreview.iRootMotionBoneIndex);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1151~1181행: Reapply_EditData_AfterDirty
void CPreview_Manager::Reapply_EditData_AfterDirty(const LOADED_DATA_CONTEXT& tLoadedData, const SELECTION_CONTEXT& tSelection, const TIMELINE_CONTEXT& tTimeline)
{
    Apply_EditData_ToPreview();

    if (tTimeline.eMode == EDITOR_TIMELINE_MODE::SEQUENCE)
    {
        Reset_SequencePreviewRuntime();
        return;
    }

    _int iKeepClipIndex = m_iPoseLayerPreviewBaseClipIndex >= 0 ? m_iPoseLayerPreviewBaseClipIndex : tSelection.iSelectedClipIndex;

    const _float fKeepTime = tTimeline.fCurTime;

    if (iKeepClipIndex < 0 || iKeepClipIndex >= static_cast<_int>(tLoadedData.ClipSettings.size()))
    {
        m_iLastClipIndex = -1;
        m_fLastAppliedTime = 0.f;
        m_fLastClipDuration = -1;
        return;
    }

    if (SUCCEEDED(m_pPreview_Level->Set_AnimationIndex(iKeepClipIndex)))
    {
        m_pPreview_Level->Set_TrackPosition(fKeepTime);

        m_iLastClipIndex = iKeepClipIndex;
        m_fLastAppliedTime = fKeepTime;
        m_fLastClipDuration = tTimeline.fDuration;
    }
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 211~259행: Start_SelectedSequencePreview
void CPreview_Manager::Start_SelectedSequencePreview()
{
    if (m_pPreview_Level == nullptr || m_pEditorContext == nullptr)
        return;


    const SELECTION_CONTEXT& tSelection = m_pEditorContext->Get_Selection();
    const TIMELINE_CONTEXT& tTimeline = m_pEditorContext->Get_Timeline();

    if (tSelection.iSelectedSequenceIndex < 0)
        return;

    m_bSequencePreviewEnded = false;

    Apply_EditData_ToPreview();
    m_pPreview_Level->Reset_RootMotionPreviewTransform();

    m_pPreview_Level->Set_ControllerEnabled(false);

    SEQUENCE_FRAME_INFO FrameInfo = {};
    if (!Resolve_SequenceFrame(tTimeline.iCurFrame, FrameInfo))
        return;

    if (FrameInfo.iStepIndex < 0)
        return;

    if (tTimeline.iCurFrame == 0)
    {
        if (FAILED(m_pPreview_Level->Play_Sequence_ByIndex(
            static_cast<_uint>(tSelection.iSelectedSequenceIndex))))
            return;
    }
    else
    {
        if (FAILED(m_pPreview_Level->Preview_Sequence_ByIndex(
            static_cast<_uint>(tSelection.iSelectedSequenceIndex),
            static_cast<_uint>(FrameInfo.iStepIndex),
            FrameInfo.fLocalTime)))
            return;

        m_pPreview_Level->Set_Playing(true);
    }

    m_iLastSequenceIndex = tSelection.iSelectedSequenceIndex;
    m_bSequenceRuntimeStarted = true;
    m_bSequenceWasPlaying = true;

    m_pEditorContext->Set_Playing(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 928~1022행: Update_SequencePreview
void CPreview_Manager::Update_SequencePreview(_float fTimeDelta)
{
    if (m_pPreview_Level == nullptr || m_pEditorContext == nullptr)
        return;

    const LOADED_DATA_CONTEXT& tLoadedData = m_pEditorContext->Get_LoadedData();
    const SELECTION_CONTEXT& tSelection = m_pEditorContext->Get_Selection();
    const PREVIEW_CONTEXT& tPreview = m_pEditorContext->Get_Preview();
    const TIMELINE_CONTEXT& tTimeline = m_pEditorContext->Get_Timeline();

    if (tSelection.iSelectedSequenceIndex < 0 || tSelection.iSelectedSequenceIndex >= tLoadedData.Sequences.size())
        return;

    if (tTimeline.iFrameCount == 0)
        return;

    const bool bSequenceChanged = m_iLastSequenceIndex != tSelection.iSelectedSequenceIndex;

    if (bSequenceChanged)
    {
        m_iLastSequenceIndex = tSelection.iSelectedSequenceIndex;

        if (!tPreview.bIsPlaying)
        {
            m_bSequenceRuntimeStarted = false;
            m_fSequenceFrameCursor = static_cast<_float>(tTimeline.iCurFrame);

            Apply_EditData_ToPreview();
            m_pPreview_Level->Set_ControllerEnabled(false);
        }
    }

    //m_pPreview_Level->Set_PlayRate(tPreview.fPlayRate);
    //m_pPreview_Level->Set_UseRootMotion(tPreview.bUseRootMotionPreview);

    if (!tPreview.bIsPlaying)
    {
        SEQUENCE_FRAME_INFO FrameInfo = {};

        if (!Resolve_SequenceFrame(tTimeline.iCurFrame, FrameInfo))
            return;

        if (FrameInfo.iStepIndex < 0)
            return;

        m_pPreview_Level->Reset_RootMotionPreviewTransform();
        m_pPreview_Level->Preview_Sequence_ByIndex(static_cast<_uint>(tSelection.iSelectedSequenceIndex), static_cast<_uint>(FrameInfo.iStepIndex), FrameInfo.fLocalTime);

        m_bSequenceRuntimeStarted = false;
        m_bSequenceWasPlaying = false;
        return;
    }
   
    if (!m_bSequenceRuntimeStarted || !m_bSequenceWasPlaying)
    {
        SEQUENCE_FRAME_INFO FrameInfo = {};

        if (Resolve_SequenceFrame(tTimeline.iCurFrame, FrameInfo))
        {
            m_pPreview_Level->Set_ControllerEnabled(false);
            m_pPreview_Level->Reset_RootMotionPreviewTransform();

            m_pPreview_Level->Preview_Sequence_ByIndex(
                static_cast<_uint>(tSelection.iSelectedSequenceIndex),
                static_cast<_uint>(FrameInfo.iStepIndex),
                FrameInfo.fLocalTime
            );
        }

        m_pPreview_Level->Set_Playing(true);
        m_bSequenceRuntimeStarted = true;
    }

    m_pPreview_Level->Update_Animation(fTimeDelta);

    const ANIM_ROOT_MOTION_DELTA& Delta = m_pPreview_Level->Get_RootMotionDelta();
    m_pPreview_Level->Apply_RootMotionDeltaToTransform(Delta);

    _uint iRuntimeFrame = 0;
    if (Calc_SequenceGlobalFrameFromRuntime(iRuntimeFrame))
        Set_SequenceTimelineFromRuntime(iRuntimeFrame);

    if (m_pPreview_Level->Is_AnimationEnded() && !m_pPreview_Level->Is_SequenceMode())
    {
        m_pEditorContext->Set_Playing(false);
        m_pPreview_Level->Set_Playing(false);

        m_bSequenceRuntimeStarted = false;
        m_bSequenceWasPlaying = false;
        m_bSequencePreviewEnded = true;
        return;
    }

    m_bSequenceWasPlaying = true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 869~926행: Resolve_SequenceFrame
bool CPreview_Manager::Resolve_SequenceFrame(_uint iGlobalFrame, SEQUENCE_FRAME_INFO& OutInfo) const
{
    if (m_pEditorContext == nullptr)
        return false;

    const LOADED_DATA_CONTEXT& tLoadedData = m_pEditorContext->Get_LoadedData();
    const SELECTION_CONTEXT& tSelection = m_pEditorContext->Get_Selection();

    if (tSelection.iSelectedSequenceIndex < 0 || tSelection.iSelectedSequenceIndex >= tLoadedData.Sequences.size())
        return false;

    const ANIM_SEQUENCE& Sequence = tLoadedData.Sequences[tSelection.iSelectedSequenceIndex];

    _uint iAccumFrame = 0;

    for (_uint iStep = 0; iStep < Sequence.Steps.size(); iStep++)
    {
        const ANIM_SEQUENCE_STEP& Step = Sequence.Steps[iStep];

        if (Step.iClipIndex < 0 || Step.iClipIndex >= tLoadedData.ClipSettings.size())
            continue;

        const ANIM_CLIP_SETTING& Clip = tLoadedData.ClipSettings[Step.iClipIndex];

        if (Clip.iEndFrame < Clip.iStartFrame)
            continue;

        const _uint iClipFrameCount = Clip.iEndFrame - Clip.iStartFrame + 1;
        const _uint iStepStartFrame = iAccumFrame;
        const _uint iStepEndFrame = iAccumFrame + iClipFrameCount - 1;

        if (iGlobalFrame >= iStepStartFrame && iGlobalFrame <= iStepEndFrame)
        {
            const _uint iFrameOffset = iGlobalFrame - iStepStartFrame;

            OutInfo.iStepIndex = static_cast<_int>(iStep);
            OutInfo.iClipIndex = Step.iClipIndex;
            OutInfo.iStepStartFrame = iStepStartFrame;
            OutInfo.iStepEndFrame = iStepEndFrame;
            OutInfo.iLocalFrame = Clip.iStartFrame + iFrameOffset;

            OutInfo.fLocalTime = 0.f;

            if (Clip.iSourceFrameCount > 1)
            {
                const _float fFrameTime = Clip.fSourceDuration / static_cast<_float>(Clip.iSourceFrameCount - 1);

                OutInfo.fLocalTime = static_cast<_float>(iFrameOffset) * fFrameTime;
            }

            return true;
        }

        iAccumFrame += iClipFrameCount;
    }

    return false;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 985~995행: Resolve_SequenceFrame
        if (Resolve_SequenceFrame(tTimeline.iCurFrame, FrameInfo))
        {
            m_pPreview_Level->Set_ControllerEnabled(false);
            m_pPreview_Level->Reset_RootMotionPreviewTransform();

            m_pPreview_Level->Preview_Sequence_ByIndex(
                static_cast<_uint>(tSelection.iSelectedSequenceIndex),
                static_cast<_uint>(FrameInfo.iStepIndex),
                FrameInfo.fLocalTime
            );
        }

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1086~1149행: Calc_SequenceGlobalFrameFromRuntime
_bool CPreview_Manager::Calc_SequenceGlobalFrameFromRuntime(_uint& OutGlobalFrame) const
{
    if (m_pPreview_Level == nullptr || m_pEditorContext == nullptr)
        return false;

    const LOADED_DATA_CONTEXT& tLoadedData = m_pEditorContext->Get_LoadedData();
    const SELECTION_CONTEXT& tSelection = m_pEditorContext->Get_Selection();

    const _int iSequenceCount = static_cast<_int>(tLoadedData.Sequences.size());
    const _int iClipCount = static_cast<_int>(tLoadedData.ClipSettings.size());

    if (tSelection.iSelectedSequenceIndex < 0 || tSelection.iSelectedSequenceIndex >= iSequenceCount)
        return false;

    const ANIM_SEQUENCE& Sequence = tLoadedData.Sequences[tSelection.iSelectedSequenceIndex];

    const _int iCurStepIndex = m_pPreview_Level->Get_CurSequenceStepIndex();

    if (iCurStepIndex < 0 || iCurStepIndex >= static_cast<_int>(Sequence.Steps.size()))
        return false;

    _uint iGlobalFrame = 0;

    for (_int i = 0; i < iCurStepIndex; i++)
    {
        const ANIM_SEQUENCE_STEP& Step = Sequence.Steps[i];

        if (Step.iClipIndex < 0 || Step.iClipIndex >= iClipCount)
            continue;

        const ANIM_CLIP_SETTING& Clip = tLoadedData.ClipSettings[Step.iClipIndex];

        if (Clip.iEndFrame < Clip.iStartFrame)
            continue;

        iGlobalFrame += Clip.iEndFrame - Clip.iStartFrame + 1;
    }

    const ANIM_SEQUENCE_STEP& CurStep = Sequence.Steps[iCurStepIndex];

    if (CurStep.iClipIndex < 0 || CurStep.iClipIndex >= iClipCount)
        return false;

    const ANIM_CLIP_SETTING& Clip = tLoadedData.ClipSettings[CurStep.iClipIndex];

    _uint iLocalFrame = 0;

    if (Clip.iSourceFrameCount > 1 && Clip.fSourceDuration > 0.f)
    {
        const _float fFrameTime = Clip.fSourceDuration / static_cast<_float>(Clip.iSourceFrameCount - 1);

        if (fFrameTime > 0.f)
            iLocalFrame = static_cast<_uint>(m_pPreview_Level->Get_TrackPosition() / fFrameTime + 0.5f);
    }

    const _uint iLocalMaxFrame = Clip.iEndFrame >= Clip.iStartFrame ? Clip.iEndFrame - Clip.iStartFrame : 0;

    if (iLocalFrame > iLocalMaxFrame)
        iLocalFrame = iLocalMaxFrame;

    OutGlobalFrame = iGlobalFrame + iLocalFrame;

    return true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1060~1084행: Set_SequenceTimelineFromRuntime
void CPreview_Manager::Set_SequenceTimelineFromRuntime(_uint iGlobalFrame)
{
    const TIMELINE_CONTEXT& tTimeline = m_pEditorContext->Get_Timeline();

    if (tTimeline.iFrameCount == 0)
        return;

    const _uint iMaxFrame = tTimeline.iFrameCount - 1;

    if (iGlobalFrame > iMaxFrame)
        iGlobalFrame = iMaxFrame;

    m_pEditorContext->Set_CurFrame(iGlobalFrame);

    if (iMaxFrame > 0)
    {
        const _float fRatio = static_cast<_float>(iGlobalFrame) / static_cast<_float>(iMaxFrame);

        m_pEditorContext->Set_CurTime(tTimeline.fDuration * fRatio);
    }
    else
    {
        m_pEditorContext->Set_CurTime(0.f);
    }
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1047~1058행: Reset_SequencePreviewRuntime
void CPreview_Manager::Reset_SequencePreviewRuntime()
{
    m_iLastClipIndex = -1;
    m_fLastAppliedTime = 0.f;
    m_fSequenceFrameCursor = 0.f;
    m_bSequenceRuntimeStarted = false;
    m_bSequenceWasPlaying = false;
    m_bSequencePreviewEnded = false;

    if (m_pPreview_Level != nullptr)
        m_pPreview_Level->Reset_RootMotionPreviewTransform();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 797~803행: Request_SequenceEnd
void CPreview_Manager::Request_SequenceEnd()
{
    if (m_pPreview_Level == nullptr)
        return;

    m_pPreview_Level->Request_SequenceEnd();
}
