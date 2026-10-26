// 포트폴리오 코드 발췌: 에셋 로드·검증·저장 / 편집 시작·종료 → 이력 저장 → Undo/Redo 복원
// 원본: UI_Editor/Private/UI_EditorContext.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 789~844행: Load_Asset
HRESULT CUI_EditorContext::Load_Asset(const _wstring& strFilePath)
{
    if (strFilePath.empty())
        return E_FAIL;

    UI_ASSET_DESC LoadedAsset = {};
    _wstring strError;

    if (FAILED(CUIAssetSerializer::Load(strFilePath, &LoadedAsset, &strError)))
    {
        if (!strError.empty())
        {
            const _wstring strLog = L"[UI Asset Load Error] " + strError + L"\n";

            OutputDebugStringW(strLog.c_str());
        }

        return E_FAIL;
    }

    UI_ASSET_VALIDATION_RESULT Validation = {};

    // 파싱에는 성공했지만 데이터 규칙이 잘못된 에셋도 나중에 에디터에서 열어 고칠 수 있도록 결과를 따로 보관
    CUIAssetValidator::Validate(LoadedAsset, &Validation);

    m_tAsset = move(LoadedAsset);
    m_tAssetValidation = move(Validation);

    // 파일에서 저장된 최대 엘리먼트 ID 다음 값부터 발급
    Rebuild_NextAssetElementID();

    m_strAssetFilePath = strFilePath;

    m_bAssetLoaded = true;
    m_bAssetDirty = false;

    // 이전 파일의 엘리먼트 선택 ID가 새 파일에 남지 않도록 초기화
    Select_Asset();

    // 이전 파일의 Undo/Redo 기록 제거
    Clear_AssetHistory();
    Reset_PreviewContext();

    m_bAssetLoaded = true;
    m_bAssetDirty = false;

    m_tAssetElementClipboard = {};

    Validate_Asset();

    // S_FALSE는 파일은 정상적으로 열린 것으로 처리, 대신 Validation 패널에서 오류를 표시
    if (m_tAssetValidation.Has_Error())
        return S_FALSE;

    return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 847~878행: Save_Asset
HRESULT CUI_EditorContext::Save_Asset(const _wstring& strFilePath)
{
    if (!m_bAssetLoaded || strFilePath.empty())
        return E_FAIL;

    // 오류가 있는 에셋은 런타임 파일로 저장하지 않음
    if (FAILED(Validate_Asset()))
        return E_FAIL;

    _wstring strError;

    if (FAILED(CUIAssetSerializer::Save(strFilePath, m_tAsset, &strError)))
    {
        if (!strError.empty())
        {
            const _wstring strLog = L"[UI Asset Save Error] " + strError + L"\n";
            
            OutputDebugStringW(strLog.c_str());
        }

        return E_FAIL;
    }

    m_strAssetFilePath = strFilePath;

    // 현재 상태를 마지막 저장 지점으로 기록
    m_iSavedAssetRevision = m_iCurAssetRevision;

    m_bAssetDirty = false;

    return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 881~898행: Validate_Asset
HRESULT CUI_EditorContext::Validate_Asset()
{
    m_tAssetValidation.Issues.clear();

    if (!m_bAssetLoaded)
        return E_FAIL;

    //  기본 검증
    CUIAssetValidator::Validate(m_tAsset, &m_tAssetValidation);

    // 에디터 Catalog가 필요한 파일 간 참조 검증
    if (m_bUIAssetCatalogReady)
    {
        Validate_ComponentReferences(m_tAsset.Root, L"Root");
    }

    return m_tAssetValidation.Has_Error() ? E_FAIL : S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 901~910행: Begin_AssetEdit
void CUI_EditorContext::Begin_AssetEdit()
{
    if (!m_bAssetLoaded || m_bAssetEditActive)
        return;

    // 실제 값이 바뀌기 전 상태를 보관
    m_tAssetEditStartState = Capture_AssetHistoryState();

    m_bAssetEditActive = true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 913~925행: Begin_AssetEdit
void CUI_EditorContext::Begin_AssetEdit(const UI_ASSET_DESC& BeforeAsset)
{
    if (!m_bAssetLoaded || m_bAssetEditActive)
        return;

    // 선택 상태와 현재 Revision은 기존 방식으로 촬영
    m_tAssetEditStartState = Capture_AssetHistoryState();

    // 에셋 데이터만 실제 변경 전 복사본으로 교체
    m_tAssetEditStartState.Asset = BeforeAsset;

    m_bAssetEditActive = true; 
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 928~950행: End_AssetEdit
void CUI_EditorContext::End_AssetEdit()
{
    if (!m_bAssetEditActive)
        return;

    m_bAssetEditActive = false;

    // 클릭만 하고 값이 변하지 않은 경우 History를 만들지 않는다.
    const json BeforJson = CUIAssetSerializer::ToJson(m_tAssetEditStartState.Asset);
    const json CurJson = CUIAssetSerializer::ToJson(m_tAsset);

    if (BeforJson == CurJson)
        return;

    Push_AssetHistory(m_UndoAssetHistory, m_tAssetEditStartState);

    // Undo 후 새로운 편집을 했다면 과거의 Redo 경로는 폐기
    m_RedoAssetHistory.clear();

    m_iCurAssetRevision = m_iNextAssetRevision++;

    m_bAssetDirty = m_iCurAssetRevision != m_iSavedAssetRevision;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 952~969행: Undo_Asset
_bool CUI_EditorContext::Undo_Asset()
{
    // 진행 중인 편집이 있으면 먼저 한 단계로 확정
    End_AssetEdit();

    if (!Can_UndoAsset())
        return false;

    Push_AssetHistory(m_RedoAssetHistory, Capture_AssetHistoryState());

    const UI_EDITOR_ASSET_HISTORY_STATE State = m_UndoAssetHistory.back();

    m_UndoAssetHistory.pop_back();

    Restore_AssetHistoryState(State);

    return true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 971~987행: Redo_Asset
_bool CUI_EditorContext::Redo_Asset()
{
    End_AssetEdit();

    if (!Can_RedoAsset())
        return false;

    Push_AssetHistory(m_UndoAssetHistory, Capture_AssetHistoryState());

    const UI_EDITOR_ASSET_HISTORY_STATE State = m_RedoAssetHistory.back();

    m_RedoAssetHistory.pop_back();

    Restore_AssetHistoryState(State);

    return true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1560~1571행: Capture_AssetHistoryState
UI_EDITOR_ASSET_HISTORY_STATE CUI_EditorContext::Capture_AssetHistoryState() const
{
    UI_EDITOR_ASSET_HISTORY_STATE State = {};

    State.Asset = m_tAsset;
    State.AssetSelection = m_tAssetSelection;
    State.ProgressSlotSelection = m_tAssetProgressSlotSelection;

    State.iRevision = m_iCurAssetRevision;

    return State;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1574~1589행: Restore_AssetHistoryState
void CUI_EditorContext::Restore_AssetHistoryState(const UI_EDITOR_ASSET_HISTORY_STATE& State)
{
    m_tAsset = State.Asset;
    m_tAssetSelection = State.AssetSelection;
    m_tAssetProgressSlotSelection = State.ProgressSlotSelection;

    m_iCurAssetRevision = State.iRevision;

    // 복원된 Tree를 기준으로 다음 ID를 다시 계산
    Rebuild_NextAssetElementID();

    Validate_Asset();

    // 현재 Revision이 마지막 저장 Revision과 다르면 미저장 상태
    m_bAssetDirty = m_iCurAssetRevision != m_iSavedAssetRevision;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1592~1601행: Push_AssetHistory
void CUI_EditorContext::Push_AssetHistory(vector<UI_EDITOR_ASSET_HISTORY_STATE>& History, const UI_EDITOR_ASSET_HISTORY_STATE& State)
{
    if (History.size() >= ASSET_HISTORY_LIMIT)
    {
        // 가장 오래된 기록부터 제거
        History.erase(History.begin());
    }

    History.push_back(State);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1248~1252행: Notify_AssetChanged
void CUI_EditorContext::Notify_AssetChanged()
{
    Mark_AssetDirty();
    Validate_Asset();
}
