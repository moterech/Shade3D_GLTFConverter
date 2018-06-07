﻿/**
 * GLTFの読み込みクラス.
 * Microsoft.glTF.CPP を使用.
 *
 *  Microsoft.glTF.CPPはnugetを使用して
 *  [win_vs2017]ディレクトリ内に packages.configと[packages]ディレクトリを配置する必要がある.
 *
 * GLTF 2.0フォーマットの参考:
 * https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md
 *
 * 以下は、Microsoft.glTF.CPP の使い方の参考になる.
 * https://github.com/Microsoft/glTF-Toolkit
 */

#include "GLTFLoader.h"
#include "StringUtil.h"

#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/Serialize.h>
#include <GLTFSDK/GLTFResourceWriter.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/GLTFResourceReader.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace Microsoft::glTF;

// 以下はGLTFSDK/rapidjsonのincludeよりも後に指定しないとビルドエラーになる.
#include "SceneData.h"

namespace {
	std::string g_errorMessage = "";		// エラーメッセージの保持用.

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
	class BinStreamReader : public IStreamReader
	{
	private:
		std::string m_basePath;		// gltfファイルの絶対パスのディレクトリ.

	public:
		BinStreamReader (std::string basePath) : m_basePath(basePath)
		{
		}

		virtual std::shared_ptr<std::istream> GetInputStream(const std::string& filename) const override
		{
			const std::string path = m_basePath + std::string("/") + filename;
			return std::make_shared<std::ifstream>(path, std::ios::binary);
		}
	};

	/**
	 * GLTFのMesh情報を取得して格納.
	 */
	void storeGLTFMeshes (GLTFDocument& gltfDoc, std::shared_ptr<GLBResourceReader>& reader, CSceneData* sceneData) {
		const size_t meshesSize = gltfDoc.meshes.Size();

		// gltfからの読み込みの場合、buffers[0]のバイナリ要素(*.bin)を取得する必要がある.
		std::shared_ptr<BinStreamReader> binStreamReader;
		std::shared_ptr<GLTFResourceReader> binReader;
		if (!reader) {
			try {
				const std::string fileDir = sceneData->getFileDir();
				binStreamReader.reset(new BinStreamReader(fileDir));
				binReader.reset(new GLTFResourceReader(*binStreamReader));
			} catch (...) {
				g_errorMessage = std::string("Bin file could not be loaded.");
				return;
			}
		}

		for (size_t i = 0; i < meshesSize; ++i) {
			const int meshIndex = sceneData->appendNewMeshData();
			CMeshData& dstMeshData = sceneData->getMeshData(meshIndex);

			const Mesh& mesh = gltfDoc.meshes[i];
			const size_t primitivesCou = mesh.primitives.size();
			if (primitivesCou == 0) continue;
			dstMeshData.name = mesh.name;

			for (size_t primLoop = 0; primLoop < primitivesCou; ++primLoop) {
				dstMeshData.primitives.push_back(CPrimitiveData());
				CPrimitiveData& dstPrimitiveData = dstMeshData.primitives.back();

				const MeshPrimitive& meshPrim = mesh.primitives[primLoop];

				// meshMode = MESH_TRIANGLES(4)の場合は、三角形.
				MeshMode meshMode = meshPrim.mode;

				// メッシュ名.
				dstPrimitiveData.name = mesh.name;

				// マテリアル番号.
				dstPrimitiveData.materialIndex = std::stoi(meshPrim.materialId);

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

					const int offsetI = acce.byteOffset / (sizeof(float) * 3);

					const size_t versCou = vers.size();
					if (versCou > 0 && acce.count > 0) {
						dstPrimitiveData.vertices.resize(acce.count);
						for (size_t j = 0; j < acce.count; ++j) {
							dstPrimitiveData.vertices[j] = sxsdk::vec3(vers[j + offsetI].x, vers[j + offsetI].y, vers[j + offsetI].z);
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

					const int offsetI = acce.byteOffset / (sizeof(float) * 3);

					const size_t versCou = normals.size();
					if (versCou > 0 && acce.count > 0) {
						dstPrimitiveData.normals.resize(acce.count);
						for (size_t j = 0; j < acce.count; ++j) {
							dstPrimitiveData.normals[j] = sxsdk::vec3(normals[j + offsetI].x, normals[j + offsetI].y, normals[j + offsetI].z);
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

					const int offsetI = acce.byteOffset / (sizeof(float));

					const size_t versCou = uvs.size();
					if (versCou > 0 && acce.count > 0) {
						dstPrimitiveData.uv0.resize(acce.count);

						for (size_t j = 0, iPos = offsetI; j < acce.count; ++j, iPos += 2) {
							dstPrimitiveData.uv0[j] = sxsdk::vec2(uvs[iPos + 0], uvs[iPos + 1]);
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

					const int offsetI = acce.byteOffset / (sizeof(float));

					const size_t versCou = uvs.size();
					if (versCou > 0 && acce.count > 0) {
						dstPrimitiveData.uv1.resize(acce.count);
						for (size_t j = 0, iPos = offsetI; j < acce.count; ++j, iPos += 2) {
							dstPrimitiveData.uv1[j] = sxsdk::vec2(uvs[iPos + 0], uvs[iPos + 1]);
						}
					}
				}

				// Color0を取得.
				if (meshPrim.color0AccessorId != "") {
					const int color0ID = std::stoi(meshPrim.color0AccessorId);
					const Accessor& acce = gltfDoc.accessors[color0ID];
					const int bufferViewID = std::stoi(acce.bufferViewId);

					if (acce.componentType == COMPONENT_FLOAT && (acce.type == TYPE_VEC4 || acce.type == TYPE_VEC3)) {
						// Color0の配列を取得.
						// floatの配列で返るため、/4 または /3 がColor0要素数.
						std::vector<float> color0;
						if (reader) color0 = reader->ReadBinaryData<float>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						else if (binReader) color0 = binReader->ReadBinaryData<float>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);

						const int fCou = (acce.type == TYPE_VEC4) ? 4 : 3;
						const int offsetI = acce.byteOffset / (sizeof(float));

						const size_t versCou = color0.size();
						if (versCou > 0 && acce.count > 0) {
							dstPrimitiveData.color0.resize(acce.count);
							if (fCou == 4) {
								for (size_t j = 0, iPos = offsetI; j < acce.count; ++j, iPos += fCou) {
									dstPrimitiveData.color0[j] = sxsdk::vec4(color0[iPos + 0], color0[iPos + 1], color0[iPos + 2], color0[iPos + 3]);
								}
							} else {
								for (size_t j = 0, iPos = offsetI; j < acce.count; ++j, iPos += fCou) {
									dstPrimitiveData.color0[j] = sxsdk::vec4(color0[iPos + 0], color0[iPos + 1], color0[iPos + 2], 1.0f);
								}
							}
						}
					} else if (acce.componentType == COMPONENT_UNSIGNED_BYTE && (acce.type == TYPE_VEC4 || acce.type == TYPE_VEC3)) {
						// Color0の配列を取得.
						// unsigned charの配列で返るため、/4 または /3 がColor0要素数.
						std::vector<unsigned char> color0;
						if (reader) color0 = reader->ReadBinaryData<unsigned char>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						else if (binReader) color0 = binReader->ReadBinaryData<unsigned char>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);

						const int fCou = (acce.type == TYPE_VEC4) ? 4 : 3;
						const int offsetI = acce.byteOffset / (sizeof(unsigned char));

						const size_t versCou = color0.size();
						if (versCou > 0 && acce.count > 0) {
							dstPrimitiveData.color0.resize(acce.count);
							if (fCou == 4) {
								for (size_t j = 0, iPos = offsetI; j < acce.count; ++j, iPos += fCou) {
									dstPrimitiveData.color0[j] = sxsdk::vec4((float)color0[iPos + 0] / 255.0f, (float)color0[iPos + 1] / 255.0f, (float)color0[iPos + 2] / 255.0f, (float)color0[iPos + 3] / 255.0f);
								}
							} else {
								for (size_t j = 0, iPos = offsetI; j < acce.count; ++j, iPos += fCou) {
									dstPrimitiveData.color0[j] = sxsdk::vec4((float)color0[iPos + 0] / 255.0f, (float)color0[iPos + 1] / 255.0f, (float)color0[iPos + 2] / 255.0f, 1.0f);
								}
							}
						}
					}

				}


				// 三角形の頂点インデックスを取得.
				if (meshPrim.indicesAccessorId != "") {
					const int indicesID = std::stoi(meshPrim.indicesAccessorId);
					const Accessor& acce = gltfDoc.accessors[indicesID];
					const int bufferViewID = std::stoi(acce.bufferViewId);

					// COMPONENT_BYTE(5120) / COMPONENT_UNSIGNED_SHORT(5123) / COMPONENT_UNSIGNED_INT(5125) / COMPONENT_FLOAT(5126)など.
					ComponentType compType = acce.componentType;

					if (compType == COMPONENT_UNSIGNED_BYTE) {		// byteデータとして取得.
						std::vector<unsigned char> indices;
						if (reader) {
							indices = reader->ReadBinaryData<unsigned char>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						} else if (binReader) {
							indices = binReader->ReadBinaryData<unsigned char>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						}

						const int offsetI = acce.byteOffset / sizeof(unsigned char);

						dstPrimitiveData.triangleIndices.resize(acce.count);
						for (size_t j = 0; j < acce.count; ++j) {
							dstPrimitiveData.triangleIndices[j] = (int)indices[j + offsetI];
						}

					} else if (compType == COMPONENT_UNSIGNED_SHORT) {	// shortデータとして取得.
						std::vector<unsigned short> indices;
						if (reader) {
							indices = reader->ReadBinaryData<unsigned short>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						} else if (binReader) {
							indices = binReader->ReadBinaryData<unsigned short>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						}

						const int offsetI = acce.byteOffset / sizeof(unsigned short);

						dstPrimitiveData.triangleIndices.resize(acce.count);
						for (size_t j = 0; j < acce.count; ++j) {
							dstPrimitiveData.triangleIndices[j] = (int)indices[j + offsetI];
						}

					} else {			// intデータとして取得.
						std::vector<int> indices;
						if (reader) {
							indices = reader->ReadBinaryData<int>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						} else if (binReader) {
							indices = binReader->ReadBinaryData<int>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						}

						const int offsetI = acce.byteOffset / sizeof(int);

						dstPrimitiveData.triangleIndices.resize(acce.count);
						for (size_t j = 0; j < acce.count; ++j) {
							dstPrimitiveData.triangleIndices[j] = indices[j + offsetI];
						}
					}
				}

				// スキンのWeightを取得.
				if (meshPrim.weights0AccessorId != "") {
					const int weightsID = std::stoi(meshPrim.weights0AccessorId);
					const Accessor& acce = gltfDoc.accessors[weightsID];
					const int bufferViewID = std::stoi(acce.bufferViewId);

					// Weightの配列を取得.
					// VEC4として入る。xyzwに対してウエイト値が入り、合計すると1.0となる.
					std::vector<float> weights;
					if (acce.componentType == COMPONENT_FLOAT) {
						if (reader) weights = reader->ReadBinaryData<float>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						else if (binReader) weights = binReader->ReadBinaryData<float>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
					}

					const size_t vCou = weights.size() / 4;
					if (vCou > 0 && vCou == acce.count) {
						dstPrimitiveData.skinWeights.resize(vCou);
						for (size_t j = 0, iPos = 0; j < vCou; ++j, iPos += 4) {
							dstPrimitiveData.skinWeights[j] = sxsdk::vec4(weights[iPos + 0], weights[iPos + 1], weights[iPos + 2], weights[iPos + 3]);
						}
					}
				}

				// スキンのJointsを取得.
				if (meshPrim.joints0AccessorId != "") {
					const int jointsID = std::stoi(meshPrim.joints0AccessorId);
					const Accessor& acce = gltfDoc.accessors[jointsID];
					const int bufferViewID = std::stoi(acce.bufferViewId);

					// Jointsの配列を取得.
					// VEC4として入る。xyzwに対してJointインデックスが入る.
					std::vector<int> joints;
					if (acce.componentType == COMPONENT_UNSIGNED_SHORT) {
						std::vector<unsigned short> joints2;
						if (reader) joints2 = reader->ReadBinaryData<unsigned short>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						else if (binReader) joints2 = binReader->ReadBinaryData<unsigned short>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						joints.resize(joints2.size());
						for (size_t j = 0; j < joints2.size(); ++j) joints[j] = (int)joints2[j];

					} else if (acce.componentType == COMPONENT_UNSIGNED_INT) {
						if (reader) joints = reader->ReadBinaryData<int>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
						else if (binReader) joints = binReader->ReadBinaryData<int>(gltfDoc, gltfDoc.bufferViews[bufferViewID]);
					}

					const size_t vCou = joints.size() / 4;
					if (vCou > 0 && vCou == acce.count) {
						dstPrimitiveData.skinJoints.resize(vCou);
						for (size_t j = 0, iPos = 0; j < vCou; ++j, iPos += 4) {
							dstPrimitiveData.skinJoints[j] = sx::vec<int,4>(joints[iPos + 0], joints[iPos + 1], joints[iPos + 2], joints[iPos + 3]);
						}
					}
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
			dstMaterialData.alphaMode   = material.alphaMode;		// ALPHA_OPAQUE / ALPHA_BLEND / ALPHA_MASK.
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

				dstMaterialData.baseColorTexCoord = (int)material.metallicRoughness.baseColorTexture.texCoord;
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

				dstMaterialData.normalTexCoord = (int)material.normalTexture.texCoord;
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
				dstMaterialData.emissionTexCoord = (int)material.emissiveTexture.texCoord;
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
				dstMaterialData.metallicRoughnessTexCoord = (int)material.metallicRoughness.metallicRoughnessTexture.texCoord;
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
				dstMaterialData.occlusionTexCoord = (int)material.occlusionTexture.texCoord;
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
		std::shared_ptr<BinStreamReader> binStreamReader;
		std::shared_ptr<GLTFResourceReader> binReader;
		if (!reader) {
			try {
				const std::string fileDir = sceneData->getFileDir();
				binStreamReader.reset(new BinStreamReader(fileDir));
				binReader.reset(new GLTFResourceReader(*binStreamReader));
			} catch (...) {
				g_errorMessage = std::string("Bin file could not be loaded.");
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
				const std::string extStr = StringUtil::getFileExtension(image.uri);
				if (extStr != "") {
					dstImageData.name     = StringUtil::getFileName(image.uri);
					dstImageData.mimeType = std::string("image/") + extStr;
				}
			} else {
				dstImageData.name     = StringUtil::getFileName(image.name);
				dstImageData.mimeType = image.mimeType;
			}

			// 画像バッファを取得.
			try {
			const std::vector<uint8_t> imageData = (reader) ? (reader->ReadBinaryData(gltfDoc, image)) : (binReader->ReadBinaryData(gltfDoc, image));
				const size_t dCou = imageData.size();
				dstImageData.imageDatas.resize(dCou);
				for (int i = 0; i < dCou; ++i) dstImageData.imageDatas[i] = imageData[i];
			} catch (GLTFException e) {
				//g_errorMessage = std::string(e.what());
				dstImageData.clear();
			}
		}
	}

	/**
	 * ノード階層を格納.
	 */
	void storeGLTFNodes (GLTFDocument& gltfDoc, std::shared_ptr<GLBResourceReader>& reader, CSceneData* sceneData) {
		const size_t nodesCou = gltfDoc.nodes.Size();

		sceneData->nodes.clear();
		if (nodesCou == 0) return;
		sceneData->nodes.resize(nodesCou + 1);		// 0番目はルートノードとする.

		for (size_t i = 0; i < nodesCou; ++i) {
			CNodeData& dstNodeD = sceneData->nodes[i];
			dstNodeD.clear();
		}

		for (size_t i = 0; i < nodesCou; ++i) {
			const Node& node = gltfDoc.nodes[i];
			CNodeData& dstNodeD = sceneData->nodes[i + 1];

			dstNodeD.name = node.name;
			if (node.GetTransformationType() == TRANSFORMATION_MATRIX) {
				// 行列としての取得.
				sxsdk::mat4 m;
				int iPos = 0;
				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						m[x][y] = node.matrix.values[iPos++];		// 転置して入れている. 
					}
				}
				sxsdk::vec3 trans, scale, rotate, shear;
				m.unmatrix(scale, shear, rotate, trans);
				dstNodeD.translation = trans;
				dstNodeD.scale       = scale;
				dstNodeD.rotation    = sxsdk::quaternion_class(rotate);
			} else {
				dstNodeD.translation = sxsdk::vec3(node.translation.x, node.translation.y, node.translation.z);
				dstNodeD.scale       = sxsdk::vec3(node.scale.x, node.scale.y, node.scale.z);
				dstNodeD.rotation    = sxsdk::quaternion_class(-node.rotation.w, node.rotation.x, node.rotation.y, node.rotation.z);
			}

			if (node.meshId != "") dstNodeD.meshIndex = std::stoi(node.meshId);
			if (node.skinId != "") dstNodeD.skinIndex = std::stoi(node.skinId);

			// 子の数.
			const size_t childCou = node.children.size();
			if (childCou == 0) continue;

			dstNodeD.childNodeIndex = std::stoi(node.children[0]) + 1;
			for (size_t j = 0; j < childCou; ++j) {
				const int cIndex = std::stoi(node.children[j]) + 1;
				sceneData->nodes[cIndex].parentNodeIndex = i + 1;
			}
			{
				int cIndex0 = std::stoi(node.children[0]) + 1;
				for (size_t j = 1; j < childCou; ++j) {
					const int cIndex = std::stoi(node.children[j]) + 1;
					sceneData->nodes[cIndex].prevNodeIndex  = cIndex0;
					sceneData->nodes[cIndex0].nextNodeIndex = cIndex;
					cIndex0 = cIndex;
				}
			}
		}

		// 親が存在しないノードをルートの子とする.
		{
			std::vector<int> rootChildIndex;
			CNodeData& rootNodeD = sceneData->nodes[0];
			rootNodeD.name = "root";

			for (size_t i = 0; i < nodesCou; ++i) {
				CNodeData& nodeD = sceneData->nodes[i + 1];
				if (nodeD.parentNodeIndex < 0) {
					rootChildIndex.push_back(i + 1);
				}
			}
			if (!rootChildIndex.empty()) {
				const size_t childCou = rootChildIndex.size();
				rootNodeD.childNodeIndex = rootChildIndex[0];
				int cIndex0 = rootNodeD.childNodeIndex;
				for (size_t j = 1; j < childCou; ++j) {
					const int cIndex = rootChildIndex[j];
					sceneData->nodes[cIndex].prevNodeIndex  = cIndex0;
					sceneData->nodes[cIndex0].nextNodeIndex = cIndex;
					cIndex0 = cIndex;
				}
				for (size_t j = 0; j < childCou; ++j) {
					const int cIndex = rootChildIndex[j];
					sceneData->nodes[cIndex].parentNodeIndex = 0;
				}
			}
		}
	}

	/**
	 * 指定のノードの子をボーンにする再帰.
	 */
	void setNodeBoneLoop (CSceneData* sceneData, const int nodeIndex, const bool setBone) {
		CNodeData& nodeD = sceneData->nodes[nodeIndex];

		const bool setBone2 = setBone || nodeD.isBone;
		if (!nodeD.isBone && nodeD.meshIndex < 0) nodeD.isBone = setBone2;

		if (nodeD.childNodeIndex >= 0) {
			int nodeIndex2 = nodeD.childNodeIndex;
			while (nodeIndex2 >= 0) {
				setNodeBoneLoop(sceneData, nodeIndex2, setBone2);
				nodeIndex2 = sceneData->nodes[nodeIndex2].nextNodeIndex;
			}
		}
	}

	/**
	 * skins要素のjointsで参照されているノードについては、ノードにボーンフラグ(isBone=true)を立てる.
	 */
	void checkNodesBone (CSceneData* sceneData) {
		const size_t nodesCou = sceneData->nodes.size();
		const size_t skinsCou = sceneData->skins.size();
		if (skinsCou == 0) return;

		for (size_t i = 0; i < skinsCou; ++i) {
			const CSkinData& skinD = sceneData->skins[i];
			const size_t jointsCou = skinD.joints.size();
			for (size_t j = 0; j < jointsCou; ++j) {
				const int index = skinD.joints[j] + 1;		// nodesの0番目はルートとして追加しているため、jointsの参照インデックスは+1.
				if (index >= 0 && index < nodesCou) {
					sceneData->nodes[index].isBone = true;
				}
			}
		}

		// ボーン対象のノードの子は、すべてボーンとして扱う.
		setNodeBoneLoop(sceneData, 0, false);
	}

	/**
	 * Skin情報を格納.
	 */
	void storeGLTFSkins (GLTFDocument& gltfDoc, std::shared_ptr<GLBResourceReader>& reader, CSceneData* sceneData) {
		const size_t skinsCou = gltfDoc.skins.Size();

		sceneData->skins.clear();
		if (skinsCou == 0) return;

		// gltfからの読み込みの場合、buffers[0]のバイナリ要素(*.bin)を取得する必要がある.
		std::shared_ptr<BinStreamReader> binStreamReader;
		std::shared_ptr<GLTFResourceReader> binReader;
		if (!reader) {
			try {
				const std::string fileDir = sceneData->getFileDir();
				binStreamReader.reset(new BinStreamReader(fileDir));
				binReader.reset(new GLTFResourceReader(*binStreamReader));
			} catch (...) {
				g_errorMessage = std::string("Bin file could not be loaded.");
				return;
			}
		}

		sceneData->skins.resize(skinsCou);

		for (size_t i = 0; i < skinsCou; ++i) {
			const Skin& skin = gltfDoc.skins[i];

			CSkinData& skinD = sceneData->skins[i];
			skinD.clear();
			skinD.name = skin.name;
			if (skin.skeletonId != "") skinD.skeletonID = std::stoi(skin.skeletonId);
			if (skin.inverseBindMatricesAccessorId != "") {
				// TODO : 不要 ?.
				const int accessorID = std::stoi(skin.inverseBindMatricesAccessorId);

			}
			
			const size_t jointsCou = skin.jointIds.size();
			if (jointsCou > 0) {
				skinD.joints.resize(jointsCou);
				for (size_t j = 0; j < jointsCou; ++j) {
					skinD.joints[j] = std::stoi(skin.jointIds[j]);
				}
			}
		}

		// skin情報より、nodesのノードがボーンになりうるかチェック.
		checkNodesBone(sceneData);
	}
}

CGLTFLoader::CGLTFLoader ()
{
}

/**
 * エラー時の文字列取得.
 */
std::string CGLTFLoader::getErrorString () const
{
	return g_errorMessage;
}

/**
 * 指定のGLTFファイルを読み込み.
 * @param[in]  fileName    読み込み形状名 (gltfまたはglb).
 * @param[out] sceneData   読み込んだGLTFのシーン情報が返る.
 */
bool CGLTFLoader::loadGLTF (const std::string& fileName, CSceneData* sceneData)
{
	if (!sceneData) return false;

	g_errorMessage = "";
	sceneData->clear();

	// ファイル名(フルパス)を格納.
	sceneData->filePath = fileName;

	std::shared_ptr<GLBResourceReader> reader;
	GLTFDocument gltfDoc;
	std::string jsonStr = "";

	// fileNameがglbファイルかどうか.
	const bool glbFile = (StringUtil::getFileExtension(fileName) == std::string("glb"));

	// gltf/glbの拡張子より、読み込みを分岐.
	if (glbFile) {
		try {
			// glbファイルを読み込み.
			auto glbStream = std::make_shared<std::ifstream>(fileName, std::ios::binary);
			auto streamReader = std::make_unique<InStream>();
			reader.reset(new GLBResourceReader(*streamReader, glbStream));

			// glbファイルからjson部を取得.
			jsonStr = reader->GetJson();
		
		} catch (GLTFException e) {
			g_errorMessage = std::string(e.what());
			return false;
		} catch (...) {
			g_errorMessage = std::string("glb file could not be loaded.");
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
			g_errorMessage = std::string("gltf file could not be loaded.");
			return false;
		}
	}

	// jsonStr内のjsonテキストのうち、.
	//    "JOINTS_0":-1
	//    "WEIGHTS_0":-1
	// のようなマイナス値があると、次の「DeserializeJson」で例外が発生する.
	//    "targets":[] 
	// のような何も定義されていないものがあっても例外発生.
	// 
	// このような記述がある場合は削除したほうがよい.
	
	// jsonデータより、不要なデータを削除.
	if (!m_checkPreDeserializeJson(jsonStr, jsonStr)) return false;

	try {
		// jsonデータをパース.
		try {
			gltfDoc = DeserializeJson(jsonStr);
		} catch (GLTFException e) {
			g_errorMessage = std::string(e.what());
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

		// スキン情報を取得.
		::storeGLTFSkins(gltfDoc, reader, sceneData);

		if (g_errorMessage != "") return false;
		return true;

	} catch (GLTFException e) {
		g_errorMessage = std::string(e.what());
		return false;
	}

	return false;
}

/**
 * DeserializeJson()を呼ぶ前に、Deserializeに失敗する要素を削除しておく.
 * @param[in]  jsonStr           gltfのjsonテキスト.
 * @param[out] outputJsonStr     修正後のjsonテキストが返る.
 */
bool CGLTFLoader::m_checkPreDeserializeJson (const std::string jsonStr, std::string& outputJsonStr)
{
	outputJsonStr = jsonStr;

	/*
	   jsonStr内のjsonテキストのうち、.
	     "JOINTS_0":-1
	      "WEIGHTS_0":-1
	   のようなマイナス値があると、次の「DeserializeJson」で例外が発生する.
	      "targets":[] 
	   のような何も定義されていないものがあっても例外発生.
	   このようなエラーにつながる部分を除去する.
	 */
	rapidjson::Document doc;
	doc.Parse(jsonStr.c_str());		// jsonとしてパース.
	if (doc.HasParseError()) return false;

	// [meshes] - [primitives] 内のチェック.
	try {
		rapidjson::Value& meshesV = doc["meshes"];
		rapidjson::SizeType num = meshesV.Size();			// meshesは配列.
		for (rapidjson::SizeType i = 0; i < num; ++i) {
			rapidjson::Value& meshD = meshesV[i];
			if (!meshD.HasMember("primitives")) continue;
			rapidjson::Value& primitives = meshD["primitives"];
			rapidjson::SizeType numP = primitives.Size();	// primitivesは配列.
			for (rapidjson::SizeType j = 0; j < numP; ++j) {
				rapidjson::Value& prV = primitives[j];

				// [meshes] - [primitives] 内の要素を列挙し、数値で値がマイナスのもの(WEIGHTS_0/TEXCOORD_0/JOINTS_0)を削除.
				if (prV.HasMember("attributes")) {
					rapidjson::Value& attributes = prV["attributes"];
					std::vector<std::string> removeKeyList;
					for(rapidjson::Value::MemberIterator itr = attributes.MemberBegin(); itr != attributes.MemberEnd(); itr++) {
						const std::string name = itr->name.GetString();
						const rapidjson::Type type = itr->value.GetType();
						if (type == rapidjson::kNumberType) {		// 数値の場合.
							if ((itr->value.GetInt()) < 0) {
								// 値がマイナスの要素は削除対象にする.
								removeKeyList.push_back(name);
							}
						}
					}
					for (size_t j = 0; j < removeKeyList.size(); ++j) {
						attributes.RemoveMember(removeKeyList[j].c_str());	// 指定のキーの要素を削除.
					}
				}

				// [meshes] - [primitives] 内の要素を列挙し、配列でサイズが0のものを削除.
				{
					std::vector<std::string> removeKeyList;
					for(rapidjson::Value::MemberIterator itr = prV.MemberBegin(); itr != prV.MemberEnd(); itr++) {
						const std::string name = itr->name.GetString();
						const rapidjson::Type type = itr->value.GetType();
						if (type == rapidjson::kArrayType) {		// 配列の場合.
							if (itr->value.Size() == 0) {
								removeKeyList.push_back(name);
							}
						}
					}
					for (size_t j = 0; j < removeKeyList.size(); ++j) {
						prV.RemoveMember(removeKeyList[j].c_str());		// 指定のキーの要素を削除.
					}
				}

				// [meshes] - [primitives] - [targets]内の要素を列挙し、"extra"と数値がマイナスのもの(WEIGHTS_0/TEXCOORD_0/JOINTS_0)を削除.
				{
					for(rapidjson::Value::MemberIterator itr = prV.MemberBegin(); itr != prV.MemberEnd(); itr++) {
						const std::string name0 = itr->name.GetString();
						const rapidjson::Type type0 = itr->value.GetType();
						if (name0 == "targets" && type0 == rapidjson::kArrayType) {		// targetsの場合 (配列).
							rapidjson::Value& targets = itr->value;

							rapidjson::SizeType numT = targets.Size();			// targetsは配列.
							for (rapidjson::SizeType k = 0; k < numT; ++k) {
								rapidjson::Value& targetV = targets[k];

								std::vector<std::string> removeKeyList;
								for(rapidjson::Value::MemberIterator itr2 = targetV.MemberBegin(); itr2 != targetV.MemberEnd(); itr2++) {
									const std::string name     = itr2->name.GetString();
									const rapidjson::Type type = itr2->value.GetType();
									if (name == "extra" || (type == rapidjson::kNumberType && (itr2->value.GetInt()) < 0)) {
										removeKeyList.push_back(name);
									}
								}
								for (size_t j = 0; j < removeKeyList.size(); ++j) {
									targetV.RemoveMember(removeKeyList[j].c_str());		// 指定のキーの要素を削除.
								}
							}
						}
					}
				}
			}
		}
	} catch (...) {
		return false;
	}

#if 1
	// [bufferViews]の要素をチェック.
	// "byteStride": 0  がエラーになる.
	try {
		rapidjson::Value& bufferViewsV = doc["bufferViews"];
		rapidjson::SizeType num = bufferViewsV.Size();			// meshesは配列.
		for (rapidjson::SizeType i = 0; i < num; ++i) {
			rapidjson::Value& bufferViewsD = bufferViewsV[i];
			if (!bufferViewsD.HasMember("byteStride")) continue;
			rapidjson::Value::MemberIterator itr = bufferViewsD.FindMember("byteStride");
			const std::string name = itr->name.GetString();
			const rapidjson::Type type = itr->value.GetType();
			if (type == rapidjson::kNumberType) {		// 数値の場合.
				if (itr->value.GetInt() == 0) {
					bufferViewsD.RemoveMember(name.c_str());
				}
			}
		}
	} catch (...) {
		return false;
	}
#endif

	// jsonテキストのインデントを整列した形でbufに格納.
	// buf.GetString() が出力されたjsonテキスト.
	rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);

#if 0
	try {
		std::ofstream outStream("E:\\Data\\User\\VCProgram\\GLTFConverter\\xxxx.gltf");
		outStream << buf.GetString();
		outStream.flush();
	} catch (...) { }
#endif

	outputJsonStr = buf.GetString();
	return true;
}
