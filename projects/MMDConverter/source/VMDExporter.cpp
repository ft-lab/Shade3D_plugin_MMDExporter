/**
 *  @brief  MMDのVMD形式の出力.
 *  @date   2014.08.11 - 2014.08.12
 */

#include "VMDExporter.h"
#include "VMDData.h"
#include "StreamCtrl.h"
#include "Util.h"

enum {
	dlg_scale_id = 101,						// scale.
	dlg_human_conv_bones_name_id = 201,		// 人体ボーンの名称をMMD向けに変更.
};

CVMDExporter::CVMDExporter(sxsdk::shade_interface &shade) : shade(shade)
{
}

CVMDExporter::~CVMDExporter()
{
}

/**
 * ファイル拡張子.
 */
const char *CVMDExporter::get_file_extension (void *aux)
{
	return "vmd";
}

/**
 * ファイルの説明文.
 */
const char *CVMDExporter::get_file_description (void *aux)
{
	return "MikuMikuDance Motion";
}

/**
 * エクスポート処理を行う.
 */
void CVMDExporter::do_export (sxsdk::plugin_exporter_interface *plugin_exporter, void *)
{
	try {
		m_pluginExporter = plugin_exporter;
		m_pluginExporter->AddRef();

		m_stream = m_pluginExporter->get_stream_interface();
	} catch(...) { }

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
	//	VMD保存.
	//------------------------------------------------------//
	try {
		// 指定のポリゴンメッシュに割り当てられたボーンより、モーションデータを出力.
		CVMDData vmdData(&shade);
		if (vmdData.SetMotion(scene, *targetShape, m_dlgData)) {
			vmdData.Export(m_stream);

			{
				const std::string fileName = Util::GetFileNameToStream(m_stream);
				std::string str = fileName + std::string(" ") + shade.gettext("msg_finish_export");
				shade.message(str.c_str());
			}
		}
	} catch (...) { }

	// ダイアログのstream情報を保存.
	StreamCtrl::SaveVMDDlgInfo(&shade, m_dlgData);
}

/****************************************************************/
/* ダイアログイベント											*/
/****************************************************************/
void CVMDExporter::initialize_dialog (sxsdk::dialog_interface& dialog, void *)
{
	// ダイアログの情報を読み込み.
	m_dlgData = StreamCtrl::LoadVMDDlgInfo(&shade);
}

void CVMDExporter::load_dialog_data (sxsdk::dialog_interface &d,void *)
{
	sxsdk::dialog_item_class* item;

	item = &(d.get_dialog_item(dlg_scale_id));
	item->set_float(m_dlgData.scale);

	item = &(d.get_dialog_item(dlg_human_conv_bones_name_id));
	item->set_bool(m_dlgData.humanConvertBoneName);
}

void CVMDExporter::save_dialog_data (sxsdk::dialog_interface &dialog,void *)
{
}

bool CVMDExporter::respond (sxsdk::dialog_interface &dialog, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();		// アクションがあったダイアログアイテムのID.

	if (id == dlg_scale_id) {
		m_dlgData.scale = item.get_float();
		return true;
	}

	if (id == dlg_human_conv_bones_name_id) {
		m_dlgData.humanConvertBoneName = item.get_bool();
		return true;
	}

	return false;
}

