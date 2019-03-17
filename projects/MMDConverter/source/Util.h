/**
 *  @brief  便利機能.
 *  @date  2014.08.08 - 2014.08.10
 */

#ifndef _UTIL_H
#define _UTIL_H

#include "GlobalHeader.h"

namespace Util {
	/**
	 * テキストをSJISに変換.
	 */
	std::string ConvUTF8ToSJIS(sxsdk::shade_interface& shade, const std::string str);

	/**
	 * テキストをSJISからUTF-8に変換.
	 */
	std::string ConvSJISToUTF8(sxsdk::shade_interface& shade, const std::string str);

	/**
	 * 指定の形状がボーンかどうか.
	 */
	bool IsBone(sxsdk::shape_class& shape);

	/**
	 * image_interfaceからマスターイメージを取得.
	 */
	sxsdk::master_image_class* GetMasterImageFromImage(sxsdk::scene_interface *scene, sxsdk::image_interface *image);

	/**
	 * ボーンのワールド座標での中心位置とボーンサイズを取得.
	 */
	sxsdk::vec3 GetBoneCenter(sxsdk::shape_class& shape, float *size);

	/**
	 * C/C++上に埋め込まれたテキストをUTF-8として取得.
	 * Win環境の場合は、C/C++のテキストはSJISになっている.
	 * MacはUTF-8かも.
	 */
	std::string GetUTF8Text(sxsdk::shade_interface& shade, const std::string str);

	/**
	 * 指定のポリゴンメッシュに割り当てられているボーンのルートを取得.
	 */
	sxsdk::shape_class* GetBoneRoot(sxsdk::shape_class& shapePolygonMesh);

	/**
	 * シーン内に存在するボーンとメッシュの組み合わせを取得.
	 */
	int GetBoneMeshsList(sxsdk::scene_interface* scene, std::vector<sxsdk::shape_class *>& retMeshList, std::vector<sxsdk::shape_class *>& retBoneRootList);

	/**
	 * streamより、ファイル名を取得.
	 */
	std::string GetFileNameToStream(sxsdk::stream_interface* stream);

}

#endif
