/**
 *  @file   main.cpp
 *  @brief  MMDでのモデルデータ、モーションデータのエクスポート機能.
 *  @author Yutaka Yoshisaka
 *  @date   2014.08.03 - 2014.08.08
 */

/*
	pmd ... モデルデータ.
	pmx ... モデルデータ (pmdを拡張したもの).
	vmd ... モーションデータ.
	vpd ... ポーズデータ.
*/

#include "sxsdk.cxx"
#include "PMDExporter.h"
#include "VMDExporter.h"

//**************************************************//
//	グローバル関数									//
//**************************************************//
/**
 * プラグインインターフェースの生成.
 */
extern "C" void STDCALL create_interface (const IID &iid, int i, void **p, sxsdk::shade_interface *shade, void *) {
	unknown_interface *u = NULL;
	
	if (iid == exporter_iid) {
		if (i == 0) {
			u = new CPMDExporter(*shade);
		}
		if (i == 1) {
			u = new CVMDExporter(*shade);
		}
	}

	if (u) {
		u->AddRef();
		*p = (void *)u;
	}
}

/**
 * インターフェースの数を返す.
 */
extern "C" int STDCALL has_interface (const IID &iid, sxsdk::shade_interface *shade) {
	if (iid == exporter_iid) return 2;

	return 0;
}

/**
 * インターフェース名を返す.
 */
extern "C" const char * STDCALL get_name (const IID &iid, int i, sxsdk::shade_interface *shade, void *) {
	// SXULより、プラグイン名を取得して渡す.
	if (iid == exporter_iid) {
		if(i == 0) {
			return CPMDExporter::name(shade);
		}
		if(i == 1) {
			return CVMDExporter::name(shade);
		}
	}

	return 0;
}

/**
 * プラグインのUUIDを返す.
 */
extern "C" sx::uuid_class STDCALL get_uuid (const IID &iid, int i, void *) {
	if (iid == exporter_iid) {
		if(i == 0) {
			return MMD_PMD_EXPORTER_INTERFACE_ID;
		}
		if(i == 1) {
			return MMD_VMD_EXPORTER_INTERFACE_ID;
		}
	}

	return sx::uuid_class(0, 0, 0, 0);
}


/**
 * バージョン情報.
 */
extern "C" void STDCALL get_info (sxsdk::shade_plugin_info &info, sxsdk::shade_interface *shade, void *) {
	info.sdk_version = SHADE_BUILD_NUMBER;
	info.recommended_shade_version = 410000;
	info.major_version = 1;
	info.minor_version = 0;
	info.micro_version = 0;
	info.build_number =  1;
}

/**
 * 常駐プラグイン.
 */
extern "C" bool STDCALL is_resident (const IID &iid, int i, void *) {
	return false;
}

