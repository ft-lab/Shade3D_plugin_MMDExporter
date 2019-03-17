/**
 *  @brief   stream保存、読み込み用.
 */

#include "StreamCtrl.h"

/**
 * PMDエクスポートダイアログの情報を取得.
 */
CPMDDlgInfo StreamCtrl::LoadPMDDlgInfo(sxsdk::shade_interface* shade)
{
	CPMDDlgInfo data;

	try {
		compointer<sxsdk::scene_interface> scene(shade->get_scene_interface());
		if (!scene) return data;
		compointer<sxsdk::stream_interface> stream(scene->get_attribute_stream_interface_with_uuid(MMD_PMD_DLG_ID));
		if (!stream) return data;

		stream->set_pointer(0);

		int iDat = 0;
		stream->read_int(iDat);
		if (iDat != MMD_PMD_DLG_VERSION) return data;

		stream->read_float(data.scale);

		stream->read_int(iDat);
		data.boneOffsetMoveRootOnly = iDat ? true : false;

		stream->read_int(iDat);
		data.toonEdge = iDat ? true : false;

		stream->read_int(iDat);
		data.humanConvertBoneName = iDat ? true : false;

		stream->read_int(iDat);
		data.humanAutoIK = iDat ? true : false;

		char szStr[260];
		stream->read(256, szStr);
		data.note_jp = szStr;
		stream->read(256, szStr);
		data.note_en = szStr;
		
	} catch (...) { }

	return data;
}

/**
 * PMDエクスポートダイアログの情報を保持.
 */
void StreamCtrl::SavePMDDlgInfo(sxsdk::shade_interface* shade, const CPMDDlgInfo& data)
{
	try {
		compointer<sxsdk::scene_interface> scene(shade->get_scene_interface());
		if (!scene) return;
		compointer<sxsdk::stream_interface> stream(scene->create_attribute_stream_interface_with_uuid(MMD_PMD_DLG_ID));
		if (!stream) return;

		stream->set_pointer(0);
		stream->set_size(0);

		int iDat = MMD_PMD_DLG_VERSION;
		stream->write_int(iDat);

		stream->write_float(data.scale);

		iDat = data.boneOffsetMoveRootOnly ? 1 : 0;
		stream->write_int(iDat);

		iDat = data.toonEdge ? 1 : 0;
		stream->write_int(iDat);

		iDat = data.humanConvertBoneName ? 1 : 0;
		stream->write_int(iDat);

		iDat = data.humanAutoIK ? 1 : 0;
		stream->write_int(iDat);

		char szStr[260];

		memset(szStr, 0, 256);
		if (data.note_jp.length() < 255) {
			strcpy(szStr, data.note_jp.c_str());
		}
		stream->write(256, szStr);

		memset(szStr, 0, 256);
		if (data.note_en.length() < 255) {
			strcpy(szStr, data.note_en.c_str());
		}
		stream->write(256, szStr);

	} catch (...) { }
}

/**
 * VMDエクスポートダイアログの情報を取得.
 */
CVMDDlgInfo StreamCtrl::LoadVMDDlgInfo(sxsdk::shade_interface* shade)
{
	CVMDDlgInfo data;

	try {
		compointer<sxsdk::scene_interface> scene(shade->get_scene_interface());
		if (!scene) return data;
		compointer<sxsdk::stream_interface> stream(scene->get_attribute_stream_interface_with_uuid(MMD_VMD_DLG_ID));
		if (!stream) return data;

		stream->set_pointer(0);

		int iDat = 0;
		stream->read_int(iDat);
		if (iDat != MMD_VMD_DLG_VERSION) return data;

		stream->read_float(data.scale);

		stream->read_int(iDat);
		data.humanConvertBoneName = iDat ? true : false;
	} catch (...) { }

	return data;
}

/**
 * VMDエクスポートダイアログの情報を保持.
 */
void StreamCtrl::SaveVMDDlgInfo(sxsdk::shade_interface* shade, const CVMDDlgInfo& data)
{
	try {
		compointer<sxsdk::scene_interface> scene(shade->get_scene_interface());
		if (!scene) return;
		compointer<sxsdk::stream_interface> stream(scene->create_attribute_stream_interface_with_uuid(MMD_VMD_DLG_ID));
		if (!stream) return;

		stream->set_pointer(0);
		stream->set_size(0);

		int iDat = MMD_VMD_DLG_VERSION;
		stream->write_int(iDat);

		stream->write_float(data.scale);

		iDat = data.humanConvertBoneName ? 1 : 0;
		stream->write_int(iDat);
	} catch (...) { }
}

