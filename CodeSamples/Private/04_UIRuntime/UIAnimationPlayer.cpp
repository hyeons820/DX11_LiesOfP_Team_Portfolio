// 포트폴리오 코드 발췌: 클립 재생/시간 이동 → 트랙 키프레임 평가 → 보간 → 실제 UI 속성 적용
// 원본: Engine/Private/UIAnimationPlayer.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 19~41행: Play
_bool CUIAnimationPlayer::Play(const string& strClipName, _bool bRestart)
{
	const UI_ANIMATION_CLIP_ASSET_DESC* pClip = Find_Clip(strClipName);

	if (pClip == nullptr)
		return false;

	// 같은 Clip이 이미 재생 중이고 Restart 요청이 없으면 그대로 유지한다.
	if (bRestart == false && m_pCurrentClip == pClip && m_bPlaying)
		return true;

	m_pCurrentClip = pClip;
	m_fCurrentTime = 0.f;

	m_bPlaying = true;
	m_bPaused = false;
	m_bFinished = false;

	// 첫 프레임을 기다리지 않고 0초 Keyframe 상태를 즉시 적용한다.
	Evaluate_CurrentClip();

	return true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 65~74행: Set_Time
_bool CUIAnimationPlayer::Set_Time(_float fTime)
{
	if (m_pCurrentClip == nullptr)
		return false;

	m_fCurrentTime = fClamp(fTime, 0.f, m_pCurrentClip->fDuration);
	Evaluate_CurrentClip();

	return true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 76~110행: Update
void CUIAnimationPlayer::Update(_float fTimeDelta)
{
	if (m_bPlaying == false || m_bPaused || m_pCurrentClip == nullptr)
		return;

	const _float fDuration = m_pCurrentClip->fDuration;

	if (fDuration <= FLT_EPSILON)
	{
		Stop();
		m_bFinished = true;
		return;
	}

	m_fCurrentTime += fTimeDelta;

	if (m_pCurrentClip->bLoop)
	{
		// Duration을 초과하면 나머지 시간부터 다시 반복한다.
		m_fCurrentTime = fmodf(m_fCurrentTime, fDuration);
	}
	else if (m_fCurrentTime >= fDuration)
	{
		// 마지막 Keyframe을 정확히 적용한 뒤 재생을 종료한다.
		m_fCurrentTime = fDuration;
		Evaluate_CurrentClip();

		m_bPlaying = false;
		m_bFinished = true;

		return;
	}

	Evaluate_CurrentClip();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 145~156행: Find_Clip
const UI_ANIMATION_CLIP_ASSET_DESC* CUIAnimationPlayer::Find_Clip(const string& strClipName) const
{
	auto iter = find_if(m_Clips.begin(), m_Clips.end(), [&strClipName](const UI_ANIMATION_CLIP_ASSET_DESC& Clip)
		{
			return Clip.strName == strClipName;
		});

	if (iter == m_Clips.end())
		return nullptr;

	return &(*iter);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 158~177행: Evaluate_CurrentClip
void CUIAnimationPlayer::Evaluate_CurrentClip()
{
	if (m_pOwner == nullptr || m_pCurrentClip == nullptr)
		return;

	for (const UI_ANIMATION_TRACK_ASSET_DESC& Track : m_pCurrentClip->Tracks)
	{
		if (Track.Keyframes.empty())
			continue;

		CUIElement* pTarget = m_pOwner->Find_Element(Track.iTargetElementID);

		if (pTarget == nullptr)
			continue;

		const _float4 vValue = Evaluate_Track(Track, m_fCurrentTime);

		Apply_Value(pTarget, Track.eProperty, vValue);
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 179~219행: Evaluate_Track
_float4 CUIAnimationPlayer::Evaluate_Track(const UI_ANIMATION_TRACK_ASSET_DESC& Track, _float fTime) const
{
	if (Track.Keyframes.empty())
		return {};

	if (Track.Keyframes.size() == 1 || fTime <= Track.Keyframes.front().fTime)
		return Track.Keyframes.front().vValue;

	if (fTime >= Track.Keyframes.back().fTime)
		return Track.Keyframes.back().vValue;

	// 현재 시간보다 뒤에 있는 첫 번째 Keyframe을 찾는다.
	auto RightIter = upper_bound(Track.Keyframes.begin(), Track.Keyframes.end(), fTime, [](_float fValue, const UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe)
		{
			return fValue < Keyframe.fTime;
		});

	if (RightIter == Track.Keyframes.begin())
		return RightIter->vValue;

	if (RightIter == Track.Keyframes.end())
		return Track.Keyframes.back().vValue;

	const UI_ANIMATION_KEYFRAME_ASSET_DESC& RightKeyframe = *RightIter;
	const UI_ANIMATION_KEYFRAME_ASSET_DESC& LeftKeyframe = *(RightIter - 1);

	const _float fKeyDuration = RightKeyframe.fTime - LeftKeyframe.fTime;

	if (fKeyDuration <= FLT_EPSILON)
		return RightKeyframe.vValue;

	_float fRatio = (fTime - LeftKeyframe.fTime) / fKeyDuration;
	fRatio = fClamp(fRatio, 0.f, 1.f);
	fRatio = Apply_Interpolation(fRatio, LeftKeyframe.eInterpolation);

	_float4 vResult = {};

	XMStoreFloat4(&vResult, XMVectorLerp(XMLoadFloat4(&LeftKeyframe.vValue), XMLoadFloat4(&RightKeyframe.vValue), fRatio));

	return vResult;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 221~242행: Apply_Interpolation
_float CUIAnimationPlayer::Apply_Interpolation(_float fRatio, UI_ANIMATION_INTERPOLATION eInterpolation) const
{
	switch (eInterpolation)
	{
	case UI_ANIMATION_INTERPOLATION::STEP:
		return 0.f;

	case UI_ANIMATION_INTERPOLATION::LINEAR:
		return fRatio;

	case UI_ANIMATION_INTERPOLATION::EASE_IN:
		return fRatio * fRatio;

	case UI_ANIMATION_INTERPOLATION::EASE_OUT:
		return 1.f - (1.f - fRatio) * (1.f - fRatio);

	case UI_ANIMATION_INTERPOLATION::EASE_IN_OUT:
		return fRatio * fRatio * (3.f - 2.f * fRatio);
	}

	return fRatio;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 244~292행: Apply_Value
void CUIAnimationPlayer::Apply_Value(CUIElement* pTarget, UI_ANIMATION_PROPERTY eProperty, const _float4& vValue)
{
	if (pTarget == nullptr)
		return;

	switch (eProperty)
	{
	case UI_ANIMATION_PROPERTY::POSITION:
		// JSON은 기준 해상도 좌표이므로 실제 런타임 해상도 배율을 적용한다.
		pTarget->Set_Position(_float2(vValue.x * m_vRuntimeScale.x, vValue.y * m_vRuntimeScale.y));
		break;

	case UI_ANIMATION_PROPERTY::SIZE:
		pTarget->Set_Size(_float2(vValue.x * m_vRuntimeScale.x, vValue.y * m_vRuntimeScale.y));
		break;

	case UI_ANIMATION_PROPERTY::SCALE:
		pTarget->Set_Scale(_float2(vValue.x, vValue.y));
		break;

	case UI_ANIMATION_PROPERTY::ROTATION:
		// 현재 UIElement의 Rotation은 Radian 단위다.
		pTarget->Set_Rotation(vValue.x);
		break;

	case UI_ANIMATION_PROPERTY::COLOR:
		pTarget->Set_Color(vValue);
		break;

	case UI_ANIMATION_PROPERTY::ALPHA:
	{
		// 기존 RGB는 유지하고 Alpha만 변경한다.
		_float4 vColor = pTarget->Get_RenderData().vColor;
		vColor.w = fClamp(vValue.x, 0.f, 1.f);

		pTarget->Set_Color(vColor);
		break;
	}

	case UI_ANIMATION_PROPERTY::UV_RECT:
		pTarget->Set_UVRect(vValue);
		break;

	case UI_ANIMATION_PROPERTY::REVEAL_PROGRESS:
		// Keyframe의 x값을 Reveal 진행률로 사용한다.
		pTarget->Set_RevealProgress(vValue.x);
		break;		
	}
}
