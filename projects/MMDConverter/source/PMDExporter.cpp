/**
 *  @brief  MMDのPMD形式の出力.
 *  @date   2014.08.03 - 2014.08.18
 */

#include "PMDExporter.h"
#include "ShapeStack.h"
#include "StreamCtrl.h"
#include "Util.h"

enum {
	dlg_scale_id = 101,						// scale.
	dlg_bone_move_root_only_id = 201,		// ルート以外は移動しない.
	dlg_toon_edge_id = 301,					// トゥーンのエッジの有効化.
	dlg_human_conv_bones_name_id = 401,		// 人体ボーンの名称をMMD向けに変更.
	dlg_human_auto_ik = 402,				// IKの自動割り当て.

	dlg_note_japanese_txt_id = 501,			// 「日本語」.
	dlg_note_japanese_area_id = 502,		// 「日本語」のテキスト入力.
	dlg_note_english_txt_id = 503,			// 「英語」.
	dlg_note_english_area_id = 504,			// 「英語」のテキスト入力.
};

CPMDExporter::CPMDExporter(sxsdk::shade_interface &shade) : shade(shade)
{
	m_pCurrentShape = NULL;
	m_pShapeStack   = NULL;
	m_pmdData       = NULL;
}

CPMDExporter::~CPMDExporter()
{
	if (m_pmdData) delete m_pmdData;
	if (m_pShapeStack) delete m_pShapeStack;
}

/**
 * ファイル拡張子.
 */
const char *CPMDExporter::get_file_extension (void *aux)
{
	return "pmd";
}

/**
 * ファイルの説明文.
 */
const char *CPMDExporter::get_file_description (void *aux)
{
	return "MikuMikuDance Model";
}

/**
 * エクスポート処理を行う.
 */
void CPMDExporter::do_export (sxsdk::plugin_exporter_interface *plugin_exporter, void *)
{
	compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());

	//------------------------------------------------------//
	//	ボーンの割り当てられたポリゴンメッシュを選択		//
	//------------------------------------------------------//
	// シーン上のメッシュとボーンの組み合わせを取得.
	std::vector<sxsdk::shape_class *> meshShapeList;
	std::vector<sxsdk::shape_class *> boneShapeList;
	if (Util::GetBoneMeshsList(scene, meshShapeList, boneShapeList) == 0) {
		shade.show_message_box(shade.gettext("msg_select_polygonmesh"), false);
		return;
	}

	// ボーンが割り当てられたポリゴンメッシュを取得.
	sxsdk::shape_class* targetShape = &(scene->active_shape());
	bool chkF = false;
	for (int i = 0; i < meshShapeList.size(); i++) {
		if (targetShape == meshShapeList[i]) {
			chkF = true;
			break;
		}
	}
	if (!chkF) {
		// ボーンが選択されている場合は、それに関連付けられたメッシュを選択.
		for (int i = 0; i < boneShapeList.size(); i++) {
			if (targetShape == boneShapeList[i]) {
				chkF = true;
				targetShape = meshShapeList[i];
				break;
			}
		}
	}
	if (!chkF) {
		if (meshShapeList.size() == 1) {
			targetShape = meshShapeList[0];
		}
	}

	//------------------------------------------------------//
	//	条件に合うかチェック								//
	//------------------------------------------------------//
	{
		if (targetShape->get_type() != sxsdk::enums::polygon_mesh) {
			shade.show_message_box(shade.gettext("msg_select_polygonmesh"), false);
			return;
		}
		const int skin_type = targetShape->get_skin_type();
		if (skin_type != 1) {		// 頂点ブレンドのスキンでない場合はスキップ.
			shade.show_message_box(shade.gettext("msg_skin_vertex_blend"), false);
			return;
		}

		sxsdk::polygon_mesh_class& pmesh = targetShape->get_polygon_mesh();
		if (pmesh.get_number_of_faces() > 65535) {
			shade.show_message_box(shade.gettext("msg_mesh_triangle_65535"), false);
			return;
		}
		if (pmesh.get_total_number_of_control_points() > 65535) {
			shade.show_message_box(shade.gettext("msg_mesh_vertex_65535"), false);
			return;
		}
	}

	//------------------------------------------------------//
	//	初期化処理											//
	//------------------------------------------------------//
	// スタッククラスを生成.
	if (m_pShapeStack) delete m_pShapeStack;
	m_pShapeStack = new CShapeStack();

	try {
		m_pluginExporter = plugin_exporter;
		m_pluginExporter->AddRef();

		m_stream = m_pluginExporter->get_stream_interface();
	} catch(...) { }

	if (m_pmdData) delete m_pmdData;
	m_pmdData = new CPMDData(&shade);

	m_localToWorldMat = sxsdk::mat4::identity;
	m_CurrentMat      = sxsdk::mat4::identity;
	m_spMat           = sxsdk::mat4::identity;
	m_SkipF = false;

	//------------------------------------------------------//
	//	エクスポート処理									//
	//------------------------------------------------------//
	// エクスポートの開始.
	//m_pluginExporter->do_export();

	try {
		if (m_pmdData->SetModel(*targetShape, m_stream, m_dlgData)) {
			// PMD形式で出力.
			m_pmdData->Export(m_stream, m_dlgData);

			{
				const std::string fileName = Util::GetFileNameToStream(m_stream);
				std::string str = fileName + std::string(" ") + shade.gettext("msg_finish_export");
				shade.message(str.c_str());
			}
		}
		delete m_pmdData;
		m_pmdData = NULL;
	} catch (...) { }

	//------------------------------------------------------//
	//	破棄処理											//
	//------------------------------------------------------//
	// スタッククラスを破棄
	if (m_pShapeStack) delete m_pShapeStack;
	m_pShapeStack = NULL;

	// ダイアログのstream情報を保存.
	StreamCtrl::SavePMDDlgInfo(&shade, m_dlgData);
}

/********************************************************************/
/* エクスポートのコールバックとして呼ばれる							*/
/********************************************************************/

/**
 * エクスポートの開始.
 */
void CPMDExporter::start (void *)
{
	m_CurrentDepth = 0;
}

/**
 * エクスポートの終了.
 */
void CPMDExporter::finish (void *)
{
	// PMD形式で出力.
	m_pmdData->Export(m_stream, m_dlgData);
}

/**
 * カレント形状の処理の開始.
 */
void CPMDExporter::begin (void *)
{
	sxsdk::mat4 cMat;

	m_pCurrentShape = NULL;
	m_SkipF         = false;

	try {
		// カレントの形状管理クラスのポインタを取得.
		m_pCurrentShape = m_pluginExporter->get_current_shape();

		// 変換行列を累積する（スタックに蓄える）.
		cMat = m_pCurrentShape->get_transformation();
		m_pShapeStack->Push(m_CurrentDepth, m_pCurrentShape, cMat);

		m_spMat = sxsdk::mat4::identity;

		// 変換行列をスタックより取得
		m_localToWorldMat = m_pShapeStack->GetLocalToWorldMatrix();

		m_CurrentDepth++;

	} catch(...) { }
}

/**
 * カレント形状の処理の終了.
 */
void CPMDExporter::end (void *)
{
	sxsdk::shape_class *pShape;
	sxsdk::mat4 lwMat;
	int objIndex, depth;

	m_pShapeStack->GetShape(&pShape, &lwMat, &objIndex, &depth);

	// 変換行列を戻す
	m_pShapeStack->Pop();

	m_SkipF          = false;
	m_pCurrentShape  = NULL;
	m_CurrentDepth   = depth;
}

/**
 * カレント形状が掃引体の上面部分の場合、掃引に相当する変換マトリクスが渡される.
 */
void CPMDExporter::set_transformation (const sxsdk::mat4 &t, void *)
{
	m_spMat = t;
}

/**
 * カレント形状が掃引体の上面部分の場合の行列クリア.
 */
void CPMDExporter::clear_transformation (void *)
{
	m_spMat = sxsdk::mat4::identity;
}

/**
 * ポリゴンメッシュの開始時に呼ばれる.
 */
void CPMDExporter::begin_polymesh (void *)
{
	if(!m_pCurrentShape) return;

	
}

/**
 * ポリゴンメッシュの頂点情報格納時に呼ばれる.
 */
void CPMDExporter::begin_polymesh_vertex (int n, void *)
{
	if (!m_pCurrentShape) return;
}

/**
 * 頂点が格納されるときに呼ばれる.
 */
void CPMDExporter::polymesh_vertex (int i, const sxsdk::vec3 &v, const sxsdk::skin_class *skin)
{
}

/**
 * ポリゴンメッシュの面情報が格納されるときに呼ばれる（Shade12の追加機能）.
 */
void CPMDExporter::polymesh_face_uvs (int n_list, const int list[], const sxsdk::vec3 *normals, const sxsdk::vec4 *plane_equation, const int n_uvs, const sxsdk::vec2 *uvs, void *)
{
	if (!m_pCurrentShape) return;

	if (n_list > 4) return;
}

/**
 * ポリゴンメッシュの終了時に呼ばれる.
 */
void CPMDExporter::end_polymesh (void *)
{
	if (!m_pCurrentShape) return;
}

/**
 * 面情報格納前に呼ばれる.
 */
void CPMDExporter::begin_polymesh_face2 (int n, int number_of_face_groups, void *)
{
}

/**
 * フェイスグループごとの面列挙前に呼ばれる.
 */
void CPMDExporter::begin_polymesh_face_group (int face_group_index, void *)
{
}

/**
 * フェイスグループごとの面列挙後に呼ばれる.
 */
void CPMDExporter::end_polymesh_face_group (void *)
{
}

/****************************************************************/
/* ダイアログイベント											*/
/****************************************************************/
void CPMDExporter::initialize_dialog (sxsdk::dialog_interface& dialog, void* aux)
{
	// ダイアログの情報を読み込み.
	m_dlgData = StreamCtrl::LoadPMDDlgInfo(&shade);
}

void CPMDExporter::load_dialog_data (sxsdk::dialog_interface &d,void *)
{
	sxsdk::dialog_item_class* item;

	item = &(d.get_dialog_item(dlg_scale_id));
	item->set_float(m_dlgData.scale);

	item = &(d.get_dialog_item(dlg_bone_move_root_only_id));
	item->set_bool(m_dlgData.boneOffsetMoveRootOnly);

	item = &(d.get_dialog_item(dlg_toon_edge_id));
	item->set_bool(m_dlgData.toonEdge);

	item = &(d.get_dialog_item(dlg_human_conv_bones_name_id));
	item->set_bool(m_dlgData.humanConvertBoneName);

	item = &(d.get_dialog_item(dlg_human_auto_ik));
	item->set_bool(m_dlgData.humanAutoIK);

	item = &(d.get_dialog_item(dlg_note_japanese_txt_id));
	item->set_text("");
	item = &(d.get_dialog_item(dlg_note_english_txt_id));
	item->set_text("");

	item = &(d.get_dialog_item(dlg_note_japanese_area_id));
	item->set_text(m_dlgData.note_jp.c_str());
	item = &(d.get_dialog_item(dlg_note_english_area_id));
	item->set_text(m_dlgData.note_en.c_str());
}

void CPMDExporter::save_dialog_data (sxsdk::dialog_interface &dialog,void *)
{
}

bool CPMDExporter::respond (sxsdk::dialog_interface &dialog, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();		// アクションがあったダイアログアイテムのID.

	if (id == dlg_scale_id) {
		m_dlgData.scale = item.get_float();
		return true;
	}

	if (id == dlg_bone_move_root_only_id) {
		m_dlgData.boneOffsetMoveRootOnly = item.get_bool();
		return true;
	}

	if (id == dlg_toon_edge_id) {
		m_dlgData.toonEdge = item.get_bool();
		return true;
	}

	if (id == dlg_human_conv_bones_name_id) {
		m_dlgData.humanConvertBoneName = item.get_bool();
		return true;
	}

	if (id == dlg_human_auto_ik) {
		m_dlgData.humanAutoIK = item.get_bool();
		return true;
	}

	if (id == dlg_note_japanese_area_id) {
		m_dlgData.note_jp = item.get_text();
		return true;
	}

	if (id == dlg_note_english_area_id) {
		m_dlgData.note_en = item.get_text();
		return true;
	}

	return false;
}

