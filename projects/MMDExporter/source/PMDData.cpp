/**
 *  @brief  MMDのモデル情報の管理 （Shade 3Dからのエクスポート）.
 *  @date  2014.08.03 - 2014.08.18.
 */

#include "PMDData.h"
#include "Util.h"
#include "RigCtrl.h"

namespace {

	void output_message_pos(sxsdk::scene_interface *scene, sxsdk::vec3& v) {
		char szStr[256];
		sprintf(szStr, "(%f, %f, %f)", v.x, v.y, v.z);
		scene->message(szStr);
	}

	/**
	 * 多角形の三角形分割クラス.
	 */
	std::vector<int> m_triangleIndex; 

	class CDivideTrianglesOutput : public sxsdk::shade_interface::output_function_class {
	private:
	public:
		virtual void output (int i0 , int i1 , int i2 , int i3) {
			const int offset = m_triangleIndex.size();
			m_triangleIndex.resize(offset + 3);
			m_triangleIndex[offset + 0] = i0;
			m_triangleIndex[offset + 1] = i1;
			m_triangleIndex[offset + 2] = i2;
		}
	};
}

extern std::string leg_ik_name_jp[] = {
	"左足ＩＫ",
	"左足ＩＫ先",
	"左つま先ＩＫ",
	"左つま先ＩＫ先",
	"右足ＩＫ",
	"右足ＩＫ先",
	"右つま先ＩＫ",
	"右つま先ＩＫ先",
	"",
};

extern std::string leg_ik_name_en[] = {
	"leg IK_L",
	"leg IK_L2",
	"toe IK_L",
	"toe IK_L2",
	"leg IK_R",
	"leg IK_R2",
	"toe IK_R",
	"toe IK_R2",
	"",
};

CPMDData::CPMDData(sxsdk::shade_interface *shade) {
	m_shade = shade;
	m_pFacialSkin = NULL;

}

CPMDData::~CPMDData() {
	if (m_pFacialSkin) delete m_pFacialSkin;
}

/**
 * 初期化処理.
 */
void CPMDData::m_Init ()
{
	m_vertices.clear();
	m_triangles.clear();

	m_modelName = "model name";
	m_comment = "comment";
	m_hasEnglish = false;
	m_filePath = "";
	m_materialCount = 0;
	m_boneCount = 0;
	m_IKCount = 0;
	m_SkinCount = 0;
	m_rigidBodyCount = 0;
	m_rigidBodyJointCount = 0;
	m_boneDispCount = 0;
	m_materials.clear();
	m_bones.clear();
	m_bonesTree.clear();
	m_IKs.clear();
	m_RigidBodys.clear();
	m_RigidBodyJoints.clear();
	m_bonesDisp.clear();
	m_bonesNameEng.clear();
	m_skinsNameEng.clear();

	m_scale = 0.01f;
	m_toonEdge = true;
	m_boneMoveRootOnly = false;
	m_humanRigBonesNameCheck = 0.0f;
	m_humanRigBonesType      = 0;

	m_pBoneRoot = NULL;

	if (m_pFacialSkin) delete m_pFacialSkin;
	m_pFacialSkin = NULL;
	m_orgSameVertexList.clear();
}

/**
 * 破棄処理.
 */
void CPMDData::m_Term ()
{
	m_Init();
}

/**
 * クリア.
 */
void CPMDData::Clear()
{
	m_Init();
}

/**
 * 指定のファイル名のフルパスを取得.
 * @param[in]   fileName ファイル名.
 * @preturn  フルパス名が返る.
 */
std::string CPMDData::GetFileFullPathName(std::string fileName)
{
	char szStr[512];
	char szFileName[256];
	int len, loop;
	char *pPos;

	char szOrgFileName[256];
	strcpy(szOrgFileName, fileName.c_str());

	len = (int)strlen(szOrgFileName);
	if (len <= 2) return "";

	// ファイル名のみ取り出し.
	pPos = strrchr(szOrgFileName, '\\');
	if (pPos) {
		strcpy(szFileName, pPos + 1);
	} else {
		strcpy(szFileName, szOrgFileName);
	}

#if SXWINDOWS
	sprintf(szStr, "%s\\%s", m_filePath.c_str(), szFileName);
#else
	sprintf(szStr, "%s/%s", m_filePath.c_str(), szFileName);
#endif
	return szStr;
}

/**
 * Shadeの形状データからPMD情報に変換.
 */
bool CPMDData::SetModel(sxsdk::shape_class& shape, sxsdk::stream_interface *stream, const CPMDDlgInfo& pmdDlgData)
{
	Clear();

	m_scale                = pmdDlgData.scale;
	m_toonEdge             = pmdDlgData.toonEdge;
	m_boneMoveRootOnly     = pmdDlgData.boneOffsetMoveRootOnly;
	m_humanConvertBoneName = pmdDlgData.humanConvertBoneName;

	{
		// ファイル名のフルパスより、セパレータ以降のファイル名をカット.
		char *pPos;
		char szFilePath[512];
		strcpy(szFilePath, stream->get_name());

#if SXWINDOWS
		pPos = strrchr(szFilePath, '\\');
		if(pPos) *pPos = '\0';
#else
		pPos = strrchr(szFilePath, '/');
		if(pPos) *pPos = '\0';
#endif
		m_filePath = szFilePath;
	}

	m_modelName    = shape.get_name();
	m_modelNameEng = m_modelName;
	m_comment      = pmdDlgData.note_jp;
	m_commentEng   = pmdDlgData.note_en;

	const int skin_type = shape.get_skin_type();
	if (skin_type != 1) {		// 頂点ブレンドのスキンでない場合はスキップ.
		m_shade->show_message_box("Not vertex blend mode.", false);
		return false;
	}

	// 一度シーケンスOffにする（初期姿勢になるわけではないが、、、）.
	compointer<sxsdk::scene_interface> scene(shape.get_scene_interface());

	const bool dirtyF       = scene->get_dirty();
	const bool sequenceMode = scene->get_sequence_mode();
	if (sequenceMode) scene->set_sequence_mode(false);

	const sxsdk::mat4 lwMat = shape.get_local_to_world_matrix();

	if (shape.get_type() == sxsdk::enums::polygon_mesh) {
		sxsdk::polygon_mesh_class& pmesh = shape.get_polygon_mesh();

		// 頂点情報を格納 (この段階では、頂点ごとの法線とＵＶは格納していない).
		const int verCou = pmesh.get_total_number_of_control_points();
		if (verCou <= 0) return false;
		m_vertices.resize(verCou);
		PMD_VERTEX_DATA vData;
		for (int i = 0; i < verCou; i++) {
			sxsdk::vertex_class& v = pmesh.vertex(i);
			vData.pos = v.get_position() * lwMat;

			// スキンでの頂点の補間 （スキン変形後のものにする）.
		//	const sxsdk::mat4 skin_m = v.get_skin().get_skin_world_matrix();
		//	vData.pos = vData.pos * skin_m;

			m_vertices[i] = vData;

		}

		pmesh.setup_normal();

		// 面情報を格納.
		PMD_TRIANGLE_DATA triData;
		int indicesList[512];
		std::vector<sxsdk::vec3> normals;
		std::vector<sxsdk::vec3> vertices;
		std::vector<sxsdk::vec2> uvs;
		normals.resize(512);
		uvs.resize(512);
		const int faceCou = pmesh.get_number_of_faces();
		for (int i = 0; i < faceCou; i++) {
			sxsdk::face_class& f = pmesh.face(i);
			const int vCou = f.get_number_of_vertices();
			if (vCou > 510) continue;
			pmesh.get_face_n_deprecated(i, indicesList, &(normals[0]));		// 法線はこれじゃないと正しく取得できない.
			f.get_vertex_indices(indicesList);
			//f.get_normals(&(normals[0]));

			for (int j = 0; j < vCou; j++) uvs[j] = f.get_face_uv(0, j);

			if (vCou == 3) {
				triData.index[0]  = indicesList[0];
				triData.index[1]  = indicesList[1];
				triData.index[2]  = indicesList[2];
				triData.normal[0] = normals[0];
				triData.normal[1] = normals[1];
				triData.normal[2] = normals[2];
				triData.uv[0]     = uvs[0];
				triData.uv[1]     = uvs[1];
				triData.uv[2]     = uvs[2];
				triData.orgFaceIndex = i;

				m_triangles.push_back(triData);
			} else {
				// 三角形分割を行う.
				vertices.resize(vCou);
				for (int j = 0; j < vCou; j++) vertices[j] = m_vertices[indicesList[j]].pos;
				::m_triangleIndex.clear();
				::CDivideTrianglesOutput divC;
				m_shade->divide_polygon(divC, vCou, &(vertices[0]), true);

				const int triCou = ::m_triangleIndex.size() / 3;
				int iPos = 0;
				for (int j = 0; j < triCou; j++) {
					triData.index[0]  = indicesList[m_triangleIndex[iPos + 0]];
					triData.index[1]  = indicesList[m_triangleIndex[iPos + 1]];
					triData.index[2]  = indicesList[m_triangleIndex[iPos + 2]];
					triData.normal[0] = normals[m_triangleIndex[iPos + 0]];
					triData.normal[1] = normals[m_triangleIndex[iPos + 1]];
					triData.normal[2] = normals[m_triangleIndex[iPos + 2]];
					triData.uv[0]     = uvs[m_triangleIndex[iPos + 0]];
					triData.uv[1]     = uvs[m_triangleIndex[iPos + 1]];
					triData.uv[2]     = uvs[m_triangleIndex[iPos + 2]];
					triData.orgFaceIndex = i;
					m_triangles.push_back(triData);
					iPos += 3;
				}
			}
		}
	}

	// 表情のデータを取得する.
	{
		if (m_pFacialSkin) delete m_pFacialSkin;
		m_pFacialSkin = new CFacialSkin(m_shade);
		m_pFacialSkin->StoreSkinData(scene, &shape, m_scale);
	}

	// ボーンの保持.
	m_SetBones(shape);

	// 頂点に対応するボーンとスキンの保持.
	m_SetVertexSkins(shape);

	// 法線/UVを、頂点ごとに割り当て.
	m_OptimizeVertexNormalUV();

	// 表情データに、頂点最適化後の情報を渡す.
	if (m_pFacialSkin) {
		m_pFacialSkin->UpdateVertices(m_orgSameVertexList);
	}

	// マテリアルを保持.
	m_SetMaterials(scene, shape);

	// IK情報を保持.
	m_SetIKs(scene, shape);

	// 足のIK(4つ分)を自動的に登録.
	if (pmdDlgData.humanAutoIK) m_SetHumanBoneIKs();

	// ボーンの表示枠情報の設定.
	m_SetBonesDisp();

	scene->set_sequence_mode(sequenceMode);
	scene->set_dirty(dirtyF);		// 保存フラグを元に戻す.

	// 範囲チェック.
	if (m_vertices.size() > 65535) {
		m_shade->show_message_box(m_shade->gettext("msg_mesh_vertex_65535"), false);
		return false;
	}
	if (m_triangles.size() > 65535) {
		m_shade->show_message_box(m_shade->gettext("msg_mesh_triangle_65535"), false);
		return false;
	}
	if (m_bones.size() > 500) {
		m_shade->show_message_box(m_shade->gettext("msg_mesh_bone_500"), false);
		return false;
	}

	return true;
}

/**
 * UV/法線が異なる頂点で頂点を増やして対応.
 */
void CPMDData::m_OptimizeVertexNormalUV()
{
	const int vCou   = m_vertices.size();
	const int triCou = m_triangles.size();
	if (vCou == 0 || triCou == 0) return;

	const int orgVCou = vCou;

	// 頂点ごとの共有三角形インデックスを一時的に保持.
	std::vector< std::vector<int> > verticesTri;
	verticesTri.resize(vCou);

	for (int i = 0; i < triCou; i++) {
		PMD_TRIANGLE_DATA& triData = m_triangles[i];
		verticesTri[triData.index[0]].push_back(i);
		verticesTri[triData.index[1]].push_back(i);
		verticesTri[triData.index[2]].push_back(i);
	}

	m_orgSameVertexList.clear();
	m_orgSameVertexList.resize(orgVCou);
	for (int i = 0; i < orgVCou; i++) m_orgSameVertexList[i].clear();

	std::vector<int> verticesSIndex;

	// 頂点ごとでUVが異なる場合の頂点の増加.
	for (int i = 0; i < orgVCou; i++) {
		std::vector<int>& vTriIndex = verticesTri[i];
		const int vvCou = vTriIndex.size();
		if (vvCou == 0) continue;

		const PMD_TRIANGLE_DATA& triData0 = m_triangles[vTriIndex[0]];

		int i0 = -1;
		if (triData0.index[0] == i) i0 = 0;
		else if (triData0.index[1] == i) i0 = 1;
		else if (triData0.index[2] == i) i0 = 2;
		if (i0 < 0) continue;

		const sxsdk::vec3& n0  = triData0.normal[i0];
		const sxsdk::vec2& uv0 = triData0.uv[i0];

		PMD_VERTEX_DATA vData0 = m_vertices[i];
		vData0.normal = n0;
		vData0.uv     = uv0;
		m_vertices[i] = vData0;

		if (vvCou == 1) continue;

		verticesSIndex.clear();
		for (int j = 1; j < vvCou; j++) {
			PMD_TRIANGLE_DATA& triData1 = m_triangles[vTriIndex[j]];

			int i1 = -1;
			if (triData1.index[0] == i) i1 = 0;
			else if (triData1.index[1] == i) i1 = 1;
			else if (triData1.index[2] == i) i1 = 2;
			if (i1 < 0) continue;

			const sxsdk::vec3& n1  = triData1.normal[i1];
			const sxsdk::vec2& uv1 = triData1.uv[i1];

			if (sx::zero(n0 - n1) && sx::zero(uv0 - uv1)) continue; 

			int index = -1;
			for (int k = 0; k < verticesSIndex.size(); k++) {
				PMD_VERTEX_DATA& vData2 = m_vertices[verticesSIndex[k]];
				if (sx::zero(vData2.normal - n1) && sx::zero(vData2.uv - uv1)) {
					index = verticesSIndex[k];
					break;
				}
			}
			if (index >= 0) {
				triData1.index[i1] = index;
			} else {
				PMD_VERTEX_DATA vData = vData0;
				vData.normal = n1;
				vData.uv     = uv1;
				m_vertices.push_back(vData);
				triData1.index[i1] = m_vertices.size() - 1;
				verticesSIndex.push_back(triData1.index[i1]);

				m_orgSameVertexList[i].push_back(triData1.index[i1]);		// 表情(FacialSkin)を格納する際の頂点情報用.
			}
		}
	}
}


/**
 * マテリアルの保持.
 */
void CPMDData::m_SetMaterials(sxsdk::scene_interface* scene, sxsdk::shape_class& shape)
{
	const int type = shape.get_type();
	if (type != sxsdk::enums::polygon_mesh) return;

	sxsdk::polygon_mesh_class& pmesh = shape.get_polygon_mesh();
	const int faceGroupCou = pmesh.get_number_of_face_groups();

	//---------------------------------------------------------.
	// とりあえず、shapeに関連する全部のマテリアルを格納.
	//---------------------------------------------------------.
	std::vector<sxsdk::surface_class *> shapeSurfaces;
	std::vector<sxsdk::master_surface_class *> shapeMasterSurfaces;
	std::vector<int> shapeSurfacesCou;

	if (shape.has_surface()) {
		shapeSurfaces.push_back(shape.get_surface());
	} else {
		shapeSurfaces.push_back(NULL);
	}
	shapeMasterSurfaces.push_back(NULL);

	const int faceGroupOffset = shapeSurfaces.size();
	for (int i = 0; i < faceGroupCou; i++) {
		sxsdk::master_surface_class *mSurface = pmesh.get_face_group_surface(i);
		if (mSurface) {
			shapeSurfaces.push_back(mSurface->get_surface());
			shapeMasterSurfaces.push_back(mSurface);
		} else {
			shapeSurfaces.push_back(NULL);
			shapeMasterSurfaces.push_back(NULL);
		}
	}
	shapeSurfacesCou.resize(shapeSurfaces.size());
	for (int i = 0; i < shapeSurfacesCou.size(); i++) shapeSurfacesCou[i] = 0;

	//---------------------------------------------------------.
	//	面ごとのsurfaceを保持.
	//---------------------------------------------------------.
	const int faceCou = pmesh.get_number_of_faces();
	std::vector<int> faceSurfaceIndex;
	faceSurfaceIndex.resize(faceCou);
	for (int i = 0; i < faceCou; i++) {
		faceSurfaceIndex[i] = -1;
		const int fIndex = pmesh.get_face_group_index(i);
		if (fIndex >= 0 && shapeSurfaces[faceGroupOffset + fIndex] != NULL) {
			faceSurfaceIndex[i] = faceGroupOffset + fIndex;
		}
	}

	const int triCou = m_triangles.size();
	std::vector<int> triSurfaceIndex;
	triSurfaceIndex.resize(triCou);
	for (int i = 0; i < triCou; i++) {
		PMD_TRIANGLE_DATA& triData = m_triangles[i];
		const int orgFaceIndex = triData.orgFaceIndex;
		triSurfaceIndex[i] = faceSurfaceIndex[orgFaceIndex];
		if (triSurfaceIndex[i] >= 0) {
			shapeSurfacesCou[triSurfaceIndex[i]]++;
		} else {
			shapeSurfacesCou[0]++;
		}
	}

	// 環境光の影響は、光源から取得.
	sxsdk::rgb_class ambientCol = sxsdk::rgb_class(0.2f, 0.2f, 0.2f);
	{
		compointer<sxsdk::distant_light_interface> distantLight(scene->get_distant_light_interface());
		if (distantLight) {
			if ((distantLight->get_number_of_lights()) > 0) {
				sxsdk::distant_light_item_class& LInfo = distantLight->distant_light_item(0);
				const float ambVal = LInfo.get_ambient();
				const sxsdk::rgb_class ambCol = LInfo.get_light_color();
				ambientCol = ambCol * ambVal;
			}
		}
	}

	//---------------------------------------------------------.
	// PMD用に並び替え.
	//---------------------------------------------------------.
	std::vector<PMD_TRIANGLE_DATA> oldTriangles = m_triangles;
	m_triangles.clear();
	for (int loop = 0; loop < shapeSurfacesCou.size(); loop++) {
		if (shapeSurfacesCou[loop] == 0) continue;
		const int sIndex = loop;
		int cou = 0;
		for (int i = 0; i < oldTriangles.size(); i++) {
			const int sIndex2 = (triSurfaceIndex[i] < 0) ? 0 : triSurfaceIndex[i];
			if (sIndex != sIndex2) continue;
			m_triangles.push_back(oldTriangles[i]);
			cou++;
		}
		if (cou == 0) continue;

		sxsdk::surface_class* pSurface = shapeSurfaces[loop];
		PMD_MATERIAL_DATA material;
		if (pSurface) {
			sxsdk::rgb_class col = (pSurface->get_diffuse_color()) * (pSurface->get_diffuse());
			material.diffuse_color = sxsdk::vec3(col.red, col.green, col.blue);

			float specularVal = pSurface->get_highlight();
			if (specularVal < 0.0f) specularVal = 0.0f;
			if (specularVal > 1.0f) specularVal = 1.0f;
			material.specular = (pSurface->get_highlight_size()) * 20.0f;
			col = (pSurface->get_highlight_color()) * specularVal;
			material.specular_color = sxsdk::vec3(col.red, col.green, col.blue);

			material.alpha = 1.0f - (pSurface->get_transparency());

			sxsdk::rgb_class ambCol = ambientCol;
			if (pSurface->get_has_ambient()) {
				ambCol = ambCol * ((pSurface->get_ambient_color()) * (pSurface->get_ambient()));
			}
			material.ambient_color = sxsdk::vec3(ambCol.red, ambCol.green, ambCol.blue);
		}
		// 対応する面頂点リストのデータ数.
		material.face_vert_count = cou * 3;

		// テクスチャを保存.
		if (loop >= 1) {
			const int mappingCou = pSurface->get_number_of_mapping_layers();
			if (mappingCou > 0) {
				int mappingIndex = -1;
				for (int i = 0; i < mappingCou; i++) {
					sxsdk::mapping_layer_class& mLayer = pSurface->mapping_layer(i);
					if (mLayer.get_type() == sxsdk::enums::diffuse_mapping && mLayer.get_pattern() == sxsdk::enums::image_pattern) {
						mappingIndex = i;
						break;
					}
				}
				if (mappingIndex >= 0) {
					sxsdk::mapping_layer_class& mLayer = pSurface->mapping_layer(mappingIndex);
					compointer<sxsdk::image_interface> image(mLayer.get_image_interface());
					if (image && image->has_image()) {
						// テクスチャ名は20バイト以内.
						char szName[64];
						sprintf(szName, "tex_%d.png", loop);
						material.tex_file_name = szName;

						sxsdk::master_image_class* pMImage = Util::GetMasterImageFromImage(scene, image);
						if (pMImage) {
							std::string name = pMImage->get_name();

							// 末尾から見て「\」「/」がある場合はカット.
							{
								int pos = name.find_last_of("\\");
								if (pos != std::string::npos) {
									name = name.substr(pos + 1);
								}
								pos = name.find_last_of("/");
								if (pos != std::string::npos) {
									name = name.substr(pos + 1);
								}
							}

							// 末尾の「.」以降をカット.
							{
								int pos = name.find_last_of(".");
								if (pos != std::string::npos) {
									name = name.substr(0, pos);
								}
							}
							name = name + ".png";

							if (name.length() < 20) {
								strcpy(szName, name.c_str());
								material.tex_file_name = szName;
							}
						}

						image->save(GetFileFullPathName(szName).c_str());
					}
				}
			}
		}

		material.edge_flag = m_toonEdge ? 1 : 0;		// トゥーンのエッジ表現.

		m_materials.push_back(material);
	}
}

/**
 * 指定のボーン名がすでに格納済みか.
 */
int CPMDData::m_FindBone(const std::string boneName)
{
	if (boneName.size() == 0) return -1;

	int boneIndex = -1;
	for (int i = 0; i < m_bones.size(); i++) {
		if (m_bones[i].bone_name.compare(boneName) == 0) {
			boneIndex = i;
			break;
		}
	}
	return boneIndex;
}

/**
 * 指定のボーン形状がすでに格納済みか.
 */
int CPMDData::m_FindBone(sxsdk::shape_class* pShape)
{
	if (!pShape) return -1;

	int boneIndex = -1;
	for (int i = 0; i < m_bones.size(); i++) {
		if (m_bones[i].pShadeShape == pShape) {
			boneIndex = i;
			break;
		}
	}
	return boneIndex;

}

/**
 * 指定のボーン間に存在するボーン番号を取得。endBoneIndexから親をたどるとstartBoneIndexに行き着くのが保証されているとする.
 */
std::vector<int> CPMDData::m_GetBonesList(const int endBoneIndex, const int startBoneIndex)
{
	std::vector<int> indexList;

	if (endBoneIndex < 0 || endBoneIndex >= m_bones.size()) return indexList;
	if (startBoneIndex < 0 || startBoneIndex >= m_bones.size()) return indexList;

	int boneIndex = endBoneIndex;
	while (boneIndex >= 0) {
		indexList.push_back(boneIndex);
		if (boneIndex == startBoneIndex) break;
		boneIndex = m_bones[boneIndex].parent_bone_index;
	}
	if (boneIndex < 0) indexList.clear();

	return indexList;
}

/**
 * ボーンの保持.
 */
void CPMDData::m_SetBones(sxsdk::shape_class& shape)
{
	const int skin_type = shape.get_skin_type();
	if (skin_type != 1) {		// 頂点ブレンドのスキンでない場合はスキップ.
		return;
	}
	if (shape.get_type() != sxsdk::enums::polygon_mesh) return;

	// 関連するボーンを取得.
	sxsdk::shape_class *pBoneRoot = Util::GetBoneRoot(shape);
	if (!pBoneRoot) return;
	m_pBoneRoot = pBoneRoot;

	// MMDのボーン名とどれくらい一致するか判定.
	m_humanRigBonesNameCheck = 0.0f;
	m_humanRigBonesType      = human_rig_type_default;
	{
		CRigCtrl rigCtrl(m_shade);
		m_humanRigBonesNameCheck = rigCtrl.CheckMMDBones(*pBoneRoot, &m_humanRigBonesType);
	}

	// ボーン情報を格納.
	m_bones.clear();
	m_SetBoneLoop(0, -1, pBoneRoot);
/*
	{
		char szStr[256];
		for (int i = 0; i < m_bones.size(); i++) {
			PMD_BONE_DATA& boneData = m_bones[i];
			sxsdk::vec3 v = boneData.bone_head_pos;
			sprintf(szStr, "%d : [%s] (%f, %f, %f) parent = %d tail = %d", i, boneData.bone_name.c_str(), v.x, v.y, v.z, boneData.parent_bone_index, boneData.tail_pos_bone_index);
			m_shade->message(szStr);
		}
	}
*/
}

void CPMDData::m_SetBoneLoop(const int depth, const int parentBoneIndex, sxsdk::shape_class* pBoneShape)
{
	if (!Util::IsBone(*pBoneShape)) return;

	const sxsdk::mat4 lwMat = pBoneShape->get_local_to_world_matrix();

	PMD_BONE_DATA boneData;
	boneData.pShadeShape   = pBoneShape;
	boneData.bone_head_pos = sxsdk::vec3(0, 0, 0) * (pBoneShape->get_transformation()) * lwMat;
	boneData.bone_name     = pBoneShape->get_name();
	if (parentBoneIndex >= 0) {
		boneData.parent_bone_index = parentBoneIndex;
	}
	const int curBoneIndex = m_bones.size();

	// ボーンの移動/回転フラグ指定.
	if (m_boneMoveRootOnly) {
		if (depth == 0) boneData.bone_type = bone_type_rotate_trans;
		else boneData.bone_type = bone_type_rotate;
	} else {
		boneData.bone_type = bone_type_rotate_trans;
	}

	// 末端ノードは非表示にする.
	if (!(pBoneShape->has_son())) {
		boneData.bone_type = bone_type_hide;
	}

	m_bones.push_back(boneData);

	if (parentBoneIndex >= 0) {
		PMD_BONE_DATA& parentBoneData = m_bones[parentBoneIndex];
		if (parentBoneData.tail_pos_bone_index <= 0) {
			parentBoneData.tail_pos_bone_index = curBoneIndex;
		}
	}

	if (pBoneShape->has_son()) {
		sxsdk::shape_class* pShape = pBoneShape->get_son();
		while (pShape->has_bro()) {
			pShape = pShape->get_bro();
			m_SetBoneLoop(depth + 1, curBoneIndex, pShape);
		}
	}
}

/**
 * 頂点に対応するボーンとスキン値の保持.
 */
void CPMDData::m_SetVertexSkins(sxsdk::shape_class& shape)
{
	if (shape.get_type() != sxsdk::enums::polygon_mesh) return;
	sxsdk::polygon_mesh_class& pmesh = shape.get_polygon_mesh();

	const int vCou   = m_vertices.size();
	const int verCou = pmesh.get_total_number_of_control_points();
	if (vCou != verCou) return;

	// 頂点ごとのスキン情報を取得.
	// MMDでは、1頂点に影響を与えることができるボーンは2つ。
	std::vector<sxsdk::skin_bind_class *> skins;
	for (int i = 0; i < vCou; i++) {
		PMD_VERTEX_DATA& vData = m_vertices[i];
		vData.bone_num[0] = -1;
		vData.bone_num[1] = -1;
		vData.bone_weight = 0;

		sxsdk::vertex_class& v = pmesh.vertex(i);
		sxsdk::skin_class& skin = v.get_skin();
		int bind_cou = skin.get_number_of_binds();
		if (bind_cou <= 0) continue;

		skins.clear();
		for (int j = 0; j < bind_cou; j++) {
			sxsdk::skin_bind_class* pSkinBind = &(skin.get_bind(j));
			skins.push_back(pSkinBind);
		}

		for (int j = 0; j < skins.size(); j++) {
			for (int k = j + 1; k < skins.size(); k++) {
				if ((skins[j]->get_weight()) < (skins[k]->get_weight())) {
					std::swap(skins[j], skins[k]);
				}
			}
		}

		int bone0     = m_FindBone(skins[0]->get_shape()->get_name());
		float weight0 = skins[0]->get_weight();
		int bone1     = -1;
		float weight1 = 0.0f;
		if (skins.size() > 1) {
			bone1   = m_FindBone(skins[1]->get_shape()->get_name());
			weight1 = skins[1]->get_weight();
		}

		if (bone0 >= 0 && bone1 >= 0) {
			vData.bone_num[0] = bone0;
			vData.bone_num[1] = bone1;
			vData.bone_weight = (int)(weight0 * 100.0f / (weight0 + weight1));
		} else if (bone0 >= 0) {
			vData.bone_num[0] = bone0;
			vData.bone_weight = 100;
		}
	}

}

/**
 * IK情報の格納.
 * MMDでのIKは、親を持たないボーンをルートとし、影響するボーンがリストで登録されている.
 * IK先がゴールに相当.
 * 例 : IK「左足IK」 ... ボーンとして「左足IK」（左足首の位置に配置、親なし）。ボーンとして「左足IK先」（左足首より少し+Z方向に配置。親は左足IK）.
 *                       IK「左足IK」は、targetボーンを「左足首」とし、影響下のボーンとして「左足」「左ひざ」を持つ.
 */
void CPMDData::m_SetIKs(sxsdk::scene_interface* scene, sxsdk::shape_class& shape)
{
	if (!m_pBoneRoot) return;

	try {
		sxsdk::ik_class& ik = scene->get_ik();
		const int ikCou = ik.get_number_of_ik();
		if (ikCou == 0) return;
		m_SetIKsLoop(ik, scene, *m_pBoneRoot);
	} catch (...) { }
}

void CPMDData::m_SetIKsLoop(sxsdk::ik_class& ik, sxsdk::scene_interface* scene, sxsdk::shape_class& shape)
{
	if (!Util::IsBone(shape)) return;

	int ikType = ik.has_ik(shape, false);
	if (ikType & 0x02) {		// IK Endを持つ場合.
		sxsdk::ik_data_class& ikData = ik.get_ik_data(shape);
		m_AddIK(ikData.get_root_shape(), ikData.get_end_shape(), ikData.get_goal_shape());
	}

	if (shape.has_son()) {
		sxsdk::shape_class* pShape = shape.get_son();
		while (pShape->has_bro()) {
			pShape = pShape->get_bro();
			m_SetIKsLoop(ik, scene, *pShape);
		}
	}
}

/**
 * 指定のIK情報を格納。この際に、IK用ボーンも生成.
 */
bool CPMDData::m_AddIK(sxsdk::shape_class* ikRootShape, sxsdk::shape_class* ikEndShape, sxsdk::shape_class* ikGoalShape)
{
	if (ikRootShape == NULL || ikEndShape == NULL || ikGoalShape == NULL) return false;

	const int ikRootBoneIndex = m_FindBone(ikRootShape);
	const int ikEndBoneIndex  = m_FindBone(ikEndShape);
	if (ikRootBoneIndex < 0 || ikEndBoneIndex < 0) return false;

	// ルートノードでは、IKの割り当てはできない.
	if (ikRootBoneIndex == 0 || ikEndBoneIndex == 0) {
		return false;
	}

	// goalのワールド座標位置.
	// ※ IKでのgoalはボールジョイント.
	sxsdk::vec3 goalWPos = sxsdk::vec3(0, 0, 0);
	if (ikGoalShape->get_type() == sxsdk::enums::part) {
		if (ikGoalShape->get_part().get_part_type() == sxsdk::enums::ball_joint) {
			compointer<sxsdk::ball_joint_interface> ball(ikGoalShape->get_ball_joint_interface());
			goalWPos = (ball->get_position()) * (ikGoalShape->get_local_to_world_matrix());
		}
	}

	float zureDist = 0.0f;
	{
		// 「左足首」.
		const int ankle_L_index = CRigCtrl::GetHumanBoneIndex(m_shade, "ankle_left", human_rig_type_default);
		int ankle_L = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, ankle_L_index, m_humanRigBonesType));

		// 「左つま先」.
		const int ankle_L_tail_index = CRigCtrl::GetHumanBoneIndex(m_shade, "ankle_left2", human_rig_type_default);
		int ankle_L_tail = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, ankle_L_tail_index, m_humanRigBonesType));

		// IK先をずらす量.
		zureDist = sxsdk::absolute(m_bones[ankle_L].bone_head_pos - m_bones[ankle_L_tail].bone_head_pos) * 0.6f;
	}

	// ikEndBoneIndex - ikRootBoneIndex の間のボーンを取得.
	std::vector<int> childBoneList = m_GetBonesList(ikEndBoneIndex, ikRootBoneIndex);
	if (childBoneList.size() == 0) return false;

	// 対応する「左足」「左足首」「右足」「右足首」のボーン名称を取得.
	const int leg_L_index   = CRigCtrl::GetHumanBoneIndex(m_shade, "leg_left", human_rig_type_default);
	const int ankle_L_index = CRigCtrl::GetHumanBoneIndex(m_shade, "ankle_left", human_rig_type_default);
	const int leg_R_index   = CRigCtrl::GetHumanBoneIndex(m_shade, "leg_right", human_rig_type_default);
	const int ankle_R_index = CRigCtrl::GetHumanBoneIndex(m_shade, "ankle_right", human_rig_type_default);

	std::string leg_L_name   = CRigCtrl::GetHumanBoneName(m_shade, leg_L_index, m_humanRigBonesType);
	std::string ankle_L_name = CRigCtrl::GetHumanBoneName(m_shade, ankle_L_index, m_humanRigBonesType);
	std::string leg_R_name   = CRigCtrl::GetHumanBoneName(m_shade, leg_R_index, m_humanRigBonesType);
	std::string ankle_R_name = CRigCtrl::GetHumanBoneName(m_shade, ankle_R_index, m_humanRigBonesType);

	int leg_bone_type = -1;
	std::string rootBoneName = m_bones[ikRootBoneIndex].bone_name;
	if (rootBoneName.compare(leg_L_name) == 0)        leg_bone_type = 0;
	else if (rootBoneName.compare(ankle_L_name) == 0) leg_bone_type = 1;
	else if (rootBoneName.compare(leg_R_name) == 0)   leg_bone_type = 2;
	else if (rootBoneName.compare(ankle_R_name) == 0) leg_bone_type = 3;

	if (leg_bone_type >= 0 && zureDist == 0.0f) return false;

	//---------------------------------------.
	//	IK影響下のボーンの属性を変更.
	//---------------------------------------.
	for (int i = 0 ; i < childBoneList.size(); i++) {
		const int bIndex = childBoneList[i];
		if (bIndex > 0) {
			PMD_BONE_DATA& bone = m_bones[bIndex];
			bone.bone_type = bone_type_ik_u;		// IK影響下（回転）.
		}
	}
	{
		const int bIndex = childBoneList[0];
		PMD_BONE_DATA& bone = m_bones[bIndex];
		bone.bone_type = bone_type_ik_c;		// IK接続先.
	}

	//---------------------------------------.
	// IK用のボーンを生成.
	//---------------------------------------.
	PMD_BONE_DATA boneData0, boneData1;

	const int ik_index      = m_bones.size();
	const int ik_tail_index = ik_index + 1;

	// IK rootに相当する位置に、すでに既存のIKのendが存在するか.
	int prev_ik_index = -1;
	for (int i = 0; i < m_IKs.size(); i++) {
		PMD_IK_DATA& ikData = m_IKs[i];
		if (ikData.ik_target_bone_index == ikRootBoneIndex) {
			prev_ik_index = ikData.ik_bone_index;
			break;
		}
	}

	if (leg_bone_type == 0) {
		boneData0.bone_name           = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_L]);
		boneData0.bone_head_pos       = m_bones[ikEndBoneIndex].bone_head_pos;
		boneData0.parent_bone_index   = -1;
		boneData0.tail_pos_bone_index = ik_tail_index;
		boneData0.bone_type           = bone_type_ik;

		boneData1.bone_name           = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_L_tail]);
		boneData1.bone_head_pos       = boneData0.bone_head_pos;
		boneData1.bone_head_pos.z -= zureDist;
		boneData1.parent_bone_index   = ik_index;
		boneData1.tail_pos_bone_index = -1;
		boneData1.bone_type           = bone_type_hide;

		m_bones.push_back(boneData0);
		m_bones.push_back(boneData1);

	} else if (leg_bone_type == 1) {
		boneData0.bone_name           = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_L]);
		boneData0.bone_head_pos       = m_bones[ikEndBoneIndex].bone_head_pos;
		boneData0.parent_bone_index   = prev_ik_index;
		boneData0.tail_pos_bone_index = ik_tail_index;
		boneData0.bone_type           = bone_type_ik;

		boneData1.bone_name           = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_L_tail]);
		boneData1.bone_head_pos       = boneData0.bone_head_pos;
		boneData1.bone_head_pos.y -= zureDist;
		boneData1.parent_bone_index   = ik_index;
		boneData1.tail_pos_bone_index = -1;
		boneData1.bone_type           = bone_type_hide;

		m_bones.push_back(boneData0);
		m_bones.push_back(boneData1);

	} else if (leg_bone_type == 2) {
		boneData0.bone_name           = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_R]);
		boneData0.bone_head_pos       = m_bones[ikEndBoneIndex].bone_head_pos;
		boneData0.parent_bone_index   = -1;
		boneData0.tail_pos_bone_index = ik_tail_index;
		boneData0.bone_type           = bone_type_ik;

		boneData1.bone_name           = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_R_tail]);
		boneData1.bone_head_pos       = boneData0.bone_head_pos;
		boneData1.bone_head_pos.z -= zureDist;
		boneData1.parent_bone_index   = ik_index;
		boneData1.tail_pos_bone_index = -1;
		boneData1.bone_type           = bone_type_hide;

		m_bones.push_back(boneData0);
		m_bones.push_back(boneData1);

	} else if (leg_bone_type == 3) {
		boneData0.bone_name           = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_R]);
		boneData0.bone_head_pos       = m_bones[ikEndBoneIndex].bone_head_pos;
		boneData0.parent_bone_index   = prev_ik_index;
		boneData0.tail_pos_bone_index = ik_tail_index;
		boneData0.bone_type           = bone_type_ik;

		boneData1.bone_name           = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_R_tail]);
		boneData1.bone_head_pos       = boneData0.bone_head_pos;
		boneData1.bone_head_pos.y -= zureDist;
		boneData1.parent_bone_index   = ik_index;
		boneData1.tail_pos_bone_index = -1;
		boneData1.bone_type           = bone_type_hide;

		m_bones.push_back(boneData0);
		m_bones.push_back(boneData1);

	} else {
		// IK goal位置に、IK endを配置する。.
		const std::string ikBoneName0 = std::string(m_bones[ikRootBoneIndex].bone_name) + "_IK";
		const std::string ikBoneName1 = ikBoneName0 + "2";

		boneData0.bone_name           = ikBoneName0;
		boneData0.bone_head_pos       = goalWPos;	//m_bones[ikEndBoneIndex].bone_head_pos;
		boneData0.parent_bone_index   = prev_ik_index;
		boneData0.tail_pos_bone_index = ik_tail_index;
		boneData0.bone_type           = bone_type_ik;

		boneData1.bone_name           = ikBoneName1;
		boneData1.bone_head_pos       = goalWPos;
		boneData1.parent_bone_index   = ik_index;
		boneData1.tail_pos_bone_index = -1;
		boneData1.bone_type           = bone_type_hide;

		m_bones.push_back(boneData0);
		m_bones.push_back(boneData1);
	}

	//---------------------------------------.
	//	IKの情報を追加.
	//---------------------------------------.
	PMD_IK_DATA ikData;
	if (childBoneList.size() <= 2) {
		ikData.iterations     = 3;
		ikData.control_weight = 1.0f;
	} else {
		ikData.iterations     = 40;
		ikData.control_weight = 0.5f;
	}

	{
		ikData.ik_bone_index        = ik_index;				// 対象となるボーンでのIK Root.
		ikData.ik_target_bone_index = childBoneList[0];		// 対象となるIK end位置のボーン.

		for (int i = 1; i < childBoneList.size(); i++) {
			ikData.ik_child_bone_index.push_back(childBoneList[i]);
		}

		ikData.shade_ik_root = childBoneList[childBoneList.size() - 1];
		ikData.shade_ik_end  = childBoneList[0];
		ikData.shade_ik_goal = ik_tail_index;

		m_IKs.push_back(ikData);
	}

	return true;
}

/**
 * 指定のIKがすでに格納済みか調べる.
 */
int CPMDData::m_GetIK(const int ikRootBoneIndex)
{
	int index = -1;
	for (int i = 0; i < m_IKs.size(); i++) {
		if (m_IKs[i].shade_ik_root == ikRootBoneIndex) {
			index = i;
			break;
		}
	}

	return index;
}

/**
 * 人体時のIK情報を自動で追加、IK用ボーンも追加される.
 */
void CPMDData::m_SetHumanBoneIKs()
{
	// MMDのリグ構成かどうか.
	if (m_humanRigBonesNameCheck < 0.5f) return;

	{
		int leg_IK_L = -1;
		int leg_IK_L_tail = -1;
		int toe_IK_L = -1;
		int toe_IK_L_tail = -1;

		// 「左足首」.
		const int ankle_L_index = CRigCtrl::GetHumanBoneIndex(m_shade, "ankle_left", human_rig_type_default);
		int ankle_L = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, ankle_L_index, m_humanRigBonesType));

		// 「左足」.
		const int leg_L_index = CRigCtrl::GetHumanBoneIndex(m_shade, "leg_left", human_rig_type_default);
		int leg_L = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, leg_L_index, m_humanRigBonesType));

		// 「左ひざ」.
		const int knee_L_index = CRigCtrl::GetHumanBoneIndex(m_shade, "knee_left", human_rig_type_default);
		int knee_L = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, knee_L_index, m_humanRigBonesType));

		// 「左つま先」.
		const int ankle_L_tail_index = CRigCtrl::GetHumanBoneIndex(m_shade, "ankle_left2", human_rig_type_default);
		int ankle_L_tail = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, ankle_L_tail_index, m_humanRigBonesType));

		// IK先をずらす量.
		const float zureDist = sxsdk::absolute(m_bones[ankle_L].bone_head_pos - m_bones[ankle_L_tail].bone_head_pos) * 0.6f;

		if (ankle_L >= 0 && ankle_L_tail >= 0 && knee_L >= 0 && ankle_L_tail >= 0 && (m_GetIK(leg_L) < 0 && m_GetIK(ankle_L) < 0)) {
			leg_IK_L      = m_bones.size();
			leg_IK_L_tail = leg_IK_L + 1;
			toe_IK_L      = leg_IK_L + 2;
			toe_IK_L_tail = leg_IK_L + 3;

			// ボーン属性の変更（これがないとMMD上で変な回転がかかったりする）.
			{
				PMD_BONE_DATA& bone = m_bones[leg_L];
				bone.bone_type = bone_type_ik_u;		// IK影響下（回転）.
			}
			{
				PMD_BONE_DATA& bone = m_bones[knee_L];
				bone.bone_type = bone_type_ik_u;		// IK影響下（回転）.
			}
			{
				PMD_BONE_DATA& bone = m_bones[ankle_L];
				bone.bone_type = bone_type_ik_u;		// IK影響下（回転）.
			}
			{
				PMD_BONE_DATA& bone = m_bones[ankle_L_tail];
				bone.bone_type = bone_type_ik_c;		// IK接続先.
			}

			{
				PMD_BONE_DATA boneData0, boneData1;
				boneData0.bone_name     = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_L]);
				boneData0.bone_head_pos = m_bones[ankle_L].bone_head_pos;
				boneData0.parent_bone_index   = -1;
				boneData0.tail_pos_bone_index = leg_IK_L_tail;
				boneData0.bone_type           = bone_type_ik;

				boneData1.bone_name     = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_L_tail]);
				boneData1.bone_head_pos = boneData0.bone_head_pos;
				boneData1.bone_head_pos.z -= zureDist;
				boneData1.parent_bone_index   = leg_IK_L;
				boneData1.tail_pos_bone_index = -1;
				boneData1.bone_type           = bone_type_hide;

				m_bones.push_back(boneData0);
				m_bones.push_back(boneData1);

				PMD_BONE_DATA boneData2, boneData3;

				boneData2.bone_name     = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_L]);
				boneData2.bone_head_pos = m_bones[ankle_L_tail].bone_head_pos;
				boneData2.parent_bone_index   = leg_IK_L;
				boneData2.tail_pos_bone_index = toe_IK_L_tail;
				boneData2.bone_type           = bone_type_ik;

				boneData3.bone_name     = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_L_tail]);
				boneData3.bone_head_pos = boneData2.bone_head_pos;
				boneData3.bone_head_pos.y -= zureDist;
				boneData3.parent_bone_index   = toe_IK_L;
				boneData3.tail_pos_bone_index = -1;
				boneData3.bone_type           = bone_type_hide;

				m_bones.push_back(boneData2);
				m_bones.push_back(boneData3);
			}

			// IK情報を登録.
			{
				PMD_IK_DATA ikData;
				ikData.iterations           = 40;
				ikData.control_weight       = 0.5f;
				ikData.ik_bone_index        = leg_IK_L;
				ikData.ik_target_bone_index = ankle_L;
				ikData.ik_child_bone_index.push_back(knee_L);
				ikData.ik_child_bone_index.push_back(leg_L);

				ikData.shade_ik_root = leg_L;
				ikData.shade_ik_end  = ankle_L;
				ikData.shade_ik_goal = leg_IK_L_tail;

				m_IKs.push_back(ikData);
			}
			{
				PMD_IK_DATA ikData;
				ikData.iterations           = 3;
				ikData.control_weight       = 1.0f;
				ikData.ik_bone_index        = toe_IK_L;
				ikData.ik_target_bone_index = ankle_L_tail;
				ikData.ik_child_bone_index.push_back(ankle_L);

				ikData.shade_ik_root = ankle_L;
				ikData.shade_ik_end  = ankle_L_tail;
				ikData.shade_ik_goal = toe_IK_L_tail;

				m_IKs.push_back(ikData);
			}
		}
	}

	{
		int leg_IK_R = -1;
		int leg_IK_R_tail = -1;
		int toe_IK_R = -1;
		int toe_IK_R_tail = -1;

		// 「右足首」.
		const int ankle_R_index = CRigCtrl::GetHumanBoneIndex(m_shade, "ankle_right", human_rig_type_default);
		int ankle_R = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, ankle_R_index, m_humanRigBonesType));

		// 「右足」.
		const int leg_R_index = CRigCtrl::GetHumanBoneIndex(m_shade, "leg_right", human_rig_type_default);
		int leg_R = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, leg_R_index, m_humanRigBonesType));

		// 「右ひざ」.
		const int knee_R_index = CRigCtrl::GetHumanBoneIndex(m_shade, "knee_right", human_rig_type_default);
		int knee_R = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, knee_R_index, m_humanRigBonesType));

		// 「右つま先」.
		const int ankle_R_tail_index = CRigCtrl::GetHumanBoneIndex(m_shade, "ankle_right2", human_rig_type_default);
		int ankle_R_tail = m_FindBone(CRigCtrl::GetHumanBoneName(m_shade, ankle_R_tail_index, m_humanRigBonesType));

		// IK先をずらす量.
		const float zureDist = sxsdk::absolute(m_bones[ankle_R].bone_head_pos - m_bones[ankle_R_tail].bone_head_pos) * 0.6f;

		if (ankle_R >= 0 && ankle_R_tail >= 0 && knee_R >= 0 && ankle_R_tail >= 0 && (m_GetIK(leg_R) < 0 && m_GetIK(ankle_R) < 0)) {
			leg_IK_R      = m_bones.size();
			leg_IK_R_tail = leg_IK_R + 1;
			toe_IK_R      = leg_IK_R + 2;
			toe_IK_R_tail = leg_IK_R + 3;

			// ボーン属性の変更（これがないとMMD上で変な回転がかかったりする）.
			{
				PMD_BONE_DATA& bone = m_bones[leg_R];
				bone.bone_type = bone_type_ik_u;		// IK影響下（回転）.
			}
			{
				PMD_BONE_DATA& bone = m_bones[knee_R];
				bone.bone_type = bone_type_ik_u;		// IK影響下（回転）.
			}
			{
				PMD_BONE_DATA& bone = m_bones[ankle_R];
				bone.bone_type = bone_type_ik_u;		// IK影響下（回転）.
			}
			{
				PMD_BONE_DATA& bone = m_bones[ankle_R_tail];
				bone.bone_type = bone_type_ik_c;		// IK接続先.
			}

			{
				PMD_BONE_DATA boneData0, boneData1;
				boneData0.bone_name     = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_R]);
				boneData0.bone_head_pos = m_bones[ankle_R].bone_head_pos;
				boneData0.parent_bone_index   = -1;
				boneData0.tail_pos_bone_index = leg_IK_R_tail;
				boneData0.bone_type           = bone_type_ik;

				boneData1.bone_name     = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_leg_IK_R_tail]);
				boneData1.bone_head_pos = boneData0.bone_head_pos;
				boneData1.bone_head_pos.z -= zureDist;
				boneData1.parent_bone_index   = leg_IK_R;
				boneData1.tail_pos_bone_index = -1;
				boneData1.bone_type           = bone_type_hide;

				m_bones.push_back(boneData0);
				m_bones.push_back(boneData1);

				PMD_BONE_DATA boneData2, boneData3;

				boneData2.bone_name     = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_R]);
				boneData2.bone_head_pos = m_bones[ankle_R_tail].bone_head_pos;
				boneData2.parent_bone_index   = leg_IK_R;
				boneData2.tail_pos_bone_index = toe_IK_R_tail;
				boneData2.bone_type           = bone_type_ik;

				boneData3.bone_name     = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[index_toe_IK_R_tail]);
				boneData3.bone_head_pos = boneData2.bone_head_pos;
				boneData3.bone_head_pos.y -= zureDist;
				boneData3.parent_bone_index   = toe_IK_R;
				boneData3.tail_pos_bone_index = -1;
				boneData3.bone_type           = bone_type_hide;

				m_bones.push_back(boneData2);
				m_bones.push_back(boneData3);
			}

			// IK情報を登録.
			{
				PMD_IK_DATA ikData;
				ikData.iterations           = 40;
				ikData.control_weight       = 0.5f;
				ikData.ik_bone_index        = leg_IK_R;
				ikData.ik_target_bone_index = ankle_R;
				ikData.ik_child_bone_index.push_back(knee_R);
				ikData.ik_child_bone_index.push_back(leg_R);

				ikData.shade_ik_root = leg_R;
				ikData.shade_ik_end  = ankle_R;
				ikData.shade_ik_goal = leg_IK_R_tail;

				m_IKs.push_back(ikData);
			}
			{
				PMD_IK_DATA ikData;
				ikData.iterations           = 3;
				ikData.control_weight       = 1.0f;
				ikData.ik_bone_index        = toe_IK_R;
				ikData.ik_target_bone_index = ankle_R_tail;
				ikData.ik_child_bone_index.push_back(ankle_R);

				ikData.shade_ik_root = ankle_R;
				ikData.shade_ik_end  = ankle_R_tail;
				ikData.shade_ik_goal = toe_IK_R_tail;

				m_IKs.push_back(ikData);
			}
		}
	}

}

/**
 * 表示枠情報を設定 （人体の基本的な構成のみ）.
 */
void CPMDData::m_SetBonesDisp()
{
	m_bonesDisp.clear();

	std::vector<bool> chkBones;
	chkBones.resize(m_bones.size(), false);

	{
		PMD_BONE_DISP_DATA data;
		data.disp_name    = Util::GetUTF8Text(*m_shade, "ＩＫ");
		data.disp_name_en = "IK"; 
		m_bonesDisp.push_back(data);
	}
	{
		PMD_BONE_DISP_DATA data;
		data.disp_name    = Util::GetUTF8Text(*m_shade, "体(上)");
		data.disp_name_en = "Body[u]";
		m_bonesDisp.push_back(data);
	}
	{
		PMD_BONE_DISP_DATA data;
		data.disp_name    = Util::GetUTF8Text(*m_shade, "腕");
		data.disp_name_en = "Arms"; 
		m_bonesDisp.push_back(data);
	}
	{
		PMD_BONE_DISP_DATA data;
		data.disp_name    = Util::GetUTF8Text(*m_shade, "指");
		data.disp_name_en = "Fingers"; 
		m_bonesDisp.push_back(data);
	}
	{
		PMD_BONE_DISP_DATA data;
		data.disp_name    = Util::GetUTF8Text(*m_shade, "体(下)");
		data.disp_name_en = "Body[l]"; 
		m_bonesDisp.push_back(data);
	}
	{
		PMD_BONE_DISP_DATA data;
		data.disp_name    = Util::GetUTF8Text(*m_shade, "足");
		data.disp_name_en = "Legs"; 
		m_bonesDisp.push_back(data);
	}
	{	// 最後は必ず「その他」にする。ここに、上記に属さないボーンが列挙される.
		PMD_BONE_DISP_DATA data;
		data.disp_name    = Util::GetUTF8Text(*m_shade, "その他");
		data.disp_name_en = "Others"; 
		m_bonesDisp.push_back(data);
	}

	// 各表示枠に対応するボーンを格納.
	{
		// ボーンのうち、IK関連のもののみ登録.
		const int dispIndex = 0;
		PMD_BONE_DISP_DATA& dData = m_bonesDisp[dispIndex];
		PMD_BONE_DISP_LIST_DATA bData;
		for (int i = 0; i < m_bones.size(); i++) {
			PMD_BONE_DATA& bone = m_bones[i];
			if (bone.bone_type != bone_type_ik) continue;
			if (!chkBones[i]) {
				bData.bone_disp_index = dispIndex;
				bData.bone_index      = i;
				dData.data.push_back(bData);
				chkBones[bData.bone_index] = true;
			}
		}
	}
	{
		// ボーンのうち、指関連のもののみ登録.
		const int dispIndex = 3;
		PMD_BONE_DISP_DATA& dData = m_bonesDisp[dispIndex];

		PMD_BONE_DISP_LIST_DATA bData;
		bData.bone_disp_index = dispIndex;

		int tBoneIndexL = -1;
		int tBoneIndexR = -1;
		{
			const int bSrcIndex    = CRigCtrl::GetHumanBoneIndex(m_shade, "wrist_left", human_rig_type_default);
			const std::string name = CRigCtrl::GetHumanBoneName(m_shade, bSrcIndex, m_humanRigBonesType);
			tBoneIndexL = m_FindBone(name);
		}
		{
			const int bSrcIndex    = CRigCtrl::GetHumanBoneIndex(m_shade, "wrist_right", human_rig_type_default);
			const std::string name = CRigCtrl::GetHumanBoneName(m_shade, bSrcIndex, m_humanRigBonesType);
			tBoneIndexR = m_FindBone(name);
		}

		if (tBoneIndexL >= 0 || tBoneIndexR >= 0) {
			for (int i = 0; i < m_bones.size(); i++) {
				PMD_BONE_DATA& bone = m_bones[i];
				if (bone.bone_type == bone_type_hide || bone.bone_type == bone_type_ik_c || chkBones[i]) continue;
				if (bone.parent_bone_index < 0) continue;

				bool chkF  = false;
				int bIndex = bone.parent_bone_index;
				for (int j = 0; j < 100; j++) {
					if (m_bones[bIndex].parent_bone_index < 0) break;
					if (bIndex == tBoneIndexL || bIndex == tBoneIndexR) {
						chkF = true;
						break;
					}
					bIndex = m_bones[bIndex].parent_bone_index;
				}
				if (chkF) {
					bData.bone_disp_index = dispIndex;
					bData.bone_index      = i;
					dData.data.push_back(bData);
					chkBones[bData.bone_index] = true;
				}
			}
		}

	}

	{
		// ボーンのうち、腕関連のもののみ登録.
		const int dispIndex = 2;
		PMD_BONE_DISP_DATA& dData = m_bonesDisp[dispIndex];

		PMD_BONE_DISP_LIST_DATA bData;
		bData.bone_disp_index = dispIndex;

		int tBoneIndexL = -1;
		int tBoneIndexR = -1;
		{
			const int bSrcIndex    = CRigCtrl::GetHumanBoneIndex(m_shade, "shoulder_left", human_rig_type_default);
			const std::string name = CRigCtrl::GetHumanBoneName(m_shade, bSrcIndex, m_humanRigBonesType);
			tBoneIndexL = m_FindBone(name);
		}
		{
			const int bSrcIndex    = CRigCtrl::GetHumanBoneIndex(m_shade, "shoulder_right", human_rig_type_default);
			const std::string name = CRigCtrl::GetHumanBoneName(m_shade, bSrcIndex, m_humanRigBonesType);
			tBoneIndexR = m_FindBone(name);
		}

		if (tBoneIndexL >= 0 || tBoneIndexR >= 0) {
			for (int i = 0; i < m_bones.size(); i++) {
				PMD_BONE_DATA& bone = m_bones[i];
				if (bone.bone_type == bone_type_hide || bone.bone_type == bone_type_ik_c || chkBones[i]) continue;
				if (bone.parent_bone_index < 0) continue;

				bool chkF  = false;
				if (i == tBoneIndexL || i == tBoneIndexR) chkF = true;
				else {
					int bIndex = bone.parent_bone_index;
					for (int j = 0; j < 100; j++) {
						if (m_bones[bIndex].parent_bone_index < 0) break;
						if (bIndex == tBoneIndexL || bIndex == tBoneIndexR) {
							chkF = true;
							break;
						}
						bIndex = m_bones[bIndex].parent_bone_index;
					}
				}
				if (chkF) {
					bData.bone_disp_index = dispIndex;
					bData.bone_index      = i;
					dData.data.push_back(bData);
					chkBones[bData.bone_index] = true;
				}
			}
		}
	}

	{
		// ボーンのうち、上半身関連のもののみ登録.
		const int dispIndex = 1;
		PMD_BONE_DISP_DATA& dData = m_bonesDisp[dispIndex];

		PMD_BONE_DISP_LIST_DATA bData;
		bData.bone_disp_index = dispIndex;

		int tBoneIndex = -1;
		{
			const int bSrcIndex    = CRigCtrl::GetHumanBoneIndex(m_shade, "upper_body", human_rig_type_default);
			const std::string name = CRigCtrl::GetHumanBoneName(m_shade, bSrcIndex, m_humanRigBonesType);
			tBoneIndex = m_FindBone(name);
		}

		if (tBoneIndex >= 0) {
			for (int i = 0; i < m_bones.size(); i++) {
				PMD_BONE_DATA& bone = m_bones[i];
				if (bone.bone_type == bone_type_hide || bone.bone_type == bone_type_ik_c || chkBones[i]) continue;
				if (bone.parent_bone_index < 0) continue;

				bool chkF  = false;
				if (i == tBoneIndex) chkF = true;
				else {
					int bIndex = bone.parent_bone_index;
					for (int j = 0; j < 100; j++) {
						if (m_bones[bIndex].parent_bone_index < 0) break;
						if (bIndex == tBoneIndex) {
							chkF = true;
							break;
						}
						bIndex = m_bones[bIndex].parent_bone_index;
					}
				}
				if (chkF) {
					bData.bone_disp_index = dispIndex;
					bData.bone_index      = i;
					dData.data.push_back(bData);
					chkBones[bData.bone_index] = true;
				}
			}
		}
	}

	{
		// ボーンのうち、足関連のもののみ登録.
		const int dispIndex = 5;
		PMD_BONE_DISP_DATA& dData = m_bonesDisp[dispIndex];

		PMD_BONE_DISP_LIST_DATA bData;
		bData.bone_disp_index = dispIndex;

		int tBoneIndexL = -1;
		int tBoneIndexR = -1;
		{
			const int bSrcIndex    = CRigCtrl::GetHumanBoneIndex(m_shade, "leg_left", human_rig_type_default);
			const std::string name = CRigCtrl::GetHumanBoneName(m_shade, bSrcIndex, m_humanRigBonesType);
			tBoneIndexL = m_FindBone(name);
		}
		{
			const int bSrcIndex    = CRigCtrl::GetHumanBoneIndex(m_shade, "leg_right", human_rig_type_default);
			const std::string name = CRigCtrl::GetHumanBoneName(m_shade, bSrcIndex, m_humanRigBonesType);
			tBoneIndexR = m_FindBone(name);
		}

		if (tBoneIndexL >= 0 || tBoneIndexR >= 0) {
			for (int i = 0; i < m_bones.size(); i++) {
				PMD_BONE_DATA& bone = m_bones[i];
				if (bone.bone_type == bone_type_hide || bone.bone_type == bone_type_ik_c || chkBones[i]) continue;
				if (bone.parent_bone_index < 0) continue;

				bool chkF  = false;
				if (i == tBoneIndexL || i == tBoneIndexR) chkF = true;
				else {
					int bIndex = bone.parent_bone_index;
					for (int j = 0; j < 100; j++) {
						if (m_bones[bIndex].parent_bone_index < 0) break;
						if (bIndex == tBoneIndexL || bIndex == tBoneIndexR) {
							chkF = true;
							break;
						}
						bIndex = m_bones[bIndex].parent_bone_index;
					}
				}
				if (chkF) {
					bData.bone_disp_index = dispIndex;
					bData.bone_index      = i;
					dData.data.push_back(bData);
					chkBones[bData.bone_index] = true;
				}
			}
		}
	}

	{
		// ボーンのうち、下半身関連のもののみ登録.
		const int dispIndex = 4;
		PMD_BONE_DISP_DATA& dData = m_bonesDisp[dispIndex];

		PMD_BONE_DISP_LIST_DATA bData;
		bData.bone_disp_index = dispIndex;

		int tBoneIndex = -1;
		{
			const int bSrcIndex    = CRigCtrl::GetHumanBoneIndex(m_shade, "lower_body", human_rig_type_default);
			const std::string name = CRigCtrl::GetHumanBoneName(m_shade, bSrcIndex, m_humanRigBonesType);
			tBoneIndex = m_FindBone(name);
		}

		if (tBoneIndex >= 0) {
			for (int i = 0; i < m_bones.size(); i++) {
				PMD_BONE_DATA& bone = m_bones[i];
				if (bone.bone_type == bone_type_hide || bone.bone_type == bone_type_ik_c || chkBones[i]) continue;
				if (bone.parent_bone_index < 0) continue;

				bool chkF  = false;
				if (i == tBoneIndex) chkF = true;
				else {
					int bIndex = bone.parent_bone_index;
					for (int j = 0; j < 100; j++) {
						if (m_bones[bIndex].parent_bone_index < 0) break;
						if (bIndex == tBoneIndex) {
							chkF = true;
							break;
						}
						bIndex = m_bones[bIndex].parent_bone_index;
					}
				}
				if (chkF) {
					bData.bone_disp_index = dispIndex;
					bData.bone_index      = i;
					dData.data.push_back(bData);
					chkBones[bData.bone_index] = true;
				}
			}
		}
	}

	{
		// 何にも属さないものは「その他」へ.
		const int dispIndex = m_bonesDisp.size() - 1;
		PMD_BONE_DISP_DATA& dData = m_bonesDisp[dispIndex];

		PMD_BONE_DISP_LIST_DATA bData;
		bData.bone_disp_index = dispIndex;

		for (int i = 0; i < m_bones.size(); i++) {
			PMD_BONE_DATA& bone = m_bones[i];
			if (bone.bone_type == bone_type_hide || bone.bone_type == bone_type_ik_c || chkBones[i]) continue;
			if (bone.parent_bone_index < 0) continue;

			bData.bone_disp_index = dispIndex;
			bData.bone_index      = i;
			dData.data.push_back(bData);
			chkBones[bData.bone_index] = true;
		}
	}
}

//---------------------------------------------------------------------------------------.

/**
 * streamに出力.
 */
bool CPMDData::Export(sxsdk::stream_interface *stream, CPMDDlgInfo& pmdInfo)
{
	m_WriteHeader(stream);
	m_WriteVertices(stream);
	m_WriteFaces(stream);
	m_WriteMaterials(stream);
	m_WriteBones(stream);
	m_WriteIKs(stream);
	m_WriteSkins(stream);
	m_WriteSkinWaku(stream);
	m_WriteBoneWaku(stream);
	m_WriteBoneList(stream);

	m_WriteExEnglishInfo(stream);

	m_WriteToonTextureList(stream);
	m_WritePhysicsRigidbodyList(stream);
	m_WritePhysicsJointList(stream);

	return false;
}

/**
 * ヘッダ部の出力.
 */
void CPMDData::m_WriteHeader(sxsdk::stream_interface *stream)
{
	char szBuff[300];

	sprintf(szBuff, "Pmd");
	stream->write(3, szBuff);

	float version = 1.0f;
	stream->write(4, &version);

	memset(szBuff, 0, 20);
	std::string str = Util::ConvUTF8ToSJIS(*m_shade, m_modelName);
	if (str.size() > 19) {
		strncpy(szBuff, str.c_str(), 19);
	} else {
		strcpy(szBuff, str.c_str());
	}
	stream->write(20, szBuff);

	memset(szBuff, 0, 256);
	str = Util::ConvUTF8ToSJIS(*m_shade, m_comment);
	if (str.size() > 255) {
		strncpy(szBuff, str.c_str(), 255);
	} else {
		strcpy(szBuff, str.c_str());
	}
	stream->write(256, szBuff);
}

/**
 * 頂点の出力.
 */
void CPMDData::m_WriteVertices(sxsdk::stream_interface *stream)
{
	int verCou = m_vertices.size();
	stream->write(4, &verCou);

	sxsdk::vec3 v;
	unsigned short sVal;
	char cVal;
	for (int i = 0; i < verCou; i++) {
		PMD_VERTEX_DATA& vData = m_vertices[i];
		sxsdk::vec3 v = vData.pos * m_scale;
		v.z = -v.z;
		stream->write(4, &v.x);
		stream->write(4, &v.y);
		stream->write(4, &v.z);

		v = vData.normal;
		v.z = -v.z;
		stream->write(4, &v.x);
		stream->write(4, &v.y);
		stream->write(4, &v.z);

		stream->write(4, &vData.uv.x);
		stream->write(4, &vData.uv.y);

		if (vData.bone_num[0] < 0) sVal = 0;	//0xffff;
		else sVal = (unsigned short)vData.bone_num[0];
		stream->write(2, &sVal);

		if (vData.bone_num[1] < 0) sVal = 0;	//0xffff;
		else sVal = (unsigned short)vData.bone_num[1];
		stream->write(2, &sVal);

		cVal = (char)vData.bone_weight;
		stream->write(1, &cVal);

		cVal = (char)vData.edge_flag;
		stream->write(1, &cVal);
	}
}

/**
 * 面の出力.
 */
void CPMDData::m_WriteFaces(sxsdk::stream_interface *stream)
{
	unsigned  short sVal;
	const int triCou = m_triangles.size();

	const int verCou = triCou * 3;
	stream->write(4, &verCou);

	for (int i = 0; i < triCou; i++) {
		PMD_TRIANGLE_DATA& triData = m_triangles[i];
		for (int j = 0; j < 3; j++) {
			//sVal = (unsigned short)triData.index[j];
			sVal = (unsigned short)triData.index[2 - j];		// -Zの逆転を行っているため、面の順番も入れ替え.
			stream->write(2, &sVal);
		}
	}
}

/**
 * マテリアルの出力.
 */
void CPMDData::m_WriteMaterials(sxsdk::stream_interface *stream)
{
	const int mCou = m_materials.size();
	stream->write(4, &mCou);
	
	char szStr[64];
	char cVal;
	for (int i = 0; i < mCou; i++) {
		PMD_MATERIAL_DATA& mData = m_materials[i];
		stream->write(4, &mData.diffuse_color.x);
		stream->write(4, &mData.diffuse_color.y);
		stream->write(4, &mData.diffuse_color.z);
		stream->write(4, &mData.alpha);
		stream->write(4, &mData.specular);
		stream->write(4, &mData.specular_color.x);
		stream->write(4, &mData.specular_color.y);
		stream->write(4, &mData.specular_color.z);
		stream->write(4, &mData.ambient_color.x);
		stream->write(4, &mData.ambient_color.y);
		stream->write(4, &mData.ambient_color.z);

		cVal = (char)mData.toon_index;
		stream->write(1, &cVal);

		cVal = (char)mData.edge_flag;
		stream->write(1, &cVal);

		stream->write(4, &mData.face_vert_count);

		memset(szStr, 0, 24);
		std::string str = Util::ConvUTF8ToSJIS(*m_shade, mData.tex_file_name);
		if (str.size() < 20) {
			strcpy(szStr, str.c_str());
		}
		stream->write(20, szStr);
	}
}

/**
 * ボーンの出力.
 */
void CPMDData::m_WriteBones(sxsdk::stream_interface *stream)
{
	unsigned short sCou = (unsigned short)m_bones.size();
	stream->write(2, &sCou);

	char szStr[256];
	char cVal;
	unsigned short sVal;
	sxsdk::vec3 v;
	for (int i = 0; i < m_bones.size(); i++) {
		PMD_BONE_DATA& boneData = m_bones[i];

		// ボーン名をMMDの日本語のものに置き換え.
		std::string str = boneData.bone_name;
		if (m_humanConvertBoneName && m_humanRigBonesType != human_rig_type_mmd_jp) {
			const int index = CRigCtrl::GetHumanBoneIndex(m_shade, str, m_humanRigBonesType);
			if (index >= 0) {
				str = CRigCtrl::GetHumanBoneName(m_shade, index, human_rig_type_mmd_jp);
			}
		}
		memset(szStr, 0, 20);
		str = Util::ConvUTF8ToSJIS(*m_shade, str);
		if (str.size() < 20) {
			strcpy(szStr, str.c_str());
		}
		stream->write(20, szStr);

		if (boneData.parent_bone_index < 0) sVal = 0xffff;
		else {
			sVal = (unsigned short)boneData.parent_bone_index;
		}
		stream->write(2, &sVal);

		sVal = (unsigned short)boneData.tail_pos_bone_index;
		stream->write(2, &sVal);

		cVal = (char)boneData.bone_type;
		stream->write(1, &cVal);

		sVal = (unsigned short)boneData.ik_parent_bone_index;
		stream->write(2, &sVal);

		v = boneData.bone_head_pos * m_scale;
		v.z = -v.z;
		stream->write(4, &v.x);
		stream->write(4, &v.y);
		stream->write(4, &v.z);
	}
}

/**
 * IKの出力.
 */
void CPMDData::m_WriteIKs(sxsdk::stream_interface *stream)
{
	int ikCou = m_IKs.size();

	unsigned short sDat = (unsigned short)ikCou;
	stream->write(2, &sDat);

	char cDat;
	for (int i = 0; i < ikCou; i++) {
		PMD_IK_DATA& ikData = m_IKs[i];

		sDat = (unsigned short)ikData.ik_bone_index;
		stream->write(2, &sDat);
		sDat = (unsigned short)ikData.ik_target_bone_index;
		stream->write(2, &sDat);
		cDat = (char)ikData.ik_child_bone_index.size();
		stream->write(1, &cDat);
		sDat = (unsigned short)ikData.iterations;
		stream->write(2, &sDat);
		stream->write(4, &ikData.control_weight);
		for (int j = 0; j < ikData.ik_child_bone_index.size(); j++) {
			sDat = (unsigned short)ikData.ik_child_bone_index[j];
			stream->write(2, &sDat);
		}
	}

}

/**
 * Skin(表情)の出力.
 */
void CPMDData::m_WriteSkins(sxsdk::stream_interface *stream)
{
	m_pFacialSkin->ExportSkinData(stream);
}

/**
 * 表情枠情報の出力.
 */
void CPMDData::m_WriteSkinWaku(sxsdk::stream_interface *stream)
{
	m_pFacialSkin->ExportSkinFrameData(stream);
}

/**
 * ボーン枠情報の出力.
 */
void CPMDData::m_WriteBoneWaku(sxsdk::stream_interface *stream)
{
	int bdCou = m_bonesDisp.size();
	if (bdCou > 255) bdCou = 255;
	unsigned char cVal = (unsigned char)bdCou;

	stream->write(1, &cVal);
	if (bdCou > 0) {
		char szStr[256];
		for (int i = 0; i < bdCou; i++) {
			std::string str = Util::ConvUTF8ToSJIS(*m_shade, m_bonesDisp[i].disp_name);
			if (str.length() > 48) str = str.substr(0, 48);
			const int len = str.length();
			memcpy(szStr, str.c_str(), len);
			szStr[len + 0] = 0x0a;
			szStr[len + 1] = 0;
			stream->write(50, szStr);
		}
	}
}

/**
 * ボーン枠用の表示リストの出力.
 */
void CPMDData::m_WriteBoneList(sxsdk::stream_interface *stream)
{
	int bdCou = m_bonesDisp.size();
	if (bdCou > 255) bdCou = 255;

	int iCou = 0;
	for (int i = 0; i < bdCou; i++) {
		iCou += m_bonesDisp[i].data.size();
	}
	stream->write(4, &iCou);

	if (iCou > 0) {
		short sVal;
		unsigned char cVal; 
		for (int i = 0; i < bdCou; i++) {
			PMD_BONE_DISP_DATA& dispData = m_bonesDisp[i];
			const int cou = dispData.data.size();
			for (int j = 0; j < cou; j++) {
				sVal = (short)dispData.data[j].bone_index;
				cVal = (unsigned char)dispData.data[j].bone_disp_index + 1;
				stream->write(2, &sVal);
				stream->write(1, &cVal);
			}
		}
	}
}


/**
 * 英語情報の出力.
 */
void CPMDData::m_WriteExEnglishInfo(sxsdk::stream_interface *stream)
{
	m_WriteEnglishHeader(stream);
	m_WriteEnglishBones(stream);
	m_WriteEnglishSkins(stream);
	m_WriteEnglishBoneWaku(stream);
}

/**
 * 英語ヘッダの出力.
 */
void CPMDData::m_WriteEnglishHeader(sxsdk::stream_interface *stream)
{
	char cVal = 1;
	stream->write(1, &cVal);

	char szStr[300];
	memset(szStr, 0, 20);
	std::string str = Util::ConvUTF8ToSJIS(*m_shade, m_modelNameEng);
	if (str.length() < 20) {
		strcpy(szStr, str.c_str());
	}
	stream->write(20, szStr);

	memset(szStr, 0, 256);
	str = Util::ConvUTF8ToSJIS(*m_shade, m_commentEng);
	if (str.size() > 255) {
		strncpy(szStr, str.c_str(), 255);
	} else {
		strcpy(szStr, str.c_str());
	}
	stream->write(256, szStr);
}

/**
 * 英語ボーン名リストの出力.
 */
void CPMDData::m_WriteEnglishBones(sxsdk::stream_interface *stream)
{
	const int bCou = m_bones.size();

	char szStr[40];
	for (int i = 0; i < bCou; i++) {
		PMD_BONE_DATA& boneData = m_bones[i];
		std::string boneName = boneData.bone_name;

		if (m_humanConvertBoneName && m_humanRigBonesType != human_rig_type_mmd_en) {
			const int index = CRigCtrl::GetHumanBoneIndex(m_shade, boneName, m_humanRigBonesType);
			if (index >= 0) {
				boneName = CRigCtrl::GetHumanBoneName(m_shade, index, human_rig_type_mmd_en);
			}
		}
		if (boneData.bone_type == bone_type_ik || boneData.bone_type == bone_type_hide || boneData.bone_type == bone_type_ik_c) {
			for (int j = 0; j < 10; j++) {
				std::string ikName = Util::GetUTF8Text(*m_shade, leg_ik_name_jp[j]);
				if (ikName.length() == 0) break;
				if (ikName.compare(boneName) == 0) {
					boneName = leg_ik_name_en[j];
					break;
				}
			}
		}

		memset(szStr, 0, 20);
		std::string str = Util::ConvUTF8ToSJIS(*m_shade, boneName);
		if (str.size() < 20) {
			strcpy(szStr, str.c_str());
		}
		stream->write(20, szStr);
	}
}

/**
 * 英語表情名リストの出力.
 */
void CPMDData::m_WriteEnglishSkins(sxsdk::stream_interface *stream)
{
	m_pFacialSkin->ExportEnglishSkinName(stream);
}

/**
 * 英語ボーン枠情報の出力.
 */
void CPMDData::m_WriteEnglishBoneWaku(sxsdk::stream_interface *stream)
{
	int bdCou = m_bonesDisp.size();
	if (bdCou > 255) bdCou = 255;

	if (bdCou > 0) {
		char szStr[256];
		for (int i = 0; i < bdCou; i++) {
			std::string str = m_bonesDisp[i].disp_name_en;
			if (str.length() > 48) str = str.substr(0, 48);
			const int len = str.length();
			memcpy(szStr, str.c_str(), len);
			szStr[len + 0] = 0x0a;
			szStr[len + 1] = 0;
			stream->write(50, szStr);
		}
	}
}

/**
 * トゥーンテクスチャリストの出力.
 */
void CPMDData::m_WriteToonTextureList(sxsdk::stream_interface *stream)
{
	// 数は10個固定.
	const int tCou = 10;

	char szStr[256];
	memset(szStr, 0, 120);
	for (int i = 0; i < tCou; i++) {
		stream->write(100, szStr);
	}
}


/**
 * 物理演算用の剛体リストを出力.
 */
void CPMDData::m_WritePhysicsRigidbodyList(sxsdk::stream_interface *stream)
{
	int rCou = 0;
	stream->write(4, &rCou);
}

/**
 * 物理演算用のジョイントリストを出力.
 */
void CPMDData::m_WritePhysicsJointList(sxsdk::stream_interface *stream)
{
	int jointCou = 0;

	stream->write(4, &jointCou);
}

