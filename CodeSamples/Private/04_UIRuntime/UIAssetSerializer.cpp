// 포트폴리오 코드 발췌: 파일 저장·읽기 → JSON 변환 요청 → 결과·오류 반환; 요소별 필드 변환은 중략
// 원본: Engine/Private/UIAssetSerializer.cpp
// 함수 내부는 원본 그대로 유지했습니다. 함수 사이의 생략은 [중략]으로 표시합니다.
// 초기화·공통 의존 코드·일부 보조 함수는 제외하여 이 파일만으로 빌드할 수 없습니다.

// [중략] include·초기화 및 공통 선언.

// 원본 191~217행: Save
HRESULT CUIAssetSerializer::Save(const _wstring& strFilePath, const UI_ASSET_DESC& Asset, _wstring* pError)
{
	if (strFilePath.empty())
		return Fail(pError, L"UI Asset save path is empty.");

	try
	{
		const json Root = ToJson(Asset);
		const string strOutput = Root.dump(4);

		ofstream File(strFilePath, ios::out | ios::trunc | ios::binary);

		if (!File.is_open())
			return Fail(pError, L"Failed to open UI Asset file.");

		File.write(strOutput.data(), static_cast<streamsize>(strOutput.size()));

		if (!File.good())
			return Fail(pError, L"Failed to write UI Asset file.");

		return S_OK;
	}
	catch (const exception&)
	{
		return Fail(pError, L"UI Asset serialization failed.");
	}
}

// [중략] 원본의 다른 함수 및 보조 구현.

// 원본 219~249행: Load
HRESULT CUIAssetSerializer::Load(const _wstring& strFilePath, UI_ASSET_DESC* pOutAsset, _wstring* pError)
{
	if (pOutAsset == nullptr)
		return Fail(pError, L"Invalid UI Asset output.");

	if (strFilePath.empty())
		return Fail(pError, L"UI Asset load path is empty.");

	try
	{
		ifstream File(strFilePath, ios::in | ios::binary);

		if (!File.is_open())
			return Fail(pError, L"Failed to open UI Asset file.");

		json Root = json::parse(File, nullptr, true, true);

		UI_ASSET_DESC LoadedAsset = {};

		if (FAILED(FromJson(Root, &LoadedAsset, pError)))
			return E_FAIL;

		*pOutAsset = move(LoadedAsset);

		return S_OK;
	}
	catch (const exception&)
	{
		return Fail(pError, L"UI Asset parsing failed");
	}
}
