// 포트폴리오 코드 발췌: 화면·카테고리·선택 상태 변경 → 표시 데이터 저장 → Dirty 표시
// 원본: GameCore/Private/EquipmentViewModel.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 3~19행: Open_Equipment
void CEquipmentViewModel::Open_Equipment()
{
    if (m_bVisible)
        return;

    // 장비창을 다시 열 때는 항상 첫 번째 장비 카테고리부터 보여준다.
    m_bVisible = true;
    m_eCategory = EQUIPMENT_CATEGORY::EQUIPMENT;
    m_Detail = {};

    m_bCostumeSelectVisible = false;
    m_eCostumeCategory = COSTUME_CATEGORY::BODY;
    m_iSelectedCostumeSlot = -1;

    Clear_CostumeSlots();
    Make_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 21~32행: Close_Equipment
void CEquipmentViewModel::Close_Equipment()
{
    if (!m_bVisible)
        return;

    m_bVisible = false;
    m_bCostumeSelectVisible = false;
    m_iSelectedCostumeSlot = -1;
    m_Detail = {};

    Make_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 34~45행: Set_Category
void CEquipmentViewModel::Set_Category(EQUIPMENT_CATEGORY eCategory)
{
    if (eCategory == EQUIPMENT_CATEGORY::END || m_eCategory == eCategory)
        return;

    m_eCategory = eCategory;
    
    // 페이지가 바뀌면 이전 페이지 슬롯의 상세 설명을 제거한다.
    m_Detail = {};

    Make_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 47~56행: Move_Category
void CEquipmentViewModel::Move_Category(_int iDirection)
{
    const _int iCategoryCount = static_cast<_int>(EQUIPMENT_CATEGORY::END);
    _int iCategoryIndex = static_cast<_int>(m_eCategory);

    // 음수 방향도 정상적으로 순환할 수 있도록 카테고리 개수를 더한다.
    iCategoryIndex = (iCategoryIndex + iDirection + iCategoryCount) % iCategoryCount;

    Set_Category(static_cast<EQUIPMENT_CATEGORY>(iCategoryIndex));
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 58~69행: Set_ItemDetail
void CEquipmentViewModel::Set_ItemDetail(const _wstring& strTitle, const _wstring& strCategory, const _wstring& strDescription, const string& strIconTexturePath)
{
    m_Detail = {};

    m_Detail.eMode = EQUIPMENT_DETAIL_MODE::ITEM;
    m_Detail.strTitle = strTitle;
    m_Detail.strCategory = strCategory;
    m_Detail.strDescription = strDescription;
    m_Detail.strIconTexturePath = strIconTexturePath;

    Make_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 122~133행: Open_CostumeSelect
void CEquipmentViewModel::Open_CostumeSelect(COSTUME_CATEGORY eCategory)
{
    if (eCategory == COSTUME_CATEGORY::END)
        return;

    m_bCostumeSelectVisible = true;
    m_eCostumeCategory = eCategory;
    m_iSelectedCostumeSlot = -1;

    Clear_CostumeSlots();
    Make_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 135~145행: Close_CostumeSelect
void CEquipmentViewModel::Close_CostumeSelect()
{
    if (!m_bCostumeSelectVisible)
        return;

    m_bCostumeSelectVisible = false;
    m_iSelectedCostumeSlot = -1;

    Clear_CostumeSlots();
    Make_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 169~176행: Clear_CostumeSlots
void CEquipmentViewModel::Clear_CostumeSlots()
{
    for (COSTUME_SLOT_DATA& SlotData : m_CostumeSlots)
        SlotData = {};

    m_iSelectedCostumeSlot = -1;
    Make_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 178~185행: Set_CostumeSlotData
void CEquipmentViewModel::Set_CostumeSlotData(_uint iIndex, const COSTUME_SLOT_DATA& Data)
{
    if (iIndex >= 3)
        return;

    m_CostumeSlots[iIndex] = Data;
    Make_Dirty();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 187~200행: Select_CostumeSlot
void CEquipmentViewModel::Select_CostumeSlot(_int iIndex)
{
    if (iIndex < 0 || iIndex >= 3)
        return;

    if (!m_CostumeSlots[iIndex].bVisible)
        return;

    if (m_iSelectedCostumeSlot == iIndex)
        return;

    m_iSelectedCostumeSlot = iIndex;
    Make_Dirty();
}
