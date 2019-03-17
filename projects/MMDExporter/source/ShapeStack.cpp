/**
 * @file   ShapeStack.cpp
 * @brief  �G�N�X�|�[�g���̃V�[���\�z�p�̃X�^�b�N.
 * @author Yutaka Yoshisaka
 * @date   2009.06.03 - 2012.03.31
 */

#include "ShapeStack.h"

#include <stdio.h>
#include <stdlib.h>

CShapeStack::CShapeStack ()
{
	m_LWMat = sxsdk::mat4::identity;
	m_pStackNode = NULL;
	m_MaxShapeCount = m_ShapeCount = 0;
}

CShapeStack::~CShapeStack ()
{
	if (m_pStackNode) {
		free(m_pStackNode);
		m_pStackNode = NULL;
	}
}

/**
 * ���̃N���A.
 */
void CShapeStack::Clear ()
{
	m_ShapeCount = 0;
	m_LWMat = sxsdk::mat4::identity;
}

/**
 * ���̃v�b�V��.
 * @param[in] depth        �K�w�̐[��.
 * @param[in] pShape       �`����̃|�C���^.
 * @param[in] tMat         �ϊ��s��.
 * @param[in] objectIndex  �I�u�W�F�N�g�ԍ�.
 */
void CShapeStack::Push (const int depth, sxsdk::shape_class *pShape, sxsdk::mat4 tMat, const int objectIndex)
{
	SHAPE_STACK_NODE *pNode;
	int cou;

	//�������̊m�ہE�g��.
	if (!m_pStackNode) {
		cou = 8;
		pNode = (SHAPE_STACK_NODE *)malloc(sizeof(SHAPE_STACK_NODE) * (cou + 1));
		if (!pNode) return;
		m_pStackNode = pNode;
		m_ShapeCount    = 0;
		m_MaxShapeCount = cou;

	} else {
		if (m_ShapeCount + 1 >= m_MaxShapeCount) {
			cou = m_MaxShapeCount + 16;
			pNode = (SHAPE_STACK_NODE *)realloc(m_pStackNode, sizeof(SHAPE_STACK_NODE) * (cou + 1));
			if (!pNode) return;
			m_pStackNode = pNode;
			m_MaxShapeCount = cou;
 		}
	}
	pNode = m_pStackNode + m_ShapeCount;
	pNode->depth       = depth;
	pNode->pShape      = pShape;
	pNode->tMat        = tMat;
	pNode->objectIndex = objectIndex;

	m_LWMat = tMat * m_LWMat;

	m_ShapeCount++;
}

/**
 * ���̃|�b�v.
 */
void CShapeStack::Pop ()
{
	SHAPE_STACK_NODE *pNode;

	if (!m_pStackNode || !m_ShapeCount) return;
	m_ShapeCount--;
	
	pNode = m_pStackNode + m_ShapeCount;
	m_LWMat = inv(pNode->tMat) * m_LWMat;
}

/**
 * ���݂̃J�����g�����擾.
 */
int CShapeStack::GetShape (sxsdk::shape_class **ppRetShape, sxsdk::mat4 *pRetMat, int *pRetObjectIndex, int *pRetDepth)
{
	SHAPE_STACK_NODE *pNode;

	if (ppRetShape) *ppRetShape = NULL;
	if (pRetMat) *pRetMat = sxsdk::mat4::identity;
	if (pRetDepth) *pRetDepth = 0;

	if (!m_pStackNode || !m_ShapeCount) return 0;

	pNode = m_pStackNode + m_ShapeCount - 1;

	if (ppRetShape) *ppRetShape = pNode->pShape;
	if (pRetMat) *pRetMat = pNode->tMat;
	if (pRetObjectIndex) *pRetObjectIndex = pNode->objectIndex;
	if (pRetDepth) *pRetDepth = pNode->depth;

	return 1;
}

