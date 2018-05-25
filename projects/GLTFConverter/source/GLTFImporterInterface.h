﻿/**
 * GLTFファイルを読み込むインポータクラス.
 */
#ifndef _GLTFIMPORTERINTERFACE_H
#define _GLTFIMPORTERINTERFACE_H

#include "GlobalHeader.h"

class CSceneData;
class CNodeData;

class CGLTFImporterInterface : public sxsdk::importer_interface {

private:
	sxsdk::shade_interface& shade;
	std::string m_tempPath;		// 作業用のディレクトリ.

private:
	virtual sx::uuid_class get_uuid (void *) { return GLTF_IMPORTER_INTERFACE_ID; }
	virtual int get_shade_version () const { return SHADE_BUILD_NUMBER; }
	
	/**
	 * 扱うファイル拡張子数3
	 */
	virtual int get_number_of_file_extensions (void * aux = 0);

	/**
	 * @brief ファイル拡張子.
	 */
	virtual const char *get_file_extension (int index, void *);

	/**
	 * @brief ファイル詳細.
	 */
	virtual const char *get_file_extension_description (int index, void*);

	/**
	 * @brief  前処理(ダイアログを出す場合など).
	 */
	virtual void do_pre_import (const sxsdk::mat4 &t, void *path);

	/**
	 * インポートを行う.
	 */
	virtual void do_import (sxsdk::scene_interface *scene, sxsdk::stream_interface *stream, sxsdk::text_stream_interface *text_stream, void *aux=0);

	/**
	 * アプリケーション終了時に呼ばれる.
	 */
	virtual void cleanup (void *aux=0);

private:
	/**
	 * GLTFを読み込んだシーン情報より、Shade3Dの形状として構築.
	 */
	void m_createGLTFScene (sxsdk::scene_interface *scene, CSceneData* sceneData);

	/**
	 * GLTFを読み込んだシーン情報より、ノードを再帰的にたどって階層構造を構築.
	 */
	void m_createGLTFNodeHierarchy (sxsdk::scene_interface *scene, CSceneData* sceneData, const int nodeIndex = 0);

	/**
	 * 指定のメッシュを生成.
	 */
	bool m_createGLTFMesh (const std::string& name, sxsdk::scene_interface *scene, CSceneData* sceneData, const int meshIndex, const sxsdk::mat4& matrix);

	/**
	 * GLTFを読み込んだシーン情報より、マスターイメージを作成.
	 */
	void m_createGLTFImages (sxsdk::scene_interface *scene, CSceneData* sceneData);

	/**
	 * GLTFを読み込んだシーン情報より、マスターサーフェスとしてマテリアルを作成.
	 */
	void m_createGLTFMaterials (sxsdk::scene_interface *scene, CSceneData* sceneData);

	/**
	 * 指定のMesh形状に対して、スキン情報を割り当て.
	 */
	void m_setMeshSkins (sxsdk::scene_interface *scene, CSceneData* sceneData);

	/**
	 * メッシュ番号を参照しているノードを取得.
	 * @return ノード番号.
	 */
	int m_findNodeFromMeshIndex (CSceneData* sceneData, const int meshIndex);

	/**
	 * 読み込んだ形状のルートから調べて、ボーンの場合に向きとボーンサイズを自動調整.
	 */
	void m_adjustBones (sxsdk::shape_class* shape);

public:
	CGLTFImporterInterface (sxsdk::shade_interface &shade);

	/**
	 * プラグイン名.
	 */
	static const char *name(sxsdk::shade_interface *shade) { return shade->gettext("gltf_importer_title"); }
};

#endif
