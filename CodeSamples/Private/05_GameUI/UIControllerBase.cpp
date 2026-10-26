// 포트폴리오 코드 발췌: 컨트롤러 초기화 → ViewModel 갱신 → 화면 반영 / BindingKey 조회·좌표 변환
// 원본: Engine/Private/UIControllerBase.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 11~22행: Initialize
HRESULT CUIControllerBase::Initialize(void* pArg)
{
    UI_CONTROLLER_DESC* pDesc = static_cast<UI_CONTROLLER_DESC*>(pArg);

    if (pDesc != nullptr)
        m_hUI = pDesc->hUI;

    if (FAILED(Bind_UI()))
        return E_FAIL;

    return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 36~44행: Update
void CUIControllerBase::Update(_float fTimeDelta)
{
    // 핸들을 사용하는 일반 컨트롤러는 인스턴스 제거 후 더 이상 캐시 포인터에 접근하지 않는다
    if (m_hUI.Is_Valid() && !m_pGameInstance->Is_AliveUIObject(m_hUI))
        return;

    Update_ViewModel(fTimeDelta);
    Apply_ViewModel();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 59~65행: Find_Element
CUIElement* CUIControllerBase::Find_Element(const _wstring& strElementTag)
{
    if (m_pUIObject == nullptr)
        return nullptr;

    return m_pUIObject->Find_Element(strElementTag);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 67~97행: WorldToScreen
_bool CUIControllerBase::WorldToScreen(_fvector vWorldPosition, _float2& vOutScreenPosition)
{
    const _float4x4* pView = m_pGameInstance->Get_Transform(D3DTS::VIEW);
    const _float4x4* pProj = m_pGameInstance->Get_Transform(D3DTS::PROJ);

    if (pView == nullptr || pProj == nullptr)
        return false;

    const _matrix matView = XMLoadFloat4x4(pView);
    const _matrix matProj = XMLoadFloat4x4(pProj);

    _vector vView = XMVector3TransformCoord(vWorldPosition, matView);
    _vector vClip = XMVector3TransformCoord(vView, matProj);

    const _float fNdcX = XMVectorGetX(vClip);
    const _float fNdcY = XMVectorGetY(vClip);
    const _float fNdcZ = XMVectorGetZ(vClip);

    if (fNdcZ < 0.f || fNdcZ > 1.f)
        return false;

    if (fNdcX < -1.2f || fNdcX > 1.2f || fNdcY < -1.2f || fNdcY > 1.2f)
        return false;

    const _float2 vViewportSize = m_pGameInstance->Get_ViewportDesc();

    vOutScreenPosition.x = fNdcX * vViewportSize.x * 0.5f;
    vOutScreenPosition.y = fNdcY * vViewportSize.y * 0.5f;

    return true;
}
