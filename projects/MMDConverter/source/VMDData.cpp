/**
 *  @brief  MMDのモーション情報の管理 （Shade 3Dからのエクスポート）.
 *  @date  2014.08.11 - 2014.08.11.
 */

#include "VMDData.h"
#include "Util.h"
#include "RigCtrl.h"

CVMDData::CVMDData(sxsdk::shade_interface *shade) : m_shade(shade)
{
}

void CVMDData::Clear()
{
	m_modelName = "";

	m_frameData.clear();
	m_frameBoneName.clear();
	m_frameBoneIndex.clear();
	m_skinData.clear();
	m_skinIndex.clear();

	m_humanConvertBoneName   = true;
	m_humanRigBonesNameCheck = 0.0f;
	m_humanRigBonesType      = 0;
	m_scale = 0.01f;
}

/**
 * モーションデータの格納.
 */
bool CVMDData::SetMotion(sxsdk::scene_interface* scene, sxsdk::shape_class& shapeMesh, const CVMDDlgInfo& dlgData)
{
	Clear();

	m_scale                = dlgData.scale;
	m_humanConvertBoneName = dlgData.humanConvertBoneName;

	sxsdk::shape_class* pBoneRoot = Util::GetBoneRoot(shapeMesh);
	if (!pBoneRoot) return false;

	// ボーンリストを取得.
	std::vector<sxsdk::shape_class *> bonesList;
	m_GetBonesList(*pBoneRoot, bonesList);
	if (bonesList.size() == 0) return false;

	m_modelName = shapeMesh.get_name();

	// MMDのボーン名とどれくらい一致するか判定.
	m_humanRigBonesNameCheck = 0.0f;
	m_humanRigBonesType      = human_rig_type_default;
	if (m_humanConvertBoneName) {
		CRigCtrl rigCtrl(m_shade);
		m_humanRigBonesNameCheck = rigCtrl.CheckMMDBones(*pBoneRoot, &m_humanRigBonesType);
	}

	for (int loop = 0; loop < bonesList.size(); loop++) {
		sxsdk::shape_class* pShape = bonesList[loop];
		if (!(pShape->has_motion())) continue;

		std::string boneName = pShape->get_name();
		// MMDの日本語のボーン名に変換.
		if (m_humanConvertBoneName && m_humanRigBonesNameCheck > 0.5f) {
			const int index = CRigCtrl::GetHumanBoneIndex(m_shade, boneName, m_humanRigBonesType);
			if (index >= 0) {
				boneName = CRigCtrl::GetHumanBoneName(m_shade, index, human_rig_type_mmd_jp);
			}
		}

		// モーション情報を格納.
		m_StoreMotionFrames(pShape, boneName);
	}

	//----------------------------------------------------.
	// IKのGoalノードを取得して出力.
	//----------------------------------------------------.
	{
		sxsdk::ik_class& ikC = scene->get_ik();
		for (int loop = 0; loop < bonesList.size(); loop++) {
			sxsdk::shape_class* pShape = bonesList[loop];

			const int ikType = ikC.has_ik(*pShape, false);
			if (!(ikType & 0x02)) continue;		// IKエンドを持たない場合はスキップ.

			// goalの形状を取得.
			sxsdk::ik_data_class& ikData = ikC.get_ik_data(*pShape);
			sxsdk::shape_class* pGoalShape = ikData.get_goal_shape();
			if (!pGoalShape || !(pGoalShape->has_motion()) || !(ikData.get_root_shape())) continue;

			// 親をたどっていくと、pBoneRootにたどり着く場合はすでに処理済.
			bool chkF = false;
			sxsdk::shape_class* pShape2 = pGoalShape;
			while (pShape2->has_dad()) {
				pShape2 = pShape2->get_dad();
				if ((pShape2->get_handle()) == (pBoneRoot->get_handle())) {
					chkF = true;
					break;
				}
			}
			if (chkF) continue;

			// IK root/endに対応するボーン名を取得.
			std::string ikRootName = ikData.get_root_shape()->get_name();
			std::string ikEndName  = pShape->get_name();
			std::string ikGoalName = pGoalShape->get_name();

			// MMDのボーン名に変換.
			if (m_humanConvertBoneName && m_humanRigBonesNameCheck > 0.5f) {
				int index = CRigCtrl::GetHumanBoneIndex(m_shade, ikRootName, m_humanRigBonesType);
				if (index >= 0) {
					ikRootName = CRigCtrl::GetHumanBoneName(m_shade, index, human_rig_type_mmd_en);
				}
				index = CRigCtrl::GetHumanBoneIndex(m_shade, ikEndName, m_humanRigBonesType);
				if (index >= 0) {
					ikEndName = CRigCtrl::GetHumanBoneName(m_shade, index, human_rig_type_mmd_en);
				}

				if (ikRootName.compare("leg_L") == 0 && ikEndName.compare("ankle_L") == 0) {
					ikGoalName = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_L]);
				}
				if (ikRootName.compare("ankle_L") == 0 && ikEndName.compare("ankle_L2") == 0) {
					ikGoalName = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_L]);
				}
				if (ikRootName.compare("leg_R") == 0 && ikEndName.compare("ankle_R") == 0) {
					ikGoalName = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_R]);
				}
				if (ikRootName.compare("ankle_R") == 0 && ikEndName.compare("ankle_R2") == 0) {
					ikGoalName = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_R]);
				}
			} else {
				// 通常のIK割り当ての場合、IK root名 + "_IK" がIKゴールに相当.
				ikGoalName = ikRootName + "_IK";
			}

			// モーション情報を格納.
			m_StoreMotionFrames(pGoalShape, ikGoalName);
		}
	}

	return true;
}

/**
 * 指定形状のモーションをm_frameDataに格納.
 */
void CVMDData::m_StoreMotionFrames(sxsdk::shape_class* pShape, const std::string& name)
{
	if (!(pShape->has_motion())) return;

	VMD_FRAME_DATA frameData;
	frameData.boneName = name;
	try {
		compointer<sxsdk::motion_interface> motion(pShape->get_motion_interface());
		const int mCou = motion->get_number_of_motion_points();

		for (int i = 0; i < mCou; i++) {
			compointer<sxsdk::motion_point_interface> mp(motion->get_motion_point_interface(i));
			if (mp) {
				const sxsdk::vec3 offsetPos = mp->get_offset();
				const sxsdk::quaternion_class qt = mp->get_rotation();
				const sxsdk::vec4 q(qt.x, qt.y, qt.z, qt.w);

				frameData.frameNo = (int)(mp->get_sequence());
				frameData.pos     = offsetPos;
				frameData.quat    = q;

				// モーションカーブは線形に近くないと、カクカクになってしまうのでできるだけ線形に.
				{
					frameData.Xax = frameData.Yax = frameData.Zax = frameData.Rax =  30;
					frameData.Xay = frameData.Yay = frameData.Zay = frameData.Ray =  30;
					frameData.Xbx = frameData.Ybx = frameData.Zbx = frameData.Rbx =  97;
					frameData.Xby = frameData.Yby = frameData.Zby = frameData.Rby =  97;
				}
				m_frameData.push_back(frameData);
			}
		}
	} catch (...) { }
}

/**
 * ボーンリストを取得.
 */
void CVMDData::m_GetBonesList(sxsdk::shape_class& boneRoot, std::vector<sxsdk::shape_class *>& bonesList)
{
	bonesList.clear();
	m_GetBonesListLoop(boneRoot, bonesList);
}

void CVMDData::m_GetBonesListLoop(sxsdk::shape_class& shape, std::vector<sxsdk::shape_class *>& bonesList)
{
	if (!Util::IsBone(shape)) return;
	bonesList.push_back(&shape);

	if (shape.has_son()) {
		sxsdk::shape_class* pShape = shape.get_son();
		while (pShape->has_bro()) {
			pShape = pShape->get_bro();
			m_GetBonesListLoop(*pShape, bonesList);
		}
	}
}

/**
 * モーションデータのエクスポート.
 */
void CVMDData::Export(sxsdk::stream_interface *stream)
{
	m_WriteHeader(stream);
	m_WriteFrameData(stream);
	m_WriteSkinData(stream);
}

/**
 * ヘッダ部の出力.
 */
void CVMDData::m_WriteHeader(sxsdk::stream_interface *stream)
{
	char szStr[64];

	// ヘッダのテキストは固定.
	memset(szStr, 0, 40);
	strcpy(szStr, "Vocaloid Motion Data 0002");
	stream->write(30, szStr);

	// モデル名.
	memset(szStr, 0, 20);
	std::string str = Util::ConvUTF8ToSJIS(*m_shade, m_modelName);
	if (str.length() < 20) strcpy(szStr, str.c_str());
	else {
		strncpy(szStr, str.c_str(), 19);
	}

	stream->write(20, szStr);
}

/**
 * フレームデータの出力.
 */
void CVMDData::m_WriteFrameData(sxsdk::stream_interface *stream)
{
	int frameCou = m_frameData.size();
	stream->write(4, &frameCou);

	char szStr[32];
	unsigned char Interpolation[64];

	for (int i = 0; i < frameCou; i++) {
		VMD_FRAME_DATA& frameData = m_frameData[i];
		std::string str = Util::ConvUTF8ToSJIS(*m_shade, frameData.boneName);
		memset(szStr, 0, 20);
		if (str.length() < 15) strcpy(szStr, str.c_str());
		stream->write(15, szStr);
		stream->write(4, &frameData.frameNo);

		sxsdk::vec3 pos = frameData.pos * m_scale;
		pos.z = -pos.z;
		stream->write(4, &pos.x);
		stream->write(4, &pos.y);
		stream->write(4, &pos.z);

		sxsdk::vec4 q = frameData.quat;
		q.z = -q.z;
		stream->write(4, &q.x);
		stream->write(4, &q.y);
		stream->write(4, &q.z);
		stream->write(4, &q.w);

		// 補間データ.
		// http://blog.goo.ne.jp/torisu_tetosuki/e/bc9f1c4d597341b394bd02b64597499d  参考.
		Interpolation[ 0] = frameData.Xax; Interpolation[ 1] = frameData.Yax; Interpolation[ 2] = frameData.Zax; Interpolation[ 3] = frameData.Rax;
		Interpolation[ 4] = frameData.Xay; Interpolation[ 5] = frameData.Yay; Interpolation[ 6] = frameData.Zay; Interpolation[ 7] = frameData.Ray;
		Interpolation[ 8] = frameData.Xbx; Interpolation[ 9] = frameData.Ybx; Interpolation[10] = frameData.Zbx; Interpolation[11] = frameData.Rbx;
		Interpolation[12] = frameData.Xby; Interpolation[13] = frameData.Yby; Interpolation[14] = frameData.Zby; Interpolation[15] = frameData.Rby;

		Interpolation[16] = frameData.Yax; Interpolation[17] = frameData.Zax; Interpolation[18] = frameData.Rax; Interpolation[19] = frameData.Xay;
		Interpolation[20] = frameData.Yay; Interpolation[21] = frameData.Zay; Interpolation[22] = frameData.Ray; Interpolation[23] = frameData.Xbx;
		Interpolation[24] = frameData.Ybx; Interpolation[25] = frameData.Zbx; Interpolation[26] = frameData.Rbx; Interpolation[27] = frameData.Xby;
		Interpolation[28] = frameData.Yby; Interpolation[29] = frameData.Zby; Interpolation[30] = frameData.Rby; Interpolation[31] =          0x01;

		Interpolation[32] = frameData.Zax; Interpolation[33] = frameData.Rax; Interpolation[34] = frameData.Xay; Interpolation[35] = frameData.Yay;
		Interpolation[36] = frameData.Zay; Interpolation[37] = frameData.Ray; Interpolation[38] = frameData.Xbx; Interpolation[39] = frameData.Ybx;
		Interpolation[40] = frameData.Zbx; Interpolation[41] = frameData.Rbx; Interpolation[42] = frameData.Xby; Interpolation[43] = frameData.Yby;
		Interpolation[44] = frameData.Zby; Interpolation[45] = frameData.Rby; Interpolation[46] =          0x01; Interpolation[47] =          0x00;

		Interpolation[48] = frameData.Rax; Interpolation[49] = frameData.Xay; Interpolation[50] = frameData.Xay; Interpolation[51] = frameData.Zay;
		Interpolation[52] = frameData.Ray; Interpolation[53] = frameData.Xbx; Interpolation[54] = frameData.Ybx; Interpolation[55] = frameData.Zbx;
		Interpolation[56] = frameData.Rbx; Interpolation[57] = frameData.Xby; Interpolation[58] = frameData.Yby; Interpolation[59] = frameData.Zby;
		Interpolation[60] = frameData.Rby; Interpolation[61] =          0x01; Interpolation[62] =          0x00; Interpolation[63] =          0x00;

		stream->write(64, Interpolation);
	}
}

/**
 * スキン（表情）モーションの出力.
 */
void CVMDData::m_WriteSkinData(sxsdk::stream_interface *stream)
{
	int skinCou = m_skinData.size();
	stream->write(4, &skinCou);

	char szStr[32];
	for (int i = 0; i < skinCou; i++) {
		VMD_SKIN_DATA& skinData = m_skinData[i];
		std::string str = Util::ConvUTF8ToSJIS(*m_shade, skinData.skinName);
		memset(szStr, 0, 20);
		if (str.length() < 15) strcpy(szStr, str.c_str());
		stream->write(15, szStr);
		stream->write(4, &skinData.frameNo);
		stream->write(4, &skinData.weight);
	}
}
