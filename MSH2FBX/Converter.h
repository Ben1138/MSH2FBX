#pragma once
#include "pch.h"
#include "MSH2FBX.h"

namespace MSH2FBX
{
	using namespace LibSWBF2::Chunks::Mesh;
	using namespace LibSWBF2::Types;
	using LibSWBF2::CRC;
	using LibSWBF2::CRCChecksum;

	// Bit Flags
	enum EChunkFilter
	{
		None = 0,
		Materials = 1,
		Models = 2,
		Animations = 4,
		Weights = 8
	};

	class Converter
	{
	public:
		// In filters, you specify what you DON'T want
		static EModelPurpose ModelIgnoreFilter;
		static EChunkFilter ChunkFilter;

		static string GetPlainFileName(const string& fileName);

		static bool Start(const string& fbxFileName);
		static bool AddMSH(MSH* msh);
		static bool Save();

	private:
		static map<MODL*, FbxNode*> MODLToFbxNode;
		static map<CRCChecksum, FbxNode*> CRCToFbxNode;

		static void ApplyTransform(const MODL& model, FbxNode* meshNode);
		static FbxDouble3 ColorToFBXColor(const Color& color);
		static void MSHToFBXScene();
		static void ANM2ToFBXAnimations(ANM2& animations);
		static void WGHTToFBXSkin(WGHT& weights, const ENVL& envelope, FbxNode* meshNode, const size_t vertexOffset, map<MODL*, FbxCluster*>& BoneToCluster);
		static bool MATDToFBXMaterial(const MATD& material, FbxNode* meshNode, int& matIndex);
		static bool MODLToFBXMesh(MODL& model, MATL& materials, FbxNode* meshNode);
		static bool MODLToFBXSkeleton(MODL& model, FbxNode* boneNode);

		// Current
		static string FbxFileName;
		static MSH* Mesh;
		static FbxScene* Scene;
		static FbxManager* Manager;
	};
}