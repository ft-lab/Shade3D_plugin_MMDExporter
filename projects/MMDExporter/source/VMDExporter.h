/**
 *  @brief  MMDのVMD形式の出力.
 *  @date   2014.08.11 - 2014.08.11
 */

#ifndef _VMDEXPORTER_H
#define _VMDEXPORTER_H

#include "GlobalHeader.h"
#include "VMDData.h"

class CVMDExporter : public sxsdk::exporter_interface {
private:
	sxsdk::shade_interface& shade;

	compointer<sxsdk::plugin_exporter_interface> m_pluginExporter;
	compointer<sxsdk::stream_interface> m_stream;
	sxsdk::shape_class *m_pCurrentShape;		///< カレントの形状クラスのポインタ.

	CVMDDlgInfo m_dlgData;						///< Exportダイアログの情報.

	virtual sx::uuid_class get_uuid (void *) { return MMD_VMD_EXPORTER_INTERFACE_ID; }
	virtual int get_shade_version () const { return SHADE_BUILD_NUMBER; }

	/**
	 * ファイル拡張子
	 */
	virtual const char *get_file_extension (void *aux = 0);

	/**
	 * ファイルの説明文
	 */
	virtual const char *get_file_description (void *aux = 0);

	/**
	 * エクスポート処理を行う
	 */
	virtual void do_export (sxsdk::plugin_exporter_interface *plugin_exporter, void *);

	/**
	 * 開いた線形状を受け付けるかどうか
	 */
	virtual bool can_accept_polyline (void *) { return false; }

	/**
	 * 閉じた線形状を受け付けるかどうか
	 */
	virtual bool can_accept_polygon (void *) { return false; }

	/**
	 * 球を受け付けるかどうか
	 */
	virtual bool can_accept_sphere (void *) { return false; }

	/**
	 * 自由曲面を受け付けるかどうか
	 */
	virtual bool can_accept_bezier_surface (void *) { return false; }

	/**
	 * ポリゴンメッシュを受け付けるかどうか
	 */
	virtual bool can_accept_polymesh (void *) { return true; }

	/**
	 * ダイアログ表示のスキップするかどうか.
	 */
	virtual bool skips_dialog (void *) { return false; }

	/**
	 * リソースに埋め込むSXULを指定.
	 */
	virtual const char *get_include_resource_name (const int index, void * aux = 0) {
		return "vmd_exporter_dlg";
	}

	/**
	 * 受け付けることのできるポリゴンメッシュ面の頂点の最大数
	 */
	virtual int get_max_vertices_per_face (void *) { return 3; }

	/**
	 * ポリゴンメッシュの面はすべて三角形分割
	 */
	virtual bool must_triangulate_polymesh (void *) { return true; }

	/**
	 * メッシュを受け付けるかどうか
	 */
	virtual bool can_accept_meshes (void *aux=0) { return false; }

	/**
	 * ポリゴンメッシュの面を分割するか
	 */
	virtual bool must_divide_polymesh (void *aux=0) { return true; }

	/**
	 * バイナリで出力
	 */
	virtual bool can_export_binary (void * = 0) { return true; }
	virtual bool can_export_text (void * = 0) { return false; }

	virtual bool cannot_select_eol (void *aux=0) { return true; }
	virtual bool can_select_filter_objects (void *aux=0) { return false; }

	/****************************************************************/
	/* ダイアログイベント											*/
	/****************************************************************/
	virtual void initialize_dialog (sxsdk::dialog_interface& dialog, void* aux = 0);
	virtual void load_dialog_data (sxsdk::dialog_interface &d,void * = 0);
	virtual void save_dialog_data (sxsdk::dialog_interface &dialog,void * = 0);
	virtual bool respond (sxsdk::dialog_interface &dialog, sxsdk::dialog_item_class &item, int action, void *);

public:
	CVMDExporter(sxsdk::shade_interface &shade);
	~CVMDExporter();

	/**
	 * プラグイン名
	 */
	static const char *name (sxsdk::shade_interface *shade) { return shade->gettext("vmd_exporter_title"); }

};

#endif
