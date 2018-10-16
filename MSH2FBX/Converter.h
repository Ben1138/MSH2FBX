#pragma once
#include "pch.h"
#include "MSH2FBX.h"

namespace MSH2FBX
{
	using namespace LibSWBF2::Chunks::Mesh;
	using namespace LibSWBF2::Types;

	class Converter
	{
	public:
		static string GetPlainName(const string& fileName);
		static bool SaveAsFBX(MSH* msh, const string& fbxFileName);

	private:
		static void ApplyTransform(const MODL& model, FbxNode* meshNode);
		static FbxDouble3 ColorToFBXColor(const Color& color);
		static bool MATDToFBXMaterial(const MATD& material, FbxNode* meshNode, int& matIndex);
		static bool MODLToFBXMesh(MODL& model, MATL& materials, FbxNode* meshNode);
		static bool MODLToFBXSkeleton(MODL& model, FbxNode* boneNode);

		static FbxScene* Scene;
		static FbxManager* Manager;
	};
}