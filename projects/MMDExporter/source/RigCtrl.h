/**
 * @brief  人体リグ用.
 * @date   2014.08.05 - 2014.08.05.
 */

#ifndef _RIGCTRL_H
#define _RIGCTRL_H

#include "GlobalHeader.h"

class RIG_BONE_INFO {
public:
	int bone_index;					// インデックス.
	int parent_bone_index;			// 親のインデックス.
	int tail_bone_index;			// 子のインデックス (参照のみで実際のボーン構成に影響することはない).
	std::string name_jp;			// 日本語名.
	std::string name_en;			// 英語名.
	std::string name_default;		// Shade 3D向けデフォルト名.
	sxsdk::vec3 pos;				// 身長1700 mm 時の位置.

	RIG_BONE_INFO(const int _bone_index, const int _parent_bone_index, const int _tail_bone_index, const std::string _name_jp, const std::string _name_en, const std::string _name_default, const sxsdk::vec3 _pos = sxsdk::vec3(0, 0, 0)) {
		bone_index        = _bone_index;
		parent_bone_index = _parent_bone_index;
		tail_bone_index   = _tail_bone_index;
		name_jp           = _name_jp;
		name_en           = _name_en;
		name_default      = _name_default;
		pos               = _pos;
	}
	RIG_BONE_INFO() {
		bone_index = -1;
	}
};

class CRigCtrl
{
private:
	sxsdk::shade_interface* m_shade;

	/**
	 * 指定のボーンから再帰でたどり、形状名を取得.
	 */
	void m_GetBonesName(const int depth, sxsdk::shape_class& shape, std::vector<std::string>& bonesName);

public:
	CRigCtrl(sxsdk::shade_interface *shade);
	virtual ~CRigCtrl();

	/**
	 * 指定のボーン構造がMMDで使用するものとほぼ一致するか調べる.
	 * @param[in]  bone_root          ルートのボーン.
	 * @param[out] pRetHumanRigType   リグの種類（human_rig_type_default/human_rig_type_mmd_jp/human_rig_type_mmd_en）.
	 * @return  完全一致の場合は1.0を返す。1.0に近づくほどMMDのボーンの可能性が高い.
	 */
	float CheckMMDBones(sxsdk::shape_class& bone_root, int* pRetHumanRigType);

	/**
	 * 人体リグの、指定のボーン名に対応するインデックスを取得.
	 */
	static int GetHumanBoneIndex(sxsdk::shade_interface *shade, std::string boneName, const int rigType = human_rig_type_default);

	/**
	 * 人体リグの、指定のボーンインデックスに対応する名前を取得.
	 */
	 static std::string GetHumanBoneName(sxsdk::shade_interface *shade, const int index, const int rigType = human_rig_type_default);

};

#endif
