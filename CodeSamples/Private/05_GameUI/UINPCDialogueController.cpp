// 포트폴리오 코드 발췌: 대화 시작 이벤트 → 문장 진행·선택 → 서비스 요청 → 종료 애니메이션·입력 반환
// 원본: GameCore/Private/UINPCDialogueController.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 85~123행: OnEvent
void CUINPCDialogueController::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE_MAP_OBJECT::NPC_TALK_START)
        return;

    if (pData == nullptr || m_pViewModel == nullptr)
        return;

    const CNPC::NPC_DIALOGUE_EVENT_DESC* pDesc = static_cast<const CNPC::NPC_DIALOGUE_EVENT_DESC*>(pData);
    
    // 유효한 NPC와 DialogueID가 포함된 시작 이벤트만 처리한다.
    if (pDesc->iTargetSpawnID == UINT_MAX || pDesc->eNPCType == NPC_TYPE::END || pDesc->strDialogueID.empty())
        return;


    const NPC_DIALOGUE_SCRIPT_DESC* pScript = m_DialogueRepository.Find_Script(pDesc->strDialogueID);

    // 현재 다른 NPC와 대화 중이면 중복 시작 이벤트를 무시한다. 
    if (m_pViewModel->Is_Visible() || pScript == nullptr)
    {
        CNPC::NPC_DIALOGUE_EVENT_DESC EndDesc{};
        EndDesc.iTargetSpawnID = pDesc->iTargetSpawnID;
        EndDesc.eNPCType = pDesc->eNPCType;

        m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_TALK_END, &EndDesc);
        return;
    }

    m_iTargetSpawnID = pDesc->iTargetSpawnID;
    m_eTargetNPCType = pDesc->eNPCType;

    m_pCurrentScript = pScript;
    m_iCurrentLineIndex = 0;

    m_iFocusedServiceSlot = -1;

    // Repository의 첫 번째 실제 대사 줄을 화면에 전달한다.
    Apply_CurrentDialogueLine();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 306~334행: Handle_PrimaryAction
void CUINPCDialogueController::Handle_PrimaryAction()
{
    if (m_pViewModel == nullptr)
        return;

    const NPC_DIALOGUE_VIEW_MODE eMode = m_pViewModel->Get_ViewMode();

    if (eMode == NPC_DIALOGUE_VIEW_MODE::DIALOGUE)
    {
        Advance_Dialogue();
        return;
    }

    if (eMode == NPC_DIALOGUE_VIEW_MODE::SERVICE_SELECTION)
    {
        const _uint iServiceCount = m_pViewModel->Get_ServiceCount();

        if (iServiceCount == 0)
            return;

        _int iSlotIndex = m_iFocusedServiceSlot;

        // 아무 항목에도 호버하지 않았다면 가장 위 항목을 선택한다.
        if (iSlotIndex < 0)
            iSlotIndex = static_cast<_int>(iServiceCount) - 1;

        Select_ServiceSlot(static_cast<_uint>(iSlotIndex));
    }
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 407~435행: Apply_CurrentDialogueLine
void CUINPCDialogueController::Apply_CurrentDialogueLine()
{
    if (m_pViewModel == nullptr || m_pCurrentScript == nullptr)
        return;

    if (m_iCurrentLineIndex >= m_pCurrentScript->DialogueLines.size())
    {
        End_Dialogue();
        return;
    }

    const NPC_DIALOGUE_LINE_DESC& Line = m_pCurrentScript->DialogueLines[m_iCurrentLineIndex];

    // 첫 줄은 대화창을 시작하고, 이후 줄은 열린 ViewModel 내용만 변경한다.
    if (!m_pViewModel->Is_Visible())
        m_pViewModel->Begin_Dialogue(Line.strSpeakerName, Line.strDialogueText);
    else
        m_pViewModel->Set_DialogueLine(Line.strSpeakerName, Line.strDialogueText);

    if (Line.iEventIndex >= 0)
    {
        CNPC::NPC_DIALOGUE_EVENT_DESC EventDesc{};
        EventDesc.iTargetSpawnID = m_iTargetSpawnID;
        EventDesc.eNPCType = m_eTargetNPCType;
        EventDesc.iEventIndex = Line.iEventIndex;

        m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_LINE_EVENT, &EventDesc);
    }
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 438~464행: Advance_Dialogue
void CUINPCDialogueController::Advance_Dialogue()
{
    if (m_pCurrentScript == nullptr)
    {
        End_Dialogue();
        return;
    }

    const size_t iLineCount = m_pCurrentScript->DialogueLines.size();

    if (m_iCurrentLineIndex + 1 < iLineCount)
    {
        // 아직 다음 대사가 남아 있으므로 인덱스를 증가시키고 화면을 갱신한다.
        ++m_iCurrentLineIndex;
        Apply_CurrentDialogueLine();
        return;
    }

    // 마지막 대사 뒤에 서비스가 있으면 선택지를 표시하고, 없으면 대화를 종료한다.
    if (!m_pCurrentScript->ServiceOptions.empty())
    {
        Enter_ServiceSelection();
        return;
    }

    End_Dialogue();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 467~485행: Enter_ServiceSelection
void CUINPCDialogueController::Enter_ServiceSelection()
{
    if (m_pViewModel == nullptr || m_pCurrentScript == nullptr || m_pCurrentScript->ServiceOptions.empty())
    {
        End_Dialogue();
        return;
    }

    vector<_wstring> ServiceLabels;
    ServiceLabels.reserve(m_pCurrentScript->ServiceOptions.size());

    for (const NPC_DIALOGUE_SERVICE_OPTION_DESC& Option : m_pCurrentScript->ServiceOptions)
        ServiceLabels.push_back(Option.strLabel);

    m_pViewModel->Show_ServiceOptions(ServiceLabels);

    // UI 배치상 가장 위에 표시되는 슬롯을 기본 선택 대상으로 사용한다.
    m_iFocusedServiceSlot = static_cast<_int>(ServiceLabels.size()) - 1;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 362~380행: Request_DialogueService
void CUINPCDialogueController::Request_DialogueService(NPC_DIALOGUE_SERVICE_TYPE eServiceType)
{
    if (m_pGameInstance == nullptr)
        return;

    if (m_iTargetSpawnID == UINT_MAX || m_eTargetNPCType == NPC_TYPE::END)
        return;

    if (eServiceType == NPC_DIALOGUE_SERVICE_TYPE::NONE || eServiceType == NPC_DIALOGUE_SERVICE_TYPE::END)
        return;

    NPC_DIALOGUE_SERVICE_REQUEST_DESC EventDesc{};
    EventDesc.iTargetSpawnID = m_iTargetSpawnID;
    EventDesc.eNPCType = m_eTargetNPCType;
    EventDesc.eServiceType = eServiceType;

    // EventManager의 동기 Broadcast를 이용해 해당 서비스 컨트롤러에 실행을 요청한다.
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_DIALOGUE_SERVICE_REQUEST, &EventDesc);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 382~404행: End_Dialogue
void CUINPCDialogueController::End_Dialogue()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible())
        return;

    CNPC::NPC_DIALOGUE_EVENT_DESC EventDesc{};

    EventDesc.iTargetSpawnID = m_iTargetSpawnID;
    EventDesc.eNPCType = m_eTargetNPCType;

    m_pViewModel->Close_Dialogue();

    m_iTargetSpawnID = UINT_MAX;
    m_eTargetNPCType = NPC_TYPE::END;

    m_pCurrentScript = nullptr;
    m_iCurrentLineIndex = 0;

    m_iFocusedServiceSlot = -1;

    // 해당 NPC가 Talking 상태를 종료하도록 알린다.
    m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_TALK_END, &EventDesc);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 501~516행: Begin_CloseAnimation
void CUINPCDialogueController::Begin_CloseAnimation()
{
    if (m_bCloseAnimationPending || !m_pGameInstance->Is_OpenUIObject(m_hUI))
        return;

    // Close 클립이 있으면 UI를 유지한 채 알파 애니메이션을 재생합니다.
    if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("Close"))
    {
        m_bCloseAnimationPending = true;
        m_pUIObject->Play_Animation("Close", true);
        return;
    }

    // Close 클립이 없으면 즉시 닫습니다.
    Finalize_Close();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 518~540행: Finalize_Close
void CUINPCDialogueController::Finalize_Close()
{
    if (m_pGameInstance->Is_OpenUIObject(m_hUI))
    {
        // Close 애니메이션이 끝난 후 Input Owner와 실제 UI를 제거합니다.
        if (FAILED(m_pGameInstance->Request_PopUIInputOwner(m_hUI)))
            return;

        if (FAILED(m_pGameInstance->Close_UIObject(m_hUI)))
            return;
    }

    if (m_bHUDLayerSuppressed)
    {
        m_pGameInstance->Release_UILayerSuppression("HUD");
        m_bHUDLayerSuppressed = false;
    }

    m_bCloseAnimationPending = false;

    if (m_pViewModel != nullptr)
        m_pViewModel->Clear_Dirty();
}
