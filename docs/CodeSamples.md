# 공개 코드 샘플 구성안

현재는 발췌 대상 선정안입니다. 아래 파일은 팀 원본에 있으며, 아직 이 저장소에 복사하지 않았습니다. 전체 파일을 가져오기보다 책임이 드러나는 핵심 함수와 필요한 자료구조를 묶는 구성을 권장합니다.

구현은 `CodeSamples/Private`, 헤더는 `CodeSamples/Public` 아래에 같은 기능별 폴더를 사용합니다. 함수 본문은 원본을 유지하고, 생략한 부분은 `[중략]`과 원본 경로로 표시합니다.

| 폴더 | 우선 공개할 원본 | 보여줄 내용 |
| --- | --- | --- |
| `01_AnimationEditor` | `Animation_Editor/Private/Animation_EditorContext.cpp`, `TimelinePanel.cpp`, `Preview_Manager.cpp` | 클립 설정 편집·저장, 프레임 구간과 이벤트, 런타임과 같은 데이터로 프리뷰 |
| `02_Animator` | `Engine/Private/Animator.cpp`, `Animator_RootMotion.cpp`, `Animator_PoseBlend.cpp`, `Animator_PoseLayer.cpp` | 시퀀스·Window·Notify, 루트 이동 추출·제거, 포즈 블렌딩과 본 마스크·가산 합성 |
| `03_UIEditor` | `UI_Editor/Private/UI_EditorContext.cpp`, `UIHierarchyPanel.cpp`, `UIAnimationPanel.cpp` | 에셋 편집, Undo/Redo, 계층과 애니메이션 트랙 |
| `04_UIRuntime` | `Engine/Private/UI_Manager.cpp`, `UIAssetValidator.cpp`, `UIAssetSerializer.cpp`, `UIAnimationPlayer.cpp` | 핸들·수명·지연 명령·입력 소유권, 에셋 검증, 키프레임 평가 |
| `05_GameUI` | `GameCore/Private/UIEquipmentController.cpp`, `EquipmentViewModel.cpp`, `UIMonsterHUDController.cpp`, `UINPCDialogueController.cpp` | UI 명령과 게임 데이터 연결, 표시 데이터 분리, 개체별 HUD·대화 이벤트 |
| `06_GPUSkinning` | `Engine/Private/GPUAnimationSystem.cpp`와 관련 Compute Shader | 작업 등록·일괄 Dispatch, 전역 버퍼 범위 할당·반환, 최종 본 행렬 계산 |

## 먼저 보여줄 함수 흐름

1. **Animator:** `Update` → 시퀀스 진행 → `Update_GameplayFlags` / `Update_TriggeredNotifies` → 루트 모션·포즈 갱신. 내부 전체를 공개하기보다 흐름과 대표 계산을 나눕니다.
2. **루트 모션:** `Update_RootMotion`과 본 계층 계산·렌더 포즈의 이동 제거. 플레이어 이동과의 연결은 공용 코드 중 본인 수정 구간만 별도로 표시합니다.
3. **UI 편집 이력:** `Begin_AssetEdit` → `End_AssetEdit` → `Undo_Asset` / `Redo_Asset`. 연속 드래그를 하나의 작업으로 묶는 기준을 설명합니다.
4. **UI 수명·입력:** 핸들 유효성 검사, 생성·해제 요청과 `Flush_PendingCommands`, `Flush_UIInputOwnerCommands`를 중심으로 발췌합니다. 에셋별 요소 생성의 반복 코드는 중략합니다.
5. **게임 UI:** Controller의 명령 처리와 ViewModel 갱신, 화면 적용을 한 흐름으로 보여줍니다. 장비·대화·HUD 중 대표 사례를 우선 선택합니다.
6. **GPU 계산:** `Register_ModelSkinningJob` → `Flush_SkinningJobs` → `Dispatch_BatchedSkinningJobs`. 행렬 계산용 셰이더와 입출력 구조체를 같이 보여줍니다.

## 헤더와 공동 수정 표시

- 각 cpp에 대응하는 원본 헤더에서 인터페이스와 필요한 구조체를 함께 제공합니다.
- `Animator.h`, `AnimationData.h`, `UI_RuntimeData.h`, `UI_AssetData.h`처럼 큰 공통 자료형은 공개 함수의 이해에 필요한 범위를 선별합니다.
- `Player.cpp`, `Model.cpp`, `Renderer.cpp`, `SoundManager.cpp`, `Inventory.cpp` 전체는 복사하지 않습니다. 본인 기여와 팀원 구현이 섞여 있으므로 관련 구간만 표시합니다.
- 시연 영상에 나타나는 팀원의 모델·맵·이펙트·전투 구현을 자신의 코드 샘플 기여와 혼동하지 않도록 설명합니다.
- 원본의 설계 문서는 예전 구조·향후 목표가 섞여 있어 통째로 공개하기보다 현재 구현과 일치하는 부분을 요약합니다.

전체 소스·리소스·외부 라이브러리·실행 파일은 포함하지 않으며, 발췌 저장소는 단독 빌드를 제공하지 않는 형태로 구성합니다.
