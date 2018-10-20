#include "pch.h"
#include "Converter.h"

namespace MSH2FBX
{
	FbxScene* Converter::Scene = nullptr;
	FbxManager* Converter::Manager = nullptr;

	EModelPurpose Converter::IgnoreFilter = (EModelPurpose)(
		EModelPurpose::Mesh_ShadowVolume | 
		EModelPurpose::Mesh_Lowrez | 
		EModelPurpose::Mesh_Collision | 
		EModelPurpose::Mesh_VehicleCollision | 
		EModelPurpose::Mesh_TerrainCut
	);

	string Converter::GetPlainName(const string& fileName)
	{
		size_t lastindex = fileName.find_last_of(".");

		if (lastindex == string::npos)
		{
			return fileName;
		}

		return fileName.substr(0, lastindex);
	}

	bool Converter::SaveAsFBX(MSH* msh, const string& fbxFileName)
	{
		if (Scene != nullptr)
		{
			Log("Scene is not NULL! Something went terribly wrong...");
			return false;
		}

		if (Manager != nullptr)
		{
			Log("Manager is not NULL! Something went terribly wrong...");
			return false;
		}

		if (msh == nullptr)
		{
			Log("Given MSH is NULL!");
			return false;
		}

		bool success = true;
		map<MODL*, FbxNode*> MODLToNodeMap;

		// Overall FBX (memory) manager
		Manager = FbxManager::Create();

		// Create FBX Scene
		Scene = FbxScene::Create(Manager, GetPlainName(fbxFileName).c_str());
		Scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit(100));	// 100 cm in a meter. we want meter as standard unit
		FbxNode* rootNode = Scene->GetRootNode();

		// Converting Models
		for (size_t i = 0; i < msh->m_MeshBlock.m_Models.size(); ++i)
		{
			MODL& model = msh->m_MeshBlock.m_Models[i];
			EModelPurpose purpose = model.GetEstimatedPurpose();

			// Do not process unwanted stuff
			if ((purpose & IgnoreFilter) != 0)
			{
				Log("Skipping Model '"+model.m_Name.m_Text+"' according to filter");
				continue;
			}

			// Create Node to attach mesh to
			FbxNode* modelNode = FbxNode::Create(Manager, model.m_Name.m_Text.c_str());
			rootNode->AddChild(modelNode);
			
			if ((purpose & EModelPurpose::Mesh) != 0)
			{
				// Create and attach Mesh
				if (!MODLToFBXMesh(model, msh->m_MeshBlock.m_MaterialList, modelNode))
				{
					Log("Failed to convert MSH Model to FBX Mesh. MODL No: " + std::to_string(i) + "  MTYP: " + std::to_string((int)model.m_ModelType.m_ModelType));
					continue;
				}
			}
			if ((purpose & EModelPurpose::Skeleton) != 0)
			{
				// Create FBXSkeleton (Bones)
				if (!MODLToFBXSkeleton(model, modelNode))
				{
					Log("Failed to convert MSH Model to FBX Skeleton. MODL No: " + std::to_string(i) + "  MTYP: " + std::to_string((int)model.m_ModelType.m_ModelType));
					continue;
				}
			}

			MODLToNodeMap[&model] = modelNode;
		}

		// Applying parentship
		// Maybe doing something more efficient in the future?
		for (size_t i = 0; i < msh->m_MeshBlock.m_Models.size(); ++i)
		{
			MODL& model = msh->m_MeshBlock.m_Models[i];
			FbxNode* modelNode = nullptr;

			// Get FbxNode of the model (child)
			auto it = MODLToNodeMap.find(&model);
			if (it != MODLToNodeMap.end())
			{
				if (it->second != nullptr)
				{
					modelNode = it->second;
				}
				else
				{
					Log("MODL '" + model.m_Parent.m_Text + "' has been mapped to NULL!");
					continue;
				}
			}
			else
			{
				Log("No FbxNode has been created for MODL '" + model.m_Parent.m_Text + "' ! This should never happen!");
				continue;
			}

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
					Log("Parent Node '"+ model.m_Parent.m_Text +"' not found!");
				}
			}

			ApplyTransform(model, modelNode);
		}

		// Export Scene to FBX
		FbxExporter* exporter = FbxExporter::Create(Manager, "");
		FbxIOSettings* settings = FbxIOSettings::Create(Manager, IOSROOT);
		Manager->SetIOSettings(settings);

		if (exporter->Initialize(fbxFileName.c_str(), -1, Manager->GetIOSettings()))
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
		Scene->Destroy();
		Scene = nullptr;
		
		exporter->Destroy();

		Manager->Destroy();
		Manager = nullptr;

		return success;
	}

	void Converter::ApplyTransform(const MODL& model, FbxNode* meshNode)
	{
		// Applying MODL Transform to FbxNode
		meshNode->LclTranslation.Set
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
		meshNode->LclRotation.Set(rot.DecomposeSphericalXYZ());

		meshNode->LclScaling.Set
		(
			FbxDouble3
			(
				model.m_Transition.m_Scale.m_X,
				model.m_Transition.m_Scale.m_Y,
				model.m_Transition.m_Scale.m_Z
			)
		);
	}

	FbxDouble3 Converter::ColorToFBXColor(const Color& color)
	{
		return FbxDouble3(color.m_Red, color.m_Green, color.m_Blue);
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

						if (!MATDToFBXMaterial(mshMat, meshNode, fbxMatIndex))
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

				// since in MSH segments vertices are local,
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

		if (purpose == EModelPurpose::Skeleton_Root || purpose == EModelPurpose::Skeleton_BoneRoot)
		{
			bone->SetSkeletonType(FbxSkeleton::eRoot);
			Log("Converting '"+model.m_Name.m_Text+"' into Bone (Root)");
		}
		else if (purpose == EModelPurpose::Skeleton_BoneLimb)
		{
			bone->SetSkeletonType(FbxSkeleton::eLimbNode);
			//bone->Size.Set(1.0);
			Log("Converting '" + model.m_Name.m_Text + "' into Bone (Limb)");
		}
		else if (purpose == EModelPurpose::Skeleton_BoneEnd)
		{
			bone->SetSkeletonType(FbxSkeleton::eLimbNode);
			Log("Converting '" + model.m_Name.m_Text + "' into Bone (End)");
		}
		else
		{
			Log("No suitable Bone Type found for '" + model.m_Name.m_Text + "' ! Model Type is: " + std::to_string((int)model.m_ModelType.m_ModelType) + "  Estimated Model Purpose is: " + std::to_string(purpose));
			bone->Destroy();
			return false;
		}

		boneNode->SetNodeAttribute(bone);
		return true;
	}
}