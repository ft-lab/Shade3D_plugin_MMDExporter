/**
 *  @file   GlobalHeader.h
 *  @brief  共通して使用する変数など.
 *  @author Yutaka Yoshisaka
 *  @date   2014.08.07 - 2014.08.11
 */

#ifndef _GLOBALHEADER_H
#define _GLOBALHEADER_H

#include "sxsdk.cxx"

/**
 * MMD のPMD（モデル） ExporterクラスのID.
 */
#define MMD_PMD_EXPORTER_INTERFACE_ID sx::uuid_class("BB163ACC-9A99-4D1D-A113-0774E99E4314")

/**
 * MMD のVMD（モーション） ExporterクラスのID.
 */
#define MMD_VMD_EXPORTER_INTERFACE_ID sx::uuid_class("61C022C9-EE87-463E-B11A-F330376658D2")

/**
 * PMD ExporterのダイアログID.
 */
#define MMD_PMD_DLG_ID sx::uuid_class("C4C70C18-FE8A-45AE-A1ED-D95873E3A674")

/**
 * VMD ExporterのダイアログID.
 */
#define MMD_VMD_DLG_ID sx::uuid_class("EDBE3B87-4CA3-4077-B0AE-E1FF578573A8")

/**
 * streamでのバージョン.
 */
#define MMD_PMD_DLG_VERSION			0x100			// PMDファイルエクスポート時に出るダイアログ.
#define MMD_VMD_DLG_VERSION			0x100			// VMDファイルエクスポート時に出るダイアログ.

/**
 * PMDをエクスポートする際のダイアログ情報.
 */
class CPMDDlgInfo {
public:
	float scale;					// Scale (default 0.01).
	bool boneOffsetMoveRootOnly;	// ボーンの移動はルート以外は行わず.
	bool toonEdge;					// トゥーンのエッジ処理を行うかどうか.
	bool humanConvertBoneName;		// 人体ボーンの名称に自動変更する.
	bool humanAutoIK;				// IKを自動的に割り当て.

	std::string note_jp;			// 日本語説明文.
	std::string note_en;			// 英語説明文.

	CPMDDlgInfo() {
		scale    = 0.01f;
		toonEdge = true;
		boneOffsetMoveRootOnly = true;
		humanConvertBoneName = true;
		humanAutoIK = true;

		note_jp = "Modeling Shade 3D";
		note_en = "Modeling Shade 3D";
	}
};

/**
 * VMDをエクスポートする際のダイアログ情報.
 */
class CVMDDlgInfo {
public:
	float scale;					// Scale (default 0.01).
	bool humanConvertBoneName;		// 人体ボーンの名称に自動変更する.

	CVMDDlgInfo() {
		scale    = 0.01f;
		humanConvertBoneName = true;
	}
};

/**
 * 人体リグのボーン名タイプ.
 */
enum {
	human_rig_type_default = 0,		// Shadeデフォルトのボーン名.
	human_rig_type_mmd_jp  = 1,		// MMDの日本語ボーン名.
	human_rig_type_mmd_en  = 2,		// MMDの英語ボーン名.
};

// 足のボーンIK用（同一名でないとMMDでモーションを割り当てできなくなるため）.
enum LEG_IK_INDEX {
	index_leg_IK_L = 0,
	index_leg_IK_L_tail,
	index_toe_IK_L,
	index_toe_IK_L_tail,
	index_leg_IK_R,
	index_leg_IK_R_tail,
	index_toe_IK_R,
	index_toe_IK_R_tail,
};
extern std::string leg_ik_name_jp[];
extern std::string leg_ik_name_en[];

#endif
