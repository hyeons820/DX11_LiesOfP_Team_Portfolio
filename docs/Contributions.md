# 담당 범위와 확인 근거

## 확인 기준

원본의 `Animation/`, `UI/`에 있는 작업 기록·사용 설명서와 현재 C++ 코드를 우선 대조했습니다. Git의 `hyeons820` 작성 커밋과 함수별 변경 내역은 기여 범위를 확인하는 보조 근거로 사용했습니다.

검토 기준은 2026-10-26 로컬 원본의 `New_UIKHS` 브랜치, 커밋 `ae8c6b53`입니다. 로컬 `origin/NewDevelop`의 `15c062a6`과 비교했을 때 Engine·GameCore·Animation_Editor·UI_Editor 소스 차이는 없었습니다. 원격 저장소의 현재 최신 상태를 확인한 결과는 아닙니다.

파일의 마지막 수정자나 변경 줄 수만으로 단독 저작 여부를 판단하지 않았습니다. 특히 공용 클래스의 수정은 기능별 기여로 구분했습니다.

## 주 담당 구현

| 기능 | 원본 기록·코드 | 대표 커밋 |
| --- | --- | --- |
| 애니메이션 클립·Window·Notify 편집 | `AnimationEditor_UserGuide.md`, `AnimationEditor_PatchNotes.html`, `TimelinePanel.cpp`, `Animation_EditorContext.cpp` | `489b66c9`, `9fc1d8db`, `8501d214` |
| 시퀀스·컨트롤러와 런타임 연결 | `AnimationControllerWorkflowPlan.md`, `Animator.cpp`, `AnimControllerPanel.cpp`, `SequencePanel.cpp` | `6ea795d8`, `d6ca9440`, `c5089f2f` |
| 루트 모션·포즈 레이어·블렌딩 | `RootMotion_변경사항.md`, `Animator_RootMotion.cpp`, `Animator_PoseBlend.cpp`, `Animator_PoseLayer.cpp` | `4618c5ec`, `c1960d46`, `185f6e23` |
| 프리뷰와 편집 편의성 | `AnimationEditor_PatchNotes.html`, `Preview_Manager.cpp`, `Animation_EditorApp.cpp` | `a3f7b776`, `5ff3c900`, `e0081079` |
| GPU 최종 본 행렬 계산 | `Animation_GPU_Skinning_Phase1_2_Record.html`, `GPUAnimationSystem.cpp`, `Model.cpp` | `2382ddf8`, `2ebac669`, `1162396e`의 단계별 변경과 현재 코드 대조 |
| UI 에셋·런타임 구조 | `Final_UI_Runtime_Editor_Architecture.html`, `UI_Manager.cpp`, `UIAssetSerializer.cpp`, `UIAssetValidator.cpp`, `UIAssetConverter.cpp` | `6eb9d186`, `4cf2d5d5`, `2335a9a7`, `da203cd8` |
| UI 편집 도구 | `UI_EditorContext.cpp`, `UICanvasPanel.cpp`, `UIInspectorPanel.cpp`, `UIHierarchyPanel.cpp` | `ca28d9c4`, `9c326494`, `91914c0f`, `db2c23a7` |
| UI 애니메이션 | `UIAnimationPanel.cpp`, `UIAnimationPlayer.cpp` | `bf4ef958` |
| HUD·메뉴·로딩 | `UIPlayerHUDController.cpp`, `UIMonsterHUDController.cpp`, `UIBossHUDController.cpp`, `UIESCMenuController.cpp`, `UILoadingController.cpp` | `20b4ec22`, `9574bc89`, `6e71de0b`, `42d0c15b`, `e7f7e2a9` |
| 장비·코스튬・미리보기 | `UIEquipmentController.cpp`, `EquipmentViewModel.cpp`, `EquipmentPreviewCharacter.cpp` | `c3f62236`, `9b3a6c09`, `bf4ef958`, `dca5ccac` |
| 대화·상점·별바라기 | `UINPCDialogueController.cpp`, `NPCDialogueRepository.cpp`, `UIShopController.cpp`, `UIStargazerTravelController.cpp` | `f864e554`, `73d69d8b`, `a70e79a2`, `c3e7d1c0` |

대표 커밋은 해당 작업을 추적하기 위한 출발점이며, 해당 기능의 모든 코드가 한 커밋에 포함되어 있다는 의미는 아닙니다.

## 협업 코드에서의 기여

- **Animator:** 사용자가 담당한 핵심 시스템입니다. 현재 파일에는 팀원의 연동·수정도 포함되어 있으므로, 공개 발췌 시 함수별 수정 이력을 확인합니다.
- **Inventory:** UI 연결과 일부 조건 수정입니다. `9b3a6c09`에서 리전 암 강화 비용 인자, 보유·강화 여부와 에르고 검사, 성공 후 비용 차감, 해제 조건 수정이 확인됩니다. 인벤토리 전체 구현으로 소개하지 않습니다.
- **Player / Model / Renderer / GameInstance:** 애니메이터·루트 모션·UI·GPU 본 행렬을 연결한 부분입니다. 각 클래스 전체를 담당 코드로 취급하지 않습니다.
- **SoundManager:** `3c08199c`, `1ffc5be8`에서 사운드 시퀀스 확장과 연동 작업이 확인됩니다. 기존 사운드 매니저는 여러 팀원의 코드이므로 추가한 함수·수정 구간만 발췌합니다.
- **Shader_Resolver:** `c4bfd0e1`은 두 줄 제거 변경입니다. 별도의 셰이더 시스템 구현 성과로 내세우지 않습니다.
- **Effect Editor / 맵·AI·전투 전체:** 이번 담당 범위로 확인하지 않았으며 README에서 개인 성과로 기재하지 않았습니다.

## 과정 문서를 읽을 때 구분한 내용

### 실제 구현과 연결된 기록

- `AnimationEditor_UserGuide.md`와 `AnimationEditor_PatchNotes.html`: 원본 참조형 클립 설정, 시퀀스 프리뷰, 루트 모션 확인, Notify 슬롯 기즈모.
- `RootMotion_변경사항.md`: 모델 공간·부모 계층·스케일 문제, 이동의 중복 적용과 메시 드리프트를 수정한 과정. 현재 일부 함수명은 당시 기록과 달라 본문 설명은 현재 코드에 맞췄습니다.
- `Final_UI_Runtime_Editor_Architecture.html`: 평면 UI와 레벨 직접 조립 구조에서 BindingKey·Hierarchy·Asset/Instance·UIHandle·수명·Factory·입력 소유권으로 확장한 과정.
- `Animation_GPU_Skinning_Phase1_2_Record.html`: 최종 본 행렬 계산을 Compute Shader로 옮기고 전역 입력·출력 버퍼 및 일괄 작업 처리로 정리한 기록.

### 설계 목표를 완료 기능으로 옮기지 않은 부분

- `gpu_animator_final_architecture_utf8.html`의 전체 포즈 샘플링·블렌딩·레이어·계층 GPU 이전은 목표 설계입니다. 현재 Animator는 CPU에서 포즈를 계산하며 GPU 경로는 최종 본 행렬 계산을 담당합니다.
- `AnimationPoseDecisionDesign.md`의 BlendSpace·노드 그래프 등 제안 사항을 일괄적으로 구현 목록에 추가하지 않았습니다. 반대로 문서 초기에 보류되었던 가산 레이어는 현재 `Animator_PoseLayer.cpp` 구현을 확인했습니다.
- 초기 `ui_viewmodel_architecture.html`에 나온 예시 클래스명은 최종 구현과 다릅니다. README에서는 현재 존재하는 `CUIObject`, Controller, ViewModel 구조로 설명했습니다.
- UI 설계서의 제안 단계와 완료 기록이 함께 있으므로, 실제 코드에서 확인한 기능만 사용했습니다. 전체 게임패드 내비게이션이나 범용 Flow 시스템 완성을 주장하지 않습니다.
- 성능 측정값과 이번 실행 테스트 결과는 없으므로 FPS 개선율이나 검증 완료 수치를 기재하지 않았습니다.

개발 기간과 팀 인원은 별도 확인이 필요합니다. 개인 커밋의 첫날·마지막 날을 프로젝트 공식 개발 기간으로 대신하지 않았습니다.
