#include "stdafx.h"
#include "API.h"

namespace MSH2FBX
{
	MSH2FBX_API Converter* Converter_Create()
	{
		return new Converter();
	}

	MSH2FBX_API Converter* Converter_Create_Start(const char* fbxFileName)
	{
		return new Converter(fs::path(fbxFileName));
	}

	MSH2FBX_API void Converter_Destroy(const Converter* converter)
	{
		delete converter;
	}


	MSH2FBX_API void Converter_Set_ModelIgnoreFilter(Converter* converter, const EModelPurpose filter)
	{
		converter->ModelIgnoreFilter = filter;
	}

	MSH2FBX_API EModelPurpose Converter_Get_ModelIgnoreFilter(const Converter* converter)
	{
		return converter->ModelIgnoreFilter;
	}


	MSH2FBX_API void Converter_Set_ChunkFilter(Converter* converter, const EChunkFilter filter)
	{
		converter->ChunkFilter = filter;
	}

	MSH2FBX_API EChunkFilter Converter_Get_ChunkFilter(const Converter* converter)
	{
		return converter->ChunkFilter;
	}


	MSH2FBX_API void Converter_Set_OverrideAnimName(Converter* converter, const char* animName)
	{
		converter->OverrideAnimName = string(animName);
	}

	MSH2FBX_API const char* Converter_Get_OverrideAnimName(const Converter* converter)
	{
		return converter->OverrideAnimName.c_str();
	}


	MSH2FBX_API void Converter_Set_EmptyMeshes(Converter* converter, const bool emptyMeshes)
	{
		converter->EmptyMeshes = emptyMeshes;
	}

	MSH2FBX_API bool Converter_Get_EmptyMeshes(const Converter* converter)
	{
		return converter->EmptyMeshes;
	}


	MSH2FBX_API void Converter_Set_BaseposeMSH(Converter* converter, const char* baseposeMSH)
	{
		converter->BaseposeMSH = string(baseposeMSH);
	}

	MSH2FBX_API const char* Converter_Get_BaseposeMSH(const Converter* converter)
	{
		return converter->BaseposeMSH.u8string().c_str();
	}

	MSH2FBX_API void Converter_SetLogCallback(const LogCallback Callback)
	{
		Converter::SetLogCallback(Callback);
	}

	MSH2FBX_API bool Converter_Start(Converter* converter, const char* fbxFileName)
	{
		return converter->Start(fs::path(fbxFileName));
	}

	MSH2FBX_API bool Converter_AddMSH(Converter* converter, const char* mshFileName)
	{
		return converter->AddMSH(fs::path(mshFileName));
	}

	MSH2FBX_API bool Converter_SaveFBX(Converter* converter)
	{
		return converter->SaveFBX();
	}

	MSH2FBX_API bool Converter_ClearFBXScene(Converter* converter)
	{
		return converter->ClearFBXScene();
	}

	MSH2FBX_API void Converter_Close(Converter* converter)
	{
		converter->Close();
	}
}