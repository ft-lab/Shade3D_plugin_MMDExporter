/**
 *  @brief  MMDのモデル情報の管理 （Shade 3Dからのエクスポート）.
 *  @date  2014.08.03 - 2014.08.13.
 */

#ifndef _PMDDATA_H
#define _PMDDATA_H

#include "GlobalHeader.h"
#include "FacialSkin.h"

#include <vector>
#include <string>

/*
	参考サイト :
	http://blog.goo.ne.jp/torisu_tetosuki/e/209ad341d3ece2b1b4df24abf619d6e4

	頂点の最大数は65536.
	面の最大数は65536.
	面は三角形で構成される.
	頂点/法線/UVは、各頂点ごとに与えられる.
*/

#define PMD_VERTEX_DATA_SIZE			38			///< 頂点情報のバイト数.
#define PMD_MATERIAL_DATA_SIZE			70			///< マテリアル情報のバイト数.
#define PMD_BONE_DATA_SIZE				39			///< ボーン情報のバイト数. 
#define PMD_RIGIDBODY_DATA_SIZE			83			///< 剛体情報のバイト数. 
#define PMD_RIGIDBODY_JOINT_DATA_SIZE	124			///< 剛体のジョイント情報のバイト数. 

/**
 * 頂点データ（格納用）.
 */
class PMD_VERTEX_DATA {
public:
	sxsdk::vec3 pos;				///< 頂点位置.
	sxsdk::vec3 normal;				///< 法線ベクトル.
	sxsdk::vec2 uv;					///< UV値.
	int bone_num[2];				///< ボーンNo.
	int bone_weight;				///< ボーン[0]に与える影響度(0 - 100).ボーン[1]は100 - bone_weightが影響度。0の場合はボーンによる影響なし.
	int edge_flag;					///< 0:通常、1:エッジ無効.

	PMD_VERTEX_DATA() {
		pos    = sxsdk::vec3(0, 0, 0);
		normal = sxsdk::vec3(0, 0, 0);
		uv     = sxsdk::vec2(0, 0);
		bone_num[0] = -1;
		bone_num[1] = -1;
		bone_weight = 0;
		edge_flag = 0;
	}
};

/**
 * 三角形データ（格納用）.
 */
typedef struct {
	int index[3];

	// 以下、作業用.
	sxsdk::vec3 normal[3];
	sxsdk::vec2 uv[3];
	int orgFaceIndex;		// 元のShadeでの面番号.
} PMD_TRIANGLE_DATA;

/**
 * マテリアルデータ（格納用）.
 */
class PMD_MATERIAL_DATA {
public:
	sxsdk::vec3 diffuse_color;		///< デフューズ色.
	float alpha;					///< アルファ値.
	float specular;					///< スペキュラ値.
	sxsdk::vec3 specular_color;		///< スペキュラ色.
	sxsdk::vec3 ambient_color;		///< 環境光色.
	int edge_flag;					///< 輪郭、影 (1でエッジを黒にする).
	int toon_index;					///< トゥーン番号.
	int face_vert_count;			///< マテリアルを割り当てる面頂点数.
	std::string tex_file_name;		///< テクスチャファイル名(20バイトギリギリもあり).

	sxsdk::master_surface_class* masterSurface;		///< Shadeでのmaster surface.

	PMD_MATERIAL_DATA() {
		diffuse_color = sxsdk::vec3(1, 1, 1);
		alpha    = 1.0f;
		specular = 0.0f;
		specular_color = sxsdk::vec3(0.3f, 0.3f, 0.3f);
		ambient_color  = sxsdk::vec3(0.1f, 0.1f, 0.1f);
		edge_flag  = 1;
		toon_index = 0;
		face_vert_count = 0;
		tex_file_name = "";
		masterSurface = NULL;
	}
};

// ボーンの種類.
enum {
	bone_type_rotate = 0,			///< 回転.
	bone_type_rotate_trans,			///< 回転と移動.
	bone_type_ik,					///< IK.
	bone_type_unknown,				///< 選択不可.
	bone_type_ik_u,					///< IK影響下.
	bone_type_rotate_u,				///< 回転影響下.
	bone_type_ik_c,					///< IK接続先.
	bone_type_hide,					///< 非表示.
	bone_type_hineri,				///< 捻り.
	bone_type_rotate_v,				///< 回転補間.
};

/**
 * ボーンデータ（格納用）.
 */
class PMD_BONE_DATA {
public:
	std::string bone_name;					///< ボーン名.
	int parent_bone_index;					///< 親ボーン番号(ない場合は0xFFFF).
	int tail_pos_bone_index;				///< tail位置のボーン番号。子ボーンの番号に相当(チェーン末端の場合は0).
	int bone_type;							///< ボーンの種類.
	int ik_parent_bone_index;				///< IKボーン番号(影響IKボーン。ない場合は0).
	sxsdk::vec3 bone_head_pos;				///< ボーンのヘッダの位置.

	// 以下、PMD出力では使われない.
	sxsdk::shape_class* pShadeShape;		///< Shadeでのボーンに対応する形状ポインタ.

	PMD_BONE_DATA() {
		bone_name = "";
		parent_bone_index    = -1;
		tail_pos_bone_index  = 0;
		bone_type            = bone_type_rotate_trans;
		ik_parent_bone_index = 0;
		bone_head_pos        = sxsdk::vec3(0, 0, 0);
		pShadeShape          = NULL;
	}
};

/**
 * ボーンのツリー階層情報.
 */
typedef struct {
	int index;				///< インデックス.
	std::string bone_name;	///< ボーン名.
	int parentIndex;		///< 親ノードのインデックス.
	int childIndex;			///< 子ノードの開始インデックス.
	int prevIndex;			///< 兄弟ノードの前のインデックス.
	int nextIndex;			///< 兄弟ノードの次のインデックス.
} BONE_TREE_NODE;

/**
 * IKデータ（格納用）.
 */
class PMD_IK_DATA {
public:
	int ik_bone_index;							///< IKボーン番号.
	int ik_target_bone_index;					///< はじめに接続するIKボーン番号.
	int iterations;								///< 再帰演算回数 (IK値1).
	float control_weight;						///< IKの影響度（ラジアン指定。π / 2の場合に360度に相当）.
	std::vector< int > ik_child_bone_index;		///< IK影響下のボーン番号(ik_chain_length分).

	// 以下、PMD出力では使われない.
	int shade_ik_root;							///< IKルートのボーン.
	int shade_ik_end;							///< IKエンドのボーン.
	int shade_ik_goal;							///< IKゴールのボーン.

	PMD_IK_DATA() {
		ik_bone_index        = -1;
		ik_target_bone_index = -1;
		iterations           = 20;
		control_weight       = 0.5f;

		shade_ik_root = -1;
		shade_ik_end  = -1;
		shade_ik_goal = -1;
	}
};

/// 物理衝突の種類.
enum {
	rigidbody_type_sphere = 0,		///< 球.
	rigidbody_type_box,				///< ボックス.
	rigidbody_type_capsule,			///< カプセル.
};

// 物理のボーン運動の種類.
enum {
	rigidbody_bone_type_static = 0,		///< 不動のボーン追従.
	rigidbody_bone_type_dynamic,		///< 物理影響を受ける.
	rigidbody_bone_type_dynamic2,		///< 物理演算（ボーン位置あわせ）.
};

/**
 * 物理剛体データ（格納用）.
 */
typedef struct {
	std::string name;				///< 名称.
	int bone_index;					///< 対象のボーン番号.0xffffの場合は、ボーンの0番目の変換行列を採用.
	int group_index;				///< グループ番号.
	int group_target;
	int type;						///< 衝突の種類(rigidbody_type_xxx).
	sxsdk::vec3 shape_size;			///< 形状サイズ.
	sxsdk::vec3 pos;				///< 位置.
	sxsdk::vec3 rot;				///< 回転(rad(x), rad(y), rad(z)).
	float rigidbody_weight;			///< 質量.
	float rigidbody_pos_dim;		///< 移動減衰値.
	float rigidbody_rot_dim;		///< 回転減衰値.
	float rigidbody_recoil;			///< 反発力.
	float rigidbody_friction;		///< 摩擦力.
	int rigidbody_type;				///< 物理のボーン運動の種類（rigidbody_bone_type_xxxx).
	
	// 作業用.
	int bullet_shape_index;			///< 作業用のbullet内での形状管理番号.
} PMD_RIGIDBODY_DATA;

/**
 * 物理ジョイントデータ（格納用）.
 */
typedef struct {
	std::string name;				///< 名称.
	int joint_a;					///< 剛体A.
	int joint_b;					///< 剛体B.
	sxsdk::vec3 pos;				///< 位置.
	sxsdk::vec3 rot;				///< 回転.
	sxsdk::vec3 constrain_pos1;		///< 移動1.
	sxsdk::vec3 constrain_pos2;		///< 移動2.
	sxsdk::vec3 constrain_rot1;		///< 回転1(rad(x), rad(y), rad(z)).
	sxsdk::vec3 constrain_rot2;		///< 回転2(rad(x), rad(y), rad(z)).
	sxsdk::vec3 spring_pos;			///< ばねの位置.
	sxsdk::vec3 spring_rot;			///< ばねの回転(rad(x), rad(y), rad(z)).
} PMD_RIGIDBODY_JOINT_DATA;

/**
 * ボーン枠用表示リスト.
 */
class PMD_BONE_DISP_LIST_DATA {
public:
	int bone_disp_index;		// ボーン枠の番号（センター0、表示枠 1-）.
	int bone_index;				// ボーン番号.
	PMD_BONE_DISP_LIST_DATA() {
		bone_disp_index = -1;
		bone_index      = -1;
	}
};

/**
 * ボーン枠表示用.
 */
class PMD_BONE_DISP_DATA {
public:
	std::string disp_name;			///< 表示名（最大50バイト）。終端は 0A 00にすること.
	std::string disp_name_en;		///< 表示名(英語）.
	std::vector<PMD_BONE_DISP_LIST_DATA> data;		///< ボーン枠内のボーンインデックス.

	PMD_BONE_DISP_DATA() {
		disp_name    = "";
		disp_name_en = "";
	}
};

/*****************************************************/

class CPMDData
{
private:
	sxsdk::shade_interface *m_shade;

	std::string m_modelName;							///< 形状名.
	std::string m_comment;								///< コメント文.
	std::string m_modelNameEng;							///< 形状名（英語）.
	std::string m_commentEng;							///< コメント文（英語）.
	bool m_hasEnglish;									///< 英語対応.

	std::string m_filePath;

	int m_materialCount;								///< マテリアルの数.
	int m_boneCount;									///< ボーンの数.
	int m_IKCount;										///< IKの数.
	int m_SkinCount;									///< 表情データの数.
	int m_boneDispCount;								///< ボーンの表示枠の数.
	int m_rigidBodyCount;								///< 物理剛体データの数.
	int m_rigidBodyJointCount;							///< 物理のジョイントデータの数.

	std::vector<PMD_VERTEX_DATA> m_vertices;			///< 頂点情報.
	std::vector<PMD_TRIANGLE_DATA> m_triangles;			///< 面頂点番号（三角形面）.

	std::vector<PMD_MATERIAL_DATA> m_materials;			///< マテリアルの格納バッファ.
	std::vector<PMD_BONE_DATA> m_bones;					///< ボーンの格納バッファ.
	std::vector< BONE_TREE_NODE > m_bonesTree;			///< ボーンの階層情報格納バッファ.
	std::vector< PMD_IK_DATA > m_IKs;					///< IKの格納バッファ.
	std::vector< PMD_RIGIDBODY_DATA > m_RigidBodys;			///< 剛体情報の格納バッファ.
	std::vector< PMD_RIGIDBODY_JOINT_DATA > m_RigidBodyJoints;		///< 剛体のジョイント情報の格納バッファ.

	std::vector< PMD_BONE_DISP_DATA > m_bonesDisp;		///< ボーン枠表示用.

	std::vector< unsigned char > m_bonesNameEng;		///< 英語のボーン名格納バッファ.
	std::vector< unsigned char > m_skinsNameEng;		///< 英語の表情名格納バッファ.

	float m_scale;										///< 出力時のスケーリング.
	bool m_toonEdge;									///< Toonのエッジを反映するか.
	bool m_boneMoveRootOnly;							///< ボーンの移動処理はRootのみに限定.

	float m_humanRigBonesNameCheck;						///< ボーンがMMD形式のボーンである割合 (1.0で完全一致).
	int m_humanRigBonesType;							///< ボーン名の種類 (human_rig_type_default / human_rig_type_mmd_jp / human_rig_type_mmd_en).
	bool m_humanConvertBoneName;						///< ボーン名を自動的に変更.

	sxsdk::shape_class* m_pBoneRoot;					///< ボーンのルート.

	CFacialSkin* m_pFacialSkin;							///< フェイシャルスキンでの表情管理用.

	std::vector< std::vector<int> > m_orgSameVertexList;	///< 同一頂点がUV/法線の都合で増加した場合リスト。表情の格納時に使用.

	/**
	 * 初期化処理.
	 */
	void m_Init ();

	/**
	 * 破棄処理.
	 */
	void m_Term ();

	/**
	 * UV/法線が異なる頂点で頂点を増やして対応.
	 */
	void m_OptimizeVertexNormalUV();

	/**
	 * マテリアルの保持.
	 */
	void m_SetMaterials(sxsdk::scene_interface* scene, sxsdk::shape_class& shape);

	/**
	 * ボーンの保持.
	 */
	void m_SetBones(sxsdk::shape_class& shape);
	void m_SetBoneLoop(const int depth, const int parentBoneIndex, sxsdk::shape_class* pBoneShape);

	/**
	 * 指定のボーン名がすでに格納済みか.
	 */
	int m_FindBone(const std::string boneName);

	/**
	 * 指定のボーン形状がすでに格納済みか.
	 */
	int m_FindBone(sxsdk::shape_class* pShape);

	/**
	 * 指定のボーン間に存在するボーン番号を取得。endBoneIndexから親をたどるとstartBoneIndexに行き着くのが保証されているとする.
	 */
	std::vector<int> m_GetBonesList(const int endBoneIndex, const int startBoneIndex);

	/**
	 * 頂点に対応するボーンとスキン値の保持.
	 */
	void m_SetVertexSkins(sxsdk::shape_class& shape);

	/**
	 * IK情報の格納.
	 */
	void m_SetIKs(sxsdk::scene_interface* scene, sxsdk::shape_class& shape);
	void m_SetIKsLoop(sxsdk::ik_class& ik, sxsdk::scene_interface* scene, sxsdk::shape_class& shape);

	/**
	 * 人体時のIK情報を自動で追加、IK用ボーンも追加される.
	 */
	void m_SetHumanBoneIKs();

	/**
	 * 指定のIKがすでに格納済みか調べる.
	 */
	int m_GetIK(const int ikRootBoneIndex);

	/**
	 * 指定のIK情報を格納。この際に、IK用ボーンも生成.
	 */
	bool m_AddIK(sxsdk::shape_class* ikRootShape, sxsdk::shape_class* ikEndShape, sxsdk::shape_class* ikGoalShape);

	/**
	 * 表示枠情報を設定.
	 */
	void m_SetBonesDisp();

	/**
	 * ヘッダ部の出力.
	 */
	void m_WriteHeader(sxsdk::stream_interface *stream);

	/**
	 * 頂点の出力.
	 */
	void m_WriteVertices(sxsdk::stream_interface *stream);

	/**
	 * 面の出力.
	 */
	void m_WriteFaces(sxsdk::stream_interface *stream);

	/**
	 * マテリアルの出力.
	 */
	void m_WriteMaterials(sxsdk::stream_interface *stream);

	/**
	 * ボーンの出力.
	 */
	void m_WriteBones(sxsdk::stream_interface *stream);

	/**
	 * IKの出力.
	 */
	void m_WriteIKs(sxsdk::stream_interface *stream);

	/**
	 * Skin(表情)の出力.
	 */
	void m_WriteSkins(sxsdk::stream_interface *stream);

	/**
	 * 表情枠情報の出力.
	 */
	void m_WriteSkinWaku(sxsdk::stream_interface *stream);

	/**
	 * ボーン枠情報の出力.
	 */
	void m_WriteBoneWaku(sxsdk::stream_interface *stream);

	/**
	 * ボーン枠用の表示リストの出力.
	 */
	void m_WriteBoneList(sxsdk::stream_interface *stream);

	/**
	 * 英語情報の出力.
	 */
	void m_WriteExEnglishInfo(sxsdk::stream_interface *stream);

	/**
	 * 英語ヘッダの出力.
	 */
	void m_WriteEnglishHeader(sxsdk::stream_interface *stream);

	/**
	 * 英語ボーン名リストの出力.
	 */
	void m_WriteEnglishBones(sxsdk::stream_interface *stream);

	/**
	 * 英語表情名リストの出力.
	 */
	void m_WriteEnglishSkins(sxsdk::stream_interface *stream);

	/**
	 * 英語ボーン枠情報の出力.
	 */
	void m_WriteEnglishBoneWaku(sxsdk::stream_interface *stream);

	/**
	 * トゥーンテクスチャリストの出力.
	 */
	void m_WriteToonTextureList(sxsdk::stream_interface *stream);

	/**
	 * 物理演算用の剛体リストを出力.
	 */
	void m_WritePhysicsRigidbodyList(sxsdk::stream_interface *stream);

	/**
	 * 物理演算用のジョイントリストを出力.
	 */
	void m_WritePhysicsJointList(sxsdk::stream_interface *stream);

	/**
	 * 指定のファイル名のフルパスを取得.
	 * @param[in]   fileName ファイル名.
	 * @preturn  フルパス名が返る.
	 */
	std::string GetFileFullPathName(std::string fileName);


public:
	CPMDData(sxsdk::shade_interface *shade);
	virtual ~CPMDData();

	/**
	 * クリア.
	 */
	void Clear();

	/**
	 * Shadeの形状データからPMD情報に変換.
	 */
	bool SetModel(sxsdk::shape_class& shape, sxsdk::stream_interface *stream, const CPMDDlgInfo& pmdDlgData);

	/**
	 * streamに出力.
	 */
	bool Export(sxsdk::stream_interface *stream, CPMDDlgInfo& pmdInfo);

	/**
	 * Meshの頂点の数.
	 */
	int GetVerticesCount() { return m_vertices.size(); }

	/**
	 * Meshを構成する三角形の数.
	 */
	int GetTrianglesCount() { return m_triangles.size(); }
};

#endif
