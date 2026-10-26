#pragma once
#include "Animation_Defines.h"
#include "AnimationData.h"
#include "Base.h"

#ifdef _DEBUG

NS_BEGIN(Anim_Editor)

// 에디터가 모델 파일에서 읽어 들인 bone 표시 정보.
// 실제 pose 계산용 행렬이 아니라 UI tree/slot 선택을 위한 이름과 parent index만 가진다.
typedef struct tagBoneInfo
{
	string		strBoneName;
	_int		iParentIndex = -1;
}BONE_INFO;

// 현재 에디터에 로드된 리소스/편집 데이터.
// 모델/스켈레톤 이름은 저장 파일 검증에 사용하고, ClipSettings/Sequences/Controller는 실제 편집 대상이다.
typedef struct tagLoadedDataContext
{
	string						strModelName;
	string						strSkeletonName;

	vector<ANIM_CLIP_SETTING>	ClipSettings;
	vector<BONE_INFO>			Bones;

	vector<ANIM_SEQUENCE>		Sequences;

	ANIM_CONTROLLER				Controller;
}LOADED_DATA_CONTEXT;

// 각 패널에서 공유하는 현재 선택 상태.
// 모든 index는 -1이면 선택 없음이고, 패널들은 직접 데이터를 들지 않고 이 선택값을 기준으로 Context setter를 호출한다.
typedef struct tagSelectionContext
{
	_int	iSelectedClipIndex = { -1 };
	_int	iSelectedBoneIndex = { -1 };
	_int	iSelectedNotifyIndex = { -1 };
	_int	iSelectedWindowIndex = { -1 };
	_int	iSelectedSpeedSectionIndex = { -1 };

	// 프리뷰 본 선택용
	_int	iHoveredPreviewBoneIndex = { -1 };
	_int	iSelectedPreviewBoneIndex = { -1 };
	string	strHoveredPreviewBoneName;
	string	strSelectedPreviewBoneName;

	// 시퀀스 선택용
	_int	iSelectedSequenceIndex = { -1 };
	_int	iSelectedSequenceStepIndex = { -1 };
	_int    iSelectedSequenceHoldConditionIndex = { -1 };

	// 애니메이션 컨트롤러용
	_int	iSelectedControllerParameterIndex = { -1 };
	_int	iSelectedControllerStateIndex = { -1 };
	_int	iSelectedControllerConditionIndex = { -1 };
	_int	iSelectedActionMapIndex = { -1 };

	// 레이어 룰
	_int	iSelectedPoseLayerRuleIndex = { -1 };
	_int	iSelectedPoseLayerConditionIndex = { -1 };

}SELECTION_CONTEXT;

// UI 기준 미리보기 상태. Preview_Manager가 Animator/PreviewObject에 적용한다.
// 실제 재생 컴포넌트 상태가 아니라 에디터 UI가 요청한 preview 의도값이다.
typedef struct tagPreviewContext
{
	bool	bPreviewAvailable = { false };
	bool	bIsPlaying = { false };
	bool	bIsLoop = { false };
	bool	bUseRootMotionPreview = { false };
	_float  fPlayRate = { 1.f };

	_int	iRootMotionBoneIndex = { -1 };
	string	strRootMotionBoneName;
}PREVIEW_CONTEXT;

enum class EDITOR_TIMELINE_MODE
{
	CLIP,
	SEQUENCE,
};

enum class EDITOR_PREVIEW_OBJECT_TYPE
{
	NONE,
	MODEL,
	PLAYER
};

// UI 기준 타임라인 상태.
// CLIP 모드에서는 선택 clip local time, SEQUENCE 모드에서는 step들을 이어붙인 가상 timeline 기준이다.
typedef struct tagTimelineContext
{
	EDITOR_TIMELINE_MODE eMode = EDITOR_TIMELINE_MODE::CLIP;

	_uint	iCurFrame = {};
	_uint	iFrameCount = {};
	_float	fCurTime = {};
	_float	fDuration = {};
}TIMELINE_CONTEXT;

// 편집 데이터가 바뀌었음을 Preview_Manager에 알리는 flag 모음.
// 패널은 dirty flag만 세우고, 실제 Animator/PreviewObject 반영은 Preview_Manager가 담당한다.
typedef struct tagDirtyContext
{
	bool	bClipListDirty = { false };				// 클립 목록 자체 변경
	bool	bClipRangeDirty = { false };			// 선택 클립의 Start/End 변경
	bool	bClipDataDirty = { false };				// Notify / Window / SpeedSection 등 내부 데이터 변경
	bool	bControllerDirty = { false };
}DIRTY_CONTEXT;


/*
 * Animation Editor Context 현재 역할
 * - 패널들이 공유하는 에디터 상태 저장소다.
 * - 모델/클립 로드 정보, 선택 상태, 미리보기 상태, 타임라인 상태, dirty flag를 가진다.
 * - 실제 PreviewObject, Level, Animator를 직접 소유하거나 호출하지 않는다.
 * - Panel은 Context를 수정하고, Preview_Manager가 Context를 읽어서 런타임 미리보기에 반영한다.
 */
class CAnimation_EditorContext final : public CBase
{
	DECLARE_SINGLETON(CAnimation_EditorContext)

private:
	CAnimation_EditorContext() = default;
	virtual ~CAnimation_EditorContext() = default;

public:
	const	LOADED_DATA_CONTEXT&		Get_LoadedData() const { return m_tLoadedData; }
	const	SELECTION_CONTEXT&			Get_Selection()  const { return m_tSelection; }
	const	PREVIEW_CONTEXT&			Get_Preview()	 const { return m_tPreview; }
	const	TIMELINE_CONTEXT&			Get_Timeline()   const { return m_tTimeline; }
	const	DIRTY_CONTEXT&				Get_Dirty()		 const { return m_tDirty; }

public:
	ANIM_CLIP_SETTING*					Get_SelectedClipSetting();					// 현재 선택 clip 설정을 반환한다.
	const ANIM_CLIP_SETTING*			Get_SelectedClipSetting() const;

	ANIM_NOTIFY*						Get_SelectedNotify();
	const ANIM_NOTIFY*					Get_SelectedNotify() const;

	ANIM_WINDOW*						Get_SelectedWindow();
	const ANIM_WINDOW*					Get_SelectedWindow() const;

	ANIM_SPEED_SECTION*					Get_SelectedSpeedSection();
	const ANIM_SPEED_SECTION*			Get_SelectedSpeedSection() const;

	ANIM_SEQUENCE*						Get_SelectedSequence();						// 현재 선택 sequence를 반환한다.
	const ANIM_SEQUENCE*				Get_SelectedSequence() const;

	ANIM_SEQUENCE_STEP*					Get_SelectedSequenceStep();
	const ANIM_SEQUENCE_STEP*			Get_SelectedSequenceStep() const;

public:
	// 컨트롤러용
	ANIM_CONTROLLER*					Get_Controller();					// 편집 중인 controller root 데이터.
	const ANIM_CONTROLLER*				Get_Controller() const;

	ANIM_CONTROLLER_STATE*				Get_SelectedControllerState();
	const ANIM_CONTROLLER_STATE*		Get_SelectedControllerState() const;

	ANIM_CONTROLLER_PARAMETER*			Get_SelectedControllerParameter();
	const ANIM_CONTROLLER_PARAMETER*	Get_SelectedControllerParameter() const;

	ANIM_CONTROLLER_CONDITION*			Get_SelectedStateCondition();			// 현재 선택 state 안의 선택 condition.
	const ANIM_CONTROLLER_CONDITION*	Get_SelectedStateCondition() const;

	ANIM_CONTROLLER_CONDITION*			Get_SelectedSequenceHoldCondition();
	const ANIM_CONTROLLER_CONDITION*	Get_SelectedSequenceHoldCondition() const;

	ANIM_POSE_LAYER_RULE*				Get_SelectedPoseLayerRule();
	const ANIM_POSE_LAYER_RULE*			Get_SelectedPoseLayerRule() const;

	ANIM_CONTROLLER_CONDITION*			Get_SelectedPoseLayerCondition();
	const ANIM_CONTROLLER_CONDITION*	Get_SelectedPoseLayerCondition() const;

	ANIM_ACTION_STATE_MAP*				Get_SelectedActionStateMap();
	const ANIM_ACTION_STATE_MAP*		Get_SelectedActionStateMap() const;

public:
	void								Set_PreviewObjectType(EDITOR_PREVIEW_OBJECT_TYPE eType);
	EDITOR_PREVIEW_OBJECT_TYPE			Get_PreviewObjectType() const;

public:
	void								Set_ModelName(const string& strModelName);
	void								Set_SkeletonName(const string& strSkeletonName);

	void								Set_ClipSettings(const vector<ANIM_CLIP_SETTING>& ClipSettings);
	void								Set_Bones(const vector<BONE_INFO>& Bones);

	void								Set_RootMotionPreviewBone(_int iBoneIndex);

public:
	void								Set_Sequences(const vector<ANIM_SEQUENCE>& Sequences);		// 로드한 sequence 목록을 교체하고 선택 상태를 보정한다.

public:
	void								Select_Clip(_int iClipIndex);					// clip 목록에서 현재 편집/미리보기할 clip을 선택한다.
	void								Select_Bone(_int iBoneindex);
	void								Select_Notify(_int iNotifyIndex);
	void								Select_Window(_int iWindowIndex);
	void								Select_SpeedSection(_int iSpeedSectionIndex);

public:
	void								Select_Sequence(_int iSequenceIndex);			// timeline을 SEQUENCE 모드로 전환하고 선택 sequence를 미리보기 대상으로 만든다.
	void								Select_SequenceStep(_int iStepIndex);

public:
	void								Clear_RootMotionPreviewBone();

public:
	void								Clear_ClipSelection();
	void								Clear_BoneSelection();
	void								Clear_NotifySelection();
	void								Clear_WindowSelection();
	void								Clear_SpeedSectionSelection();
	void								Clear_Selection();

public:
	void								Clear_SequenceSelection();

public:
	void								Set_SelectedClipExportName(const string& strExportName);
	void								Set_SelectedClipLoop(_bool bLoop);
	void								Set_SelectedClipRootMotion(_bool bUseRootMotion);
	void								Set_SelectedClipReverse(_bool bReverse);
	void								Set_SelectedClipPlayRate(_float fPlayRate);
	void								Set_SelectedClipBlendIn(_float fBlendInTime);
	void								Set_SelectedClipBlendOut(_float fBlendOutTime);
	void								Set_SelectedClipFrameRange(_uint iStartFrame, _uint iEndFrame);	// 선택 clip의 사용할 frame 구간을 변경한다.

	// 클립 세팅 복제, 삭제
	void								Duplicate_SelectedClipSetting();
	void								Remove_SelectedClipSetting();

	void								Register_RemovableClipHash(_uint64 iClipHash);
	_bool								Is_SelectedClipRemovable() const;

public:
	void								Set_PreviewAvailable(bool bAvailable);
	void								Set_Playing(bool bPlaying);		// 미리보기 재생/정지 UI 상태를 변경한다.
	void								Set_Loop(bool bLoop);
	void								Set_PlayRate(_float fPlayRate);
	void								Set_UseRootMotion(bool bUse);

	void								Set_CurFrame(_uint iFrame);		// 타임라인 현재 frame을 갱신한다.
	void								Set_CurTime(_float fTime);		// 타임라인 현재 local time을 갱신한다.
	void								Set_FrameCount(_uint iFrameCount);
	void								Set_Duration(_float fDuration);

	void								Set_ClipListDirty(bool bDirty);
	void								Set_ClipRangeDirty(bool bDirty);
	void								Set_ClipDataDirty(bool bDirty);
	void								Set_ControllerDirty(bool bDirty);
	void								Clear_DirtyFlags();

public:
	void								Add_Notify();
	void								Remove_SelectedNotify();

	void								Add_SelectedNotifySlot();
	void								Remove_SelectedNotifySlot();

	void								Add_Window();
	void								Remove_SelectedWindow();

	void								Add_SpeedSection();
	void								Remove_SelectedSpeedSection();

public:
	void								Add_Sequence();
	void								Remove_SelectedSequence();

	void								Add_SelectedSequenceStep();
	void								Remove_SelectedSequenceStep();

	void								Add_SelectedSequenceHoldCondition();
	void								Remove_SelectedSequenceHoldCondition();
	void								Select_SequenceHoldCondition(_int iConditionIndex);

public:
	//컨트롤러용
	void								Add_ControllerState();							// 새 controller state를 만들고 첫 state면 entry로 지정한다.
	void								Remove_SelectedControllerState();				// 선택 state를 삭제하고 entry/attack map 참조를 정리한다.
	void								Sort_States();
	void								Select_ControllerState(_int iStateIndex);

	void								Add_ControllerParameter();						// 새 parameter를 만들고 선택한다.
	void								Remove_SelectedControllerParameter();			// 선택 parameter를 삭제하고 condition 참조를 정리한다.
	void								Select_ControllerParameter(_int iParameterIndex);

	void								Add_SelectedStateCondition();					// 선택 state에 진입 조건을 추가한다.
	void								Remove_SelectedStateCondition();
	void								Sort_StatesByPriority();
	void								Select_StateCondition(_int iConditionIndex);

	void								Add_PoseLayerRule();
	void								Remove_SelectedPoseLayerRule();
	void								Select_PoseLayerRule(_int iRuleIndex);

	void								Add_SelectedPoseLayerCondition();
	void								Remove_SelectedPoseLayerCondition();
	void								Select_PoseLayerCondition(_int iConditionIndex);

public:
	void								Set_SelectedNotifyName(const string& strName);
	void								Set_SelectedNotifyType(ANIM_NOTIFY_TYPE eType);
	void								Set_SelectedNotifyFrame(_uint iFrame);
	void								Set_SelectedNotifyPayload(const string& strPayload);

public:
	void								Set_SelectedNotifySlotName(const string& strSlotName);
	void								Set_SelectedNotifySlotBone(_int iBoneIndex);
	void								Set_SelectedNotifySlotOffsetPosition(const _float3& vOffsetPosition);
	void								Set_SelectedNotifySlotOffsetRotation(const _float3& vOffsetRotation);
	void								Set_SelectedNotifySlotOffsetScale(const _float3& vOffsetScale);

	void								Apply_SelectedBoneNotifySlot();

public:
	void								Set_SelectedWindowName(const string& strName);
	void								Set_SelectedWindowType(ANIM_WINDOW_TYPE eType);
	void								Set_SelectedWindowFrameRange(_uint iStartFrame, _uint iEndFrame);
	void								Set_SelectedWindowFrameRange(_int iWindowIndex, _uint iStartFrame, _uint iEndFrame);
	void								Set_SelectedWindowValue(_float fValue);
	void								Set_SelectedWindowPayload(const string& strPayload);

public:
	void								Set_SelectedSpeedSectionFrameRange(_uint iStartFrame, _uint iEndFrame);
	void								Set_SelectedSpeedSectionFrameRange(_int iSpeedSectionIndex, _uint iStartFrame, _uint iEndFrame);
	void								Set_SelectedSpeedSectionPlayRate(_float fPlayRate);

public:
	void								Set_SelectedSequenceName(const string& strName);
	void                                Set_SelectedSequenceFlow(ANIM_SEQUENCE_FLOW eFlow);
	void                                Set_SelectedSequenceStepBlendMode(ANIM_SEQUENCE_STEP_BLEND_MODE eMode);
	void                                Set_SelectedSequenceRestartRule(ANIM_SEQUENCE_RESTART_RULE eRule);
	void                                Set_SelectedSequenceStartCancelByWindow(bool bEnable);
	void                                Set_SelectedSequenceAllowEndToLoopReentry(bool bEnable);
	void                                Set_SelectedSequenceLoopToEndBlendTime(_float fTime);
	void                                Set_SelectedSequenceEndBlendOutTime(_float fTime);
	void                                Set_SelectedSequenceReentryBlendTime(_float fTime);
	void                                Set_SelectedSequenceReentryStartOffsetTime(_float fTime);

	void								Set_SelectedSequenceStepType(ANIM_SEQUENCE_STEP_TYPE eStepType);
	void								Set_SelectedSequenceStepClip(_int iClipIndex);

public:
	void								Set_HoveredPreviewBone(_int iBoneIndex, const string& strBoneName);
	void								Set_SelectedPreviewBone(_int iBoneIndex, const string& strBoneName);
	void								Clear_HoveredPreviewBone();
	void								Clear_SelectedPreviewBone();

public:
	void								Set_SelectedSequenceHoldConditionParameter(_int iParameterIndex);
	void								Set_SelectedSequenceHoldConditionCompareType(ANIM_CONTROLLER_COMPARE_TYPE eCompareType);
	void								Set_SelectedSequenceHoldConditionBoolValue(bool bValue);
	void								Set_SelectedSequenceHoldConditionIntValue(_int iValue);
	void								Set_SelectedSequenceHoldConditionFloatValue(_float fValue);

public:
	// 컨트롤러용
	void								Set_ControllerName(const string& strName);			// controller 이름과 hash를 갱신한다.
	void								Set_ControllerEntryState(_int iStateIndex);			// controller 시작 state를 지정하거나 해제한다.

	// State setter
	void								Set_SelectedControllerStateName(const string& strName);
	void								Set_SelectedControllerStateMotionType(ANIM_CONTROLLER_MOTION_TYPE eMotionType);
	void								Set_SelectedControllerStateMotion(_int iMotionIndex);	// state motion type에 맞춰 clip/sequence 참조를 연결한다.
	void								Set_SelectedControllerStateEntryMode(ANIM_STATE_ENTRY_MODE eEntryMode);
	void								Set_SelectedControllerStateRequestRule(ANIM_STATE_REQUEST_RULE eRequestRule);
	void								Set_SelectedControllerStatePriority(_int iPriority);
	void								Set_SelectedControllerStateCanRestart(bool bCanRestart);
	void								Set_SelectedControllerStateHoldRule(ANIM_STATE_HOLD_RULE eHoldRule);

	void								Set_SelectedControllerStateAllowPoseLayers(bool bAllow);

	// Condition Setter
	void								Set_SelectedStateConditionParameter(_int iParameterIndex);	// condition이 참조할 parameter를 연결하고 기본 compare type을 보정한다.
	void								Set_SelectedStateConditionCompareType(ANIM_CONTROLLER_COMPARE_TYPE eCompareType);
	void								Set_SelectedStateConditionBoolValue(bool bValue);
	void								Set_SelectedStateConditionIntValue(_int iValue);
	void								Set_SelectedStateConditionFloatValue(_float fValue);

	// Parameter setter
	void								Set_SelectedControllerParameterName(const string& strName);
	void								Set_SelectedControllerParameterType(ANIM_CONTROLLER_PARAM_TYPE eType);
	void								Set_SelectedControllerParameterDefaultBool(bool bValue);
	void								Set_SelectedControllerParameterDefaultInt(_int iValue);
	void								Set_SelectedControllerParameterDefaultFloat(_float fValue);

	// Action
	void                                Add_ActionStateMap();
	void                                Remove_SelectedActionStateMap();
	void								Sort_ActionStateMaps();
	void                                Select_ActionStateMap(_int iMapIndex);

	void                                Set_SelectedActionStateMapActionName(const string& strActionName);
	void                                Set_SelectedActionStateMapState(_int iStateIndex);


	// PoseLayer
	void								Set_SelectedPoseLayerRuleName(const string& strName);
	void								Set_SelectedPoseLayerRuleEnable(bool bEnable);
	void								Set_SelectedPoseLayerRuleMotionType(ANIM_CONTROLLER_MOTION_TYPE eMotionType);
	void								Set_SelectedPoseLayerRuleMotion(_int iMotionIndex);
	void								Set_SelectedPoseLayerRuleMaskRootBone(_int iBoneIndex);
	void								Set_SelectedPoseLayerRulePreviewRootBone();
	void								Set_SelectedPoseLayerRuleWeight(_float fWeight);
	void								Set_SelectedPoseLayerRuleLoop(bool bLoop);
	void								Set_SelectedPoseLayerRulePlayRate(_float fPlayRate);
	void								Set_SelectedPoseLayerRuleBlendType(ANIM_POSE_LAYER_BLEND_TYPE eBlendType);

	void								Set_SelectedPoseLayerConditionParam(_int iParamIndex);
	void								Set_SelectedPoseLayerConditionCompareType(ANIM_CONTROLLER_COMPARE_TYPE eCompareType);
	void								Set_SelectedPoseLayerConditionBoolValue(bool bValue);
	void								Set_SelectedPoseLayerConditionIntValue(_int iValue);
	void								Set_SelectedPoseLayerConditionFloatValue(_float fValue);

public:
	void								Move_SelectedSequenceStep(_int iTargetIndex);
	void								Move_SequenceStep(_int iFromIndex, _int iToIndex);

public:
	void								Initialize();
	void								Clear();

private:
	bool								Is_SpeedSectionRangeOverlapped(_int iIgnoreIndex, _uint iStartFrame, _uint iEndFrame) const;

	void								Sync_PreviewTimeline();			// 선택 clip 설정을 기준으로 timeline 길이/frame 수를 다시 계산한다.
	void								Sync_SequenceTimeline();		// 선택 sequence의 step 길이를 합산해 timeline을 갱신한다.

private:
	LOADED_DATA_CONTEXT					m_tLoadedData = {};	// 현재 로드된 모델/스켈레톤/clip/bone/notify 편집 데이터.
	SELECTION_CONTEXT					m_tSelection = {};	// 패널들이 공유하는 현재 선택 index 모음.
	PREVIEW_CONTEXT						m_tPreview = {};	// 재생/loop/playrate/root motion 같은 미리보기 UI 상태.
	TIMELINE_CONTEXT					m_tTimeline = {};	// 현재 frame/time과 선택 clip의 timeline 길이 정보.
	DIRTY_CONTEXT						m_tDirty = {};		// 편집 데이터가 runtime에 다시 반영되어야 하는지 표시하는 flag.

	EDITOR_PREVIEW_OBJECT_TYPE			m_ePreviewObjectType = EDITOR_PREVIEW_OBJECT_TYPE::NONE;

	unordered_set<_uint64>				m_RemovebleClipHashes;

public:
	virtual void Free() override;
};

NS_END

#endif