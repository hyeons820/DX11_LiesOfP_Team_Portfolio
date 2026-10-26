// 포트폴리오 코드 발췌: 생성·해제 요청 → 프레임 경계 반영 → 핸들 세대 관리 / 입력 소유권 Push·Pop 반영
// 원본: Engine/Private/UI_Manager.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 146~158행: Late_Update
void CUI_Manager::Late_Update(_float fTimeDelta)
{
	for (auto& Pair : m_UIObjects)
	{
		if (Pair.second == nullptr)
			continue;

		Pair.second->Late_Update(fTimeDelta);
	}

	Flush_UIInputOwnerCommands();
	Flush_PendingCommands();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 398~462행: Request_SpawnUIObject
HRESULT CUI_Manager::Request_SpawnUIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle)
{
	if (pOutHandle == nullptr)
		return E_FAIL;

	pOutHandle->Invalidate();

	if (strAssetID.empty() || strInstanceTag.empty())
		return E_FAIL;

	const UI_ASSET_DESC* pAssetDesc = m_UIAssetManager.Find_Asset(strAssetID);

	if (pAssetDesc == nullptr)
		return E_FAIL;

	// 이미 생성되었거나 생성 예약된 동일 태그 차단
	if (Find_UIObject(strInstanceTag) != nullptr)
		return E_FAIL;

	if (m_PendingSpawnInstanceTags.find(strInstanceTag) != m_PendingSpawnInstanceTags.end())
		return E_FAIL;

	const _bool bSingleton = Is_SingletonPolicy(pAssetDesc->eInstancePolicy);

	if (bSingleton)
	{
		auto iterSingleton = m_SingletonUIHandles.find(strAssetID);

		if (iterSingleton != m_SingletonUIHandles.end())
		{
			if (Is_ValidUIHandle(iterSingleton->second))
				return E_FAIL;

			m_SingletonUIHandles.erase(iterSingleton);
		}
	}

	UI_HANDLE Handle = Allocate_UIRuntimeSlot();

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	Slot.Instance.strAssetID = strAssetID;
	Slot.Instance.strInstanceTag = strInstanceTag;
	Slot.Instance.strLayer = pAssetDesc->strLayer;
	Slot.Instance.eInstancePolicy = pAssetDesc->eInstancePolicy;
	Slot.Instance.eLifetime = pAssetDesc->eLifetime;
	Slot.Instance.eState = UI_RUNTIME_STATE::PENDING_SPAWN;
	Slot.Instance.pUIObject = nullptr;
	Slot.Instance.pController = nullptr;

	m_PendingSpawnInstanceTags.emplace(strInstanceTag);

	if (bSingleton)
		m_SingletonUIHandles[strAssetID] = Handle;

	UI_RUNTIME_COMMAND Command = {};
	Command.eType = UI_RUNTIME_COMMAND_TYPE::SPAWN;
	Command.hUI = Handle;

	m_PendingUICommands.push_back(Command);

	*pOutHandle = Handle;

	return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1585~1615행: Allocate_UIRuntimeSlot
UI_HANDLE CUI_Manager::Allocate_UIRuntimeSlot()
{
	UI_HANDLE Handle = {};

	// 이전에 반환된 슬롯이 있으면 재사용
	if (!m_FreeUIRuntimeSlotIndices.empty())
	{
		const _uint iSlotIndex = m_FreeUIRuntimeSlotIndices.back();
		m_FreeUIRuntimeSlotIndices.pop_back();

		UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[iSlotIndex];

		Slot.bOccupied = true;

		Handle.iIndex = iSlotIndex;
		Handle.iGeneration = Slot.iGeneration;

		return Handle;
	}

	// 빈 슬롯이 없으면 새 슬롯 생성
	UI_RUNTIME_SLOT Slot = {};
	Slot.bOccupied = true;

	m_UIRuntimeSlots.push_back(Slot);

	Handle.iIndex = static_cast<_uint>(m_UIRuntimeSlots.size() - 1);
	Handle.iGeneration = Slot.iGeneration;

	return Handle;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 791~796행: Is_ValidUIHandle
	if (!Is_ValidUIHandle(Handle))
	{
		m_SingletonUIHandles.erase(iterSingleton);

		return Spawn_UIObject(strAssetID, strInstanceTag, pOutHandle);
	}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1641~1655행: Is_ValidUIHandle
_bool CUI_Manager::Is_ValidUIHandle(const UI_HANDLE& Handle) const
{
	if (!Handle.Is_Valid())
		return false;

	if (Handle.iIndex >= m_UIRuntimeSlots.size())
		return false;

	const UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (!Slot.bOccupied)
		return false;

	return Slot.iGeneration == Handle.iGeneration;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2168~2171행: Is_ValidUIHandle
	if (pButton == nullptr || !Is_ValidUIHandle(Handle))
	{
		return false;
	}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1786~1826행: Execute_SpawnUIObject
void CUI_Manager::Execute_SpawnUIObject(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	if (Slot.Instance.eState != UI_RUNTIME_STATE::PENDING_SPAWN)
		return;

	const string strAssetID = Slot.Instance.strAssetID;
	const _wstring strInstanceTag = Slot.Instance.strInstanceTag;
	const UI_ASSET_DESC* pAssetDesc = m_UIAssetManager.Find_Asset(strAssetID);

	if (pAssetDesc == nullptr)
	{
		Cancel_PendingSpawn(Handle);
		return;
	}

	if (FAILED(Create_UIObject(*pAssetDesc, strInstanceTag)))
	{
		Cancel_PendingSpawn(Handle);
		return;
	}

	CUIObject* pUIObject = Find_UIObject(strInstanceTag);

	if (pUIObject == nullptr)
	{
		Cancel_PendingSpawn(Handle);
		return;
	}

	Slot.Instance.pUIObject = pUIObject;
	Slot.Instance.eState = UI_RUNTIME_STATE::ACTIVE;

	pUIObject->Set_RenderSuppressed(Is_UILayerSuppressed(Slot.Instance.strLayer) || Is_UIInputOwnerRenderSuppressed(Handle));

	m_PendingSpawnInstanceTags.erase(strInstanceTag);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 348~372행: Request_ReleaseUIObject
void CUI_Manager::Request_ReleaseUIObject(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	// 이미 제거 예약된 핸들의 중복 요청 차단
	if (Slot.Instance.eState == UI_RUNTIME_STATE::PENDING_RELEASE)
		return;

	Slot.Instance.eState = UI_RUNTIME_STATE::PENDING_RELEASE;

	Request_RemoveUIInputOwner(Handle);

	// Flush 전까지 객체가 살아 있어도 화면 표시 X
	if (Slot.Instance.pUIObject != nullptr)
		Slot.Instance.pUIObject->Set_Visible(false);

	UI_RUNTIME_COMMAND Command = {};
	Command.eType = UI_RUNTIME_COMMAND_TYPE::RELEASE;
	Command.hUI = Handle;

	m_PendingUICommands.push_back(Command);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 375~395행: Request_ReleaseLifetime
void CUI_Manager::Request_ReleaseLifetime(UI_LIFETIME eLifetime)
{
	for (_uint iSlotIndex = 0; iSlotIndex < m_UIRuntimeSlots.size(); ++iSlotIndex)
	{
		UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[iSlotIndex];
		if (!Slot.bOccupied)
			continue;

		if (Slot.Instance.eState == UI_RUNTIME_STATE::PENDING_RELEASE)
			continue;

		if (Slot.Instance.eLifetime != eLifetime)
			continue;

		UI_HANDLE Handle = {};
		Handle.iIndex = iSlotIndex;
		Handle.iGeneration = Slot.iGeneration;

		Request_ReleaseUIObject(Handle);
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1853~1883행: Flush_PendingCommands
void CUI_Manager::Flush_PendingCommands()
{
	// 컨트롤러가 UIObject를 비소유로 참조하므로 먼저 제거
	for (const _wstring& strControllerTag : m_PendingRemoveUIControllers)
	{
		Remove_UIControllerImmediate(strControllerTag);
	}

	m_PendingRemoveUIControllers.clear();

	// 처리 중 새 Command가 추가돼도 현재 반복자가 깨지지 않게 교체
	vector<UI_RUNTIME_COMMAND> Commands;
	Commands.swap(m_PendingUICommands);

	for (const UI_RUNTIME_COMMAND& Command : Commands)
	{
		switch (Command.eType)
		{
		case UI_RUNTIME_COMMAND_TYPE::SPAWN:
			Execute_SpawnUIObject(Command.hUI);
			break;

		case UI_RUNTIME_COMMAND_TYPE::RELEASE:
			Remove_UIObjectImmediate(Command.hUI);
			break;

		default:
			break;
		}
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1707~1758행: Remove_UIObjectImmediate
void CUI_Manager::Remove_UIObjectImmediate(const UI_HANDLE& Handle)
{
	// 핸들 검사
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	const string strAssetID = Slot.Instance.strAssetID;
	const _wstring strInstanceTag = Slot.Instance.strInstanceTag;
	const _wstring strControllerTag = Slot.Instance.strControllerTag;
	
	// 연결된 컨트롤러 제거
	// 컨트롤러가 UIObject와 Element를 비소유로 참조하므로 먼저 제거
	if (!strControllerTag.empty())
	{
		Remove_UIControllerImmediate(strControllerTag);

		Slot.Instance.strControllerTag.clear();
		Slot.Instance.pController = nullptr;
	}

	m_PendingSpawnInstanceTags.erase(strInstanceTag);

	CUIObject* pExpectedObject = Slot.Instance.pUIObject;

	// 싱글톤 Registry 정리
	if (Is_SingletonPolicy(Slot.Instance.eInstancePolicy))
	{
		auto iterSingleton = m_SingletonUIHandles.find(strAssetID);

		if (iterSingleton != m_SingletonUIHandles.end() && iterSingleton->second == Handle)
		{
			m_SingletonUIHandles.erase(iterSingleton);
		}
	}

	// UIObject 제거
	auto iter = m_UIObjects.find(strInstanceTag);

	// 같은 태그가 재사용되었더라도 같은 인스턴스인지 확인
	if (iter != m_UIObjects.end() && iter->second == pExpectedObject)
	{
		Safe_Release(iter->second);
		m_UIObjects.erase(iter);
	}

	Remove_UIInputOwnerImmediate(Handle);

	// UIObject 제거가 끝난 뒤 슬롯 반환
	Release_UIRuntimeSlot(Handle);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1618~1637행: Release_UIRuntimeSlot
void CUI_Manager::Release_UIRuntimeSlot(const UI_HANDLE& Handle)
{
	if (!Is_ValidUIHandle(Handle))
		return;

	UI_RUNTIME_SLOT& Slot = m_UIRuntimeSlots[Handle.iIndex];

	// 다음 인스턴스를 위해 기존 런타임 정보 초기화
	Slot.Instance = {};
	Slot.bOccupied = false;

	// 기존 핸들을 무효화하기 위해 세대 증가
	++Slot.iGeneration;

	// 오버플로로 0이 되면 유효 세대인 1로 변경
	if (Slot.iGeneration == 0)
		Slot.iGeneration = 1;

	m_FreeUIRuntimeSlotIndices.push_back(Handle.iIndex);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 465~493행: Request_PushUIInputOwner
HRESULT CUI_Manager::Request_PushUIInputOwner(const UI_HANDLE& Handle, UI_INPUT_OWNER_KIND eKind)
{
	if (UI_INPUT_OWNER_KIND::END == eKind)
		return E_FAIL;

	if (!Is_OpenUIObject(Handle))
		return E_FAIL;

	if (Is_UIInputOwner(Handle))
		return E_FAIL;

	if (Has_PendingUIInputOwnerCommand(Handle))
		return E_FAIL;

	if (UI_INPUT_OWNER_KIND::SCREEN == eKind)
	{
		if (Has_UIInputOwnerKind(UI_INPUT_OWNER_KIND::SCREEN) || Has_PendingUIInputOwnerPushKind(UI_INPUT_OWNER_KIND::SCREEN))
			return E_FAIL;
	}

	UI_INPUT_OWNER_COMMAND Command = {};
	Command.eType = UI_INPUT_OWNER_COMMAND_TYPE::PUSH;
	Command.hUI = Handle;
	Command.eKind = eKind;

	m_PendingUIInputOwnerCommands.push_back(Command);

	return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 496~514행: Request_PopUIInputOwner
HRESULT CUI_Manager::Request_PopUIInputOwner(const UI_HANDLE& Handle)
{
	if (m_UIInputOwnerStack.empty())
		return E_FAIL;

	if (!(m_UIInputOwnerStack.back().hUI == Handle))
		return E_FAIL;

	if (Has_PendingUIInputOwnerCommand(Handle))
		return E_FAIL;

	UI_INPUT_OWNER_COMMAND Command = {};
	Command.eType = UI_INPUT_OWNER_COMMAND_TYPE::POP;
	Command.hUI = Handle;

	m_PendingUIInputOwnerCommands.push_back(Command);

	return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1969~1982행: Request_RemoveUIInputOwner
void CUI_Manager::Request_RemoveUIInputOwner(const UI_HANDLE& Handle)
{
	m_PendingUIInputOwnerCommands.erase(remove_if(m_PendingUIInputOwnerCommands.begin(), m_PendingUIInputOwnerCommands.end(), 
		[&Handle](const UI_INPUT_OWNER_COMMAND& Command) 
		{ 
			return Command.hUI == Handle; 
		}), m_PendingUIInputOwnerCommands.end());

	UI_INPUT_OWNER_COMMAND Command = {};
	Command.eType = UI_INPUT_OWNER_COMMAND_TYPE::REMOVE;
	Command.hUI = Handle;

	m_PendingUIInputOwnerCommands.push_back(Command);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1995~2064행: Flush_UIInputOwnerCommands
void CUI_Manager::Flush_UIInputOwnerCommands()
{
	if (true == m_PendingUIInputOwnerCommands.empty())
		return;

	vector<UI_INPUT_OWNER_COMMAND> Commands;
	Commands.swap(m_PendingUIInputOwnerCommands);

	_bool bOwnerChanged = false;

	for (const UI_INPUT_OWNER_COMMAND& Command : Commands)
	{
		switch (Command.eType)
		{
		case UI_INPUT_OWNER_COMMAND_TYPE::PUSH:
		{
			if (!Is_OpenUIObject(Command.hUI))
				break;

			if (Is_UIInputOwner(Command.hUI))
				break;

			if (Command.eKind == UI_INPUT_OWNER_KIND::SCREEN && Has_UIInputOwnerKind(UI_INPUT_OWNER_KIND::SCREEN))
				break;

			UI_INPUT_OWNER_ENTRY Entry = {};
			Entry.hUI = Command.hUI;
			Entry.eKind = Command.eKind;

			m_UIInputOwnerStack.push_back(Entry);
			bOwnerChanged = true;
			break;
		}

		case UI_INPUT_OWNER_COMMAND_TYPE::POP:
		{
			if (m_UIInputOwnerStack.empty())
				break;

			if (!(m_UIInputOwnerStack.back().hUI == Command.hUI))
				break;

			m_UIInputOwnerStack.pop_back();
			bOwnerChanged = true;
			break;
		}

		case UI_INPUT_OWNER_COMMAND_TYPE::REMOVE:
		{
			const size_t iPreviousSize = m_UIInputOwnerStack.size();
			Remove_UIInputOwnerImmediate(Command.hUI);

			if (iPreviousSize != m_UIInputOwnerStack.size())
				bOwnerChanged = true;

			break;
		}
		}
	}

	if (bOwnerChanged)
	{
		// 최상단 Input Owner가 바뀌었으므로 기존 버튼의 포커스와 캡처를 제거한다.
		Clear_ButtonFocus();
		Reset_ButtonPointerInput();

		// Push 또는 Pop 결과를 같은 프레임의 렌더링에 즉시 반영한다.
		Refresh_UIRenderSuppression();
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1542~1560행: Make_UIRuntimeElementTag
_wstring CUI_Manager::Make_UIRuntimeElementTag(const UI_ELEMENT_ASSET_DESC& ElementDesc, const _wstring& strBindingPrefix) const
{
	_wstring strLocalTag;

	if (!ElementDesc.strBindingKey.empty())
	{
		strLocalTag = StringToWString(ElementDesc.strBindingKey);
	}
	else
	{
		// BindingKey가 없는 장식 엘리먼트는 ID 기반 내부 키 사용
		strLocalTag = L"__Element_" + to_wstring(ElementDesc.iElementID);
	}

	if (strBindingPrefix.empty())
		return strLocalTag;

	return strBindingPrefix + L"." + strLocalTag;
}
