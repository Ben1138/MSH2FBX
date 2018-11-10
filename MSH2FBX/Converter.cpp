#include "pch.h"
#include "Converter.h"

namespace MSH2FBX
{
	fs::path Converter::FbxFilePath;

	FbxScene* Converter::Scene = nullptr;
	FbxManager* Converter::Manager = nullptr;
	MSH* Converter::Mesh = nullptr;

	EChunkFilter Converter::ChunkFilter = EChunkFilter::None;
	EModelPurpose Converter::ModelIgnoreFilter = EModelPurpose::Miscellaneous;

	string Converter::OverrideAnimName = "";
	map<MODL*, FbxNode*> Converter::MODLToFbxNode;
	map<CRCChecksum, FbxNode*> Converter::CRCToFbxNode;


	FbxNode* Converter::FindNode(MODL& model)
	{
		// Get FbxNode of the model (child)
		auto it = MODLToFbxNode.find(&model);
		if (it != MODLToFbxNode.end())
		{
			if (it->second != nullptr)
			{
				return it->second;
			}
			else
			{
				Log("MODL '" + model.m_Parent.m_Text + "' has been mapped to NULL!");
				return nullptr;
			}
		}
		else
		{
			Log("No FbxNode has been created for MODL '" + model.m_Parent.m_Text + "' ! This should never happen!");
			return nullptr;
		}
	}

	bool Converter::Start(const fs::path& fbxFilePath)
	{
		if (Scene != nullptr)
		{
			Log("Scene is not NULL!");
			return false;
		}

		if (Manager != nullptr)
		{
			Log("Manager is not NULL!");
			return false;
		}

		if (Mesh != nullptr)
		{
			Log("Still a MSH present!");
			return false;
		}

		if (FbxFilePath != "")
		{
			Log("Still a Fbx File Name present!");
		}

		MODLToFbxNode.clear();
		CRCToFbxNode.clear();
		FbxFilePath = fbxFilePath;

		// Overall FBX (memory) manager
		Manager = FbxManager::Create();

		// Create FBX Scene
		Scene = FbxScene::Create(Manager, fbxFilePath.filename().u8string().c_str());
		Scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
		return true;
	}

	bool Converter::AddMSH(MSH* msh)
	{
		if (Mesh != nullptr)
		{
			Log("MSH is not NULL!");
			return false;
		}

		Mesh = msh;
		MSHToFBXScene();
		Mesh = nullptr;

		return true;
	}

	bool Converter::Save()
	{
		if (Scene == nullptr)
		{
			Log("Scene is NULL!");
			return false;
		}

		if (Manager == nullptr)
		{
			Log("Manager is NULL!");
			return false;
		}

		if (Mesh != nullptr)
		{
			Log("There is still a MSH in progress!");
			return false;
		}

		if (FbxFilePath == "")
		{
			Log("No Fbx File Name present!");
		}

		bool success = true;

		// Export Scene to FBX
		FbxExporter* exporter = FbxExporter::Create(Manager, "");
		FbxIOSettings* settings = FbxIOSettings::Create(Manager, IOSROOT);
		Manager->SetIOSettings(settings);

		if (exporter->Initialize(FbxFilePath.u8string().c_str(), -1, Manager->GetIOSettings()))
		{
			if (!exporter->Export(Scene, false))
			{
				Log("Exporting failed!");
				success = false;
			}
		}
		else
		{
			Log("Initializing export failed!");
			success = false;
		}

		// Free all		
		exporter->Destroy();
		Close();
		return success;
	}

	void Converter::Close()
	{
		if (Scene == nullptr)
		{
			Log("Cannot close Scene since its NULL!");
			return;
		}

		if (Manager == nullptr)
		{
			Log("Cannot close Scene since its NULL!");
			return;
		}

		// Free all
		Scene->Destroy();
		Scene = nullptr;

		Manager->Destroy();
		Manager = nullptr;

		Mesh = nullptr;
		FbxFilePath = "";
	}

	void Converter::MSHToFBXScene()
	{
		FbxNode* rootNode = Scene->GetRootNode();
		map<MODL*, FbxCluster*> BoneToCluster;

		// Converting Models
		if ((ChunkFilter & EChunkFilter::Models) == 0)
		{
			for (size_t i = 0; i < Mesh->m_MeshBlock.m_Models.size(); ++i)
			{
				MODL& model = Mesh->m_MeshBlock.m_Models[i];
				EModelPurpose purpose = model.GetEstimatedPurpose();

				// Do not process unwanted stuff
				if ((purpose & ModelIgnoreFilter) != 0)
				{
					Log("Skipping Model '" + model.m_Name.m_Text + "' according to filter");
					continue;
				}

				// Create Node to attach mesh to
				FbxNode* modelNode = FbxNode::Create(Manager, model.m_Name.m_Text.c_str());
				rootNode->AddChild(modelNode);

				// generate crc checksum from name (should match those in msh file)
				CRCChecksum crc = CRC::CalcLowerCRC(model.m_Name.m_Text.c_str());
				CRCToFbxNode[crc] = modelNode;

				if ((purpose & EModelPurpose::Mesh) != 0)
				{
					// Create and attach Mesh
					if (!MODLToFBXMesh(model, Mesh->m_MeshBlock.m_MaterialList, modelNode))
					{
						Log("Failed to convert MSH Model to FBX Mesh. MODL No: " + std::to_string(i) + "  MTYP: " + std::to_string((int)model.m_ModelType.m_ModelType));
						continue;
					}
					else
					{
						Log("converting " + model.m_Name.m_Text);
					}
				}
				else if ((purpose & EModelPurpose::Skeleton) != 0)
				{
					// Create FBXSkeleton (Bones)
					if (!MODLToFBXSkeleton(model, modelNode))
					{
						Log("Failed to convert MSH Model to FBX Skeleton. MODL No: " + std::to_string(i) + "  MTYP: " + std::to_string((int)model.m_ModelType.m_ModelType));
						continue;
					}
				}
				else // everything else is just interpreted as a point with an empty mesh
				{
					FbxMesh* mesh = FbxMesh::Create(Manager, model.m_Name.m_Text.c_str());
					modelNode->AddNodeAttribute(mesh);
				}

				MODLToFbxNode[&model] = modelNode;
			}

			// Applying parentship and weights
			// Maybe doing something more efficient in the future?
			for (size_t i = 0; i < Mesh->m_MeshBlock.m_Models.size(); ++i)
			{
				MODL& model = Mesh->m_MeshBlock.m_Models[i];
				EModelPurpose purpose = model.GetEstimatedPurpose();

				// Do not process unwanted stuff
				if ((purpose & ModelIgnoreFilter) != 0)
				{
					continue;
				}

				FbxNode* modelNode = FindNode(model);

				// Change parentship (if any)
				if (model.m_Parent.m_Text != "")
				{
					// Get FBXNode of Parent
					FbxNode* parentNode = rootNode->FindChild(model.m_Parent.m_Text.c_str());

					if (parentNode != nullptr)
					{
						rootNode->RemoveChild(modelNode);
						parentNode->AddChild(modelNode);
					}
					else
					{
						Log("Parent Node '" + model.m_Parent.m_Text + "' not found!");
					}
				}

				// Applying MODL Transform to FbxNode
				modelNode->LclTranslation.Set
				(
					FbxDouble3
					(
						model.m_Transition.m_Translation.m_X,
						model.m_Transition.m_Translation.m_Y,
						model.m_Transition.m_Translation.m_Z
					)
				);

				FbxQuaternion rot;
				rot.Set
				(
					model.m_Transition.m_Rotation.m_X,
					model.m_Transition.m_Rotation.m_Y,
					model.m_Transition.m_Rotation.m_Z,
					model.m_Transition.m_Rotation.m_W
				);
				modelNode->LclRotation.Set(rot.DecomposeSphericalXYZ());

				modelNode->LclScaling.Set
				(
					FbxDouble3
					(
						model.m_Transition.m_Scale.m_X,
						model.m_Transition.m_Scale.m_Y,
						model.m_Transition.m_Scale.m_Z
					)
				);
			}

			// Converting Weights
			// Execute this AFTER all Bones (MODLs) are converted to FbxNodes
			// and their Transforms have been applied respectively
			for (size_t i = 0; i < Mesh->m_MeshBlock.m_Models.size(); ++i)
			{
				MODL& model = Mesh->m_MeshBlock.m_Models[i];
				EModelPurpose purpose = model.GetEstimatedPurpose();

				// Do not process unwanted stuff
				if ((purpose & ModelIgnoreFilter) != 0)
				{
					continue;
				}
				
				FbxNode* modelNode = FindNode(model);
				FbxAMatrix& matrixMeshNode = modelNode->EvaluateGlobalTransform();

				if ((ChunkFilter & EChunkFilter::Weights) == 0 && (purpose & EModelPurpose::Mesh) != 0)
				{
					size_t vertexOffset = 0;

					for (size_t i = 0; i < model.m_Geometry.m_Segments.size(); ++i)
					{
						SEGM& segment = model.m_Geometry.m_Segments[i];
						WGHTToFBXSkin(segment.m_WeightList, model.m_Geometry.m_Envelope, matrixMeshNode, vertexOffset, BoneToCluster);
						vertexOffset += segment.m_VertexList.m_Vertices.size();
					}

					FbxSkin* skin = FbxSkin::Create(Scene, string(model.m_Name.m_Text + "_Skin").c_str());

					for (auto it = BoneToCluster.begin(); it != BoneToCluster.end(); it++)
					{
						skin->AddCluster(it->second);
					}

					FbxMesh* mesh = (FbxMesh*)modelNode->GetNodeAttribute();
					mesh->AddDeformer(skin);
				}
			}
		}

		// Converting Animations
		if ((ChunkFilter & EChunkFilter::Animations) == 0)
		{
			ANM2ToFBXAnimations(Mesh->m_Animations);
		}
	}

	FbxDouble3 Converter::ColorToFBXColor(const Color& color)
	{
		return FbxDouble3(color.m_Red, color.m_Green, color.m_Blue);
	}
	
	void Converter::WGHTToFBXSkin(WGHT& weights, const ENVL& envelope, const FbxAMatrix& matrixMeshNode, const size_t vertexOffset, map<MODL*, FbxCluster*>& BoneToCluster)
	{
		if (Mesh == nullptr)
		{
			Log("Mesh (MSH) is NULL!");
			return;
		}

		// for each vertex...
		for (size_t i = 0; i < weights.m_Weights.size(); ++i)
		{
			// each vertex consist of 4 weights (for 4 bones)
			for (int j = 0; j < weights.m_Weights[i].size(); ++j)
			{
				BoneWeight& weight = weights.m_Weights[i][j];
				uint32_t& ei = weight.m_EnvelopeIndex;

				// Do not process if the weight is 0.0 anyway
				if (weight.m_WeightValue == 0.0f)
				{
					continue;
				}

				if (ei < envelope.m_ModelIndices.size())
				{
					if (envelope.m_ModelIndices[ei] < Mesh->m_MeshBlock.m_Models.size())
					{
						MODL& bone = Mesh->m_MeshBlock.m_Models[envelope.m_ModelIndices[ei]];

						// In MSH the end point represents the Bone, while in FBX the start point represents the bone.
						// This leads to inconsistent application of weights. 
						// To prevent that, we have to map the weights onto the Nodes parent

						// Example layout: root_r_upperarm --> bone_r_upperarm --> bone_r_forearm --> eff_r_forearm
						// In MSH, weights for upperarm and forearm are applied to: bone_r_forearm and eff_r_forearm
						// But in FBX, we want to apply them to: bone_r_upperarm and bone_r_forearm
						FbxNode* BoneNode = FindNode(bone)->GetParent();
						if (BoneNode != nullptr)
						{
							FbxCluster* cluster = nullptr;

							auto it2 = BoneToCluster.find(&bone);
							if (it2 != BoneToCluster.end())
							{
								cluster = it2->second;
							}
							else
							{
								cluster = FbxCluster::Create(Scene, string(bone.m_Name.m_Text + "_Cluster").c_str());
								cluster->SetLinkMode(FbxCluster::eTotalOne);
								cluster->SetLink(BoneNode);
								BoneToCluster[&bone] = cluster;
							}

							// TODO: do all 4 weights really add up to 1.0 in MSH? investigation needed!
							cluster->AddControlPointIndex((int)(i + vertexOffset), (double)weight.m_WeightValue);
							cluster->SetTransformMatrix(matrixMeshNode);

							// bone node transform matrix
							FbxAMatrix& matrixBoneNode = BoneNode->EvaluateGlobalTransform();
							cluster->SetTransformLinkMatrix(matrixBoneNode);
						}
						else
						{
							Log("MODL (Bone) '" + bone.m_Parent.m_Text + "' has been mapped to NULL!");
						}
					}
					else
					{
						Log("Model Index " + std::to_string(envelope.m_ModelIndices[i]) + " is out of Range " + std::to_string(Mesh->m_MeshBlock.m_Models.size()));
					}
				}
				else
				{
					Log("Envelope Index " + std::to_string(weight.m_EnvelopeIndex) + " is out of Range " + std::to_string(envelope.m_ModelIndices.size()));
				}
			}
		}
	}

	void Converter::ANM2ToFBXAnimations(ANM2& animations)
	{
		if (animations.m_AnimationCycle.m_Animations.size() == 0)
		{
			Log("No Animation Circle found!");
			return;
		}

		// assuming only one animation per msh, this should be temporary
		Animation& anim = animations.m_AnimationCycle.m_Animations[0];
		string& animName = OverrideAnimName != "" ? OverrideAnimName : anim.m_AnimationName;
		FbxAnimStack* animStack = FbxAnimStack::Create(Scene, animName.c_str());

		FbxTime start, end;
		start.SetSecondDouble(anim.m_FirstFrame / anim.m_FrameRate);
		end.SetSecondDouble(anim.m_LastFrame / anim.m_FrameRate);
		animStack->SetLocalTimeSpan(FbxTimeSpan(start, end));

		FbxAnimLayer* animLayer = FbxAnimLayer::Create(Scene, string(animName + "_Layer").c_str());
		animStack->AddMember(animLayer);

		// for every bone...
		for (size_t i = 0; i < animations.m_KeyFrames.m_BoneFrames.size(); ++i)
		{
			BoneFrames& bf = animations.m_KeyFrames.m_BoneFrames[i];
			FbxNode* boneNode = nullptr;

			// get respective Bone to animate from stored CRC checksum
			auto it = CRCToFbxNode.find(bf.m_CRCchecksum);
			if (it != CRCToFbxNode.end())
			{
				if (!it->second)
				{
					Log("CRC '" + std::to_string(bf.m_CRCchecksum) + "' has been mapped to null pointer!");
					continue;
				}
				boneNode = it->second;
			}
			else
			{
				Log("Could not find a Bone for CRC: " + std::to_string(bf.m_CRCchecksum));
				continue;
			}

			// Translation
			FbxAnimCurve* tranCurveX = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
			FbxAnimCurve* tranCurveY = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
			FbxAnimCurve* tranCurveZ = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

			tranCurveX->KeyModifyBegin();
			tranCurveY->KeyModifyBegin();
			tranCurveZ->KeyModifyBegin();
			for (size_t j = 0; j < bf.m_TranslationFrames.size(); ++j)
			{
				TranslationFrame& t = bf.m_TranslationFrames[j];

				FbxTime time;
				time.SetSecondDouble(t.m_FrameIndex / anim.m_FrameRate);
				tranCurveX->KeySet(tranCurveX->KeyAdd(time), time, t.m_Translation.m_X, FbxAnimCurveDef::eInterpolationLinear);
				tranCurveY->KeySet(tranCurveY->KeyAdd(time), time, t.m_Translation.m_Y, FbxAnimCurveDef::eInterpolationLinear);
				tranCurveZ->KeySet(tranCurveZ->KeyAdd(time), time, t.m_Translation.m_Z, FbxAnimCurveDef::eInterpolationLinear);
			}
			tranCurveX->KeyModifyEnd();
			tranCurveY->KeyModifyEnd();
			tranCurveZ->KeyModifyEnd();

			// Rotation
			FbxAnimCurve* rotCurveX = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
			FbxAnimCurve* rotCurveY = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
			FbxAnimCurve* rotCurveZ = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

			rotCurveX->KeyModifyBegin();
			rotCurveY->KeyModifyBegin();
			rotCurveZ->KeyModifyBegin();
			for (size_t j = 0; j < bf.m_RotationFrames.size(); ++j)
			{
				RotationFrame& t = bf.m_RotationFrames[j];

				FbxQuaternion quaternion;
				quaternion.Set
				(
					t.m_Rotation.m_X,
					t.m_Rotation.m_Y,
					t.m_Rotation.m_Z,
					t.m_Rotation.m_W
				);

				FbxAMatrix rotMatrix;
				rotMatrix.SetQOnly(quaternion);
				FbxVector4 rot = rotMatrix.GetROnly();

				FbxTime time;
				time.SetSecondDouble(t.m_FrameIndex / anim.m_FrameRate);
				rotCurveX->KeySet(rotCurveX->KeyAdd(time), time, (float)rot[0], FbxAnimCurveDef::eInterpolationLinear);
				rotCurveY->KeySet(rotCurveY->KeyAdd(time), time, (float)rot[1], FbxAnimCurveDef::eInterpolationLinear);
				rotCurveZ->KeySet(rotCurveZ->KeyAdd(time), time, (float)rot[2], FbxAnimCurveDef::eInterpolationLinear);
			}
			rotCurveX->KeyModifyEnd();
			rotCurveY->KeyModifyEnd();
			rotCurveZ->KeyModifyEnd();
		}
	}

	bool Converter::MATDToFBXMaterial(const MATD& material, FbxNode* meshNode, int& matIndex)
	{
		if (Manager == nullptr)
		{
			Log("FbxManager is NULL!");
			return false;
		}

		if (meshNode == nullptr)
		{
			Log("Given FbxNode is NULL!");
			return false;
		}

		matIndex = meshNode->GetMaterialIndex(material.m_Name.m_Text.c_str());

		if (matIndex < 0)
		{
			FbxSurfacePhong* fbxMaterial = FbxSurfacePhong::Create(Manager, material.m_Name.m_Text.c_str());
			fbxMaterial->Diffuse.Set(ColorToFBXColor(material.m_Data.m_Diffuse));
			fbxMaterial->Ambient.Set(ColorToFBXColor(material.m_Data.m_Ambient));
			fbxMaterial->Specular.Set(ColorToFBXColor(material.m_Data.m_Specular));

			FbxFileTexture* fbxTexture = FbxFileTexture::Create(Scene, material.m_Texture0.m_Text.c_str());
			fbxTexture->SetFileName(material.m_Texture0.m_Text.c_str()); // Resource file is in current directory.
			fbxTexture->SetTextureUse(FbxTexture::eStandard);
			fbxTexture->SetMappingType(FbxTexture::eUV);
			fbxTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);

			fbxMaterial->Diffuse.ConnectSrcObject(fbxTexture);

			matIndex = meshNode->AddMaterial(fbxMaterial);
		}

		return true;
	}

	bool Converter::MODLToFBXMesh(MODL& model, MATL& materials, FbxNode* meshNode)
	{
		if (Manager == nullptr)
		{
			Log("FbxManager is NULL!");
			return false;
		}

		if (meshNode == nullptr)
		{
			Log("Given FbxNode is NULL!");
			return false;
		}

		FbxMesh* mesh = FbxMesh::Create(Manager, model.m_Name.m_Text.c_str());

		vector<FbxVector4> vertices;
		vector<FbxVector4> normals;
		vector<FbxVector2> uvs;
		size_t vertexOffset = 0;

		vector<FbxCluster*> boneClusters;

		// crawl all Segments
		for (size_t i = 0; i < model.m_Geometry.m_Segments.size(); ++i)
		{
			SEGM& segment = model.m_Geometry.m_Segments[i];

			if (segment.m_VertexList.m_Vertices.size() == segment.m_NormalList.m_Normals.size())
			{
				// grab the segments vertices, normals and UVs
				for (size_t j = 0; j < segment.m_VertexList.m_Vertices.size(); ++j)
				{
					Vector3& v = segment.m_VertexList.m_Vertices[j];
					vertices.emplace_back(v.m_X, v.m_Y, v.m_Z);

					Vector3& n = segment.m_NormalList.m_Normals[j];
					normals.emplace_back(n.m_X, n.m_Y, n.m_Z);

					// UVs are optional
					if (j < segment.m_UVList.m_UVs.size())
					{
						Vector2& uv = segment.m_UVList.m_UVs[j];
						uvs.emplace_back(uv.m_X, uv.m_Y);
					}
				}

				// convert MSH triangle strips to polygons
				segment.m_TriangleList.CalcPolygons();
				for (size_t j = 0; j < segment.m_TriangleList.m_Polygons.size(); ++j)
				{
					auto& poly = segment.m_TriangleList.m_Polygons[j];

					uint32_t& mshMatIndex = segment.m_MaterialIndex.m_MaterialIndex;
					int fbxMatIndex = -1;

					if (mshMatIndex >= 0 && mshMatIndex < materials.m_Materials.size())
					{
						MATD& mshMat = materials.m_Materials[segment.m_MaterialIndex.m_MaterialIndex];

						if ((ChunkFilter & EChunkFilter::Materials) == 0 && !MATDToFBXMaterial(mshMat, meshNode, fbxMatIndex))
						{
							Log("Could not convert MSH Material '" + mshMat.m_Name.m_Text + "' to FbxMaterial!");
						}
					}

					mesh->BeginPolygon(fbxMatIndex);
					for (size_t k = 0; k < poly.m_VertexIndices.size(); ++k)
					{
						mesh->AddPolygon((int)(poly.m_VertexIndices[k] + vertexOffset));
					}
					mesh->EndPolygon();
				}

				// since in MSH vertices are local in their respective segments,
				// we have to store an offset because in FBX vertices are global
				vertexOffset += segment.m_VertexList.m_Vertices.size();
			}
			else
			{
				Log("Inconsistent lengths of vertices and normals in Segment No: " + std::to_string(i));
				mesh->Destroy(true);
				mesh = nullptr;
				return false;
			}
		}

		// Set vertices (control points)
		mesh->InitControlPoints((int)vertices.size());
		FbxVector4* cps = mesh->GetControlPoints();
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			cps[i] = vertices[i];
		}

		// Set Normals
		auto elementNormal = mesh->CreateElementNormal();
		elementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
		elementNormal->SetReferenceMode(FbxGeometryElement::eDirect);
		for (size_t i = 0; i < normals.size(); ++i)
		{
			elementNormal->GetDirectArray().Add(normals[i]);
		}

		// Set UVs (if any)
		if (uvs.size() > 0)
		{
			auto elementUV = mesh->CreateElementUV("DiffuseUVs");
			elementUV->SetMappingMode(FbxGeometryElement::eByControlPoint);
			elementUV->SetReferenceMode(FbxGeometryElement::eDirect);
			for (size_t i = 0; i < uvs.size(); ++i)
			{
				elementUV->GetDirectArray().Add(uvs[i]);
			}
		}

		meshNode->SetNodeAttribute(mesh);
		return true;
	}

	bool Converter::MODLToFBXSkeleton(MODL& model, FbxNode* boneNode)
	{
		if (Manager == nullptr)
		{
			Log("FbxManager is NULL!");
			return false;
		}

		if (boneNode == nullptr)
		{
			Log("Given FbxNode is NULL!");
			return false;
		}

		EModelPurpose purpose = model.GetEstimatedPurpose();
		FbxSkeleton* bone = FbxSkeleton::Create(Manager, model.m_Name.m_Text.c_str());
		bone->Size.Set(1.0f);

		switch (purpose)
		{
			case EModelPurpose::Skeleton_Root:
			case EModelPurpose::Skeleton_BoneRoot:
				bone->SetSkeletonType(FbxSkeleton::eRoot);
				break;
			case EModelPurpose::Skeleton_BoneLimb:
			case EModelPurpose::Skeleton_BoneEnd:
				bone->SetSkeletonType(FbxSkeleton::eLimbNode);
				break;
			default:
				Log("No suitable Bone Type found for '" + model.m_Name.m_Text + "' ! Model Type is: " + std::to_string((int)model.m_ModelType.m_ModelType) + "  Estimated Model Purpose is: " + std::to_string(purpose));
				bone->Destroy();
				return false;
		}

		boneNode->SetNodeAttribute(bone);
		return true;
	}
}