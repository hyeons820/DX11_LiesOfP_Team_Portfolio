// 포트폴리오 코드 발췌: 패널 렌더 → 재생 시간 제어 → 키프레임 편집·이력 기록 → 트랙 보간
// 원본: UI_Editor/Private/UIAnimationPanel.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 63~87행: Render
void CUIAnimationPanel::Render()
{
	ImGui::Begin(GetImGuiWindowName().c_str(), &m_IsOpen);

	if (m_pEditorContext == nullptr || !m_pEditorContext->Is_AssetLoaded())
	{
		ImGui::TextDisabled("Open a UI Asset to edit animations.");
		ImGui::End();
		return;
	}

	Synchronize_State();

	Render_Playback();

	ImGui::Separator();

	Render_ClipList();

	ImGui::SameLine();

	Render_ClipInspector();

	ImGui::End();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 93~133행: Synchronize_State
void CUIAnimationPanel::Synchronize_State()
{
	UI_EDITOR_ANIMATION_STATE& State = m_pEditorContext->Get_AnimationState();
	const UI_ASSET_DESC& Asset = m_pEditorContext->Get_Asset();

	if (Asset.Animations.empty())
	{
		State = {};
		return;
	}

	if (State.iSelectedClipIndex < 0)
		State.iSelectedClipIndex = 0;

	if (State.iSelectedClipIndex >= static_cast<_int>(Asset.Animations.size()))
		State.iSelectedClipIndex = static_cast<_int>(Asset.Animations.size()) - 1;

	const UI_ANIMATION_CLIP_ASSET_DESC& Clip = Asset.Animations[State.iSelectedClipIndex];

	State.fCurrentTime = fClamp(State.fCurrentTime, 0.f, Clip.fDuration);


	// Undo/Redo나 Track 삭제 후 선택 인덱스가 잘못된 메모리를 가리키는 것을 맏음
	if (State.iSelectedTrackIndex < -1 || State.iSelectedTrackIndex >= static_cast<_int>(Clip.Tracks.size()))
	{
		State.iSelectedTrackIndex = -1;
		State.iSelectedKeyframeIndex = -1;
		return;
	}

	if (State.iSelectedTrackIndex < 0)
	{
		State.iSelectedKeyframeIndex = -1;
		return;
	}

	const UI_ANIMATION_TRACK_ASSET_DESC& Track = Clip.Tracks[State.iSelectedTrackIndex];

	if (State.iSelectedKeyframeIndex < -1 || State.iSelectedKeyframeIndex >= static_cast<_int>(Track.Keyframes.size()))
		State.iSelectedKeyframeIndex = -1;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 135~177행: Render_Playback
void CUIAnimationPanel::Render_Playback()
{
	UI_EDITOR_ANIMATION_STATE& State = m_pEditorContext->Get_AnimationState();
	const UI_ASSET_DESC& Asset = m_pEditorContext->Get_Asset();

	const _bool bHasClip = State.iSelectedClipIndex >= 0 && State.iSelectedClipIndex < static_cast<_int>(Asset.Animations.size());

	if (!bHasClip)
		ImGui::BeginDisabled();

	if (ImGui::Button(State.bPlaying ? "Pause" : "Play"))
		State.bPlaying = !State.bPlaying;

	ImGui::SameLine();

	if (ImGui::Button("Stop"))
	{
		State.bPlaying = false;
		State.fCurrentTime = 0.f;
	}

	ImGui::SameLine();

	if (bHasClip)
	{
		const UI_ANIMATION_CLIP_ASSET_DESC& Clip = Asset.Animations[State.iSelectedClipIndex];

		ImGui::SetNextItemWidth(300.f);
		ImGui::SliderFloat("Time", &State.fCurrentTime, 0.f, Clip.fDuration, "%.3f");

		ImGui::SameLine();
		ImGui::Text("/ %.3f sec", Clip.fDuration);
	}
	else
	{
		_float fEmptyTime = 0.f;
		ImGui::SetNextItemWidth(300.f);
		ImGui::SliderFloat("Time", &fEmptyTime, 0.f, 1.f, "%.3f");
	}

	if (!bHasClip)
		ImGui::EndDisabled();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 342~361행: Track_AssetEdit
void CUIAnimationPanel::Track_AssetEdit(const UI_ASSET_DESC& BeforeAsset, _bool bChanged)
{
	if (bChanged)
	{
		if (!m_bTrackingAssetEdit)
		{
			// InputText와 DragFloat의 연속 변경을 Undo 한 단계로 묶는다.
			m_pEditorContext->Begin_AssetEdit(BeforeAsset);
			m_bTrackingAssetEdit = true;
		}

		m_pEditorContext->Notify_AssetChanged();
	}

	if (m_bTrackingAssetEdit && !ImGui::IsAnyItemActive())
	{
		m_pEditorContext->End_AssetEdit();
		m_bTrackingAssetEdit = false;
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 516~711행: Render_KeyframeEditor
void CUIAnimationPanel::Render_KeyframeEditor(UI_ANIMATION_CLIP_ASSET_DESC& Clip)
{
	UI_EDITOR_ANIMATION_STATE& State = m_pEditorContext->Get_AnimationState();

	ImGui::BeginChild("AnimationKeyframeEditor", ImVec2(0.f, 360.f), true);

	if (State.iSelectedTrackIndex < 0 || State.iSelectedTrackIndex >= static_cast<_int>(Clip.Tracks.size()))
	{
		ImGui::TextDisabled("Select a Track.");
		ImGui::EndChild();
		return;
	}

	UI_ANIMATION_TRACK_ASSET_DESC& Track = Clip.Tracks[State.iSelectedTrackIndex];
	const UI_ELEMENT_ASSET_DESC* pTargetElement = m_pEditorContext->Find_AssetElement(Track.iTargetElementID);

	ImGui::Text("Property: %s", ToString(Track.eProperty));
	ImGui::Text("Current Time: %.3f", State.fCurrentTime);

	if (ImGui::Button("Add Key @ Current Time"))
	{
		const _int iExistingKeyframe = Find_KeyframeIndexAtTime(Track, State.fCurrentTime);

		if (iExistingKeyframe >= 0)
		{
			// 같은 시간에 Keyframe을 중복 생성하지 않고 기존 Keyframe을 선택한다.
			State.iSelectedKeyframeIndex = iExistingKeyframe;
		}
		else
		{
			m_pEditorContext->Begin_AssetEdit();

			UI_ANIMATION_KEYFRAME_ASSET_DESC NewKeyframe = {};
			NewKeyframe.fTime = State.fCurrentTime;
			NewKeyframe.eInterpolation = UI_ANIMATION_INTERPOLATION::LINEAR;

			if (Track.Keyframes.empty())
				NewKeyframe.vValue = pTargetElement != nullptr ? Make_DefaultKeyframeValue(*pTargetElement, Track.eProperty) : _float4{};
			else
				NewKeyframe.vValue = Evaluate_TrackValue(Track, State.fCurrentTime);

			auto InsertIter = lower_bound(Track.Keyframes.begin(), Track.Keyframes.end(), NewKeyframe.fTime, [](const UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe, _float fTime)
				{
					return Keyframe.fTime < fTime;
				});

			State.iSelectedKeyframeIndex = static_cast<_int>(distance(Track.Keyframes.begin(), InsertIter));
			Track.Keyframes.insert(InsertIter, NewKeyframe);

			m_pEditorContext->Notify_AssetChanged();
			m_pEditorContext->End_AssetEdit();
		}
	}

	ImGui::SameLine();

	const _bool bHasSelectedKeyframe = State.iSelectedKeyframeIndex >= 0 && State.iSelectedKeyframeIndex < static_cast<_int>(Track.Keyframes.size());

	if (!bHasSelectedKeyframe)
		ImGui::BeginDisabled();

	if (ImGui::Button("Remove Key"))
	{
		m_pEditorContext->Begin_AssetEdit();

		Track.Keyframes.erase(Track.Keyframes.begin() + State.iSelectedKeyframeIndex);
		State.iSelectedKeyframeIndex = Track.Keyframes.empty() ? -1 : min(State.iSelectedKeyframeIndex, static_cast<_int>(Track.Keyframes.size()) - 1);

		m_pEditorContext->Notify_AssetChanged();
		m_pEditorContext->End_AssetEdit();

		ImGui::EndChild();
		return;
	}

	if (!bHasSelectedKeyframe)
		ImGui::EndDisabled();

	ImGui::Separator();
	ImGui::TextUnformatted("Keyframes");

	for (_int iKeyframeIndex = 0; iKeyframeIndex < static_cast<_int>(Track.Keyframes.size()); ++iKeyframeIndex)
	{
		const UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe = Track.Keyframes[iKeyframeIndex];
		const string strLabel = to_string(iKeyframeIndex) + " : " + to_string(Keyframe.fTime) + " sec";

		ImGui::PushID(iKeyframeIndex);

		if (ImGui::Selectable(strLabel.c_str(), State.iSelectedKeyframeIndex == iKeyframeIndex))
		{
			State.iSelectedKeyframeIndex = iKeyframeIndex;
			State.fCurrentTime = Keyframe.fTime;
		}

		ImGui::PopID();
	}

	if (State.iSelectedKeyframeIndex < 0 || State.iSelectedKeyframeIndex >= static_cast<_int>(Track.Keyframes.size()))
	{
		ImGui::EndChild();
		return;
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Selected Keyframe");

	UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe = Track.Keyframes[State.iSelectedKeyframeIndex];
	const UI_ASSET_DESC BeforeAsset = m_pEditorContext->Get_Asset();
	_bool bChanged = false;

	const _float fMinimumTime = State.iSelectedKeyframeIndex > 0 ? Track.Keyframes[State.iSelectedKeyframeIndex - 1].fTime + 0.001f : 0.f;
	const _float fMaximumTime = State.iSelectedKeyframeIndex + 1 < static_cast<_int>(Track.Keyframes.size()) ? Track.Keyframes[State.iSelectedKeyframeIndex + 1].fTime - 0.001f : Clip.fDuration;

	if (ImGui::DragFloat("Key Time", &Keyframe.fTime, 0.001f, fMinimumTime, fMaximumTime, "%.3f sec", ImGuiSliderFlags_AlwaysClamp))
	{
		State.fCurrentTime = Keyframe.fTime;
		bChanged = true;
	}

	switch (Track.eProperty)
	{
	case UI_ANIMATION_PROPERTY::POSITION:
		bChanged |= ImGui::DragFloat2("Position", &Keyframe.vValue.x, 1.f);
		break;

	case UI_ANIMATION_PROPERTY::SIZE:
		bChanged |= ImGui::DragFloat2("Size", &Keyframe.vValue.x, 1.f);
		break;

	case UI_ANIMATION_PROPERTY::SCALE:
		bChanged |= ImGui::DragFloat2("Scale", &Keyframe.vValue.x, 0.01f);
		break;

	case UI_ANIMATION_PROPERTY::ROTATION:
	{
		_float fRotationDegree = XMConvertToDegrees(Keyframe.vValue.x);

		if (ImGui::DragFloat("Rotation", &fRotationDegree, 1.f, -360.f, 360.f, "%.1f deg"))
		{
			// 런타임 Rotation은 Radian 단위이므로 저장 전에 변환한다.
			Keyframe.vValue.x = XMConvertToRadians(fRotationDegree);
			bChanged = true;
		}

		break;
	}

	case UI_ANIMATION_PROPERTY::COLOR:
		bChanged |= ImGui::ColorEdit4("Color", &Keyframe.vValue.x);
		break;

	case UI_ANIMATION_PROPERTY::ALPHA:
		bChanged |= ImGui::DragFloat("Alpha", &Keyframe.vValue.x, 0.01f, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		break;

	case UI_ANIMATION_PROPERTY::UV_RECT:
		bChanged |= ImGui::DragFloat4("UV Rect", &Keyframe.vValue.x, 0.001f, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		break;

	case UI_ANIMATION_PROPERTY::REVEAL_PROGRESS:
		bChanged |= ImGui::DragFloat("Reveal Progress", &Keyframe.vValue.x, 0.01f, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		break;
	}

	if (ImGui::BeginCombo("Interpolation", ToString(Keyframe.eInterpolation)))
	{
		const UI_ANIMATION_INTERPOLATION Interpolations[] =
		{
			UI_ANIMATION_INTERPOLATION::STEP,
			UI_ANIMATION_INTERPOLATION::LINEAR,
			UI_ANIMATION_INTERPOLATION::EASE_IN,
			UI_ANIMATION_INTERPOLATION::EASE_OUT,
			UI_ANIMATION_INTERPOLATION::EASE_IN_OUT
		};

		for (UI_ANIMATION_INTERPOLATION eInterpolation : Interpolations)
		{
			const _bool bSelected = Keyframe.eInterpolation == eInterpolation;

			if (ImGui::Selectable(ToString(eInterpolation), bSelected))
			{
				Keyframe.eInterpolation = eInterpolation;
				bChanged = true;
			}

			if (bSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	Track_AssetEdit(BeforeAsset, bChanged);

	ImGui::EndChild();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 724~733행: Find_KeyframeIndexAtTime
_int CUIAnimationPanel::Find_KeyframeIndexAtTime(const UI_ANIMATION_TRACK_ASSET_DESC& Track, _float fTime, _float fEpsilon) const
{
	for (_int iKeyframeIndex = 0; iKeyframeIndex < static_cast<_int>(Track.Keyframes.size()); ++iKeyframeIndex)
	{
		if (fabsf(Track.Keyframes[iKeyframeIndex].fTime - fTime) <= fEpsilon)
			return iKeyframeIndex;
	}

	return -1;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 767~797행: Evaluate_TrackValue
_float4 CUIAnimationPanel::Evaluate_TrackValue(const UI_ANIMATION_TRACK_ASSET_DESC& Track, _float fTime) const
{
	if (Track.Keyframes.empty())
		return {};

	if (Track.Keyframes.size() == 1 || fTime <= Track.Keyframes.front().fTime)
		return Track.Keyframes.front().vValue;

	if (fTime >= Track.Keyframes.back().fTime)
		return Track.Keyframes.back().vValue;

	auto RightIter = upper_bound(Track.Keyframes.begin(), Track.Keyframes.end(), fTime, [](_float fValue, const UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe)
		{
			return fValue < Keyframe.fTime;
		});

	const UI_ANIMATION_KEYFRAME_ASSET_DESC& RightKeyframe = *RightIter;
	const UI_ANIMATION_KEYFRAME_ASSET_DESC& LeftKeyframe = *(RightIter - 1);
	const _float fKeyDuration = RightKeyframe.fTime - LeftKeyframe.fTime;

	if (fKeyDuration <= FLT_EPSILON)
		return RightKeyframe.vValue;

	_float fRatio = (fTime - LeftKeyframe.fTime) / fKeyDuration;
	fRatio = Apply_Interpolation(fClamp(fRatio, 0.f, 1.f), LeftKeyframe.eInterpolation);

	_float4 vResult = {};
	XMStoreFloat4(&vResult, XMVectorLerp(XMLoadFloat4(&LeftKeyframe.vValue), XMLoadFloat4(&RightKeyframe.vValue), fRatio));

	return vResult;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 799~820행: Apply_Interpolation
_float CUIAnimationPanel::Apply_Interpolation(_float fRatio, UI_ANIMATION_INTERPOLATION eInterpolation) const
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
