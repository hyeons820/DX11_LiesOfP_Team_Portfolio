#pragma once
#include "UIPanel.h"

#ifdef _DEBUG

NS_BEGIN(UI_Editor)

class CUIAnimationPanel final : public CUIPanel
{
private:
	CUIAnimationPanel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CUIAnimationPanel() = default;

public:
	virtual const char* Get_ID() const override;
	virtual const char* Get_DisplayName() const override;
	virtual DOCK_GROUP	GetDefaultDockGroup() const override;

public:
	virtual HRESULT		Initialize() override;
	virtual void		Update(_float fDeltaTime) override;
	virtual void		Render() override;
	virtual void		ShutDown() override;

private:
	void				Synchronize_State();		// 에셋 변경·Undo·삭제 이후 선택 인덱스를 유효 범위로 보정
	void				Render_ClipList();			// 왼쪽 Clip 목록과 추가·삭제 버튼
	void				Render_ClipInspector();		// 선택 Clip 이름, 길이, Loop 설정
	void				Render_Playback();			// 재생, 일시정지, 정지, 재생 시간 조절

private:
	string				Make_UniqueClipName() const;
	void				Track_AssetEdit(const UI_ASSET_DESC& BeforeAsset, _bool bChanged);

private:
	void				Render_TrackEditor();		// 선택한 Clip의 Track 목록과 Track 추가·삭제 UI
	void				Render_TrackList(UI_ANIMATION_CLIP_ASSET_DESC& Clip);		// Track 선택 목록
	void				Render_KeyframeEditor(UI_ANIMATION_CLIP_ASSET_DESC& Clip);	// 선택 Track의 Keyframe 편집 UI


private:
	// 같은 Element·Property Track 중복 검사
	_bool				Has_DuplicateTrack(const UI_ANIMATION_CLIP_ASSET_DESC& Clip, _uint iElementID, UI_ANIMATION_PROPERTY eProperty) const;
	// 동일 시간 Keyframe 검색
	_int				Find_KeyframeIndexAtTime(const UI_ANIMATION_TRACK_ASSET_DESC& Track, _float fTime, _float fEpsilon = 0.0001f) const;	
	// Element 현재 설정값을 첫 Keyframe 값으로 변환
	_float4				Make_DefaultKeyframeValue(const UI_ELEMENT_ASSET_DESC& Element, UI_ANIMATION_PROPERTY eProperty) const;
	// 새 Keyframe을 추가할 시간의 보간값 계산
	_float4				Evaluate_TrackValue(const UI_ANIMATION_TRACK_ASSET_DESC& Track, _float fTime) const;	
	// 편집기용 보간 계산
	_float				Apply_Interpolation(_float fRatio, UI_ANIMATION_INTERPOLATION eInterpolation) const;	

private:
	_bool					m_bTrackingAssetEdit = { false };
	UI_ANIMATION_PROPERTY	m_ePendingProperty = { UI_ANIMATION_PROPERTY::POSITION };

public:
	static CUIAnimationPanel* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END

#endif