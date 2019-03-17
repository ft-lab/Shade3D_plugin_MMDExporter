/**
 * @brief  人体リグ用.
 * @date   2014.08.05 - 2014.08.09.
 */

#include "RigCtrl.h"
#include "Util.h"

// ----------------------------------------------------.
// 人体リグのボーン情報（MMDの初音ミクモデル、A-Pose）.
// ----------------------------------------------------.

namespace {

// MMDのファイルの制約で、ボーン名は15バイト以内である必要がある.
RIG_BONE_INFO rigBoneInfo [] = {
	RIG_BONE_INFO(  0,   -1,    1, "センター"      , "center"          , "center"               , sxsdk::vec3(      0,   725.0f,       0)),
	RIG_BONE_INFO(  1,    0,   -1, "センター先"    , "center2"         , "center2"              , sxsdk::vec3(      0,   925.0f,       0)),

	RIG_BONE_INFO(  2,    0,    3, "上半身"        , "upper body"      , "upper_body"           , sxsdk::vec3(      0,  1175.0f,       0)),

	RIG_BONE_INFO(  3,    2,    4, "首"            , "neck"            , "neck"                 , sxsdk::vec3(      0,  1383.0f,       0)),
	RIG_BONE_INFO(  4,    3,    5, "頭"            , "head"            , "head"                 , sxsdk::vec3(      0,  1443.0f,       0)),
	RIG_BONE_INFO(  5,    4,   -1, "頭先"          , "head2"           , "head2"                , sxsdk::vec3(      0,  1564.0f,   20.0f)),

	RIG_BONE_INFO(  6,    4,    7, "左目"          , "eye_L"           , "eye_left"             , sxsdk::vec3(  51.0f,  1529.0f,   65.0f)),
	RIG_BONE_INFO(  7,    6,   -1, "左目先"        , "eye_L2"          , "eye_left2"            , sxsdk::vec3(  51.0f,  1529.0f,   92.0f)),

	RIG_BONE_INFO(  8,    4,    9, "右目"          , "eye_R"           , "eye_right"            , sxsdk::vec3( -51.0f,  1529.0f,   65.0f)),
	RIG_BONE_INFO(  9,    8,   -1, "右目先"        , "eye_R2"          , "eye_right2"           , sxsdk::vec3( -51.0f,  1529.0f,   92.0f)),

	RIG_BONE_INFO( 10,    0,   11, "下半身"        , "lower body"      , "lower_body"           , sxsdk::vec3(      0,   960.0f,       0)),
	RIG_BONE_INFO( 11,   10,   -1, "下半身先"      , "lower body2"     , "lower_body2"          , sxsdk::vec3(      0,   960.0f,   78.0f)),

	RIG_BONE_INFO( 12,    2,   13, "左肩"          , "shoulder_L"      , "shoulder_left"        , sxsdk::vec3(  33.0f,  1370.0f,       0)),
	RIG_BONE_INFO( 13,   12,   14, "左腕"          , "arm_L"           , "arm_left"             , sxsdk::vec3( 102.0f,  1340.0f,       0)),
	RIG_BONE_INFO( 14,   13,   15, "左ひじ"        , "elbow_L"         , "elbow_left"           , sxsdk::vec3( 284.0f,  1216.0f,  -19.0f)),
	RIG_BONE_INFO( 15,   14,   16, "左手首"        , "wrist_L"         , "wrist_left"           , sxsdk::vec3( 432.0f,  1094.0f,  -19.0f)),
	RIG_BONE_INFO( 16,   15,   -1, "左手先"        , "wrist_L2"        , "wrist_left2"          , sxsdk::vec3( 489.0f,  1054.0f,  -19.0f)),
	RIG_BONE_INFO( 17,   15,   18, "左親指１"      , "thumb1_L"        , "thumb1_left"          , sxsdk::vec3( 480.0f,  1086.0f,   43.0f)),
	RIG_BONE_INFO( 18,   17,   19, "左親指２"      , "thumb2_L"        , "thumb2_left"          , sxsdk::vec3( 514.0f,  1063.0f,   74.0f)),
	RIG_BONE_INFO( 19,   18,   -1, "左親指先"      , "thumb3_L"        , "thumb3_left"          , sxsdk::vec3( 534.0f,  1043.0f,   93.0f)),
	RIG_BONE_INFO( 20,   15,   21, "左人指１"      , "fore1_L"         , "fore1_left"           , sxsdk::vec3( 507.0f,  1075.0f,    8.0f)),
	RIG_BONE_INFO( 21,   20,   22, "左人指２"      , "fore2_L"         , "fore2_left"           , sxsdk::vec3( 540.0f,  1053.0f,   21.0f)),
	RIG_BONE_INFO( 22,   21,   23, "左人指３"      , "fore3_L"         , "fore3_left"           , sxsdk::vec3( 588.0f,  1026.0f,   35.0f)),
	RIG_BONE_INFO( 23,   22,   -1, "左人指先"      , "fore4_L"         , "fore4_left"           , sxsdk::vec3( 615.0f,  1002.0f,   41.0f)),
	RIG_BONE_INFO( 24,   15,   25, "左中指１"      , "middle1_L"       , "middle1_left"         , sxsdk::vec3( 510.0f,  1076.0f,  -33.0f)),
	RIG_BONE_INFO( 25,   24,   26, "左中指２"      , "middle2_L"       , "middle2_left"         , sxsdk::vec3( 544.0f,  1052.0f,  -34.0f)),
	RIG_BONE_INFO( 26,   25,   27, "左中指３"      , "middle3_L"       , "middle3_left"         , sxsdk::vec3( 602.0f,  1020.0f,  -33.0f)),
	RIG_BONE_INFO( 27,   26,   -1, "左中指先"      , "middle4_L"       , "middle4_left"         , sxsdk::vec3( 646.0f,  1008.0f,  -33.0f)),
	RIG_BONE_INFO( 28,   15,   29, "左薬指１"      , "third1_L"        , "third1_left"          , sxsdk::vec3( 512.0f,  1078.0f,  -69.0f)),
	RIG_BONE_INFO( 29,   28,   30, "左薬指２"      , "third2_L"        , "third2_left"          , sxsdk::vec3( 546.0f,  1052.0f,  -73.0f)),
	RIG_BONE_INFO( 30,   29,   31, "左薬指３"      , "third3_L"        , "third3_left"          , sxsdk::vec3( 595.0f,  1022.0f,  -74.0f)),
	RIG_BONE_INFO( 31,   30,   -1, "左薬指先"      , "third4_L"        , "third4_left"          , sxsdk::vec3( 628.0f,   997.0f,  -75.0f)),
	RIG_BONE_INFO( 32,   15,   33, "左小指１"      , "little1_L"       , "little1_left"         , sxsdk::vec3( 497.0f,  1082.0f, -108.0f)),
	RIG_BONE_INFO( 33,   32,   34, "左小指２"      , "little2_L"       , "little2_left"         , sxsdk::vec3( 529.0f,  1058.0f, -115.0f)),
	RIG_BONE_INFO( 34,   33,   35, "左小指３"      , "little3_L"       , "little3_left"         , sxsdk::vec3( 572.0f,  1032.0f, -117.0f)),
	RIG_BONE_INFO( 35,   34,   -1, "左小指先"      , "little4_L"       , "little4_left"         , sxsdk::vec3( 607.0f,  1010.0f, -117.0f)),

	RIG_BONE_INFO( 36,   10,   37, "左足"          , "leg_L"           , "leg_left"             , sxsdk::vec3(  70.0f,   943.0f,       0)),
	RIG_BONE_INFO( 37,   36,   38, "左ひざ"        , "knee_L"          , "knee_left"            , sxsdk::vec3(  70.0f,   547.0f,   27.0f)),
	RIG_BONE_INFO( 38,   37,   39, "左足首"        , "ankle_L"         , "ankle_left"           , sxsdk::vec3(  70.0f,    91.0f,    9.0f)),
	RIG_BONE_INFO( 39,   38,   -1, "左つま先"      , "ankle_L2"        , "ankle_left2"          , sxsdk::vec3(  70.0f,    30.0f,  200.0f)),

	RIG_BONE_INFO( 40,    2,   41, "右肩"          , "shoulder_R"      , "shoulder_right"       , sxsdk::vec3( -33.0f,  1370.0f,       0)),
	RIG_BONE_INFO( 41,   40,   42, "右腕"          , "arm_R"           , "arm_right"            , sxsdk::vec3(-102.0f,  1340.0f,       0)),
	RIG_BONE_INFO( 42,   41,   43, "右ひじ"        , "elbow_R"         , "elbow_right"          , sxsdk::vec3(-284.0f,  1216.0f,  -19.0f)),       
	RIG_BONE_INFO( 43,   42,   44, "右手首"        , "wrist_R"         , "wrist_right"          , sxsdk::vec3(-432.0f,  1094.0f,  -19.0f)),
	RIG_BONE_INFO( 44,   43,   -1, "右手先"        , "wrist_R2"        , "wrist_right2"         , sxsdk::vec3(-489.0f,  1054.0f,  -19.0f)),
	RIG_BONE_INFO( 45,   43,   46, "右親指１"      , "thumb1_R"        , "thumb1_right"         , sxsdk::vec3(-480.0f,  1086.0f,   43.0f)),
	RIG_BONE_INFO( 46,   45,   47, "右親指２"      , "thumb2_R"        , "thumb2_right"         , sxsdk::vec3(-514.0f,  1063.0f,   74.0f)),
	RIG_BONE_INFO( 47,   46,   -1, "右親指先"      , "thumb3_R"        , "thumb3_right"         , sxsdk::vec3(-534.0f,  1043.0f,   93.0f)),
	RIG_BONE_INFO( 48,   43,   49, "右人指１"      , "fore1_R"         , "fore1_right"          , sxsdk::vec3(-507.0f,  1075.0f,    8.0f)),
	RIG_BONE_INFO( 49,   48,   50, "右人指２"      , "fore2_R"         , "fore2_right"          , sxsdk::vec3(-540.0f,  1053.0f,   21.0f)),
	RIG_BONE_INFO( 50,   49,   51, "右人指３"      , "fore3_R"         , "fore3_right"          , sxsdk::vec3(-588.0f,  1026.0f,   35.0f)),
	RIG_BONE_INFO( 51,   50,   -1, "右人指先"      , "fore4_R"         , "fore4_right"          , sxsdk::vec3(-615.0f,  1002.0f,   41.0f)),
	RIG_BONE_INFO( 52,   43,   53, "右中指１"      , "middle1_R"       , "middle1_right"        , sxsdk::vec3(-510.0f,  1076.0f,  -33.0f)),
	RIG_BONE_INFO( 53,   52,   54, "右中指２"      , "middle2_R"       , "middle2_right"        , sxsdk::vec3(-544.0f,  1052.0f,  -34.0f)),
	RIG_BONE_INFO( 54,   53,   55, "右中指３"      , "middle3_R"       , "middle3_right"        , sxsdk::vec3(-602.0f,  1020.0f,  -33.0f)),
	RIG_BONE_INFO( 55,   54,   -1, "右中指先"      , "middle4_R"       , "middle4_right"        , sxsdk::vec3(-646.0f,  1008.0f,  -33.0f)),
	RIG_BONE_INFO( 56,   43,   57, "右薬指１"      , "third1_R"        , "third1_right"         , sxsdk::vec3(-512.0f,  1078.0f,  -69.0f)),
	RIG_BONE_INFO( 57,   56,   58, "右薬指２"      , "third2_R"        , "third2_right"         , sxsdk::vec3(-546.0f,  1052.0f,  -73.0f)),
	RIG_BONE_INFO( 58,   57,   59, "右薬指３"      , "third3_R"        , "third3_right"         , sxsdk::vec3(-595.0f,  1022.0f,  -74.0f)),
	RIG_BONE_INFO( 59,   58,   -1, "右薬指先"      , "third4_R"        , "third4_right"         , sxsdk::vec3(-628.0f,   997.0f,  -75.0f)),
	RIG_BONE_INFO( 60,   43,   61, "右小指１"      , "little1_R"       , "little1_right"        , sxsdk::vec3(-497.0f,  1082.0f, -108.0f)),
	RIG_BONE_INFO( 61,   60,   62, "右小指２"      , "little2_R"       , "little2_right"        , sxsdk::vec3(-529.0f,  1058.0f, -115.0f)),
	RIG_BONE_INFO( 62,   61,   63, "右小指３"      , "little3_R"       , "little3_right"        , sxsdk::vec3(-572.0f,  1032.0f, -117.0f)),
	RIG_BONE_INFO( 63,   62,   -1, "右小指先"      , "little4_R"       , "little4_right"        , sxsdk::vec3(-607.0f,  1010.0f, -117.0f)),

	RIG_BONE_INFO( 64,   10,   65, "右足"          , "leg_R"           , "leg_right"            , sxsdk::vec3( -70.0f,   943.0f,       0)),
	RIG_BONE_INFO( 65,   64,   66, "右ひざ"        , "knee_R"          , "knee_right"           , sxsdk::vec3( -70.0f,   547.0f,   27.0f)),
	RIG_BONE_INFO( 66,   65,   67, "右足首"        , "ankle_R"         , "ankle_right"          , sxsdk::vec3( -70.0f,    91.0f,    9.0f)),
	RIG_BONE_INFO( 67,   66,   -1, "右つま先"      , "ankle_R2"        , "ankle_right2"         , sxsdk::vec3( -70.0f,    30.0f,  200.0f)),

	RIG_BONE_INFO( -1,   -1,   -1, ""              , ""                , "")
};

}

CRigCtrl::CRigCtrl(sxsdk::shade_interface *shade) {
	m_shade = shade;
}

CRigCtrl::~CRigCtrl() {
}

/**
 * 指定のボーン構造がMMDで使用するものとほぼ一致するか調べる.
 * @param[in]  bone_root          ルートのボーン.
 * @param[out] pRetHumanRigType   リグの種類（human_rig_type_default/human_rig_type_mmd_jp/human_rig_type_mmd_en）.
 * @return  完全一致の場合は1.0を返す。1.0に近づくほどMMDのボーンの可能性が高い.
 */
float CRigCtrl::CheckMMDBones(sxsdk::shape_class& bone_root, int* pRetHumanRigType)
{
	// ボーン名を取得.
	std::vector<std::string> bonesName;
	m_GetBonesName(0, bone_root, bonesName);
	if (bonesName.size() == 0) return 0.0f;

	// 比較する最低限のボーンの数.
	int templateBonesCou = 0;
	while (true) {
		RIG_BONE_INFO& bInfo = rigBoneInfo[templateBonesCou];
		if (bInfo.bone_index < 0) break;
		templateBonesCou++;
	}
	if (templateBonesCou == 0) return 0.0f;

	int bCouJP      = 0;
	int bCouEN      = 0;
	int bCouDefault = 0;
	{
		// MMDの日本語ボーン名と完全一致するか.
		for (int i = 0; i < templateBonesCou; i++) {
			RIG_BONE_INFO& bInfo = rigBoneInfo[i];
			const std::string name = Util::GetUTF8Text(*m_shade, bInfo.name_jp);			// C/C++のソースから取得したテキストはSJISであるため、UTF-8に変換.
			for (int j = 0; j < bonesName.size(); j++) {
				if (name.compare(bonesName[j]) == 0) {
					bCouJP++;
					break;
				}
			}
		}
	}
	{
		// MMDの英語ボーン名と完全一致するか.
		for (int i = 0; i < templateBonesCou; i++) {
			RIG_BONE_INFO& bInfo = rigBoneInfo[i];
			const std::string name = bInfo.name_en;
			for (int j = 0; j < bonesName.size(); j++) {
				if (name.compare(bonesName[j]) == 0) {
					bCouEN++;
					break;
				}
			}
		}
	}
	{
		// デフォルトボーン名と完全一致するか.
		for (int i = 0; i < templateBonesCou; i++) {
			RIG_BONE_INFO& bInfo = rigBoneInfo[i];
			const std::string name = bInfo.name_default;
			for (int j = 0; j < bonesName.size(); j++) {
				if (name.compare(bonesName[j]) == 0) {
					bCouDefault++;
					break;
				}
			}
		}
	}

	int bCou    = bCouJP;
	int rigType = human_rig_type_mmd_jp;
	if (bCou < bCouEN) {
		bCou    = bCouEN;
		rigType = human_rig_type_mmd_en;
	}
	if (bCou < bCouDefault) {
		bCou    = bCouDefault;
		rigType = human_rig_type_default;
	}
	if (pRetHumanRigType) *pRetHumanRigType = rigType;

	return (float)bCou / (float)templateBonesCou;
}

/**
 * 指定のボーンから再帰でたどり、形状名を取得.
 */
void CRigCtrl::m_GetBonesName(const int depth, sxsdk::shape_class& shape, std::vector<std::string>& bonesName)
{
	if (!Util::IsBone(shape)) return;

	bonesName.push_back(shape.get_name());

	if (shape.has_son()) {
		sxsdk::shape_class *pShape = shape.get_son();
		while (pShape->has_bro()) {
			pShape = pShape->get_bro();
			m_GetBonesName(depth + 1, *pShape, bonesName);
		}
	}
}

/**
 * 人体リグの、指定のボーン名に対応するインデックスを取得.
 */
int CRigCtrl::GetHumanBoneIndex(sxsdk::shade_interface *shade, std::string boneName, const int rigType)
{
	int index = -1;

	int iPos = 0;
	while (true) {
		RIG_BONE_INFO& rInfo = rigBoneInfo[iPos];
		if (rInfo.bone_index < 0) break;
		if (rigType == human_rig_type_default) {
			if (rInfo.name_default.compare(boneName) == 0) {
				index = iPos;
				break;
			}
		} else if (rigType == human_rig_type_mmd_jp) {
			const std::string nameUTF8 = Util::GetUTF8Text(*shade, rInfo.name_jp);
			if (nameUTF8.compare(boneName) == 0) {
				index = iPos;
				break;
			}
		} else if (rigType == human_rig_type_mmd_en) {
			if (rInfo.name_en.compare(boneName) == 0) {
				index = iPos;
				break;
			}
		}
		iPos++;
	}

	return index;
}

/**
 * 人体リグの、指定のボーンインデックスに対応する名前を取得.
 */
std::string CRigCtrl::GetHumanBoneName(sxsdk::shade_interface *shade, const int index, const int rigType)
{
	if (index < 0) return "";

	if (rigType == human_rig_type_default) {
		return rigBoneInfo[index].name_default;
	}
	if (rigType == human_rig_type_mmd_jp) {
		const std::string nameUTF8 = Util::GetUTF8Text(*shade, rigBoneInfo[index].name_jp);
		return nameUTF8;
	}
	if (rigType == human_rig_type_mmd_en) {
		return rigBoneInfo[index].name_en;
	}

	return "";
}

