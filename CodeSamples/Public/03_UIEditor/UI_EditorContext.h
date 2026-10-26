#pragma once
#include "UIEditor_Defines.h"
#include "UIAssetValidator.h"
#include "Base.h"
#include "imgui.h"

#ifdef _DEBUG

NS_BEGIN(UI_Editor)

// 현재 인스펙터에서 선택한 ProgressBar 슬롯, 에디터 선택 상태이므로 UI 에셋에는 저장 X
typedef struct tagUIEditorAssetProgressSlotSelection
{
	_uint iElementID = UINT_MAX;
	_uint iSlotIndex = UINT_MAX;
}UI_ASSET_PROGRESS_SLOT_SELECTION;

enum class UI_EDITOR_ASSET_SELECTION_TYPE
{
	NONE,
	ASSET,
	ELEMENT,
};

typedef struct tagUIEditorAssetSelection
{
	UI_EDITOR_ASSET_SELECTION_TYPE	eType = UI_EDITOR_ASSET_SELECTION_TYPE::NONE;

	// 루트도 일반 엘리먼트와 동일하게 ID로 선택
	_uint							iElementID = UINT_MAX;
}UI_EDITOR_ASSET_SELECTION;

// 엘리먼트와 그 아래의 전체 자식 Tree를 보관하는 Clipboard
typedef struct tagUIAssetElementClipboard
{
	_bool                   bHasElement = { false };

	// 복사 직후 붙여넣을 때 원본의 형제로 넣기 위한 ID
	_uint                   iSourceElementID = { UINT_MAX };

	UI_ELEMENT_ASSET_DESC   Element;
} UI_ASSET_ELEMENT_CLIPBOARD;

// Undo/Redo 한 단계에서 복원할 UI 에셋 상태
typedef struct tagUIEditorAssetHistoryState
{
	UI_ASSET_DESC						Asset;
	UI_EDITOR_ASSET_SELECTION			AssetSelection;
	UI_ASSET_PROGRESS_SLOT_SELECTION	ProgressSlotSelection;

	// 저장 상태와 현재 상태를 구분하는 고유 번호
	_uint64								iRevision = { 0 };
}UI_EDITOR_ASSET_HISTORY_STATE;


// 하이어라키에서 드롭한 위치에 따른 삽입 방식
enum class UI_ASSET_HIERARCHY_DROP_MODE
{
	INTO,   // Target Container의 자식
	BEFORE, // Target의 바로 앞 형제
	AFTER   // Target의 바로 뒤 형제
};

// Button의 배타적인 입력 상태 미리보기
enum class UI_EDITOR_BUTTON_PREVIEW_STATE
{
	NORMAL,
	HOVERED,
	PRESSED,
	DISABLED,
};

// UI 에셋에는 저장하지 않는 에디터 전용 미리보기 상태
typedef struct tagUIEditorPreviewContext
{
	// Screen Layout
	_bool							bShowSafeArea = { true };
	_float							fSafeAreaRatio = { 0.9f };

	// Reusable Component
	_bool							bShowGrid = { true };
	_bool							bShowLocalOrigin = { true };

	// Projected Preview, -1 ~ 1 범위의 가상 화면 위치
	_float2							vMockScreenPosition = { 0.f, 0.f };

	_float							fMockDistance = { 5.f };

	// 이후  Binding Preview에서 사용할 가짜 게임 데이터
	_float							fMockHP = { 1.f };

	_bool							bMockGroggy = { false };
	_bool							bMockFatal = { false };
	_bool							bMockVisible = { true };

	// Button Preview
	UI_EDITOR_BUTTON_PREVIEW_STATE	eButtonState = UI_EDITOR_BUTTON_PREVIEW_STATE::NORMAL;

	// Selected와 Focused는 Hovered/Pressed와 동시에 적용 가능
	_bool	bButtonSelected = { false };
	_bool	bButtonFocused = { false };
}UI_EDITOR_PREVIEW_CONTEXT;

// JSON에 저장하지 않는 애니메이션 에디터의 현재 작업 상태
typedef struct tagUIEditorAnimationState
{
	_int	iSelectedClipIndex = { -1 };		// 현재 Animation 패널에서 선택한 Clip
	_int	iSelectedTrackIndex = { -1 };		// 다음 단계에서 사용할 선택 Track
	_int	iSelectedKeyframeIndex = { -1 };	// 다음 단계에서 사용할 선택 Keyframe

	_float	fCurrentTime = { 0.f };			// 현재 미리보기 재생 시간
	_bool	bPlaying = { false };			// Animation 패널 재생 여부
} UI_EDITOR_ANIMATION_STATE;

// 에디터가 UI 폴더에서 찾은 v2 UI 에셋 정보
struct UI_ASSET_CATALOG_ENTRY
{
	UI_ASSET_DESC	Asset;
	_wstring		strFilePath;

	// 같은 에셋ID를 가진 파일이 둘 이상 존재하는지 표시
	_bool			bDuplicateAssetID = { false };
};

class CUI_EditorContext final : public CBase
{
	DECLARE_SINGLETON(CUI_EditorContext)

private:
	CUI_EditorContext() = default;
	virtual ~CUI_EditorContext() = default;

public:
	UI_EDITOR_MODE							Get_Mode() const { return m_eMode; }
	
	// 현재 편집 중인 Schema v2 에셋
	UI_ASSET_DESC&							Get_Asset() { return m_tAsset; }							
	const UI_ASSET_DESC&					Get_Asset() const { return m_tAsset; }
	// 마지막 Asset 검증 결과
	const UI_ASSET_VALIDATION_RESULT&		Get_AssetValidation() const { return m_tAssetValidation; }	
	// 현재 에셋이 로드되거나 저장된 파일 경로
	const _wstring&							Get_AssetFilePath() const { return m_strAssetFilePath; }

	// v2 에셋/엘리먼트 선택 상태
	const UI_EDITOR_ASSET_SELECTION&		Get_AssetSelection() const { return m_tAssetSelection; }
	UI_ELEMENT_ASSET_DESC*					Get_SelectedAssetElement();
	const UI_ELEMENT_ASSET_DESC*			Get_SelectedAssetElement() const;

	const UI_ASSET_PROGRESS_SLOT_SELECTION& Get_AssetProgressSlotSelection() const { return m_tAssetProgressSlotSelection; }

	UI_PROGRESS_SLOT_ASSET_DESC*			Get_SelectedAssetProgressSlot();
	const UI_PROGRESS_SLOT_ASSET_DESC*		Get_SelectedAssetProgressSlot() const;

	UI_EDITOR_PREVIEW_CONTEXT&				Get_PreviewContext() { return m_tPreviewContext; }
	const UI_EDITOR_PREVIEW_CONTEXT&		Get_PreviewContext() const { return m_tPreviewContext; }

	UI_EDITOR_ANIMATION_STATE&				Get_AnimationState() { return m_tAnimationState; }
	const UI_EDITOR_ANIMATION_STATE&		Get_AnimationState() const { return m_tAnimationState; }

	void									Select_AssetProgressSlot(_uint iElementID, _uint iSlotIndex);

	void									Clear_AssetProgressSlotSelection();
	void									Reset_PreviewContext();

public:
	void									Set_Mode(UI_EDITOR_MODE eMode) { m_eMode = eMode; }
	// 현재 에셋에 저장되지 않은 변경이 생겼음을 기록
	void									Mark_AssetDirty() { m_bAssetDirty = true; }

public:
	_bool									Can_CopyAssetElement() const;
	_bool									Can_PasteAssetElement() const;

	_bool									Can_UndoAsset() const { return !m_UndoAssetHistory.empty(); }
	_bool									Can_RedoAsset() const { return !m_RedoAssetHistory.empty(); }

	// 편집할 v2 에셋이 준비되어 있는지 확인
	_bool									Is_AssetLoaded() const { return m_bAssetLoaded; }
	// 현재 에셋에 저장되지 않은 변경이 있는지 확인
	_bool									Is_AssetDirty() const { return m_bAssetDirty; }
	// 엘리먼트가 아니라 에셋 메타데이터가 선택되었는지 확인
	_bool									Is_AssetSelected() const { return m_tAssetSelection.eType == UI_EDITOR_ASSET_SELECTION_TYPE::ASSET; }

public:
	_uint									Add_AssetElement(UI_ELEMENT_TYPE eType);
	_int									Add_AssetProgressSlot(_uint iElementID, UI_ASSET_PROGRESS_SLOT_TYPE eType);

public:
	UI_ELEMENT_ASSET_DESC*					Find_AssetElement(_uint iElementID);
	const UI_ELEMENT_ASSET_DESC*			Find_AssetElement(_uint iElementID) const;

	UI_ELEMENT_ASSET_DESC*					Find_AssetParentElement(_uint iElementID);

public:	
	void									Select_Asset();
	void									Select_AssetElement(_uint iElementID);
	void									Clear_AssetSelection();

public:
	_bool									Remove_AssetElement(_uint iElementID);
	_bool									Remove_AssetProgressSlot(_uint iElementID, _uint iSlotIndex);

public:
	HRESULT									New_Asset(UI_ASSET_KIND eKind);
	HRESULT									Load_Asset(const _wstring& strFilePath);
	HRESULT									Save_Asset(const _wstring& strFilePath);
	HRESULT									Validate_Asset();

	void									Begin_AssetEdit();
	void									Begin_AssetEdit(const UI_ASSET_DESC& BeforeAsset);
	void									End_AssetEdit();

	_bool									Undo_Asset();
	_bool									Redo_Asset();

	HRESULT									Copy_SelectedAssetElement();
	_uint									Paste_AssetElement();

	_bool									Move_AssetElement(_uint iSourceElementID, _uint iTargetElementID, UI_ASSET_HIERARCHY_DROP_MODE eDropMode);

	void									Clear_AssetHistory();

	void									Notify_AssetChanged();

public:
	HRESULT									Refresh_UIAssetCatalog(const _wstring& strRootPath);

	const UI_ASSET_CATALOG_ENTRY*			Find_UIAssetCatalogEntry(const string& strAssetID) const;
	const vector< UI_ASSET_CATALOG_ENTRY>&	Get_UIAssetCatalog() const { return m_UIAssetCatalog; }

	_bool									Is_UIAssetCatalogReady() const { return m_bUIAssetCatalogReady; }

	_bool									Request_OpenReferencedComponent(const string& strComponentAssetID);
	_bool									Consume_AssetOpenRequest(_wstring& strOutFilePath);

public:
	void									Request_EditorPreviewFont(const string& strFontAssetID);
	void									Process_EditorPreviewFontRequests();
	ImFont*									Find_EditorPreviewFont(const string& strFontAssetID) const;

public:
	HRESULT									Initialize();

private:
	UI_ELEMENT_ASSET_DESC*					Resolve_AssetInsertParent();
	void									Rebuild_NextAssetElementID();

private:
	UI_ELEMENT_ASSET_DESC*					Find_AssetElementRecursive(UI_ELEMENT_ASSET_DESC& Element, _uint iElementID);
	const UI_ELEMENT_ASSET_DESC*			Find_AssetElementRecursive(const UI_ELEMENT_ASSET_DESC& Element, _uint iElementID) const;

	void									Find_MaxAssetElementID(const UI_ELEMENT_ASSET_DESC& Element, _uint& iMaxElementID) const;
	UI_ELEMENT_ASSET_DESC*					Find_AssetParentElementRecursive(UI_ELEMENT_ASSET_DESC& Parent, _uint iElementID);

private:
	UI_EDITOR_ASSET_HISTORY_STATE			Capture_AssetHistoryState() const;

	void									Restore_AssetHistoryState(const UI_EDITOR_ASSET_HISTORY_STATE& State);
	void									Push_AssetHistory(vector< UI_EDITOR_ASSET_HISTORY_STATE>& History, const UI_EDITOR_ASSET_HISTORY_STATE& State);

	void									Prepare_PastedAssetElement(UI_ELEMENT_ASSET_DESC& Element);

private:
	void									Validate_ComponentReferences(const UI_ELEMENT_ASSET_DESC& Element, const _wstring& strPath);

	_bool									Has_ComponentReferenceCycle(const string& strAssetID, unordered_set<string>& VisitingAssetIDs, unordered_set<string>& CompletedAssetIDs, vector<string>& ReferenceChain, string* pOutCyclePath) const;
	_bool									Has_ComponentReferenceCycleInTree(const UI_ELEMENT_ASSET_DESC& Element, unordered_set<string>& VisitingAssetIDs, unordered_set<string>& CompletedAssetIDs, vector<string>& ReferenceChain, string* pOutCyclePath) const;

private:
	UI_EDITOR_MODE							m_eMode = UI_EDITOR_MODE::EDIT;

private:
	// 버전 2

	UI_ASSET_DESC							m_tAsset;
	UI_ASSET_VALIDATION_RESULT				m_tAssetValidation;
	UI_EDITOR_ASSET_SELECTION				m_tAssetSelection;
	
	_wstring								m_strAssetFilePath;

	_bool									m_bAssetLoaded = { false };
	_bool									m_bAssetDirty = { false };

	_uint									m_iNextAssetElementID = { 1 };	// 새 v2 엘리먼트에 발급할 다음 고유 ID

	// 인스펙터에서 현재 선택한 ProgressBar 슬롯
	UI_ASSET_PROGRESS_SLOT_SELECTION		m_tAssetProgressSlotSelection;

	UI_ASSET_ELEMENT_CLIPBOARD				m_tAssetElementClipboard;

	// JSON과 Undo History에 들어가지 않는 에디터 미리보기 데이터
	UI_EDITOR_PREVIEW_CONTEXT				m_tPreviewContext;

private:
	static constexpr size_t					ASSET_HISTORY_LIMIT = 100;

	vector<UI_EDITOR_ASSET_HISTORY_STATE>	m_UndoAssetHistory;
	vector<UI_EDITOR_ASSET_HISTORY_STATE>	m_RedoAssetHistory;

	UI_EDITOR_ASSET_HISTORY_STATE			m_tAssetEditStartState;

	_bool									m_bAssetEditActive = { false };

	_uint64									m_iCurAssetRevision = { 0 };
	_uint64									m_iSavedAssetRevision = { 0 };
	_uint64									m_iNextAssetRevision = { 1 };

private:
	// JSON과 Undo/Redo에는 저장하지 않는 에디터 검색 캐시
	vector<UI_ASSET_CATALOG_ENTRY>			m_UIAssetCatalog;

	_bool									m_bUIAssetCatalogReady = { false };

	_wstring								m_strPendingAssetOpenPath;

private:
	// FontAssetID -> ImGui 미리보기 폰트
	// ImFont의 소유권은 ImGui FontAtlas에 있으므로 직접 delete하지 않는다.
	unordered_map<string, ImFont*>			m_EditorPreviewFonts;

	// 현재 프레임 중 요청되어 다음 BeginFrame 전에 로드할 폰트 ID
	unordered_set<string>					m_PendingEditorPreviewFontIDs;

private:
	// JSON과 Undo History에는 들어가지 않는 Animation 패널 작업 상태
	UI_EDITOR_ANIMATION_STATE				m_tAnimationState;

public:
	virtual void Free() override;
};

NS_END

#endif
