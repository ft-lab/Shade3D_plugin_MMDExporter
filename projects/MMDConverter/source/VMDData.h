/**
 *  @brief  MMDのモーション情報の管理 （Shade 3Dからのエクスポート）.
 *  @date  2014.08.11 - 2014.08.11.
 */

#ifndef _VMDDATA_H
#define _VMDDATA_H

#include "GlobalHeader.h"

/**
 * フレームデータ.
 */
class VMD_FRAME_DATA {
public:
	std::string boneName;			///< ボーン名.
	int boneIndex;					///< 対応するボーン番号.
	int frameNo;					///< フレーム番号.
	sxsdk::vec3 pos;				///< ボーンの位置（0の場合は位置変更なし）.
	sxsdk::vec4 quat;				///< クォータニオンのXYZW情報（0の場合は回転変更なし）.

	// 補間パラメータは、(0, 0) - (127,127)の空間上で、2つのベジェ制御点をXYZRで与えたもの.
	// (0, 0) - (ax, ay) - (bx, by) - (127, 127) の4点のベジェ.
	unsigned char Xax, Xay, Yax, Yay, Zax, Zay, Rax, Ray;
	unsigned char Xbx, Xby, Ybx, Yby, Zbx, Zby, Rbx, Rby;

	VMD_FRAME_DATA() {
		boneName  = "";
		boneIndex = -1;
		frameNo   = 0;
		pos       = sxsdk::vec3(0, 0, 0);
		quat      = sxsdk::vec4(0, 0, 0, 0);

		Xax = Yax = Zax = Rax =  64;
		Xay = Yay = Zay = Ray =  20;
		Xbx = Ybx = Zbx = Rbx =  64;
		Xby = Yby = Zby = Rby = 107;
	}
};

/**
 * 表情データ.
 */
class VMD_SKIN_DATA {
public:
	std::string skinName;			///< 表情名.
	int frameNo;					///< フレーム番号.
	float weight;					///< ウエイト値(1.0で完全に変換).

	int skinIndex;					///< 対応する表情番号.

	VMD_SKIN_DATA() {
		skinName  = "";
		skinIndex = -1;
		frameNo   = 0;
		weight    = 0.0f;
	}
};

class CVMDData {
private:
	sxsdk::shade_interface *m_shade;
	std::string m_modelName;					///< 形状名.

	std::vector< VMD_FRAME_DATA > m_frameData;	///< フレームデータの格納バッファ.
	std::vector< std::string > m_frameBoneName;	///< フレームデータに対応するボーン名の保持用. 
	std::vector<int> m_frameBoneIndex;			///< フレームデータに対応するボーンのボーン番号を保持.
	std::vector< VMD_SKIN_DATA > m_skinData;	///< 表情データの格納バッファ.
	std::vector<int> m_skinIndex;				///< 表情データに対応する表情番号を保持.

	float m_humanRigBonesNameCheck;				///< ボーンがMMD形式のボーンである割合 (1.0で完全一致).
	int m_humanRigBonesType;					///< ボーン名の種類 (human_rig_type_default / human_rig_type_mmd_jp / human_rig_type_mmd_en).
	bool m_humanConvertBoneName;				///< ボーン名を自動的に変更.
	float m_scale;								///< 出力時のスケーリング.

	/**
	 * ボーンリストを取得.
	 */
	void m_GetBonesList(sxsdk::shape_class& boneRoot, std::vector<sxsdk::shape_class *>& bonesList); 
	void m_GetBonesListLoop(sxsdk::shape_class& shape, std::vector<sxsdk::shape_class *>& bonesList); 

	/**
	 * 指定形状のモーションをm_frameDataに格納.
	 */
	void m_StoreMotionFrames(sxsdk::shape_class* pShape, const std::string& name);

	/**
	 * ヘッダ部の出力.
	 */
	void m_WriteHeader(sxsdk::stream_interface *stream);

	/**
	 * フレームデータの出力.
	 */
	void m_WriteFrameData(sxsdk::stream_interface *stream);

	/**
	 * スキン（表情）モーションの出力.
	 */
	void m_WriteSkinData(sxsdk::stream_interface *stream);

public:
	CVMDData(sxsdk::shade_interface *shade);

	void Clear();

	/**
	 * モーションデータの格納.
	 */
	bool SetMotion(sxsdk::scene_interface* scene, sxsdk::shape_class& shapeMesh, const CVMDDlgInfo& dlgData);

	/**
	 * モーションデータのエクスポート.
	 */
	void Export(sxsdk::stream_interface *stream);

};

#endif
