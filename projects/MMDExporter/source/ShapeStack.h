/**
 * @file   ShapeStack.h
 * @brief  エクスポート時のシーン構築用のスタック.
 * @author Yutaka Yoshisaka
 * @date   2009.06.03 - 2012.03.31
 */

#ifndef _SHAPESTACK_H
#define _SHAPESTACK_H

#include "GlobalHeader.h"

typedef struct {
	int depth;						///< 階層の深さ.
	sxsdk::shape_class *pShape;		///< 形状へのポインタ.
	sxsdk::mat4 tMat;				///< 形状の変換行列.
	int objectIndex;				///< 格納するオブジェクト番号.
} SHAPE_STACK_NODE;

/**
 * 形状および変換行列のスタック管理クラス.
 */
class CShapeStack
{
private:
	int m_MaxShapeCount;	///< 形状の最大数.
	int m_ShapeCount;		///< 形状の数.

	SHAPE_STACK_NODE *m_pStackNode;

	sxsdk::mat4 m_LWMat;	///< ルートからの累積変換行列.

public:
	CShapeStack ();
	~CShapeStack ();

	/**
	 * 情報のクリア.
	 */
	void Clear ();

	/**
	 * 情報のプッシュ.
	 * @param[in] depth        階層の深さ.
	 * @param[in] pShape       形状情報のポインタ.
	 * @param[in] tMat         変換行列.
	 * @param[in] objectIndex  オブジェクト番号.
	 */
	void Push (const int depth, sxsdk::shape_class *pShape, sxsdk::mat4 tMat, const int objectIndex = -1);

	/**
	 * 情報のポップ.
	 */
	void Pop ();

	/**
	 * 現在のカレント情報を取得.
	 */
	int GetShape (sxsdk::shape_class **ppRetShape, sxsdk::mat4 *pRetMat, int *pRetObjectIndex = NULL,
		int *pRetDepth = NULL);

	/**
	 * 現在の変換行列を取得.
	 */
	sxsdk::mat4 GetLocalToWorldMatrix () { return m_LWMat; }

	/**
	 * スタックの要素数の取得.
	 */
	inline int GetElementCount () { return m_ShapeCount; }

	/**
	 * 要素へのポインタを取得.
	 */
	inline SHAPE_STACK_NODE *GetShapeStackPointer () { return m_pStackNode; }
};

#endif
