/**
 * @file   ShapeStack.h
 * @brief  �G�N�X�|�[�g���̃V�[���\�z�p�̃X�^�b�N.
 * @author Yutaka Yoshisaka
 * @date   2009.06.03 - 2012.03.31
 */

#ifndef _SHAPESTACK_H
#define _SHAPESTACK_H

#include "GlobalHeader.h"

typedef struct {
	int depth;						///< �K�w�̐[��.
	sxsdk::shape_class *pShape;		///< �`��ւ̃|�C���^.
	sxsdk::mat4 tMat;				///< �`��̕ϊ��s��.
	int objectIndex;				///< �i�[����I�u�W�F�N�g�ԍ�.
} SHAPE_STACK_NODE;

/**
 * �`�󂨂�ѕϊ��s��̃X�^�b�N�Ǘ��N���X.
 */
class CShapeStack
{
private:
	int m_MaxShapeCount;	///< �`��̍ő吔.
	int m_ShapeCount;		///< �`��̐�.

	SHAPE_STACK_NODE *m_pStackNode;

	sxsdk::mat4 m_LWMat;	///< ���[�g����̗ݐϕϊ��s��.

public:
	CShapeStack ();
	~CShapeStack ();

	/**
	 * ���̃N���A.
	 */
	void Clear ();

	/**
	 * ���̃v�b�V��.
	 * @param[in] depth        �K�w�̐[��.
	 * @param[in] pShape       �`����̃|�C���^.
	 * @param[in] tMat         �ϊ��s��.
	 * @param[in] objectIndex  �I�u�W�F�N�g�ԍ�.
	 */
	void Push (const int depth, sxsdk::shape_class *pShape, sxsdk::mat4 tMat, const int objectIndex = -1);

	/**
	 * ���̃|�b�v.
	 */
	void Pop ();

	/**
	 * ���݂̃J�����g�����擾.
	 */
	int GetShape (sxsdk::shape_class **ppRetShape, sxsdk::mat4 *pRetMat, int *pRetObjectIndex = NULL,
		int *pRetDepth = NULL);

	/**
	 * ���݂̕ϊ��s����擾.
	 */
	sxsdk::mat4 GetLocalToWorldMatrix () { return m_LWMat; }

	/**
	 * �X�^�b�N�̗v�f���̎擾.
	 */
	inline int GetElementCount () { return m_ShapeCount; }

	/**
	 * �v�f�ւ̃|�C���^���擾.
	 */
	inline SHAPE_STACK_NODE *GetShapeStackPointer () { return m_pStackNode; }
};

#endif
