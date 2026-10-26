// 포트폴리오 코드 발췌: Update → 컨트롤러·시간·시퀀스 → 루트 모션·Window·Notify → 포즈·블렌딩·레이어
// 원본: Engine/Private/Animator.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 2266~2331행: Update
// 공동 수정: 본인 구현·연동 외 팀원의 수정 이력이 포함된 함수입니다.
void CAnimator::Update(_float fTimeDelta)
{
	Clear_TriggeredNotifies();
	m_ActiveCustomWindows.clear();

	if (m_bControllerEnabled)
		Update_Controller();

	if (m_bPlaying == false)
		return;

	if (m_bRawMode)
	{
		Update_RawTime(fTimeDelta);
		Update_RawPose();
		return;
	}

	if (m_iCurClipIndex < 0)
		return;

	const ANIM_CLIP_SETTING* pBeforeClip = Get_CurClip();

	_float fPrevFrame = 0.f;

	if (pBeforeClip != nullptr)
		fPrevFrame = Calc_SampleFramePosition(*pBeforeClip, m_fLocalTime);

	const _float fPrevLocalTime = m_fLocalTime;

	// 1. 현재 clip의 local time을 먼저 진행한다.
	const ANIM_TIME_ADVANCE_RESULT TimeResult = Update_Time(fTimeDelta);

	_bool bStepChanged = false;

	// 2. sequence라면 시간 진행 결과에 따라 step을 바꿀 수 있다.
	if (m_bSequenceMode)
		bStepChanged = Update_Sequence();

	const ANIM_CLIP_SETTING* pAfterClip = Get_CurClip();

	_float fCurFrame = 0.f;

	if (pAfterClip != nullptr)
		fCurFrame = Calc_SampleFramePosition(*pAfterClip, m_fLocalTime);

	const _float fCurLocalTime = m_fLocalTime;

	// 3. window, notify, root motion은 이전 위치와 현재 위치 사이를 기준으로 평가한다.
	Update_RootMotion(fPrevLocalTime, fCurLocalTime, bStepChanged);

	Update_GameplayFlags();

	if (!bStepChanged)
		Update_TriggeredNotifies(fPrevFrame, fCurFrame, TimeResult.bLooped);

	Update_CurPose();						// 전환 목표 포즈

	if (m_bStateBlending)
		Update_StateBlend(fTimeDelta);		// 이전 포즈에서 목표 포즈로 블렌드

	Update_PoseLayers(fTimeDelta);			// base pose 위에 overlay pose 합성

	Clear_ConsumedTriggers();

}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1275~1372행: Update_Controller
// 공동 수정: 본인 구현·연동 외 팀원의 수정 이력이 포함된 함수입니다.
void CAnimator::Update_Controller()
{
	if (!m_bControllerEnabled)
		return;

	Reset_ActionResult();

	// 1. 최초 진입
	if (m_iCurControllerStateIndex < 0)
	{
		const _int iEntry = m_EditData.Controller.iEntryStateIndex;

		if (iEntry >= 0)
			Enter_ControllerState(iEntry);

		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();

		return;
	}

	_uint64 iAcceptedActionHash = 0;
	const _int iActionState = Find_ActionRequestedControllerState(iAcceptedActionHash);

	if (iActionState >= 0)
	{
		const _int iPrevStateIndex = m_iCurControllerStateIndex;

		if (SUCCEEDED(Enter_ControllerState(iActionState)))
		{
			m_bActionAcceptedThisFrame = true;
			m_iAcceptedActionHash = iAcceptedActionHash;

			if (iPrevStateIndex >= 0 && iPrevStateIndex < m_EditData.Controller.States.size())
				m_iPrevControllerStateHash = m_EditData.Controller.States[iPrevStateIndex].iNameHash;

			if (m_iCurControllerStateIndex >= 0 && m_iCurControllerStateIndex < m_EditData.Controller.States.size())
				m_iCurControllerStateHash = m_EditData.Controller.States[m_iCurControllerStateIndex].iNameHash;

			m_bStateChangedThisFrame = iPrevStateIndex != m_iCurControllerStateIndex;
		}

		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();
		
		return;
	}

	// 2. 명시 요청 처리
	//    요청 상태는 RequestRule을 통해 현재 Clip Window를 검사한다.
	const _int iRequestedState = Find_RequestedControllerState();

	if (iRequestedState >= 0)
	{
		Enter_ControllerState(iRequestedState);

		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();

		return;
	}

	// 3. END step 중 현재 state 조건이 다시 만족되면 sequence를 처음부터 재시작한다.
	if (Try_RestartCurSequenceState())
	{
		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();

		return;
	}

	// 4. 현재 상태 유지
	//    일회성 Clip이 끝나기 전에 Idle/Move 같은 Auto 상태가 덮는 것을 막는다.

	if (Should_HoldCurState())
	{
		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();

		return;

	}

	// 5. 자동 상태 처리
	const _int iAutoState = Find_AutoControllerState();

	if (iAutoState >= 0)
		Enter_ControllerState(iAutoState);

	Clear_ConsumedTriggers();
	Clear_ControllerRequests();
	Clear_ActionRequests();
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1163~1212행: Enter_ControllerState
// 공동 수정: 본인 구현·연동 외 팀원의 수정 이력이 포함된 함수입니다.
HRESULT CAnimator::Enter_ControllerState(_int iStateIndex)
{
	if (iStateIndex < 0 || iStateIndex >= m_EditData.Controller.States.size())
		return E_FAIL;

	const ANIM_CONTROLLER_STATE& ToState = m_EditData.Controller.States[iStateIndex];

	if (iStateIndex == m_iCurControllerStateIndex && !ToState.bCanRestart)
		return S_OK;

	const _int iFromStateIndex = m_iCurControllerStateIndex;
	const _int iFromClipIndex = m_iCurClipIndex;
	const _float fFromLocalTime = m_fLocalTime;
	const POSE FromPose = m_CurPose;

	// 이전 state와 새 state의 clip 설정을 기준으로 전환 블렌드 시간을 계산한다.
	_float fBlendDuration = 0.f;

	if (iFromStateIndex >= 0 && iFromStateIndex < m_EditData.Controller.States.size())
	{
		const ANIM_CONTROLLER_STATE& FromState = m_EditData.Controller.States[iFromStateIndex];
		fBlendDuration = Get_StateBlendDuration(FromState, ToState);
	}

	HRESULT hr = E_FAIL;

	// State는 실제 재생 단위인 clip 또는 sequence로 풀어서 진입한다.
	if (ToState.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::CLIP)
	{
		if (ToState.iMotionIndex >= 0)
			hr = Play_Clip_ByIndex(ToState.iMotionIndex);
	}
	else if (ToState.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
	{
		if (ToState.iMotionIndex >= 0)
			hr = Play_Sequence_ByIndex(ToState.iMotionIndex);
	}

	if (SUCCEEDED(hr))
	{
		m_iCurControllerStateIndex = iStateIndex;

		if (iFromClipIndex >= 0 && fBlendDuration > 0.f)
			Begin_StateBlendFromPose(iFromClipIndex, FromPose, fBlendDuration);
		else
			Stop_StateBlend();
	}

	return hr;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1304~1316행: Enter_ControllerState
		if (SUCCEEDED(Enter_ControllerState(iActionState)))
		{
			m_bActionAcceptedThisFrame = true;
			m_iAcceptedActionHash = iAcceptedActionHash;

			if (iPrevStateIndex >= 0 && iPrevStateIndex < m_EditData.Controller.States.size())
				m_iPrevControllerStateHash = m_EditData.Controller.States[iPrevStateIndex].iNameHash;

			if (m_iCurControllerStateIndex >= 0 && m_iCurControllerStateIndex < m_EditData.Controller.States.size())
				m_iCurControllerStateHash = m_EditData.Controller.States[m_iCurControllerStateIndex].iNameHash;

			m_bStateChangedThisFrame = iPrevStateIndex != m_iCurControllerStateIndex;
		}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1044~1125행: Is_ConditionPassed
_bool CAnimator::Is_ConditionPassed(const ANIM_CONTROLLER_CONDITION& Condition) const
{
	if (Condition.iParameterIndex < 0 || Condition.iParameterIndex >= m_ControllerParamValues.size())
		return false;

	const CONTROLLER_PARAM_VALUE& Value = m_ControllerParamValues[Condition.iParameterIndex];

	switch (Condition.eCompareType)
	{
	case ANIM_CONTROLLER_COMPARE_TYPE::True:
		return Value.eType == ANIM_CONTROLLER_PARAM_TYPE::BOOL && Value.bBool == true;

	case ANIM_CONTROLLER_COMPARE_TYPE::False:
		return Value.eType == ANIM_CONTROLLER_PARAM_TYPE::BOOL && Value.bBool == false;

	case ANIM_CONTROLLER_COMPARE_TYPE::TRIGGERED:
		return Value.eType == ANIM_CONTROLLER_PARAM_TYPE::TRIGGER && Value.bTriggered;

	case ANIM_CONTROLLER_COMPARE_TYPE::EQUAL:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt == Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return fabsf(Value.fFloat - Condition.fFloatValue) < 0.0001f;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::BOOL)
			return Value.bBool == Condition.bBoolValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::NOT_EQUAL:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt != Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return fabsf(Value.fFloat - Condition.fFloatValue) >= 0.0001f;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::BOOL)
			return Value.bBool != Condition.bBoolValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::GREATER:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt > Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return Value.fFloat > Condition.fFloatValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::GREATER_EQUAL:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt >= Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return Value.fFloat >= Condition.fFloatValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::LESS:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt < Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return Value.fFloat < Condition.fFloatValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::LESS_EQUAL:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt <= Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return Value.fFloat <= Condition.fFloatValue;

		return false;

	default:
		return false;
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1127~1136행: Is_StateConditionPassed
_bool CAnimator::Is_StateConditionPassed(const ANIM_CONTROLLER_STATE& State) const
{
	for (const auto& Condition : State.EnterConditions)
	{
		if (!Is_ConditionPassed(Condition))
			return false;
	}

	return true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1138~1161행: Can_EnterRequestedState
// 공동 수정: 본인 구현·연동 외 팀원의 수정 이력이 포함된 함수입니다.
_bool CAnimator::Can_EnterRequestedState(const ANIM_CONTROLLER_STATE& State) const
{
	switch (State.eRequestRule)
	{
	case ANIM_STATE_REQUEST_RULE::ALWAYS:
		return true;

	case ANIM_STATE_REQUEST_RULE::COMBO_WINDOW:
		return Can_ComboInput();

	case ANIM_STATE_REQUEST_RULE::CANCEL_WINDOW:
		return Can_Cancel();

	case ANIM_STATE_REQUEST_RULE::GUARD_INPUT_WINDOW:
		return Can_Cancel() || Is_InWindow(ANIM_WINDOW_TYPE::GUARD_INPUT);

	case ANIM_STATE_REQUEST_RULE::DASH_INPUT_WINDOW:
		return Can_Cancel() || Is_InWindow(ANIM_WINDOW_TYPE::DASH_INPUT);

	case ANIM_STATE_REQUEST_RULE::NEVER:
	default:
		return false;
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2056~2074행: Update_Time
CAnimator::ANIM_TIME_ADVANCE_RESULT CAnimator::Update_Time(_float fTimeDelta)
{
	ANIM_TIME_ADVANCE_RESULT Result = {};

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return Result;

	Result = Advance_ClipLocalTime(*pClip, fTimeDelta, Calc_EffectivePlayRate(*pClip), m_bLoop, m_fLocalTime);

	if (Result.bEnded)
	{
		m_bPlaying = false;
		m_bEnded = true;
	}

	return Result;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1830~1860행: Advance_ClipLocalTime
CAnimator::ANIM_TIME_ADVANCE_RESULT CAnimator::Advance_ClipLocalTime(const ANIM_CLIP_SETTING& Clip, _float fTimeDelta, _float fPlayRate, _bool bLoop, _float& InOutLocalTime) const
{
	ANIM_TIME_ADVANCE_RESULT Result = {};

	const _float fDuration = Calc_ClipDuration(Clip);
	if (fDuration <= 0.f)
	{
		InOutLocalTime = 0.f;
		Result.bEnded = true;
		return Result;
	}

	if (fTimeDelta > 0.f && fPlayRate > 0.f)
		InOutLocalTime += fTimeDelta * 1000.f * fPlayRate;

	if (InOutLocalTime >= fDuration)
	{
		if (bLoop)
		{
			InOutLocalTime = fmodf(InOutLocalTime, fDuration);
			Result.bLooped = true;
		}
		else
		{
			InOutLocalTime = fDuration;
			Result.bEnded = true;
		}
	}

	return Result;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1980~1998행: Calc_EffectivePlayRate
_float CAnimator::Calc_EffectivePlayRate(const ANIM_CLIP_SETTING& Clip) const
{
	_float fRate = m_fPlayRate;

	const _uint iFrame = Get_LocalFrame();

	for (const ANIM_SPEED_SECTION& Section : Clip.SpeedSections)
	{
		if (iFrame < Section.iStartFrame || iFrame > Section.iEndFrame)
			continue;

		if (Section.fPlayRate > 0.f)
			fRate = Section.fPlayRate;

		break;
	}

	return fRate;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2000~2017행: Calc_SampleLocalTime
_float CAnimator::Calc_SampleLocalTime(const ANIM_CLIP_SETTING& Clip, _float fElapsedLocalTime) const
{
	const _float fDuration = Calc_ClipDuration(Clip);

	if (fDuration <= 0.f)
		return 0.f;

	if (fElapsedLocalTime < 0.f)
		fElapsedLocalTime = 0.f;

	if (fElapsedLocalTime > fDuration)
		fElapsedLocalTime = fDuration;

	if (Clip.bReverse)
		return fDuration - fElapsedLocalTime;

	return fElapsedLocalTime;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2019~2042행: Calc_SampleFramePosition
_float CAnimator::Calc_SampleFramePosition(const ANIM_CLIP_SETTING& Clip, _float fElapsedLocalTime) const
{
	const _uint iFrameCount = Calc_ClipFrameCount(Clip);

	if (iFrameCount <= 1)
		return 0.f;

	const _float fDuration = Calc_ClipDuration(Clip);

	if (fDuration <= 0.f)
		return 0.f;

	const _float fSampleLocalTime = Calc_SampleLocalTime(Clip, fElapsedLocalTime);

	_float fRatio = fSampleLocalTime / fDuration;

	if (fRatio < 0.f)
		fRatio = 0.f;

	if (fRatio > 1.f)
		fRatio = 1.f;

	return fRatio * static_cast<_float>(iFrameCount - 1);
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1608~1685행: Update_Sequence
// 공동 수정: 본인 구현·연동 외 팀원의 수정 이력이 포함된 함수입니다.
_bool CAnimator::Update_Sequence()
{
	if (!m_bSequenceMode)
		return false;

	const ANIM_SEQUENCE* pSequence = Get_CurSequence();
	const ANIM_SEQUENCE_STEP* pStep = Get_CurSequenceStep();

	if (pSequence == nullptr || pStep == nullptr)
	{
		Stop_Sequence();
		return false;
	}

	// 외부에서 End 요청이 들어오면 현재 step 종류와 무관하게 가장 가까운 End step으로 보낸다.
	if (m_bSequenceEndRequested)
	{
		m_bSequenceEndRequested = false;

		const _int iEndStep = Find_SequenceEndStepIndex();

		if (iEndStep >= 0)
			return SUCCEEDED(Enter_SequenceStep(static_cast<_uint>(iEndStep)));

		Stop_Sequence();

		return false;
	}

	// Loop step은 hold 조건이 살아 있으면 계속 반복하고, 끊기면 End step으로 넘어가는 로직에서 
	// 다음 스텝으로 넘어가도록 변경함
	if (pStep->eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP)
	{
		/*if (!Is_SequenceStepHold(*pStep))
		{
			const _int iEndStep = Find_SequenceEndStepIndex();

			if (iEndStep >= 0)
				return SUCCEEDED(Enter_SequenceStep(static_cast<_uint>(iEndStep)));

			Stop_Sequence();

			return false;
		}*/

		if (!Is_SequenceStepHold(*pStep))
		{
			const _int iNextStep = m_iCurSequenceStepIndex + 1;

			if (iNextStep < static_cast<_int>(pSequence->Steps.size()))
				return SUCCEEDED(Enter_SequenceStep(static_cast<_uint>(iNextStep)));

			Stop_Sequence();
			return false;
		}
		return false;
	}

	if (!m_bEnded)
		return false;

	// Start/Active 같은 일반 step은 clip이 끝났을 때 다음 step으로 진행한다.
	if (pStep->eStepType == ANIM_SEQUENCE_STEP_TYPE::END)
	{
		Stop_Sequence();
		return false;
	}

	const _int iNextStep = m_iCurSequenceStepIndex + 1;

	if (iNextStep >= pSequence->Steps.size())
	{
		Stop_Sequence();
		return false;
	}

	return SUCCEEDED(Enter_SequenceStep(static_cast<_uint>(iNextStep)));
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1558~1606행: Enter_SequenceStep
HRESULT CAnimator::Enter_SequenceStep(_uint iStepIndex)
{
	const ANIM_SEQUENCE* pSequence = Get_CurSequence();
	if (pSequence == nullptr)
		return E_FAIL;

	if (iStepIndex >= pSequence->Steps.size())
		return E_FAIL;

	const ANIM_SEQUENCE_STEP& Step = pSequence->Steps[iStepIndex];

	if (Step.iClipIndex < 0 || Step.iClipIndex >= m_EditData.ClipSettings.size())
		return E_FAIL;

	const _int iFromClipIndex = m_iCurClipIndex;
	const _float fFromLocalTime = m_fLocalTime;
	const _bool bInternalStepChange = m_iCurSequenceStepIndex >= 0;
	ANIM_SEQUENCE_STEP FromStep = {};

	if (bInternalStepChange)
		FromStep = *Get_CurSequenceStep();

	m_iCurSequenceStepIndex = static_cast<_int>(iStepIndex);

	if (FAILED(Play_Clip_ByIndex_Internal(static_cast<_uint>(Step.iClipIndex), true)))
		return E_FAIL;

	if (Step.eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP)
		m_bLoop = true;
	else
		m_bLoop = false;

	if (bInternalStepChange &&
		iFromClipIndex >= 0 &&
		iFromClipIndex != Step.iClipIndex &&
		iFromClipIndex < m_EditData.ClipSettings.size() &&
		Should_BlendSequenceStep(*pSequence, FromStep, Step))
	{
		const ANIM_CLIP_SETTING& FromClip = m_EditData.ClipSettings[iFromClipIndex];
		const ANIM_CLIP_SETTING& ToClip = m_EditData.ClipSettings[Step.iClipIndex];

		const _float fBlendDuration = Get_SequenceStepBlendDuration(*pSequence, FromClip, ToClip, FromStep, Step);

		if (fBlendDuration > 0.f)
			Begin_StateBlend(iFromClipIndex, fFromLocalTime, fBlendDuration);
	}

	return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1520~1556행: Play_Clip_ByIndex_Internal
HRESULT CAnimator::Play_Clip_ByIndex_Internal(_uint iClipIndex, _bool bKeepSequenceState)
{
	if (iClipIndex >= m_EditData.ClipSettings.size())
		return E_FAIL;

	if (!bKeepSequenceState)
	{
		m_bSequenceMode = false;
		m_iCurSequeneIndex = -1;
		m_iCurSequenceStepIndex = -1;
		m_bSequenceEndRequested = false;
	}

	m_bRawMode = false;
	m_iCurRawAnimationIndex = -1;
	m_fRawTime = 0.f;
	m_bRawLoop = false;

	m_iCurClipIndex = static_cast<_int>(iClipIndex);
	m_fLocalTime = 0.f;

	const ANIM_CLIP_SETTING& Clip = m_EditData.ClipSettings[iClipIndex];

	m_bLoop = Clip.bLoop;
	m_fPlayRate = Clip.fPlayRate;
	m_bUseRootMotion = Clip.bUseRootMotion;

	m_bPlaying = true;
	m_bEnded = false;

	Clear_RootMotionDelta();
	Update_GameplayFlags();
	Clear_TriggeredNotifies();
	Update_CurPose();

	return S_OK;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2099~2151행: Update_GameplayFlags
void CAnimator::Update_GameplayFlags()
{
	m_ActiveCustomWindows.clear();

	m_GameplayFlags = {};

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();
	if (pClip == nullptr)
		return;

	const _uint iFrame = Get_LocalFrame();

	for (const ANIM_WINDOW& Window : pClip->Windows)
	{
		if (iFrame < Window.iStartFrame || iFrame > Window.iEndFrame)
			continue;

		switch (Window.eType)
		{
		case ANIM_WINDOW_TYPE::MOVE_LOCK:
			m_GameplayFlags.bCanMove = false;
			break;
		case ANIM_WINDOW_TYPE::ROTATE_LOCK:
			m_GameplayFlags.bCanRotate = false;
			break;
		case ANIM_WINDOW_TYPE::TARGET_TRACKING_LOCK:
			m_GameplayFlags.bCanTargetTrack = false;
			break;
		case ANIM_WINDOW_TYPE::HIT_ACTIVE:
			m_GameplayFlags.bHitActive = true;
			break;
		case ANIM_WINDOW_TYPE::INVINCIBLE:
			m_GameplayFlags.bInvincible = true;
			break;
		case ANIM_WINDOW_TYPE::SUPER_ARMOR:
			m_GameplayFlags.bSuperArmor = true;
			break;
		case ANIM_WINDOW_TYPE::GUARD_ACTIVE:
			m_GameplayFlags.bGuardActive = true;
			break;
		case ANIM_WINDOW_TYPE::PARRY_ACTIVE:
			m_GameplayFlags.bParryActive = true;
			break;
		case ANIM_WINDOW_TYPE::DAMAGE_REDUCE:
			if (Window.fValue > 0.f)
				m_GameplayFlags.fDamageRate = Window.fValue;
			break;
		case ANIM_WINDOW_TYPE::CUSTOM:
			m_ActiveCustomWindows.push_back(&Window);
			continue;
		}
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2155~2221행: Update_TriggeredNotifies
void CAnimator::Update_TriggeredNotifies(_float fPrevFrame, _float fCurFrame, _bool bLooped)
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr || pClip->Notifies.empty())
		return;

	const _uint iFrameCount = Calc_ClipFrameCount(*pClip);

	if (iFrameCount <= 0)
		return;

	const _float fLastFrame = static_cast<_float>(iFrameCount - 1);

	if (fabsf(fPrevFrame - fCurFrame) < 0.0001f && !bLooped)
		return;

	if (!bLooped)
	{
		if (fCurFrame > fPrevFrame)
		{
			// 정방향 Sample 진행
			for (const ANIM_NOTIFY& Notify : pClip->Notifies)
			{
				const _float fNotifyFrame = static_cast<_float>(Notify.iFrame);

				if (fNotifyFrame > fPrevFrame && fNotifyFrame <= fCurFrame)
					m_TriggeredNotifies.push_back(&Notify);
			}
		}
		else
		{
			// 역방향 Sample 진행
			for (const ANIM_NOTIFY& Notify : pClip->Notifies)
			{
				const _float fNotifyFrame = static_cast<_float>(Notify.iFrame);

				if (fNotifyFrame < fPrevFrame && fNotifyFrame >= fCurFrame)
					m_TriggeredNotifies.push_back(&Notify);
			}
		}
		return;
	}

	if (!pClip->bReverse)
	{
		// 정방향 loop : Prev -> Last, 0 -> Cur
		for (const ANIM_NOTIFY& Notify : pClip->Notifies)
		{
			const _float fNotifyFrame = static_cast<_float>(Notify.iFrame);

			if ((fNotifyFrame > fPrevFrame && fNotifyFrame <= fLastFrame) || (fNotifyFrame >= 0.f && Notify.iFrame <= fCurFrame))
				m_TriggeredNotifies.push_back(&Notify);
		} 
	}
	else
	{
		// 역방향 loop : Prev -> 0, Last -> Cur
		for (const ANIM_NOTIFY& Notify : pClip->Notifies)
		{
			const _float fNotifyFrame = static_cast<_float>(Notify.iFrame);

			if ((fNotifyFrame < fPrevFrame && fNotifyFrame >= 0.f) || (fNotifyFrame <= fLastFrame && Notify.iFrame >= fCurFrame))
				m_TriggeredNotifies.push_back(&Notify);
		}
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 2078~2095행: Update_CurPose
void CAnimator::Update_CurPose()
{
	if (m_bRawMode)
	{
		Update_RawPose();
		return;
	}

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();
	if (pClip == nullptr)
	{
		m_CurPose = m_BasePose;
		return;
	}

	if (!Sample_ClipPose(*pClip, m_fLocalTime, m_CurPose, m_KeyFrameIndices, true))
		m_CurPose = m_BasePose;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1862~1880행: Sample_ClipPose
_bool CAnimator::Sample_ClipPose(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, POSE& OutPose, vector<KeyFrameIndices>& KeyFrameCache, _bool bApplyRootMotionStrip) const
{
	if (!Is_ValidRawAnimationIndex(Clip.iRawAnimationIndex))
		return false;

	CAnimation* pAnimation = m_Animations[Clip.iRawAnimationIndex];
	OutPose = m_BasePose;

	if (KeyFrameCache.size() != OutPose.size())
		KeyFrameCache.resize(OutPose.size());

	const _float fRawTime = Calc_LocalToRawTime(Clip, fLocalTime);
	pAnimation->Get_Pose(OutPose, fRawTime, KeyFrameCache);

	if (bApplyRootMotionStrip && m_iRootBoneIndex >= 0)
		Strip_RootMotion(OutPose, Clip);

	return true;
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 1899~1904행: Sample_Pose
_bool CAnimator::Sample_Pose(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, POSE& OutPose) const
{
	vector<KeyFrameIndices> TempKeyFrameIndices;

	return Sample_ClipPose(Clip, fLocalTime, OutPose, TempKeyFrameIndices, false);
}
