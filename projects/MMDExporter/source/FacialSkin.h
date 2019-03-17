/**
 *   @brief  表情パターンの管理クラス.
 *   @date   2014.08.13 - 2014.08.13.
 */

/*
	Meshの頂点番号をひとまとめにして、それぞれに任意の頂点座標を保持する.
	base / まゆ(eyebrow) / 目(eyes) / リップ(mouth) / その他(other)、に対して、パターンを登録することになる.

	Shadeでは、選択した頂点を一度複製し、skin用パートに格納.

	[skin]
	   [eyebrow]
	       original ... PolygonMesh
	       まじめ   ... PolygonMesh
		   困る     ... PolygonMesh
		   にこり   ... PolygonMesh
		   怒り     ... PolygonMesh

		[eyes]
	       original ... PolygonMesh
		   まばたき ... PolygonMesh
		   笑い     ... PolygonMesh
		   ウインク ... PolygonMesh

	のように格納。
	originalは、元のポリゴンメッシュと同じ位置のもの。これを複製して、それぞれの動きを与える。.
*/

#ifndef _FACIALSKIN_H
#define _FACIALSKIN_H

#include "GlobalHeader.h"
#include "BSPSearch.h"

/**
 * 表情の種類.
 */
enum {
	skin_type_base = 0,			///< 元の頂点と同一のもの.これはskinのグループとしては存在しない.
	skin_type_eyebrow,			///< 眉.
	skin_type_eye,				///< 目.
	skin_type_mouth,			///< 口.
	skin_type_other,			///< その他.
};

/**
 * 表情での頂点データ.
 */
class FACE_SKIN_VERTEX_DATA {
public:
	int vert_index;					///< 頂点番号（baseの場合は頂点リストにある番号、base以外はbaseの番号）.
	sxsdk::vec3 pos;				///< 頂点位置（baseの場合は頂点座標、base以外の場合はオフセット値）.
	int org_i;						///< 頂点が増加する場合の、元の FACE_SKIN_DATA::v_data でのインデックス.
	FACE_SKIN_VERTEX_DATA() {
		vert_index = -1;
		pos        = sxsdk::vec3(0, 0, 0);
		org_i      = -1;
	}
};

/**
 * 表情データ（格納用）.
 */
class FACE_SKIN_DATA {
public:
	std::string name;								///< 表情名.
	int type;										///< 表情の種類 (skin_type_xxx).
	std::vector<FACE_SKIN_VERTEX_DATA> v_data;		///< 表情用の頂点情報.

	sxsdk::shape_class* pShape;						///< 対応するポリゴンメッシュ.
	bool baseSkin;									///< 各表情の先頭はbase.

	FACE_SKIN_DATA() {
		name     = "";
		type     = skin_type_base;
		pShape   = NULL;
		baseSkin = false;
	}

};

class CFacialSkin
{
private:
	sxsdk::shade_interface *m_shade;
	
	std::vector<FACE_SKIN_DATA> m_faceSkinData;		///< face skin情報.
	std::vector<int> m_skinGroupIndex;				///< skinの種類ごとの先頭のインデックス.

	std::vector<std::string> m_skinTypeName;		///< skinTypeに対応する名称.

	CBSPSearch* m_pBSPSearch;						///< 頂点を検索するクラス.

	float m_scale;									///< 出力時のスケーリング.

	/**
	 * skinのパートを探す.
	 */
	sxsdk::shape_class* m_GetSkinPart(sxsdk::scene_interface* scene);

	/**
	 * 指定のSkinTypeのパートを探す.
	 */
	sxsdk::shape_class* m_GetSkinTypePart(sxsdk::shape_class* pSkinPart, const int skinType);

	/**
	 * 指定の形状のメッシュ情報を格納.
	 */
	bool m_AddMesh(sxsdk::shape_class* pShape, const int skinType, FACE_SKIN_DATA& skinData, const bool firstF = false);

	/**
	 * 指定のポリゴンメッシュの頂点を取得し、BSP構造に格納.
	 */
	bool m_SetMeshVertices(sxsdk::shape_class* pShape);

	/**
	 * 指定の頂点の一番近くにある頂点インデックスを取得.
	 */
	int m_GetNearVertex(sxsdk::vec3& pos, const float dist = (float)1e-3);

	/**
	 * 指定の英語名を日本語に変換できる場合に変換.
	 */
	std::string m_ConvSkinName_EngToJP(const std::string skinName);

public:
	CFacialSkin(sxsdk::shade_interface* shade);
	virtual ~CFacialSkin();

	/**
	 * クリア.
	 */
	void Clear();

	/**
	 * 現在のシーンから、face skin用のメッシュ情報を取得.
	 */
	bool StoreSkinData(sxsdk::scene_interface* scene, sxsdk::shape_class *pTargetMesh, const float scale);

	/**
	 * 頂点の最適化の反映（法線/UVの違いで頂点が増える場合）.
	 */
	void UpdateVertices(std::vector< std::vector<int> >& sameVertexList);

	/**
	 * 表情データをエクスポート.
	 */
	bool ExportSkinData(sxsdk::stream_interface *stream);

	/**
	 * 表情枠データをエクスポート.
	 */
	bool ExportSkinFrameData(sxsdk::stream_interface *stream);

	/**
	 * 英語の表情名をエクスポート.
	 */
	bool ExportEnglishSkinName(sxsdk::stream_interface *stream);

};

#endif
