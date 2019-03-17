/**
 *  @brief  便利機能.
 *  @date  2014.08.08 - 2014.08.08
 */

#include "Util.h"

/**
 * テキストをSJISに変換.
 */
std::string Util::ConvUTF8ToSJIS(sxsdk::shade_interface& shade, const std::string str)
{
	std::string str2 = shade.encode(str.c_str(), sxsdk::enums::shift_jis_encoding);
	return str2;
}

/**
 * テキストをSJISからUTF-8に変換.
 */
std::string Util::ConvSJISToUTF8(sxsdk::shade_interface& shade, const std::string str)
{
	std::string str2 = shade.decode(str.c_str(), sxsdk::enums::shift_jis_encoding);
	return str2;
}

/**
 * C/C++上に埋め込まれたテキストをUTF-8として取得.
 * Win環境の場合は、C/C++のテキストはSJISになっている.
 * MacはUTF-8かも.
 */
std::string Util::GetUTF8Text(sxsdk::shade_interface& shade, const std::string str)
{
#if SXWINDOWS
	return ConvSJISToUTF8(shade, str);
#else
	return str;
#endif

}

/**
 * streamより、ファイル名を取得.
 */
std::string Util::GetFileNameToStream(sxsdk::stream_interface* stream)
{
	char *pPos;
	char szFilePath[512];

	strcpy(szFilePath, stream->get_file_path());

	// 作業用のファイル名の場合.
	pPos = strrchr(szFilePath, '.');
	if (pPos) {
		if (strcmp(pPos, ".sxfiletmp") == 0) {
			*pPos = '\0';
		}
	}

	std::string name = "";

	name = szFilePath;

#if SXWINDOWS
	pPos = strrchr(szFilePath, '\\');
	if(pPos) name = (pPos + 1);
#else
	pPos = strrchr(szFilePath, '/');
	if(pPos) name = (pPos + 1);
#endif

	return name;
}

/**
 * 指定の形状がボーンかどうか.
 */
bool Util::IsBone(sxsdk::shape_class& shape)
{
	if (shape.get_type() != sxsdk::enums::part) return false;
	sxsdk::part_class& part = shape.get_part();
	if (part.get_part_type() == sxsdk::enums::bone_joint) return true;
	return false;
}

/**
 * ボーンのワールド座標での中心位置とボーンサイズを取得.
 */
sxsdk::vec3 Util::GetBoneCenter(sxsdk::shape_class& shape, float *size)
{
	if (size) *size = 0.0f;
	if (!Util::IsBone(shape)) return sxsdk::vec3(0, 0, 0);

	const sxsdk::mat4 lwMat = shape.get_local_to_world_matrix();
	const sxsdk::vec3 center = sxsdk::vec3(0, 0, 0) * shape.get_transformation() * lwMat;

	try {
		compointer<sxsdk::bone_joint_interface> bone(shape.get_bone_joint_interface());
		if (size) *size = bone->get_size();
	} catch (...) { }

	return center;
}

/**
 * image_interfaceからマスターイメージを取得.
 */
sxsdk::master_image_class* Util::GetMasterImageFromImage (sxsdk::scene_interface *scene, sxsdk::image_interface *image) {
	sxsdk::master_image_class *retMasterImage = NULL;
	try {
		const sxsdk::shape_class &rootShape = scene->get_shape();
		if (!rootShape.has_son()) return NULL;

		sxsdk::shape_class *pS = rootShape.get_son();
		while (pS) {
			if (!pS->has_bro()) break;
			pS = pS->get_bro();
			if (!pS) break;
			if (pS->get_type() != sxsdk::enums::part) continue;
			sxsdk::part_class &part = pS->get_part();
			if (part.get_part_type() == sxsdk::enums::master_image_part) {
				if (pS->has_son()) {
					sxsdk::shape_class *pS2 = pS->get_son();
					while (pS2) {
						if (!pS2->has_bro()) break;
						pS2 = pS2->get_bro();
						if (!pS2) break;
						if (pS2->get_type() == sxsdk::enums::master_image) {
							sxsdk::master_image_class &mImage = pS2->get_master_image();
							sxsdk::image_interface *image2 = mImage.get_image();
							if (image2 && image2->has_image() && image2->is_same_as(image)) {
								retMasterImage = &mImage;
								break;
							}
						}
					}
				}
				break;
			}
		}
	} catch (...) { }
	return retMasterImage;
}

/**
 * 指定のポリゴンメッシュに割り当てられているボーンのルートを取得.
 */
sxsdk::shape_class* Util::GetBoneRoot(sxsdk::shape_class& shapePolygonMesh)
{
	sxsdk::shape_class *pBoneRoot = NULL;
	if (shapePolygonMesh.get_type() != sxsdk::enums::polygon_mesh) return pBoneRoot;

	sxsdk::polygon_mesh_class& pmesh = shapePolygonMesh.get_polygon_mesh();
	const int verCou = pmesh.get_total_number_of_control_points();

	for (int i = 0; i < verCou; i++) {
		sxsdk::vertex_class& v = pmesh.vertex(i);
		sxsdk::skin_class& skin = v.get_skin();
		const int bind_cou = skin.get_number_of_binds();
		if (bind_cou <= 0) continue;

		for (int j = 0; j < bind_cou; j++) {
			sxsdk::shape_class *pShape = skin.get_bind(j).get_shape();

			sxsdk::shape_class *pBoneShape = NULL;
			if (pShape->get_type() == sxsdk::enums::part) {
				sxsdk::part_class *part = &(pShape->get_part());
				if (part->get_part_type() == sxsdk::enums::bone_joint) {
					pBoneShape = pShape;
				}
			}
			if (pBoneShape) {
				while (pBoneShape->has_dad()) {
					sxsdk::part_class *part = pBoneShape->get_dad();
					if (part->get_part_type() == sxsdk::enums::bone_joint) {
						pBoneShape = part;
					} else {
						break;
					}
				}
			}
			if (pBoneShape) {
				pBoneRoot = pBoneShape;
				break;
			}
		}
		if (pBoneRoot) break;
	}

	return pBoneRoot;
}

/**
 * シーン内に存在するボーンとメッシュの組み合わせを取得.
 */
int Util::GetBoneMeshsList(sxsdk::scene_interface* scene, std::vector<sxsdk::shape_class *>& retMeshList, std::vector<sxsdk::shape_class *>& retBoneRootList)
{
	retMeshList.clear();
	retBoneRootList.clear();
	
	sxsdk::shape_class* pRootShape = &(scene->get_shape());
	if (!(pRootShape->has_son())) return 0;

	sxsdk::shape_class* pShape = pRootShape->get_son();
	while (pShape->has_bro()) {
		pShape = pShape->get_bro();

		if ((pShape->get_type()) == sxsdk::enums::polygon_mesh) {
			sxsdk::shape_class *pBoneRoot = GetBoneRoot(*pShape);
			if (pBoneRoot) {
				retMeshList.push_back(pShape);
				retBoneRootList.push_back(pBoneRoot);
			}
		}
	}

	return retMeshList.size();
}

