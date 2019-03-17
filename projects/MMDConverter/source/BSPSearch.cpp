/**
 *  @file   BSPSearch.cpp
 *  @brief  近接頂点を検索する.
 */

#include "BSPSearch.h"

#include <stdio.h>
#include <stdlib.h>

CBSPSearch::CBSPSearch (const std::vector<sxsdk::vec3> &vertices)
{
	m_bbMin = m_bbMax = sxsdk::vec3(0.0f, 0.0f, 0.0f);
	m_vertices.clear();
	try {
		const int vCou = vertices.size();
		m_vertices.resize(vCou);

		for (int i = 0; i < vCou; ++i) {
			const sxsdk::vec3 &v = vertices[i];
			m_vertices[i] = v;
			if (i == 0) {
				m_bbMin = m_bbMax = v;
			} else {
				if (m_bbMin.x > v.x) m_bbMin.x = v.x;
				if (m_bbMin.y > v.y) m_bbMin.y = v.y;
				if (m_bbMin.z > v.z) m_bbMin.z = v.z;
				if (m_bbMax.x < v.x) m_bbMax.x = v.x;
				if (m_bbMax.y < v.y) m_bbMax.y = v.y;
				if (m_bbMax.z < v.z) m_bbMax.z = v.z;
			}
		}
	} catch (...) { }

	set_buildSetting(16, 20);
}

CBSPSearch::~CBSPSearch ()
{
	m_clear();
}

void CBSPSearch::m_clear ()
{
	for (int i = 0; i < m_nodes.size(); ++i) {
		if (m_nodes[i].v_index) free(m_nodes[i].v_index);
	}
	m_nodes.clear();
}

int CBSPSearch::m_select_axis (const sxsdk::vec3 &bbMin, const sxsdk::vec3 &bbMax)
{
	const float mx = bbMax.x - bbMin.x;
	const float my = bbMax.y - bbMin.y;
	const float mz = bbMax.z - bbMin.z;
	int axis = -1;
	if (mx < my) {
		if (my < mz) axis = 2;
		else axis = 1;
	} else {
		if (mx < mz) axis = 2;
		else axis = 0;
	}
	return axis;
}

/**
 * 空間分割.
 */
void CBSPSearch::build ()
{
	if (m_vertices.size() == 0) return;

	m_clear();

	BSP_NODE node_root;
	m_nodes.push_back(node_root);
	BSP_NODE &node = m_nodes.back();
	node.bbMin = m_bbMin;
	node.bbMax = m_bbMax;
	const int v_size = m_vertices.size();
	node.v_index = (int *)malloc(sizeof(int) * v_size);
	for (int i = 0; i < v_size; ++i) node.v_index[i] = i;
	node.v_index_size = v_size;
	node.parent_node = -1;
	m_temp_index.resize(v_size);
	
	m_build(0, 0);
	m_temp_index.clear();
}

void CBSPSearch::m_build (const int depth, const int index)
{
	m_nodes[index].left_node  = -1;
	m_nodes[index].right_node = -1;
	if (depth >= m_maxDepth || m_nodes[index].v_index_size <= m_minVertices) return;

	m_nodes.resize(m_nodes.size() + 2);
	const int index_left  = m_nodes.size() - 2;
	const int index_right = m_nodes.size() - 1;
	BSP_NODE &node       = m_nodes[index];
	BSP_NODE &node_left  = m_nodes[index_left];
	BSP_NODE &node_right = m_nodes[index_right];
	const int v_size = node.v_index_size;
	node.axis = m_select_axis(node.bbMin, node.bbMax);

	node.left_node  = index_left;
	node.right_node = index_right;
	node_left.bbMin = node_right.bbMin = node.bbMin;
	node_left.bbMax = node_right.bbMax = node.bbMax;
	node_left.v_index  = NULL;
	node_right.v_index = NULL;
	node_left.v_index_size  = 0;
	node_right.v_index_size = 0;
	node_left.parent_node  = index;
	node_right.parent_node = index;

	int left_cou = 0;
	switch (node.axis) {
	case 0:
		node.median = (node.bbMin.x + node.bbMax.x) * 0.5f;
		node_left.bbMax.x  = node.median;
		node_right.bbMin.x = node.median;
		for (int i = 0; i < v_size; ++i) {
			const sxsdk::vec3 &v = m_vertices[ node.v_index[i] ];
			m_temp_index[i] = 1;
			if (v.x < node.median) {
				left_cou++;
				m_temp_index[i] = 0;
			}
		}
		break;
	case 1:
		node.median = (node.bbMin.y + node.bbMax.y) * 0.5f;
		node_left.bbMax.y  = node.median;
		node_right.bbMin.y = node.median;

		for (int i = 0; i < v_size; ++i) {
			const sxsdk::vec3 &v = m_vertices[ node.v_index[i] ];
			m_temp_index[i] = 1;
			if (v.y < node.median) {
				left_cou++;
				m_temp_index[i] = 0;
			}
		}
		break;
	case 2:
		node.median = (node.bbMin.z + node.bbMax.z) * 0.5f;
		node_left.bbMax.z  = node.median;
		node_right.bbMin.z = node.median;
		for (int i = 0; i < v_size; ++i) {
			const sxsdk::vec3 &v = m_vertices[ node.v_index[i] ];
			m_temp_index[i] = 1;
			if (v.z < node.median) {
				left_cou++;
				m_temp_index[i] = 0;
			}
		}
		break;
	}

	if (left_cou > 0) {
		int iPos = 0;
		node_left.v_index = (int *)malloc(sizeof(int) * left_cou);
		node_left.v_index_size = left_cou;
		for (int i = 0; i < v_size; ++i) {
			if (!m_temp_index[i]) node_left.v_index[iPos++] = node.v_index[i];
		}
	}
	if (v_size - left_cou > 0) {
		int iPos = 0;
		node_right.v_index = (int *)malloc(sizeof(int) * (v_size - left_cou));
		node_right.v_index_size = v_size - left_cou;
		for (int i = 0; i < v_size; ++i) {
			if (m_temp_index[i]) node_right.v_index[iPos++] = node.v_index[i];
		}
	}
	free(node.v_index);
	node.v_index = NULL;

	m_build(depth + 1, index_left);
	m_build(depth + 1, index_right);
}

/**
 * 指定の頂点位置に近接する頂点を検索.
 */
int CBSPSearch::search_vertices (const sxsdk::vec3 &v, const float distance, std::vector<int> &indices)
{
	// 末端のノードを検索.
	const int leaf_node = m_search_leaf_node(v);
	if (leaf_node < 0) return 0;
	indices.clear();

	// 検索範囲を計算.
	sxsdk::vec3 searchBBMin, searchBBMax;
	searchBBMin = searchBBMax = v;
	searchBBMin.x -= distance;
	searchBBMin.y -= distance;
	searchBBMin.z -= distance;
	searchBBMax.x += distance;
	searchBBMax.y += distance;
	searchBBMax.z += distance;
	{
		BSP_NODE &node = m_nodes[0];
		if (searchBBMin.x < node.bbMin.x) searchBBMin.x = node.bbMin.x;
		if (searchBBMin.y < node.bbMin.y) searchBBMin.y = node.bbMin.y;
		if (searchBBMin.z < node.bbMin.z) searchBBMin.z = node.bbMin.z;
		if (searchBBMax.x > node.bbMax.x) searchBBMax.x = node.bbMax.x;
		if (searchBBMax.y > node.bbMax.y) searchBBMax.y = node.bbMax.y;
		if (searchBBMax.z > node.bbMax.z) searchBBMax.z = node.bbMax.z;
	}

	// searchBBMin - searchBBMaxが完全に内包するか調べる.
	int current_node = leaf_node;
	while (1) {
		const BSP_NODE &node = m_nodes[current_node];
		if (node.bbMin.x <= searchBBMin.x && node.bbMin.y <= searchBBMin.y && node.bbMin.z <= searchBBMin.z && 
			node.bbMax.x >= searchBBMax.x && node.bbMax.y >= searchBBMax.y && node.bbMax.z >= searchBBMax.z) break;

		const int parent_index = m_nodes[current_node].parent_node;
		if (parent_index < 0) break;
		current_node = parent_index;
	}
	
	// 再帰的に近接頂点を探す.
	m_search_vertices_loop(current_node, v, distance, indices);

	return indices.size();
}

void CBSPSearch::m_search_vertices_loop (const int index, const sxsdk::vec3 &v, const float distance, std::vector<int> &indices)
{
	const BSP_NODE &node = m_nodes[index];

	if (node.v_index) {
		for (int i = 0; i < node.v_index_size; ++i) {
			const sxsdk::vec3 &target_v = m_vertices[node.v_index[i]];
			const sxsdk::vec3 dd = target_v - v;
			if (std::abs(dd.x) <= distance &&  std::abs(dd.y) <= distance && std::abs(dd.z) <= distance) indices.push_back(node.v_index[i]);
		}
	}

	if (node.left_node >= 0) m_search_vertices_loop(node.left_node, v, distance, indices);
	if (node.right_node >= 0) m_search_vertices_loop(node.right_node, v, distance, indices);
}

/**
 * 指定の頂点を内包する末端のノードを検索.
 */
int CBSPSearch::m_search_leaf_node (const sxsdk::vec3 &v)
{
	if (m_nodes.size() == 0) return -1;
	{
		BSP_NODE &node = m_nodes[0];
		if (node.bbMin.x > v.x || node.bbMin.y > v.y || node.bbMin.z > v.z || v.x > node.bbMax.x || v.y > node.bbMax.y || v.z > node.bbMax.z) return -1;
	}

	int leaf_node = 0;
	while (1) {
		BSP_NODE &node = m_nodes[leaf_node];
		int next_node = -1;
		if (node.bbMin.x <= v.x && node.bbMin.y <= v.y && node.bbMin.z <= v.z && v.x <= node.bbMax.x && v.y <= node.bbMax.y && v.z <= node.bbMax.z) {
			switch (node.axis) {
			case 0:
				if (v.x < node.median) next_node = node.left_node;
				else next_node = node.right_node;
				break;
			case 1:
				if (v.y < node.median) next_node = node.left_node;
				else next_node = node.right_node;
				break;
			case 2:
				if (v.z < node.median) next_node = node.left_node;
				else next_node = node.right_node;
				break;
			}
		}
		if (next_node < 0) break;
		leaf_node = next_node;
	}
	return leaf_node;
}
