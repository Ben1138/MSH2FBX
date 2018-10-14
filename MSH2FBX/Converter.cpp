#include "pch.h"
#include "Converter.h"

namespace MSH2FBX
{
	FbxScene* Converter::Scene = nullptr;


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

		if (msh == nullptr)
		{
			Log("Given MSH is NULL!");
			return false;
		}

		// Overall FBX (memory) manager
		FbxManager* manager = FbxManager::Create();

		// Create FBX Scene
		Scene = FbxScene::Create(manager, GetPlainName(fbxFileName).c_str());
		FbxNode* rootNode = Scene->GetRootNode();

		// Converting Models
		for (size_t i = 0; i < msh->m_MSH2.m_Models.size(); ++i)
		{
			MODL& model = msh->m_MSH2.m_Models[i];

			// Create Node to attach mesh to
			FbxNode* meshNode = FbxNode::Create(manager, model.m_Name.m_Text.c_str());
			meshNode->SetShadingMode(FbxNode::eTextureShading);
			rootNode->AddChild(meshNode);
			Scene->AddNode(meshNode);

			// Create and attach Mesh
			if (!MODLToFBXMesh(manager, msh->m_MSH2.m_Models[i], msh->m_MSH2.m_MaterialList, meshNode))
			{
				Log("Failed to convert MSH Model to FBX Mesh. MODL No: " + std::to_string(i) + "  MTYP: " + std::to_string(model.m_ModelType.m_ModelType));
				continue;
			}
		}

		// Export Scene to FBX
		FbxExporter* exporter = FbxExporter::Create(manager, "");
		FbxIOSettings* settings = FbxIOSettings::Create(manager, IOSROOT);
		manager->SetIOSettings(settings);

		if (!exporter->Initialize(fbxFileName.c_str(), -1, manager->GetIOSettings()))
		{
			Log("Initializing export failed!");
		}

		if (!exporter->Export(Scene, false))
		{
			Log("Exporting failed!");
		}

		// Free all
		Scene->Destroy();
		Scene = nullptr;
		
		exporter->Destroy();
		manager->Destroy();

		return true;
	}

	FbxDouble3 Converter::ColorToFBXColor(const Color& color)
	{
		return FbxDouble3(color.m_Red, color.m_Green, color.m_Blue);
	}

	bool Converter::MATDToFBXMaterial(FbxManager* manager, const MATD& material, FbxNode* meshNode, int& matIndex)
	{
		if (meshNode == nullptr)
		{
			Log("Given FbxNode is NULL!");
			return false;
		}

		matIndex = meshNode->GetMaterialIndex(material.m_Name.m_Text.c_str());

		if (matIndex < 0)
		{
			FbxSurfacePhong* fbxMaterial = FbxSurfacePhong::Create(manager, material.m_Name.m_Text.c_str());
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

	bool Converter::MODLToFBXMesh(FbxManager* manager, MODL& model, MATL& materials, FbxNode* meshNode)
	{
		if (manager == nullptr)
		{
			Log("Given FbxManager is NULL!");
			return false;
		}

		if (meshNode == nullptr)
		{
			Log("Given FbxNode is NULL!");
			return false;
		}

		FbxMesh* mesh = FbxMesh::Create(manager, model.m_Name.m_Text.c_str());

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

						if (!MATDToFBXMaterial(manager, mshMat, meshNode, fbxMatIndex))
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
}