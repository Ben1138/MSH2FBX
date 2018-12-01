#pragma once
#include "stdafx.h"

#ifdef MSH2FBXDLL_EXPORTS
#define MSH2FBX_API __declspec(dllexport)
#else
#define MSH2FBX_API __declspec(dllimport)
#endif

using ConverterLib::Converter;
using ConverterLib::EChunkFilter;
using LibSWBF2::Chunks::Mesh::EModelPurpose;
namespace fs = std::filesystem;

typedef void(*LogCallback)(const char* log);

namespace MSH2FBX
{
	extern "C"
	{
		// CONSTRUCTORS - DESTRUCTOR //
		MSH2FBX_API Converter* Converter_Create();
		MSH2FBX_API Converter* Converter_Create_Start(const char* fbxFileName);
		MSH2FBX_API void Converter_Destroy(const Converter* converter);

		// PROPERTIES //
		MSH2FBX_API void Converter_Set_ModelIgnoreFilter(Converter* converter, const EModelPurpose filter);
		MSH2FBX_API EModelPurpose Converter_Get_ModelIgnoreFilter(const Converter* converter);

		MSH2FBX_API void Converter_Set_ChunkFilter(Converter* converter, const EChunkFilter filter);
		MSH2FBX_API EChunkFilter Converter_Get_ChunkFilter(const Converter* converter);

		MSH2FBX_API void Converter_Set_OverrideAnimName(Converter* converter, const char* animName);
		MSH2FBX_API const char* Converter_Get_OverrideAnimName(const Converter* converter);

		MSH2FBX_API void Converter_Set_EmptyMeshes(Converter* converter, const bool emptyMeshes);
		MSH2FBX_API bool Converter_Get_EmptyMeshes(const Converter* converter);

		MSH2FBX_API void Converter_Set_BaseposeMSH(Converter* converter, const char* baseposeMSH);
		MSH2FBX_API const char* Converter_Get_BaseposeMSH(const Converter* converter);

		// METHODS //
		MSH2FBX_API void Converter_SetLogCallback(Converter* converter, const LogCallback Callback);
		MSH2FBX_API bool Converter_Start(Converter* converter, const char* fbxFileName);
		MSH2FBX_API bool Converter_AddMSH(Converter* converter, const char* mshFileName);
		MSH2FBX_API bool Converter_SaveFBX(Converter* converter);
		MSH2FBX_API bool Converter_ClearFBXScene(Converter* converter);
		MSH2FBX_API void Converter_Close(Converter* converter);
	}
}