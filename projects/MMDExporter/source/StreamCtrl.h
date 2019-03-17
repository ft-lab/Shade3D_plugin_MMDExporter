/**
 *  @brief   stream保存、読み込み用.
 */

#ifndef _STREAMCTRL_H
#define _STREAMCTRL_H

#include "GlobalHeader.h"

namespace StreamCtrl {
	/**
	 * PMDエクスポートダイアログの情報を取得.
	 */
	CPMDDlgInfo LoadPMDDlgInfo(sxsdk::shade_interface* shade);

	/**
	 * PMDエクスポートダイアログの情報を保持.
	 */
	void SavePMDDlgInfo(sxsdk::shade_interface* shade, const CPMDDlgInfo& data);

	/**
	 * VMDエクスポートダイアログの情報を取得.
	 */
	CVMDDlgInfo LoadVMDDlgInfo(sxsdk::shade_interface* shade);

	/**
	 * VMDエクスポートダイアログの情報を保持.
	 */
	void SaveVMDDlgInfo(sxsdk::shade_interface* shade, const CVMDDlgInfo& data);

}

#endif
