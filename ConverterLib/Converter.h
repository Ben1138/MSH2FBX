#pragma once

namespace Converter
{
	using namespace LibSWBF2::Logging;
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

	typedef function<void(string)> LogCallback;

	class Converter
	{
	public:
		Converter() = default;
		Converter(const fs::path& fbxFileName);
		~Converter();

		// In filters, you specify what you DON'T want
		EModelPurpose ModelIgnoreFilter = EModelPurpose::Miscellaneous;
		EChunkFilter ChunkFilter = EChunkFilter::None;
		string OverrideAnimName = "";
		MSH* Basepose = nullptr;
		bool EmptyMeshes = false;

		bool Start(const fs::path& fbxFileName);
		bool AddMSH(MSH* msh);
		bool Save();
		void Close();

	private:
		map<MODL*, FbxNode*> MODLToFbxNode;
		map<CRCChecksum, FbxNode*> CRCToFbxNode;

		FbxNode* FindNode(MODL* model);
		FbxNode* FindNode(const CRCChecksum checksum);
		FbxDouble3 ColorToFBXColor(const Color& color);
		FbxDouble4 QuaternionToEuler(const Vector4& Quaternion);
		void ApplyTransform(FbxNode* modelNode, const Vector3& Translation, const Vector4& Rotation);
		void ApplyTransform(FbxNode* modelNode, const Vector3& Translation, const Vector4& Rotation, const Vector3& Scale);
		void MSHToFBXScene();
		void ANM2ToFBXAnimations(ANM2& animations);
		void WGHTToFBXSkin(WGHT& weights, const ENVL& envelope, const FbxAMatrix& matrixMeshNode, const size_t vertexOffset, map<MODL*, FbxCluster*>& BoneToCluster);
		bool MATDToFBXMaterial(const MATD& material, FbxNode* meshNode, int& matIndex);
		bool MODLToFBXMesh(MODL& model, MATL& materials, FbxNode* meshNode);
		bool MODLToFBXSkeleton(MODL& model, FbxNode* boneNode);

		// Current State
		bool Running = false;
		fs::path FbxFilePath;
		MSH* Mesh = nullptr;
		FbxScene* Scene = nullptr;
		FbxManager* Manager = nullptr;
		FbxPose* Bindpose = nullptr;

		// Logging
		void Log(const string msg);
		LogCallback m_OnLogCallback;
	};
}