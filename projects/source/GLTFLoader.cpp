﻿/**
 * GLTFの読み込みクラス.
 * Microsoft.glTF.CPP を使用.
 *
 *  Microsoft.glTF.CPPはnugetを使用して
 *  [win_vs2017]ディレクトリ内に packages.configと[packages]ディレクトリを配置する必要がある.
 *
 * GLTF 2.0フォーマットの参考:
 * https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md
 */

#include "GLTFLoader.h"

#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/Serialize.h>
#include <GLTFSDK/GLTFResourceWriter.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace Microsoft::glTF;

#include "SceneData.h"

namespace {
	/**
	 * リソース読み込み用.
	 */
	class InStream : public IStreamReader
	{
	public:
		InStream() : m_stream(std::make_shared<std::stringstream>(std::ios_base::app | std::ios_base::binary | std::ios_base::in | std::ios_base::out))
		{
		}

		virtual std::shared_ptr<std::istream> GetInputStream(const std::string&) const override
		{
			return std::dynamic_pointer_cast<std::istream>(m_stream);
		}

	private:
		std::shared_ptr<std::stringstream> m_stream;
	};

	/**
	 * バイナリ読み込み用.
	 */
	class BinStream : public IStreamReader
	{
	private:
		std::string m_basePath;		// gltfファイルの絶対パスのディレクトリ.

	public:
		BinStream (std::string basePath) : m_basePath(basePath)
		{
		}

		virtual std::shared_ptr<std::istream> GetInputStream(const std::string& filename) const override
		{
			const std::string path = m_basePath + std::string("/") + filename;
			return std::make_shared<std::ifstream>(path, std::ios::binary);
		}
	};

	/**
	 * 指定のファイル名の拡張子を取得.
	 */
	std::string getFileExtension (const std::string fileName) {
		std::string fileName2 = fileName;
		std::transform(fileName2.begin(), fileName2.end(), fileName2.begin(), ::tolower);

		const int iPos = fileName2.find_last_of(".");
		if (iPos == std::string::npos) return "";
		return fileName2.substr(iPos + 1);
	}

	/**
	 * GLTFのMesh情報を取得して格納.
	 */
	void storeGLTFMeshes (GLTFDocument& gltfDoc, std::shared_ptr<GLBResourceReader>& reader, CSceneData* sceneData) {
		const size_t meshesSize = gltfDoc.meshes.Size();

		// gltfからの読み込みの場合、buffers[0]のバイナリ要素(*.bin)を取得する必要がある.
		std::shared_ptr<BinStream> binStreamReader;
		std::shared_ptr<GLTFResourceReader> binReader;
		if (!reader) {
			const size_t size = gltfDoc.buffers.Size();
			if (size > 0 && gltfDoc.buffers[0].uri != "") {
				try {
					const std::string fileDir = sceneData->getFileDir();
					if (size > 0) {
						binStreamReader.reset(new BinStream(fileDir));
						binReader.reset(new GLTFResourceReader(*binStreamReader));
					}
				} catch (...) {
					return;
				}
			} else {
				return;
			}
		}

		for (size_t i = 0; i < meshesSize; ++i) {
			sceneData->meshes.push_back(CMeshData());
			CMeshData& dstMeshData = sceneData->meshes[i];

			const Mesh& mesh = gltfDoc.meshes[i];
			const size_t primitivesCou = mesh.primitives.size();
			if (primitivesCou == 0) continue;
			const int primitiveIndex = 0;		// TODO : 複数のprimitiveがある場合は追加処理が必要かも.
			const MeshPrimitive& meshPrim = mesh.primitives[primitiveIndex];

			// メッシュ名.
			dstMeshData.name = mesh.name;

			// マテリアル番号.
			dstMeshData.materialIndex = std::stoi(meshPrim.materialId);

			// 頂点座標を取得.
			if (meshPrim.positionsAccessorId != "") {
				// positionsAccessorIdを取得 → accessorsよりbufferViewIdを取得 → ResourceReaderよりバッファ情報を取得、とたどる.
				const int positionID = std::stoi(meshPrim.positionsAccessorId);
				const Accessor& acce = gltfDoc.accessors[positionID];
				const int bufferViewID = std::stoi(acce.bufferViewId);

				// 頂点座標の配列を取得.
				std::vector<Vector3> vers;
				if (reader) vers = reader->ReadBinaryData<Vector3>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
				else if (binReader) vers = binReader->ReadBinaryData<Vector3>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);

				const size_t versCou = vers.size();
				if (versCou > 0) {
					dstMeshData.vertices.resize(versCou);
					for (size_t j = 0; j < versCou; ++j) {
						dstMeshData.vertices[j] = sxsdk::vec3(vers[j].x, vers[j].y, vers[j].z);
					}
				}
			}

			// 法線を取得.
			if (meshPrim.normalsAccessorId != "") {
				const int normalID = std::stoi(meshPrim.normalsAccessorId);
				const Accessor& acce = gltfDoc.accessors[normalID];
				const int bufferViewID = std::stoi(acce.bufferViewId);

				// 法線の配列を取得.
				std::vector<Vector3> normals;
				if (reader) normals = reader->ReadBinaryData<Vector3>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
				else if (binReader) normals = binReader->ReadBinaryData<Vector3>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);

				const size_t versCou = normals.size();
				if (versCou > 0) {
					dstMeshData.normals.resize(versCou);
					for (size_t j = 0; j < versCou; ++j) {
						dstMeshData.normals[j] = sxsdk::vec3(normals[j].x, normals[j].y, normals[j].z);
					}
				}
			}

			// UV0を取得.
			if (meshPrim.uv0AccessorId != "") {
				const int uv0ID = std::stoi(meshPrim.uv0AccessorId);
				const Accessor& acce = gltfDoc.accessors[uv0ID];
				const int bufferViewID = std::stoi(acce.bufferViewId);

				// UV0の配列を取得.
				// floatの配列で返るため、/2 がUV要素数.
				std::vector<float> uvs;
				if (reader) uvs = reader->ReadBinaryData<float>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
				else if (binReader) uvs = binReader->ReadBinaryData<float>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);

				const size_t versCou = uvs.size() / 2;
				if (versCou > 0) {
					dstMeshData.uv0.resize(versCou);
					for (size_t j = 0, iPos = 0; j < versCou; ++j, iPos += 2) {
						dstMeshData.uv0[j] = sxsdk::vec2(uvs[iPos + 0], uvs[iPos + 1]);
					}
				}
			}
			// UV1を取得.
			if (meshPrim.uv1AccessorId != "") {
				const int uv1ID = std::stoi(meshPrim.uv1AccessorId);
				const Accessor& acce = gltfDoc.accessors[uv1ID];
				const int bufferViewID = std::stoi(acce.bufferViewId);

				// UV1の配列を取得.
				// floatの配列で返るため、/2 がUV要素数.
				std::vector<float> uvs;
				if (reader) uvs = reader->ReadBinaryData<float>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
				else if (binReader) uvs = binReader->ReadBinaryData<float>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);

				const size_t versCou = uvs.size() / 2;
				if (versCou > 0) {
					dstMeshData.uv1.resize(versCou);
					for (size_t j = 0, iPos = 0; j < versCou; ++j, iPos += 2) {
						dstMeshData.uv1[j] = sxsdk::vec2(uvs[iPos + 0], uvs[iPos + 1]);
					}
				}
			}

			// 三角形の頂点インデックスを取得.
			if (meshPrim.indicesAccessorId != "") {
				const int indicesID = std::stoi(meshPrim.indicesAccessorId);
				const Accessor& acce = gltfDoc.accessors[indicesID];
				const int bufferViewID = std::stoi(acce.bufferViewId);

				// 頂点インデックスの配列を取得.
				if (reader) {
					dstMeshData.triangleIndices = reader->ReadBinaryData<int>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
				} else if (binReader) {
					dstMeshData.triangleIndices = binReader->ReadBinaryData<int>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
				}
			}
		}
	}

	/**
	 * GLTFのMaterial情報を取得して格納.
	 */
	void storeGLTFMaterials (GLTFDocument& gltfDoc, std::shared_ptr<GLBResourceReader>& reader, CSceneData* sceneData) {
		const size_t materialsSize   = gltfDoc.materials.Size();
		const size_t imagesSize      = gltfDoc.images.Size();

		// マテリアル情報を取得.
		for (size_t i = 0; i < materialsSize; ++i) {
			sceneData->materials.push_back(CMaterialData());
			CMaterialData& dstMaterialData = sceneData->materials.back();

			const Material& material = gltfDoc.materials[i];

			dstMaterialData.name = material.name;
			dstMaterialData.alphaCutOff = material.alphaCutoff;
			dstMaterialData.alphaMode   = material.alphaMode;
			dstMaterialData.doubleSided = material.doubleSided;
			{
				Color4 col = material.metallicRoughness.baseColorFactor;
				dstMaterialData.baseColorFactor = sxsdk::rgb_class(col.r, col.g, col.b);
			}
			{
				Color3 col = material.emissiveFactor;
				dstMaterialData.emissionFactor = sxsdk::rgb_class(col.r, col.g, col.b);
			}
			dstMaterialData.metallicFactor    = material.metallicRoughness.metallicFactor;
			dstMaterialData.roughnessFactor   = material.metallicRoughness.roughnessFactor;
			dstMaterialData.occlusionStrength = material.occlusionTexture.strength;

			// BaseColorのテクスチャIDを取得.
			if (material.metallicRoughness.baseColorTexture.textureId != "") {
				// テクスチャIDを取得.
				const int textureID = std::stoi(material.metallicRoughness.baseColorTexture.textureId);

				// イメージIDを取得.
				const Texture& texture = gltfDoc.textures[textureID];
				if (texture.imageId != "") {
					dstMaterialData.baseColorImageIndex = std::stoi(texture.imageId);
				}
			}

			// 法線のテクスチャIDを取得.
			if (material.normalTexture.textureId != "") {
				// テクスチャIDを取得.
				const int textureID = std::stoi(material.normalTexture.textureId);

				// イメージIDを取得.
				const Texture& texture = gltfDoc.textures[textureID];
				if (texture.imageId != "") {
					dstMaterialData.normalImageIndex = std::stoi(texture.imageId);
				}
			}

			// EmissionのテクスチャIDを取得.
			if (material.emissiveTexture.textureId != "") {
				// テクスチャIDを取得.
				const int textureID = std::stoi(material.emissiveTexture.textureId);

				// イメージIDを取得.
				const Texture& texture = gltfDoc.textures[textureID];
				if (texture.imageId != "") {
					dstMaterialData.emissionImageIndex = std::stoi(texture.imageId);
				}
			}

			// Metallic/RoughnessのテクスチャIDを取得.
			if (material.metallicRoughness.metallicRoughnessTexture.textureId != "") {
				// テクスチャIDを取得.
				const int textureID = std::stoi(material.metallicRoughness.metallicRoughnessTexture.textureId);

				// イメージIDを取得.
				const Texture& texture = gltfDoc.textures[textureID];
				if (texture.imageId != "") {
					dstMaterialData.metallicRoughnessImageIndex = std::stoi(texture.imageId);
				}
			}

			// OcclusionのテクスチャIDを取得.
			if (material.occlusionTexture.textureId != "") {
				// テクスチャIDを取得.
				const int textureID = std::stoi(material.occlusionTexture.textureId);

				// イメージIDを取得.
				const Texture& texture = gltfDoc.textures[textureID];
				if (texture.imageId != "") {
					dstMaterialData.occlusionImageIndex = std::stoi(texture.imageId);
				}
			}
		}

		// イメージ名はGLTFには格納されないようであるので、.
		// 形状名とマテリアル情報より「xxx_baseColor」などのように復元する.
		for (size_t i = 0; i < materialsSize; ++i) {
			const CMaterialData& materialD = sceneData->materials[i];
			if (materialD.name == "") continue;

			if (materialD.baseColorImageIndex >= 0) {
				CImageData& imageD = sceneData->images[materialD.baseColorImageIndex];
				if (imageD.name == "") {
					imageD.name = materialD.name + std::string("_baseColor");
					imageD.imageMask = CImageData::gltf_image_mask_base_color;
				}
			}
			if (materialD.emissionImageIndex >= 0) {
				CImageData& imageD = sceneData->images[materialD.emissionImageIndex];
				if (imageD.name == "") {
					imageD.name = materialD.name + std::string("_emission");
					imageD.imageMask = CImageData::gltf_image_mask_emission;
				}
			}
			if (materialD.normalImageIndex >= 0) {
				CImageData& imageD = sceneData->images[materialD.normalImageIndex];
				if (imageD.name == "") {
					imageD.name = materialD.name + std::string("_normal");
					imageD.imageMask = CImageData::gltf_image_mask_normal;
				}
			}
			if (materialD.metallicRoughnessImageIndex >= 0) {
				CImageData& imageD = sceneData->images[materialD.metallicRoughnessImageIndex];
				if (imageD.name == "") {
					if (materialD.occlusionImageIndex == materialD.metallicRoughnessImageIndex) {
						// Occlusionも含む場合.
						imageD.name = materialD.name + std::string("_occlusionRoughnessMetallic");
						imageD.imageMask = CImageData::gltf_image_mask_occlusion | CImageData::gltf_image_mask_roughness | CImageData::gltf_image_mask_metallic;
					} else {
						imageD.name = materialD.name + std::string("_roughnessMetallic");
						imageD.imageMask = CImageData::gltf_image_mask_roughness | CImageData::gltf_image_mask_metallic;
					}
				}
			}

			if (materialD.occlusionImageIndex >= 0) {
				CImageData& imageD = sceneData->images[materialD.occlusionImageIndex];
				if (imageD.name == "") {
					imageD.name = materialD.name + std::string("_occlusion");
					imageD.imageMask = CImageData::gltf_image_mask_occlusion;
				}
			}
		}
		for (size_t i = 0; i < imagesSize; ++i) {
			CImageData& imageD = sceneData->images[i];
			if (imageD.name == "") {
				imageD.name = std::string("image_") + std::to_string(i);
			}
		}
	}

	/**
	 * GLTFのImage情報を取得して格納.
	 */
	void storeGLTFImages (GLTFDocument& gltfDoc, std::shared_ptr<GLBResourceReader>& reader, CSceneData* sceneData) {
		const size_t imagesSize = gltfDoc.images.Size();

		// GLTFから読み込む場合は、外部の画像を読み込みすることになる.
		// そのためのReaderを作成.
		std::shared_ptr<BinStream> binStreamReader;
		std::shared_ptr<GLTFResourceReader> binReader;
		if (!reader) {
			try {
				const std::string fileDir = sceneData->getFileDir();
				binStreamReader.reset(new BinStream(fileDir));
				binReader.reset(new GLTFResourceReader(*binStreamReader));
			} catch (...) {
				return;
			}
		}

		for (size_t i = 0; i < imagesSize; ++i) {
			sceneData->images.push_back(CImageData());
			CImageData& dstImageData = sceneData->images.back();

			const Image& image = gltfDoc.images[i];
			if (!reader) {
				if (image.uri == "") continue;
				// 画像ファイルの拡張子を取得.
				const std::string extStr = ::getFileExtension(image.uri);
				if (extStr != "") {
					dstImageData.name     = image.uri;
					dstImageData.mimeType = std::string("image/") + extStr;
				}
			} else {
				dstImageData.name     = image.name;
				dstImageData.mimeType = image.mimeType;
			}

			// 画像バッファを取得.
			const std::vector<uint8_t> imageData = (reader) ? (reader->ReadBinaryData(gltfDoc, image)) : (binReader->ReadBinaryData(gltfDoc, image));
			const size_t dCou = imageData.size();
			dstImageData.imageDatas.resize(dCou);
			for (int i = 0; i < dCou; ++i) dstImageData.imageDatas[i] = imageData[i];
		}
	}

	/**
	 * ノード階層を格納.
	 */
	void storeGLTFNodes (GLTFDocument& gltfDoc, std::shared_ptr<GLBResourceReader>& reader, CSceneData* sceneData) {
		if (!reader) return;
	}
}

CGLTFLoader::CGLTFLoader ()
{
}

/**
 * 指定のGLTFファイルを読み込み.
 * @param[in]  fileName    読み込み形状名 (gltfまたはglb).
 * @param[out] sceneData   読み込んだGLTFのシーン情報が返る.
 */
bool CGLTFLoader::loadGLTF (const std::string& fileName, CSceneData* sceneData)
{
	if (!sceneData) return false;

	sceneData->clear();

	// ファイル名を格納.
	{
		sceneData->filePath = fileName;
		sceneData->fileName = "";
		{
			const int iPos = fileName.find_last_of("/");
			if (iPos > 0) {
				sceneData->fileName = fileName.substr(iPos + 1);
			}
		}
		if ((sceneData->fileName) == "") {
			const int iPos = fileName.find_last_of("\\");
			if (iPos > 0) {
				sceneData->fileName = fileName.substr(iPos + 1);
			}
		}
	}

	std::shared_ptr<GLBResourceReader> reader;
	GLTFDocument gltfDoc;
	std::string jsonStr = "";

	// fileNameがglbファイルかどうか.
	const bool glbFile = (::getFileExtension(fileName) == std::string("glb"));

	// gltf/glbの拡張子より、読み込みを分岐.
	if (glbFile) {
		try {
			// glbファイルを読み込み.
			auto glbStream = std::make_shared<std::ifstream>(fileName, std::ios::binary);
			auto streamReader = std::make_unique<InStream>();
			reader.reset(new GLBResourceReader(*streamReader, glbStream));

			// glbファイルからjson部を取得.
			jsonStr = reader->GetJson();
		
		} catch (...) {
			return false;
		}
	} else {				// gltfファイルを読み込み.
		// 拡張子gltfを読み込んだ場合は、.
		// 読み込んだgltfファイル(json)からbuffers/imagesのuriを使ってbinや画像を別途読み込む必要がある.
		try {
			std::ifstream gltfStream(fileName);
			if (!gltfStream) return false;

			// json部を取得.
			jsonStr = "";
			{
				std::string str;
				while (!gltfStream.eof()) {
					std::getline(gltfStream, str);
					jsonStr += str + std::string("\n");
				}
			}
		} catch (...) {
			return false;
		}
	}

	{
		// jsonデータをパース.
		try {
			gltfDoc = DeserializeJson(jsonStr);		// TODO : glbによってはここで例外発生する場合がある.
		} catch (...) {
			return false;
		}

		// 各要素の数を取得.
		const size_t meshesSize      = gltfDoc.meshes.Size();
		const size_t materialsSize   = gltfDoc.materials.Size();
		const size_t accessorsSize   = gltfDoc.accessors.Size();
		const size_t buffersSize     = gltfDoc.buffers.Size();
		const size_t bufferViewsSize = gltfDoc.bufferViews.Size();
		const size_t imagesSize      = gltfDoc.images.Size();

		// Asset情報を取得.
		sceneData->assetVersion   = gltfDoc.asset.version;
		sceneData->assetGenerator = gltfDoc.asset.generator;
		sceneData->assetCopyRight = gltfDoc.asset.copyright;

		// メッシュ情報を取得.
		::storeGLTFMeshes(gltfDoc, reader, sceneData);

		// イメージ情報を取得.
		::storeGLTFImages(gltfDoc, reader, sceneData);

		// マテリアル情報を取得.
		::storeGLTFMaterials(gltfDoc, reader, sceneData);

		// ノード階層を取得.
		::storeGLTFNodes(gltfDoc, reader, sceneData);

		return true;
	}
}

