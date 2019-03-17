/**
 *   @brief  表情パターンの管理クラス.
 *   @date   2014.08.13 - 2014.08.13.
 */

#include "FacialSkin.h"
#include "Util.h"

namespace {
	// 表情名の変換一覧.
	std::string skinNameConvList[][2] = {
		{"まばたき"   , "blink"},
		{"はぅ"       , "close><"},
		{"笑い"       , "smile"},
		{"ウィンク"   , "wink"},

		{"真面目"     , "serious"},
		{"困る"       , "sadness"},
		{"にこり"     , "cheerful"},
		{"怒り"       , "anger"},
		{"上"         , "go up"},
		{"下"         , "go down"},

		{"あ"         , "a"},
		{"い"         , "i"},
		{"う"         , "u"},
		{"お"         , "o"},
		{"にやり"     , "grin"},

		{""           , ""}
	};
}

CFacialSkin::CFacialSkin(sxsdk::shade_interface* shade)
{
	m_shade = shade;

	m_skinTypeName.push_back("base");
	m_skinTypeName.push_back("eyebrow");
	m_skinTypeName.push_back("eye");
	m_skinTypeName.push_back("mouth");
	m_skinTypeName.push_back("other");
	m_pBSPSearch = NULL;
	m_scale = 0.01f;
}

CFacialSkin::~CFacialSkin()
{
	Clear();
}

/**
 * クリア.
 */
void CFacialSkin::Clear()
{
	m_faceSkinData.clear();
	m_skinGroupIndex.clear();
	if (m_pBSPSearch) delete m_pBSPSearch;
	m_pBSPSearch = NULL;
}

/**
 * skinのパートを探す.
 */
sxsdk::shape_class* CFacialSkin::m_GetSkinPart(sxsdk::scene_interface* scene)
{
	sxsdk::shape_class& rootShape = scene->get_shape();
	if (!rootShape.has_son()) return NULL;

	sxsdk::shape_class* pRetSkinShape = NULL;
	sxsdk::shape_class* pShape = rootShape.get_son();
	while (pShape->has_bro()) {
		pShape = pShape->get_bro();
		if (strcmp(pShape->get_name(), "skin") == 0) {
			pRetSkinShape = pShape;
			break;
		}
	}

	return pRetSkinShape;
}

/**
 * 指定のSkinTypeのパートを探す.
 */
sxsdk::shape_class* CFacialSkin::m_GetSkinTypePart(sxsdk::shape_class* pSkinPart, const int skinType)
{
	if (!(pSkinPart->has_son())) return NULL;
	if (skinType < 0 || skinType >= m_skinTypeName.size()) return NULL;

	sxsdk::shape_class* pRetShape = NULL;
	sxsdk::shape_class* pShape = pSkinPart->get_son();
	while (pShape->has_bro()) {
		pShape = pShape->get_bro();
		if (strcmp(pShape->get_name(), m_skinTypeName[skinType].c_str()) == 0) {
			pRetShape = pShape;
			break;
		}
	}

	return pRetShape;
}

/**
 * 指定の形状のメッシュ情報を格納.
 */
bool CFacialSkin::m_AddMesh(sxsdk::shape_class* pShape, const int skinType, FACE_SKIN_DATA& skinData, const bool firstF)
{
	if (pShape->get_type() != sxsdk::enums::polygon_mesh) return false;

	sxsdk::polygon_mesh_class& pmesh = pShape->get_polygon_mesh();
	const int verCou = pmesh.get_total_number_of_control_points();
	if (verCou <= 0) return false;

	skinData.name   = pShape->get_name();
	skinData.pShape = pShape;
	skinData.type   = skinType;
	skinData.v_data.clear();
	skinData.baseSkin = firstF;

	const sxsdk::mat4 lwMat = pShape->get_local_to_world_matrix();

	FACE_SKIN_VERTEX_DATA vData;
	for (int i = 0; i < verCou; i++) {
		sxsdk::vertex_class& v = pmesh.vertex(i);
		vData.pos = v.get_position() * lwMat;

		if (firstF) {
			// オリジナルの頂点番号を取得.
			vData.vert_index = m_GetNearVertex(vData.pos);
		}

		skinData.v_data.push_back(vData);
	}

	return true;
}

/**
 * 現在のシーンから、face skin用のメッシュ情報を取得.
 */
bool CFacialSkin::StoreSkinData(sxsdk::scene_interface* scene, sxsdk::shape_class *pTargetMesh, const float scale)
{
	Clear();
	m_scale = scale;

	// [skin]のパートを取得.
	sxsdk::shape_class* skinPart = m_GetSkinPart(scene);
	if (!skinPart) return false;

	// BSPの作成.
	if (!m_SetMeshVertices(pTargetMesh)) return false;

	// 各skin用パートを取得し、その中のポリゴンメッシュを格納.
	for (int loop = 1; loop < m_skinTypeName.size(); loop++) {
		sxsdk::shape_class* pTypePart = m_GetSkinTypePart(skinPart, loop);
		if (!pTypePart || !(pTypePart->has_son())) continue;

		const int skinIndex = m_faceSkinData.size();
		bool firstF = true;
		FACE_SKIN_DATA skinData;
		sxsdk::shape_class *pShape = pTypePart->get_son();
		while (pShape->has_bro()) {
			pShape = pShape->get_bro();
			if (m_AddMesh(pShape, loop, skinData, firstF)) {
				if (!firstF) {		// m_faceSkinData[skinIndex] での頂点数と同じ数である必要がある.
					if (m_faceSkinData[skinIndex].v_data.size() != skinData.v_data.size()) continue;
				}
				m_faceSkinData.push_back(skinData);
			}
			firstF = false;
		}
		if (!firstF) {
			m_skinGroupIndex.push_back(skinIndex);
		}
	}

	return true;
}

/**
 * 指定のポリゴンメッシュの頂点を取得し、BSPの空間に格納.
 */
bool CFacialSkin::m_SetMeshVertices(sxsdk::shape_class* pShape)
{
	if (pShape->get_type() != sxsdk::enums::polygon_mesh) return false;

	if (m_pBSPSearch) delete m_pBSPSearch;
	m_pBSPSearch = NULL;

	std::vector<sxsdk::vec3> vers;

	sxsdk::polygon_mesh_class& pmesh = pShape->get_polygon_mesh();
	const int verCou = pmesh.get_total_number_of_control_points();
	if (verCou <= 0) return false;
	const sxsdk::mat4 lwMat = pShape->get_local_to_world_matrix();

	vers.resize(verCou);
	for (int i = 0; i < verCou; i++) {
		vers[i] = pmesh.vertex(i).get_position() * lwMat;
	}

	// BSPの作成.
	m_pBSPSearch = new CBSPSearch(vers);
	m_pBSPSearch->build();

	return true;
}

/**
 * 指定の頂点の一番近くにある頂点インデックスを取得.
 */
int CFacialSkin::m_GetNearVertex(sxsdk::vec3& pos, const float dist)
{
	if (!m_pBSPSearch) return -1;

	std::vector<int> indices;
	if (m_pBSPSearch->search_vertices(pos, dist, indices) == 0) return -1;
	if (indices.size() == 1) return indices[0];

	int minIndex  = indices[0];
	float minDist = sxsdk::absolute((m_pBSPSearch->get_vertex(minIndex)) - pos);
	for (int i = 1; i < indices.size(); i++) {
		const int index  = indices[i];
		const float dist = sxsdk::absolute((m_pBSPSearch->get_vertex(index)) - pos);
		if (minDist > dist) {
			minDist  = dist;
			minIndex = index;
		}
	}

	return minIndex;
}

/**
 * 頂点の最適化の反映（法線/UVの違いで頂点が増える場合）.
 */
void CFacialSkin::UpdateVertices(std::vector< std::vector<int> >& sameVertexList)
{
	if (m_faceSkinData.size() == 0) return;

	// baseの頂点を増加.
	for (int loop = 0; loop < m_faceSkinData.size(); loop++) {
		FACE_SKIN_DATA& skinData = m_faceSkinData[loop];
		if (!skinData.baseSkin) continue;
		
		const int vCou = skinData.v_data.size();
		FACE_SKIN_VERTEX_DATA vData2;
		for (int i = 0; i < vCou; i++) {
			FACE_SKIN_VERTEX_DATA& vData = skinData.v_data[i];
			std::vector<int>& vSameList  = sameVertexList[vData.vert_index];

			if (vSameList.size() > 0) {
				for (int j = 0; j < vSameList.size(); j++) {
					vData2.pos            = vData.pos;
					vData2.vert_index     = vSameList[j];
					vData2.org_i          = i;
					skinData.v_data.push_back(vData2);
				}
			}
		}
	}

	// base以外の頂点を増加.
	int baseIndex = -1;
	int curSkinType = -1;
	for (int loop = 0; loop < m_faceSkinData.size(); loop++) {
		FACE_SKIN_DATA& skinData = m_faceSkinData[loop];
		if (skinData.baseSkin) {
			curSkinType = skinData.type;
			baseIndex   = loop;
			continue;
		}
		FACE_SKIN_DATA& baseSkinData = m_faceSkinData[baseIndex];
		if (baseSkinData.v_data.size() == skinData.v_data.size()) continue;

		FACE_SKIN_VERTEX_DATA vData2;
		const int stPos = skinData.v_data.size();
		for (int i = stPos; i < baseSkinData.v_data.size(); i++) {
			FACE_SKIN_VERTEX_DATA& vData = baseSkinData.v_data[i];
			vData2.pos = skinData.v_data[vData.org_i].pos;
			skinData.v_data.push_back(vData2);
		}
	}
}

/**
 * 指定の英語名を日本語に変換できる場合に変換.
 */
std::string CFacialSkin::m_ConvSkinName_EngToJP(const std::string skinName)
{
	std::string retSkinName = skinName;

	for (int i = 0; i < 100; i++) {
		if (skinNameConvList[i][0].length() == 0) break;
		if (skinName.compare(skinNameConvList[i][1]) == 0) {
			retSkinName = Util::GetUTF8Text(*m_shade, skinNameConvList[i][0]);
			break;
		}
	}

	return retSkinName;
}


/**
 * 表情データをエクスポート.
 */
bool CFacialSkin::ExportSkinData(sxsdk::stream_interface *stream)
{
	int curSkinType       = -1;
	int skinTypeBaseIndex = -1;

	int sCou = 0;
	for (int i = 0; i < m_faceSkinData.size(); i++) {
		FACE_SKIN_DATA& skinData = m_faceSkinData[i];
		if (skinData.baseSkin) continue;
		sCou++;
	}
	sCou++;		// baseの分を追加.

	unsigned short sVal = (unsigned short)sCou;
	stream->write(2, &sVal);

	//-------------------------------------------------------.
	//	基準となるbase用の頂点をまとめる.
	//-------------------------------------------------------.
	char cVal;
	int iVal;
	char szName[64];
	sxsdk::vec3 v;
	std::vector<FACE_SKIN_VERTEX_DATA> baseVersList;
	std::vector<int> skinVOffset;
	for (int i = 0; i < m_skinGroupIndex.size(); i++) {
		const int sIndex = m_skinGroupIndex[i];
		FACE_SKIN_DATA& skinData = m_faceSkinData[sIndex];

		skinVOffset.push_back(baseVersList.size());
		for (int j = 0; j < skinData.v_data.size(); j++) {
			baseVersList.push_back(skinData.v_data[j]);
		}
	}
	{
		std::string str = "base";
		memset(szName, 0, 24);
		strcpy(szName, str.c_str());
		stream->write(20, szName);

		iVal = baseVersList.size();
		stream->write(4, &iVal);

		cVal = skin_type_base;
		stream->write(1, &cVal);

		for (int j = 0; j < baseVersList.size(); j++) {
			FACE_SKIN_VERTEX_DATA& vData = baseVersList[j];
			v = vData.pos * m_scale;
			v.z = -v.z;
			stream->write(4, &vData.vert_index);
			stream->write(4, &v.x);
			stream->write(4, &v.y);
			stream->write(4, &v.z);
		}
	}

	//-------------------------------------------------------.
	// base以外の表情データを出力.
	//-------------------------------------------------------.
	int offsetIPos = -1;
	for (int i = 0; i < m_faceSkinData.size(); i++) {
		FACE_SKIN_DATA& skinData = m_faceSkinData[i];
		if (curSkinType != skinData.type) {
			curSkinType       = skinData.type;
			skinTypeBaseIndex = i;
		}
		FACE_SKIN_DATA& baseSkinData = m_faceSkinData[skinTypeBaseIndex];
		if (skinData.baseSkin) {
			offsetIPos++;
			continue;
		}
		const int offsetI = (offsetIPos >= 0) ? skinVOffset[offsetIPos] : 0;

		std::string str = Util::ConvUTF8ToSJIS(*m_shade, m_ConvSkinName_EngToJP(skinData.name));
		memset(szName, 0, 24);
		if (str.length() < 20) {
			strcpy(szName, str.c_str());
		} else {
			strncpy(szName, str.c_str(), 19);
		}
		stream->write(20, szName);

		iVal = skinData.v_data.size();
		stream->write(4, &iVal);

		cVal = (char)skinData.type;
		stream->write(1, &cVal);

		for (int j = 0; j < skinData.v_data.size(); j++) {
			FACE_SKIN_VERTEX_DATA& vDataBase = baseSkinData.v_data[j];
			FACE_SKIN_VERTEX_DATA& vData     = skinData.v_data[j];

			v = (vData.pos - vDataBase.pos) * m_scale;
			v.z = -v.z;
			iVal = j + offsetI;
			stream->write(4, &iVal);
			stream->write(4, &v.x);
			stream->write(4, &v.y);
			stream->write(4, &v.z);
		}
	}

	return true;
}


/**
 * 表情枠データをエクスポート.
 */
bool CFacialSkin::ExportSkinFrameData(sxsdk::stream_interface *stream)
{
	// baseを除く表情数を取得.
	int sCou = 0;
	for (int i = 0; i < m_faceSkinData.size(); i++) {
		FACE_SKIN_DATA& skinData = m_faceSkinData[i];
		if (skinData.baseSkin) continue;
		sCou++;
	}

	unsigned char cVal = (int)sCou;
	stream->write(1, &cVal);
	unsigned short sVal;

	int skinPos = 0;
	for (int i = 0; i < m_faceSkinData.size(); i++) {
		FACE_SKIN_DATA& skinData = m_faceSkinData[i];
		if (skinData.baseSkin) continue;

		sVal = (unsigned short)(skinPos + 1);		// baseは除いたインデックス.
		stream->write(2, &sVal);

		skinPos++;
	}

	return true;
}

/**
 * 英語の表情名をエクスポート.
 */
bool CFacialSkin::ExportEnglishSkinName(sxsdk::stream_interface *stream)
{
	char szName[40];
	for (int i = 0; i < m_faceSkinData.size(); i++) {
		FACE_SKIN_DATA& skinData = m_faceSkinData[i];
		if (skinData.baseSkin) continue;

		std::string str = Util::ConvUTF8ToSJIS(*m_shade, skinData.name);
		memset(szName, 0, 24);
		if (str.length() < 20) {
			strcpy(szName, str.c_str());
		} else {
			strncpy(szName, str.c_str(), 19);
		}
		stream->write(20, szName);
	}

	return true;
}
