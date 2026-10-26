// 포트폴리오 코드 발췌: 열기 이벤트·UI 명령 → 인벤토리와 선택 데이터 → 장착 → ViewModel 반영·미리보기 → 닫기
// 원본: GameCore/Private/UIEquipmentController.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 157~184행: OnEvent
void CUIEquipmentController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
    UNREFERENCED_PARAMETER(pData);

    if (eType != EVENT_TYPE_UI::EQUIPMENT_OPEN_REQUEST)
        return;

    if (m_pViewModel == nullptr || m_pViewModel->Is_Visible())
        return;

    // ESC Screen 위에 장비창을 Modal Input Owner로 올린다.
    if (!m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN))
        return;
    if (FAILED(Bind_PlayerInventory()))
        return;

    m_pViewModel->Set_CurrentErgo(m_pInventory->Get_Ergo());

    m_bCloseAfterCameraRestore = false;
    m_bWasTopInputOwner = false;

    m_pViewModel->Open_Equipment();


    Refresh_MainEquipmentSlots();
    Refresh_MainCostumeSlotIcons();
    Select_DefaultSlotForCurrentCategory();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 38~155행: On_UICommand
void CUIEquipmentController::On_UICommand(const UI_COMMAND_EVENT& Event)
{
    if (m_pViewModel == nullptr || Event.hUI != m_hUI)
        return;

    if (!m_pGameInstance->Is_TopUIInputOwner(m_hUI))
        return;

    if (m_pViewModel->Is_CostumeSelectVisible())
    {
        if (Event.strCommandKey == "PrevCostumeCategory")
        {
            m_pViewModel->Move_CostumeCategory(1);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "NextCostumeCategory")
        {
            m_pViewModel->Move_CostumeCategory(-1);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "AccessoriesCategory")
        {
            m_pViewModel->Set_CostumeCategory(COSTUME_CATEGORY::ACCESSORY);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "MaskCategory")
        {
            m_pViewModel->Set_CostumeCategory(COSTUME_CATEGORY::MASK);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "CostumeCategory")
        {
            m_pViewModel->Set_CostumeCategory(COSTUME_CATEGORY::BODY);
            Refresh_CostumeSlots();
        }
        else if (Event.strCommandKey == "SelectCostumeSlot")
        {
            if (Event.strRuntimeElementTag == L"CostumeSelectPage.CostumeSlot0.SlotButton")
                Select_CostumeSlot(0);
            else if (Event.strRuntimeElementTag == L"CostumeSelectPage.CostumeSlot1.SlotButton")
                Select_CostumeSlot(1);
            else if (Event.strRuntimeElementTag == L"CostumeSelectPage.CostumeSlot2.SlotButton")
                Select_CostumeSlot(2);
        }
        else if (Event.strCommandKey == "ActivateInputHint")
        {
            if (Event.strRuntimeElementTag == L"CostumeSelectPage.BackHint.HintButton")
                Close_CostumeSelect();
            else if (Event.strRuntimeElementTag == L"CostumeSelectPage.EquipHint.HintButtonSpace")
                Equip_SelectedCostume();
            else if (Event.strRuntimeElementTag == L"CostumeSelectPage.UnequipHint.HintButton")
                Unequip_SelectedCostume();
        }

        return;
    }

    if (Event.strCommandKey == "ShowEquipmentPage")
    {
        m_pViewModel->Set_Category(EQUIPMENT_CATEGORY::EQUIPMENT);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "ShowUseItemPage")
    {
        m_pViewModel->Set_Category(EQUIPMENT_CATEGORY::USE_ITEM);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "ShowCostumePage")
    {
        m_pViewModel->Set_Category(EQUIPMENT_CATEGORY::COSTUME);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "PrevEquipmentCategory")
    {
        m_pViewModel->Move_Category(-1);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "NextEquipmentCategory")
    {
        m_pViewModel->Move_Category(1);
        Select_DefaultSlotForCurrentCategory();
    }
    else if (Event.strCommandKey == "ActivateInputHint")
    {
        if (Event.strRuntimeElementTag == L"CloseHint.HintButton" || Event.strRuntimeElementTag == L"CompactCloseHint.HintButton")
            Close_Equipment();
    }
    else if (Event.strCommandKey == "EquipmentItemSlot")
    {
        if (Event.strRuntimeElementTag == L"WeaponSlotPrimary.SlotButton" ||
            Event.strRuntimeElementTag == L"WeaponSlotSecondary.SlotButton" ||
            Event.strRuntimeElementTag == L"UpperBeltSlot1.SlotButton" ||
            Event.strRuntimeElementTag == L"LowerBeltSlot1.SlotButton" ||
            Event.strRuntimeElementTag == L"LowerBeltSlot2.SlotButton" ||
            Event.strRuntimeElementTag == L"PouchSlot1.SlotButton" ||
            Event.strRuntimeElementTag == L"PouchSlot3.SlotButton")
        {
            Select_MainDetailSlot(Event.strRuntimeElementTag);
        }
        else if (Event.strRuntimeElementTag == L"LegionArmSlot.SlotButton")
        {
            // 리전 암 메인 슬롯은 선택 시 기존 리전 암 교체 화면을 연다.
            m_pGameInstance->Broadcast(EVENT_TYPE_UI::LEGION_ARM_CHANGE_OPEN_REQUEST, nullptr);
        }
        else if (Event.strRuntimeElementTag == L"AccessoryCostumeSlot.SlotButton")
        {
            Open_CostumeSelect(COSTUME_CATEGORY::ACCESSORY);
        }
        else if (Event.strRuntimeElementTag == L"HeadCostumeSlot.SlotButton")
        {
            Open_CostumeSelect(COSTUME_CATEGORY::MASK);
        }
        else if (Event.strRuntimeElementTag == L"BodyCostumeSlot.SlotButton")
        {
            Open_CostumeSelect(COSTUME_CATEGORY::BODY);
        }
    }
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 299~372행: Update_ViewModel
void CUIEquipmentController::Update_ViewModel(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible())
    {
        m_bPreviewDragging = false;
        return;
    }

    if (m_bCloseAfterCameraRestore)
    {
        m_bCloseAfterCameraRestore = false;
        m_pViewModel->Close_Equipment();
        return;
    }

    const _bool bIsTopInputOwner = m_pGameInstance->Is_TopUIInputOwner(m_hUI);

    if (!bIsTopInputOwner)
    {
        // 리전 암 교체 등 하위 Modal이 열린 동안에는 다음 복귀를 기다린다.
        m_bWasTopInputOwner = false;
        m_bPreviewDragging = false;
        return;
    }

    if (!m_bWasTopInputOwner)
    {
        // 하위 Modal이 닫힌 뒤 장비창으로 돌아온 첫 프레임에 실제 장착 정보를 다시 읽는다.
        Refresh_MainEquipmentSlots();
        Refresh_MainCostumeSlotIcons();
        m_bWasTopInputOwner = true;
    }

    if (m_bCostumeSelectOpening && m_pUIObject->Is_AnimationFinished())
    {
        // 기존 CostumePage의 퇴장이 끝나면 렌더 대상에서 제외합니다.
        m_pPages[static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME)]->Set_Visible(false);
        m_bCostumeSelectOpening = false;
    }

    Update_PreviewRotation();

    if (m_pViewModel->Is_CostumeSelectVisible())
    {
        if (m_pGameInstance->Key_Down(DIK_Q))
        {
            // 이전 코스튬 카테고리로 이동한다.
            m_pViewModel->Move_CostumeCategory(1);
            Refresh_CostumeSlots();
        }
        else if (m_pGameInstance->Key_Down(DIK_E))
        {
            // 다음 코스튬 카테고리로 이동한다.
            m_pViewModel->Move_CostumeCategory(-1);
            Refresh_CostumeSlots();
        }
        else if (m_pGameInstance->Key_Down(DIK_ESCAPE))
            Close_CostumeSelect();
        else if (m_pGameInstance->Key_Down(DIK_SPACE))
            Equip_SelectedCostume();
        else if (m_pGameInstance->Key_Down(DIK_R))
            Unequip_SelectedCostume();

        return;
    }

    if (m_pGameInstance->Key_Down(DIK_ESCAPE))
    {
        Close_Equipment();
        m_bWasTopInputOwner = false;
    }
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 374~513행: Apply_ViewModel
void CUIEquipmentController::Apply_ViewModel()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
        return;

    const _bool bShouldOpen = m_pViewModel->Is_Visible();
    const _bool bActuallyOpen = m_pGameInstance->Is_OpenUIObject(m_hUI);

    if (bShouldOpen)
    {
        if (!m_bHUDLayerSuppressed)
        {
            m_pGameInstance->Acquire_UILayerSuppression("HUD");
            m_bHUDLayerSuppressed = true;
        }

        if (!bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
            {
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                m_pViewModel->Close_Equipment();
                return;
            }

            if (FAILED(m_pGameInstance->Request_PushUIInputOwner(m_hUI, UI_INPUT_OWNER_KIND::MODAL_FULLSCREEN)))
            {
                m_pGameInstance->Close_UIObject(m_hUI);
                m_pGameInstance->Release_UILayerSuppression("HUD");
                m_bHUDLayerSuppressed = false;
                m_pViewModel->Close_Equipment();
                return;
            }

            // UI와 Input Owner가 모두 열린 뒤 프리뷰를 활성화
            Begin_EquipmentPreview();
        }
    }
    else
    {
        if (bActuallyOpen)
        {
            if (FAILED(m_pGameInstance->Request_PopUIInputOwner(m_hUI)))
                return;

            if (FAILED(m_pGameInstance->Close_UIObject(m_hUI)))
                return;
        }

        // 장비창이 닫힌 뒤 프리뷰 캐릭터와 카메라를 함께 종료
        End_EquipmentPreview();

        if (m_bHUDLayerSuppressed)
        {
            m_pGameInstance->Release_UILayerSuppression("HUD");
            m_bHUDLayerSuppressed = false;
        }

        m_pViewModel->Clear_Dirty();
        return;
    }

    Apply_ErgoProfile();
    const _uint iCurrentCategory = static_cast<_uint>(m_pViewModel->Get_Category());

    const _bool bCostumeSelectVisible = m_pViewModel->Is_CostumeSelectVisible();

    for (_uint iIndex = 0; iIndex < 3; ++iIndex)
    {
        const _bool bKeepCostumePageForAnimation = m_bCostumeSelectOpening && iIndex == static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME);
        const _bool bSelected = (!bCostumeSelectVisible && iIndex == iCurrentCategory) || bKeepCostumePageForAnimation;

        m_pPages[iIndex]->Set_Visible(bSelected);
        m_pCategoryButtons[iIndex]->Set_Selected(iIndex == iCurrentCategory);
    }

    Apply_CostumeSelectViewModel();

    if (bCostumeSelectVisible)
    {
        m_pDetailPanel->Set_Visible(false);
        m_pInputHintActionGroup->Set_Visible(false);
        m_pInputHintCompactGroup->Set_Visible(false);

        m_pViewModel->Clear_Dirty();
        return;
    }

    const EQUIPMENT_DETAIL_DATA& Detail = m_pViewModel->Get_Detail();

    const _bool bShowItem = Detail.eMode == EQUIPMENT_DETAIL_MODE::ITEM;
    const _bool bShowAssembledWeapon = Detail.eMode == EQUIPMENT_DETAIL_MODE::WEAPON_ASSEMBLED;
    const _bool bShowSingleWeapon = Detail.eMode == EQUIPMENT_DETAIL_MODE::WEAPON_SINGLE;
    const _bool bShowWeapon = bShowAssembledWeapon || bShowSingleWeapon;
    const _bool bShowDetail = bShowItem || bShowWeapon;

    // 상세 모드에 따라 각 이미지·텍스트 그룹을 배타적으로 표시한다.
    m_pDetailPanel->Set_Visible(bShowDetail);
    m_pDetailIcon->Set_Visible(bShowItem);
    m_pDetailText->Set_Visible(bShowItem);
    m_pWeaponType1->Set_Visible(bShowAssembledWeapon);
    m_pWeaponType2->Set_Visible(bShowSingleWeapon);
    m_pWeaponTypeDetailText->Set_Visible(bShowWeapon);

    m_pInputHintActionGroup->Set_Visible(bShowDetail);
    m_pInputHintCompactGroup->Set_Visible(!bShowDetail);

    if (bShowItem)
    {
        m_pDetailTitle->Set_Text(Detail.strTitle);
        m_pDetailCategory->Set_Text(Detail.strCategory);
        m_pDetailBodyText->Set_Text(Detail.strDescription);

        if (FAILED(Set_ImageTexture(L"DetailIcon", Detail.strIconTexturePath)))
            return;
    }
    else if (bShowWeapon)
    {
        m_pDetailTitle->Set_Text(Detail.strTitle);
        m_pWeaponTitleText->Set_Text(Detail.strWeaponTitle);
        m_pWeaponBodyText->Set_Text(Detail.strDescription);

        if (bShowAssembledWeapon)
        {
            if (FAILED(Set_ImageTexture(L"DetailIcon_Weapon", Detail.strIconTexturePath)))
                return;

            if (FAILED(Set_ImageTexture(L"DetailIcon_Grip", Detail.strSubIconTexturePath)))
                return;
        }
        else
        {
            if (FAILED(Set_ImageTexture(L"DetailIcon_Weapon2", Detail.strIconTexturePath)))
                return;
        }
    }

    m_pViewModel->Clear_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 528~541행: Bind_PlayerInventory
HRESULT CUIEquipmentController::Bind_PlayerInventory()
{
    CPlayer* pPlayer = dynamic_cast<CPlayer*>(m_pGameInstance->Get_Player());

    if (pPlayer == nullptr)
        return E_FAIL;

    m_pInventory = pPlayer->Get_Inventory();

    if (m_pInventory == nullptr)
        return E_FAIL;

    return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 605~626행: Open_CostumeSelect
void CUIEquipmentController::Open_CostumeSelect(COSTUME_CATEGORY eCategory)
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr || m_bCostumeSelectOpening)
        return;

    m_pViewModel->Open_CostumeSelect(eCategory);
    Refresh_CostumeSlots();

    // 선택창과 기존 CostumePage를 함께 표시한 상태로 기존 페이지를 퇴장시킵니다.
    m_pCostumeSelectPage->Set_Visible(true);
    m_pPages[static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME)]->Set_Visible(true);

    if (m_pUIObject->Has_Animation("CostumeSelectPageOpen"))
    {
        m_bCostumeSelectOpening = true;
        m_pUIObject->Play_Animation("CostumeSelectPageOpen", true);
    }
    else
    {
        m_pPages[static_cast<_uint>(EQUIPMENT_CATEGORY::COSTUME)]->Set_Visible(false);
    }
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 644~696행: Refresh_CostumeSlots
void CUIEquipmentController::Refresh_CostumeSlots()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    vector<_uint64> ItemIDs;

    if (FAILED(m_pInventory->Get_ItemIDs(ITEM_TYPE::BODY_PART, ItemIDs)))
        return;

    const COSTUME_CATEGORY eCategory = m_pViewModel->Get_CostumeCategory();
    const PLAYER_PART eTargetPart = Convert_ToPlayerPart(eCategory);

    if (eTargetPart == PLAYER_PART::END)
        return;

    const _uint64 iEquippedID = m_pInventory->Get_EquippedPartID(eTargetPart);

    m_pViewModel->Clear_CostumeSlots();

    _uint iSlotIndex = 0;

    for (const _uint64 iItemID : ItemIDs)
    {
        const ITEM_DATA* pItemData = CItemTable::Find(iItemID);

        if (pItemData == nullptr || pItemData->eType != ITEM_TYPE::BODY_PART)
            continue;

        if (pItemData->BodyPartData.ePartSlot != eTargetPart)
            continue;

        if (iSlotIndex >= 3)
            break;

        COSTUME_SLOT_DATA SlotData{};

        SlotData.iItemID = iItemID;
        SlotData.bVisible = true;
        SlotData.bEquipped = iItemID == iEquippedID;
        SlotData.strName = pItemData->strDisplayName;
        SlotData.strCategory = Get_CostumeCategoryText(eCategory);
        SlotData.strDescription = pItemData->strDesc;
        SlotData.strIconTexturePath = Resolve_CostumeIconPath(iItemID);

        m_pViewModel->Set_CostumeSlotData(iSlotIndex, SlotData);
        ++iSlotIndex;
    }

    // 현재 카테고리에 보유 코스튬이 하나 이상 있다면 첫 슬롯을 기본 선택한다.
    if (iSlotIndex > 0)
        m_pViewModel->Select_CostumeSlot(0);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 730~736행: Select_CostumeSlot
void CUIEquipmentController::Select_CostumeSlot(_uint iSlotIndex)
{
    if (m_pViewModel == nullptr)
        return;

    m_pViewModel->Select_CostumeSlot(static_cast<_int>(iSlotIndex));
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 738~763행: Equip_SelectedCostume
void CUIEquipmentController::Equip_SelectedCostume()
{
    if (m_pViewModel == nullptr || m_pInventory == nullptr)
        return;

    const _int iSelectedSlot = m_pViewModel->Get_SelectedCostumeSlot();

    if (iSelectedSlot < 0)
        return;

    const COSTUME_SLOT_DATA& SlotData = m_pViewModel->Get_CostumeSlotData(static_cast<_uint>(iSelectedSlot));

    if (!SlotData.bVisible || SlotData.iItemID == 0 || SlotData.bEquipped)
        return;

    // Inventory::Equip에서 실제 플레이어의 CPartSet까지 즉시 적용한다.
    if (!m_pInventory->Equip(SlotData.iItemID))
        return;

    // 실제 장착에 성공한 경우에만 프리뷰에도 동일한 코스튬을 적용한다.
    if (m_pPreviewCharacter != nullptr)
        m_pPreviewCharacter->Equip_PreviewCostume(SlotData.iItemID);

    Refresh_CostumeSlots();
    Refresh_MainCostumeSlotIcons();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 907~926행: Begin_EquipmentPreview
void CUIEquipmentController::Begin_EquipmentPreview()
{
    if ((m_pPreviewCharacter == nullptr || m_pPreviewBackground == nullptr) && FAILED(Bind_PreviewObjects()))
        return;

    // 프리뷰 캐릭터가 바라보는 높이를 기준으로 카메라를 설정
    const _vector vPreviewEye = XMVectorSet(0.f, -999.f, -5.5f, 1.f);
    const _vector vPreviewAt = XMVectorSet(0.f, -999.f, 0.f, 1.f);
    const _float fPreviewFovy = XMConvertToRadians(22.f);

    // 프리뷰용 View/Projection을 먼저 바인딩할 수 있도록 카메라 모드를 활성화한다.
    m_pGameInstance->Begin_UIPreviewCamera(vPreviewEye, vPreviewAt, fPreviewFovy);

    Sync_PreviewCostumes();

    m_pPreviewBackground->Set_PreviewActive(true);
    m_pPreviewCharacter->Set_PreviewWeaponSlot(0);
    m_pPreviewCharacter->Apply_EquipmentPreset();
    m_pPreviewCharacter->Set_PreviewActive(true);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 929~942행: End_EquipmentPreview
void CUIEquipmentController::End_EquipmentPreview()
{
    if (m_pPreviewBackground != nullptr)
        m_pPreviewBackground->Set_PreviewActive(false);

    if (m_pPreviewCharacter != nullptr)
    {
        m_pPreviewCharacter->Clear_StatRoomPreset();
        m_pPreviewCharacter->Set_PreviewActive(false);
    }

    // 비정상 종료나 외부 종료 경로에서도 프리뷰 카메라가 남지 않도록 보장한다.
    m_pGameInstance->End_UIPreviewCamera();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1019~1040행: Sync_PreviewCostumes
void CUIEquipmentController::Sync_PreviewCostumes()
{
    if (m_pInventory == nullptr || m_pPreviewCharacter == nullptr)
        return;

    const _uint64 iBodyID = m_pInventory->Get_EquippedPartID(PLAYER_PART::BODY);
    const _uint64 iHelmetID = m_pInventory->Get_EquippedPartID(PLAYER_PART::HELMET);
    const _uint64 iAccessoryID = m_pInventory->Get_EquippedPartID(PLAYER_PART::ACC);

    // 의상은 비어 있으면 프리뷰의 기본 의상으로 복구한다.
    m_pPreviewCharacter->Equip_PreviewCostume(iBodyID != 0 ? iBodyID : ItemID::BODY_BASE);

    if (iHelmetID != 0)
        m_pPreviewCharacter->Equip_PreviewCostume(iHelmetID);
    else
        m_pPreviewCharacter->Unequip_PreviewCostume(PLAYER_PART::HELMET);

    if (iAccessoryID != 0)
        m_pPreviewCharacter->Equip_PreviewCostume(iAccessoryID);
    else
        m_pPreviewCharacter->Unequip_PreviewCostume(PLAYER_PART::ACC);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 515~526행: Close_Equipment
void CUIEquipmentController::Close_Equipment()
{
    if (m_pViewModel == nullptr || !m_pViewModel->Is_Visible() || m_bCloseAfterCameraRestore)
        return;

    // 현재 UI 프레임에서는 장비 UI와 프리뷰 객체를 유지한 채, 다음 CameraManager::Late_Update부터 게임 카메라를 사용하도록 요청
    m_pGameInstance->End_UIPreviewCamera();

    // 다음 프레임의 카메라/파이프라인 갱신이 끝난 후 실제 장비창을 닫는다.
    m_bCloseAfterCameraRestore = true;
    m_bPreviewDragging = false;
}
