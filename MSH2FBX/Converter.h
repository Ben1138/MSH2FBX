#pragma once
#include "pch.h"
#include "MSH2FBX.h"
#include <filesystem>

namespace MSH2FBX
{
	using namespace LibSWBF2::Chunks::Mesh;
	using namespace LibSWBF2::Types;
	using LibSWBF2::CRC;
	using LibSWBF2::CRCChecksum;
	namespace fs = std::filesystem;

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
		static string OverrideAnimName;
		static MSH* Basepose;

		static bool Start(const fs::path& fbxFileName);
		static bool AddMSH(MSH* msh);
		static bool Save();
		static void Close();

	private:
		static map<MODL*, FbxNode*> MODLToFbxNode;
		static map<CRCChecksum, FbxNode*> CRCToFbxNode;

		static FbxNode* FindNode(MODL* model);
		static FbxNode* FindNode(const CRCChecksum checksum);
		static FbxDouble3 ColorToFBXColor(const Color& color);
		static FbxDouble4 QuaternionToEuler(const Vector4& Quaternion);
		static void ApplyTransform(FbxNode* modelNode, const Vector3& Translation, const Vector4& Rotation);
		static void ApplyTransform(FbxNode* modelNode, const Vector3& Translation, const Vector4& Rotation, const Vector3& Scale);
		static void MSHToFBXScene();
		static void ANM2ToFBXAnimations(ANM2& animations);
		static void WGHTToFBXSkin(WGHT& weights, const ENVL& envelope, const FbxAMatrix& matrixMeshNode, const size_t vertexOffset, map<MODL*, FbxCluster*>& BoneToCluster);
		static bool MATDToFBXMaterial(const MATD& material, FbxNode* meshNode, int& matIndex);
		static bool MODLToFBXMesh(MODL& model, MATL& materials, FbxNode* meshNode);
		static bool MODLToFBXSkeleton(MODL& model, FbxNode* boneNode);

		// Current State
		static fs::path FbxFilePath;
		static MSH* Mesh;
		static FbxScene* Scene;
		static FbxManager* Manager;
	};
}