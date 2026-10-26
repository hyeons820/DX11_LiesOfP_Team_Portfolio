// 포트폴리오 코드 발췌: 에셋 검증 진입 → 요소 계층 재귀·BindingKey 검사 → 애니메이션 트랙 검사
// 원본: Engine/Private/UIAssetValidator.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 561~607행: Validate
HRESULT CUIAssetValidator::Validate(const UI_ASSET_DESC& Asset, UI_ASSET_VALIDATION_RESULT* pOutResult)
{
    if (pOutResult == nullptr)
        return E_FAIL;

    pOutResult->Issues.clear();

    VALIDATION_CONTEXT Context = {};
    Context.pResult = pOutResult;

    if (Asset.iSchemaVersion != UI_SCHEMA_VERSION)
        Add_Error(Context, L"SchemaVersion", L"지원하지 않는 UI 스키마 버전입니다.");

    if (Asset.strAssetID.empty())
        Add_Error(Context, L"Asset.ID", L"Asset ID는 비어 있을 수 없습니다.");

    if (Asset.strName.empty())
        Add_Warning(Context, L"Asset.Name", L"Asset 표시 이름이 비어 있습니다.");

    if (Asset.strLayer.empty())
        Add_Error(Context, L"Asset.Layer", L"Asset Layer는 비어 있을 수 없습니다.");

    if (!Is_Finite(Asset.vReferenceResolution) || Asset.vReferenceResolution.x <= 0.f || Asset.vReferenceResolution.y <= 0.f)
        Add_Error(Context, L"Canvas.ReferenceResolution", L"기준 해상도는 유한한 양수여야 합니다.");

    if (!Is_Finite(Asset.fMatch) || Asset.fMatch < 0.f || Asset.fMatch > 1.f)
        Add_Error(Context, L"Canvas.Match", L"Canvas Match는 0과 1 사이여야 합니다.");

    if (Asset.iSortOrder > UINT_MAX / UI_ELEMENT_SORT_RANGE)
    {
        Add_Error(Context, L"Asset.SortOrder", L"Asset SortOrder가 런타임 정렬 범위를 초과합니다.");
    }

    if (Asset.eKind == UI_ASSET_KIND::PROJECTED)
    {
        if (!Is_Finite(Asset.Projected.vScreenOffset))
            Add_Error(Context, L"Projected.ScreenOffset", L"화면 오프셋은 유한한 값이어야 합니다.");

        if (Asset.Projected.strAnchorSource.empty())
            Add_Warning(Context, L"Projected.AnchorSource", L"Projected UI의 AnchorSource가 비어 있습니다.");
    }

    Validate_Element(Asset.Root, Context, L"Root");
    Validate_Animations(Asset, Context);

    return pOutResult->Has_Error() ? E_FAIL : S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 438~557행: Validate_Element
    void Validate_Element(const UI_ELEMENT_ASSET_DESC& Element, VALIDATION_CONTEXT& Context, const _wstring& strPath)
    {
        // Element ID는 에셋 전체에서 고유해야 함
        const _bool bIDInserted = Context.ElementIDs.insert(Element.iElementID).second;

        if (!bIDInserted)
            Add_Error(Context, strPath + L".ID", L"중복된 Element ID입니다.");

        if (Element.strName.empty())
            Add_Warning(Context, strPath + L".Name", L"Element 표시 이름이 비어 있습니다.");

        if (Element.Render.iSortOrder >= UI_ELEMENT_LOCAL_SORT_RANGE)
        {
            Add_Error(Context, strPath + L".Render.SortOrder", L"Element SortOrder는 0 이상 999 이하여야 합니다.");
        }

        if (!Element.strBindingKey.empty())
        {
            if (Has_ReservedBindingPrefix(Element.strBindingKey))
            {
                Add_Error(Context, strPath + L".BindingKey", L"'__'로 시작하는 BindingKey는 런타임 내부 예약어입니다.");
            }

            if (Has_BindingNamespaceSeparator(Element.strBindingKey))
            {
                Add_Error(Context, strPath + L".BindingKey", L"BindingKey에는 컴포넌트 Namespace 구분자인 '.'을 사용할 수 없습니다.");
            }

            const _bool bBindingInserted = Context.BindingKeys.insert(Element.strBindingKey).second;

            if (!bBindingInserted)
            {
                Add_Error(Context, strPath + L".BindingKey", L"중복된 BindingKey입니다.");
            }
        }       
        
        const UITransform& Transform = Element.Transform;

        if (!Is_Finite(Transform.vPosition))
            Add_Error(Context, strPath + L".Transform.Position", L"Position에 유한하지 않은 값이 있습니다.");

        if (!Is_Finite(Transform.vSize) || Transform.vSize.x < 0.f || Transform.vSize.y < 0.f)
            Add_Error(Context, strPath + L".Transform.Size", L"Size는 유한한 0 이상의 값이어야 합니다.");

        if (Element.eType != UI_ELEMENT_TYPE::CONTAINER && (Transform.vSize.x == 0.f || Transform.vSize.y == 0.f))
            Add_Warning(Context, strPath + L".Transform.Size", L"표시 Element의 크기가 0입니다.");

        if (!Is_Finite(Transform.vPivot))
            Add_Error(Context, strPath + L".Transform.Pivot", L"Pivot에 유한하지 않은 값이 있습니다.");

        if (!Is_Finite(Transform.fRotation))
            Add_Error(Context, strPath + L".Transform.Rotation", L"Rotation은 유한한 값이어야 합니다.");

        if (!Is_Finite(Transform.vScale))
            Add_Error(Context, strPath + L".Transform.Scale", L"Scale에 유한하지 않은 값이 있습니다.");

        if (!Is_Finite(Element.Render.vColor))
            Add_Error(Context, strPath + L".Render.Color", L"Color에 유한하지 않은 값이 있습니다.");

        if (!Is_Finite(Element.Render.vUVRect))
            Add_Error(Context, strPath + L".Render.UVRect", L"UVRect에 유한하지 않은 값이 있습니다.");

        // 현재 DISSOLVE는 두 번째 텍스처를 보유할 수 있는 Image에서만 지원한다.
        if (Element.Render.eRevealType == UI_REVEAL_TYPE::DISSOLVE && Element.eType != UI_ELEMENT_TYPE::IMAGE)
            Add_Error(Context, strPath + L".Render.RevealType", L"DISSOLVE Reveal은 Image Element에서만 사용할 수 있습니다.");

        if (Element.eType == UI_ELEMENT_TYPE::IMAGE)
            Validate_Image(Element, Context, strPath);
        else if (Element.eType == UI_ELEMENT_TYPE::PROGRESS_BAR)
            Validate_Progress(Element, Context, strPath);
        else if (Element.eType == UI_ELEMENT_TYPE::TEXT)
        {
            const UI_TEXT_ASSET_DESC& Text = Element.Text;

            if (Text.strFontAssetID.empty())
            {
                Add_Error(Context, strPath + L".Text.FontAssetID", L"Text의 FontAssetID가 비어 있습니다.");
            }

            if (!Is_Finite(Text.fFontSize) || Text.fFontSize <= 0.f)
            {
                Add_Error(Context, strPath + L".Text.FontSize", L"FontSize는 유한한 0보다 큰 값이어야 합니다.");
            }

            if (!Is_Finite(Text.fLineSpacing) || Text.fLineSpacing <= 0.f)
            {
                Add_Error(Context, strPath + L".Text.LineSpacing", L"LineSpacing은 유한한 0보다 큰 값이어야 합니다.");
            }

            // Text는 단독으로 렌더링되는 Leaf Element
            if (!Element.Children.empty())
            {
                Add_Error(Context, strPath + L".Children", L"Text Element는 직접 자식을 가질 수 없습니다.");
            }
        }
        else if (Element.eType == UI_ELEMENT_TYPE::BUTTON)
        {
            Validate_Button(Element, Context, strPath);
        }
        else if (Element.eType == UI_ELEMENT_TYPE::COMPONENT_INSTANCE)
        {
            if (Element.ComponentInstance.strComponentAssetID.empty())
            {
                Add_Error(Context, strPath + L".ComponentInstance.AssetID", L"Component Instance의 참조 Asset ID가 비어 있습니다.");
            }

            // Component Instance는 참조 노드이므로 자신의 Children을 갖지 않음
            if (!Element.Children.empty())
            {
                Add_Error(Context, strPath + L".Children", L"Component Instance는 직접 자식을 가질 수 없습니다.");
            }
        }

        for (_uint i = 0; i < static_cast<_uint>(Element.Children.size()); ++i)
        {
            const _wstring strChildPath = strPath + L".Children[" + to_wstring(i) + L"]";

            Validate_Element(Element.Children[i], Context, strChildPath);
        }
    }

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 363~436행: Validate_Animations
    void Validate_Animations(const UI_ASSET_DESC& Asset, VALIDATION_CONTEXT& Context)
    {
        unordered_set<string> ClipNames;

        for (_uint iClipIndex = 0; iClipIndex < static_cast<_uint>(Asset.Animations.size()); ++iClipIndex)
        {
            const UI_ANIMATION_CLIP_ASSET_DESC& Clip = Asset.Animations[iClipIndex];

            const _wstring strClipPath = L"Animations[" + to_wstring(iClipIndex) + L"]";

            if (Clip.strName.empty())
            {
                Add_Error(Context, strClipPath + L".Name", L"Animation Clip 이름이 비어 있습니다.");
            }
            else if (!ClipNames.insert(Clip.strName).second)
            {
                Add_Error(Context, strClipPath + L".Name", L"중복된 Animation Clip 이름입니다.");
            }

            if (!Is_Finite(Clip.fDuration) || Clip.fDuration <= 0.f)
                Add_Error(Context, strClipPath + L".Duration", L"Animation Clip 길이는 유한한 0보다 큰 값이어야 합니다.");

            // 같은 Element 속성을 한 Clip에서 두 Track이 동시에 제어하지 못하게 한다.
            unordered_set<_uint64> TrackKeys;

            for (_uint iTrackIndex = 0; iTrackIndex < static_cast<_uint>(Clip.Tracks.size()); ++iTrackIndex)
            {
                const UI_ANIMATION_TRACK_ASSET_DESC& Track = Clip.Tracks[iTrackIndex];

                const _wstring strTrackPath = strClipPath + L".Tracks[" + to_wstring(iTrackIndex) + L"]";

                if (FindElementByID(Asset.Root, Track.iTargetElementID) == nullptr)
                    Add_Error(Context, strTrackPath + L".TargetElementID", L"Animation Track 대상 ElementID가 존재하지 않습니다.");

                const _uint64 iTrackKey =
                    (static_cast<_uint64>(Track.iTargetElementID) << 32) |
                    static_cast<_uint64>(Track.eProperty);

                if (!TrackKeys.insert(iTrackKey).second)
                    Add_Error(Context, strTrackPath, L"같은 Element와 Property를 제어하는 Animation Track이 중복되었습니다.");

                if (Track.Keyframes.empty())
                {
                    Add_Warning(Context, strTrackPath + L".Keyframes", L"Animation Track에 Keyframe이 없습니다.");
                    continue;
                }

                _float fPreviousTime = -1.f;

                for (_uint iKeyIndex = 0; iKeyIndex < static_cast<_uint>(Track.Keyframes.size()); ++iKeyIndex)
                {
                    const UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe = Track.Keyframes[iKeyIndex];

                    const _wstring strKeyPath = strTrackPath + L".Keyframes[" + to_wstring(iKeyIndex) + L"]";

                    if (!Is_Finite(Keyframe.fTime))
                        Add_Error(Context, strKeyPath + L".Time", L"Keyframe 시간은 유한한 값이어야 합니다.");
                    else
                    {
                        if (Keyframe.fTime < 0.f || Keyframe.fTime > Clip.fDuration)
                            Add_Error(Context, strKeyPath + L".Time", L"Keyframe 시간이 Animation Clip 범위를 벗어났습니다.");

                        if (Keyframe.fTime < fPreviousTime)
                            Add_Error(Context, strKeyPath + L".Time", L"Keyframe은 시간 오름차순으로 정렬되어야 합니다.");
                    }

                    if (!Is_Finite(Keyframe.vValue))
                        Add_Error(Context, strKeyPath + L".Value", L"Keyframe 값에 유한하지 않은 값이 있습니다.");

                    fPreviousTime = Keyframe.fTime;
                }
            }
        }
    }
