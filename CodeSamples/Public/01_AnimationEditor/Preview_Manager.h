#pragma once
#include "Animation_Defines.h"
#include "Base.h"
#include "Animation_EditorContext.h"

#ifdef _DEBUG

NS_BEGIN(Engine)
class CGameInstance;
NS_END


NS_BEGIN(Anim_Editor)

class CLevel_Preview;
class CAnimation_EditorContext;

/*
 * Preview_Manager 현재 역할
 * - Animation_EditorContext와 실제 preview runtime 사이를 동기화한다.
 * - Context의 선택/재생/seek/dirty 상태를 Level_Preview -> PreviewObject -> Animator에 적용한다.
 * - Animator가 진행한 현재 local time을 다시 Context timeline에 반영한다.
 * - preview camera와 render target도 관리한다.
 *
 * 주의
 * - raw animation 시간 계산, pose sampling, loop/end 정책은 여기서 직접 처리하지 않는다.
 * - 재생 정책은 Animator, pose 확인용 sampling은 PreviewObject가 맡는다.
 */
class CPreview_Manager final : public CBase
{
private:
    typedef struct tagSequenceFrameInfo
    {
        _int iStepIndex = -1;
        _int iClipIndex = -1;

        _uint iStepStartFrame = 0;
        _uint iStepEndFrame = 0;

        _uint iLocalFrame = 0;
        _float fLocalTime = 0.f;
    }SEQUENCE_FRAME_INFO;

private:
    CPreview_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPreview_Manager() = default;

public:
    HRESULT                 Set_Model(const _wstring& strModelName);	                        // 미리보기할 모델을 교체한다.
    void                    Set_ClipSettings(const vector<ANIM_CLIP_SETTING>& ClipSettings);	// Context/파일에서 만든 clip 설정을 preview runtime에 전달한다.

    HRESULT                 Set_PlayerPartsModel(const _wstring& strFolderName, const _wstring& strMasterFolderName, const vector<_wstring>& PartModelNames);

    HRESULT                 Set_RawAnimationSource(const _wstring& strFolderName, const _wstring& strAnimationBaseName, ANIMATION_TIME_UNIT eTimeUnit);

public:
    void                    Set_AnimatorBool(const _wstring& strName, bool bValue);
    void                    Set_AnimatorInt(const _wstring& strName, _int iValue);
    void                    Set_AnimatorFloat(const _wstring& strName, _float fValue);
    void                    Set_AnimatorTrigger(const _wstring& strName);

    bool                    Is_PoseLayerTestMode() const { return m_iPoseLayerPreviewBaseClipIndex >= 0; }

    void                    Preview_BaseLoop(bool bLoop);
    void                    Preview_PlayBase(bool bPlay);
    void                    Preview_SetBaseClip(_int iClipIndex);
    void                    Preview_EnsureBaseClip(_int iClipIndex);
    void                    Preview_ClearPoseLayerTest();

    void                    Start_SelectedSequencePreview();
    void                    Restart_SequencePreview();

public:
    HRESULT                 Initialize();
    void                    Update(_float fTimeDelta);		                                    // Context 상태를 preview runtime에 적용하고 timeline을 되돌려 받는다.
    HRESULT                 Render_Preview();			                                        // preview render target에 현재 preview scene을 렌더링한다.

public:
    HRESULT                 Resize(_uint iWidth, _uint iHeight);
    void                    Ready_Camera();
    void                    Update_Camera(_float fTimeDelta);
    void                    Clear_Model();

    void                    Build_BoneScreenPoints(const ImVec2& vImageMin, const ImVec2& vImageSize, vector<BONE_SCREEN_POINT>& OutPoints) const;
    bool                    Build_SelectedNotifySlotWorldMatrix(_float4x4& OutWorld) const;

    void                    Request_SequenceEnd();
    
    bool                    Has_SelectedSequenceLoopHoldConditions() const;

    void                    Force_SelectedSequenceLoopHoldConditionsTrue();

private:
    bool                    Resolve_SequenceFrame(_uint iGlobalFrame, SEQUENCE_FRAME_INFO& OutInfo) const;
    void                    Update_SequencePreview(_float fTimeDelta);
    void                    Apply_EditData_ToPreview();

    void                    Reset_SequencePreviewRuntime();
    void                    Set_SequenceTimelineFromRuntime(_uint iGlobalFrame);
    _bool                   Calc_SequenceGlobalFrameFromRuntime(_uint& OutGlobalFrame) const;

    void                    Reapply_EditData_AfterDirty(const LOADED_DATA_CONTEXT& tLoadedData, const SELECTION_CONTEXT& tSelection, const TIMELINE_CONTEXT& tTimeline);

    void                    Apply_HoldConditionPassValue(const ANIM_CONTROLLER_PARAMETER& Param, const ANIM_CONTROLLER_CONDITION& Condition);

private:
    // 카메라
    void                    Reset_Camera();
    void                    Reset_ModelTransform();
    void                    Reset_CameraAndModel();

private:
    ID3D11Device*           m_pDevice = { nullptr };		        // preview render target/resource 생성에 사용하는 device.
    ID3D11DeviceContext*    m_pContext = { nullptr };	            // preview rendering에 사용하는 immediate context.
    CGameInstance*          m_pGameInstance = { nullptr };	        // 엔진 facade. render target, transform, command 실행에 사용한다.

    CLevel_Preview*         m_pPreview_Level = { nullptr };	        // preview scene과 PreviewObject를 소유하는 level.

    ID3D11Texture2D*        m_pPreviewDepthTexture = { nullptr };	// preview MRT용 depth texture.
    ID3D11DepthStencilView* m_pPreviewDSV = { nullptr };	        // preview MRT에 연결되는 depth stencil view.

    CAnimation_EditorContext* m_pEditorContext = { nullptr };	    // 패널들이 수정한 editor state를 읽는 진입점.

private:
    PREVIEW_MODEL_TYPE      m_ePreviewModelType = PREVIEW_MODEL_TYPE::SINGLE_MODEL; // 현재 preview object 종류.

private:
    _uint                   m_iWidth = {};			                // 현재 preview render target 너비.
    _uint                   m_iHeight = {};			                // 현재 preview render target 높이.

    _int                    m_iLastClipIndex = -1;		            // 마지막으로 runtime에 적용한 선택 clip index.
    _float                  m_fLastAppliedTime = {};		        // 마지막으로 runtime에 적용/반영한 timeline local time.
    _float                  m_fLastClipDuration = -1.f;	            // 마지막으로 runtime에 적용한 clip duration.
    _int                    m_iPoseLayerPreviewBaseClipIndex = -1; // PoseLayer 테스트용 base clip index. -1이면 일반 preview 모드.

    _float3                 m_vCameraTarget = { 20.f, 110.f, 0.f };	// orbit/free camera가 바라보는 preview 중심점.

    _float                  m_fYaw = 0.f;			                // preview camera 좌우 회전값.
    _float                  m_fPitch = XMConvertToRadians(-5.f);	// preview camera 상하 회전값.
    _float                  m_fDistance = 380.f;		            // target으로부터 camera까지의 거리.

    _float                  m_fRotateSpeed = 0.005f;            // preview camera 회전 속도.
    _float                  m_fZoomSpeed = 30.f;                // preview camera zoom 속도.
    _float                  m_fPanSpeed = 0.0025f;              // preview camera pan 속도.
    _float                  m_fMoveSpeed = 500.f;               // preview camera keyboard 이동 속도.

    _bool                   m_bTab = { false };                 // 키보드 멈춰

private:
    _int                    m_iLastSequenceIndex = -1;          // 마지막으로 preview한 sequence index.
    _float                  m_fSequenceFrameCursor = 0.f;       // sequence preview용 전역 frame cursor.

    _bool                   m_bSequenceRuntimeStarted = { false };
    _bool                   m_bSequenceWasPlaying = { false };
    _bool                   m_bSequencePreviewEnded = { false };

    _int                    m_iAppliedRootMotionBoneIndex = { -2 };

public:
    static  CPreview_Manager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END

#endif