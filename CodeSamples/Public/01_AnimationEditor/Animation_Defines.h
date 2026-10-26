#pragma once

#include <Windows.h>
#include <process.h>
#include "Core_Defines.h"
#include "AnimationData.h"

namespace Anim_Editor
{
	enum class DOCK_GROUP
	{
		LEFT,
		CENTER,
		RIGHT,
		BOTTOMLEFT,
		BOTTOMRIGTH,
		FLOATING,
	};

	enum class PREVIEW_MODEL_TYPE
	{
		SINGLE_MODEL,
		PLAYER_PARTS,
	};

	typedef struct tagBonePoint
	{
		_uint iBoneIndex = {};
		string strBoneName;
		_float3 vWorldPosition = {};
	}BONE_POINT;

	typedef struct tagBoneScreenPoint
	{
		_uint   iBoneIndex = {};
		string  strBoneName;
		ImVec2  vScreen;
		_float  fDepth = {};        // 가까운 본 우선 선택용
		bool    bVisible = false;
	}BONE_SCREEN_POINT;

	//const unsigned int	g_iAnimation_EditorSizeX = { 2560 };
	//const unsigned int	g_iAnimation_EditorSizeY = { 1440 };
	//const unsigned int	g_iAnimation_EditorSizeX = { 1800 };
	//const unsigned int	g_iAnimation_EditorSizeY = { 1000 };
	const unsigned int	g_iAnimation_EditorSizeX = { 1400 };
	const unsigned int	g_iAnimation_EditorSizeY = { 800 };

	static _wstring g_strPreviewTargetTag = L"RT_Preview";
	static _wstring g_strPreviewMRTTag = L"MRT_Preview";
}

extern HINSTANCE g_hInstance;
extern HWND g_hWnd;

using namespace Anim_Editor;