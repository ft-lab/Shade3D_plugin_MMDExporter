/**
 *  @file   BSPSearch.h
 *  @brief  近接頂点を検索する.
 */

#ifndef _BSPSEARCH_H
#define _BSPSEARCH_H

#include "sxsdk.cxx"

class BSP_NODE {
public:
	sxsdk::vec3 bbMin, bbMax;
	int axis;
	float median;
	int *v_index;
	int v_index_size;
	int left_node;
	int right_node;
	int parent_node;

public:
	BSP_NODE () {
		v_index = NULL;
		v_index_size = 0;
		left_node   = -1;
		right_node  = -1;
		parent_node = -1;
	}
};

class CBSPSearch {
private:
	std::vector<sxsdk::vec3> m_vertices;
	sxsdk::vec3 m_bbMin, m_bbMax;

	std::vector<BSP_NODE> m_nodes;

	int m_maxDepth;			// 再帰する最大の深さ.
	int m_minVertices;		// 検索を打ち切る1ノードでの頂点数.
	std::vector<int> m_temp_index;

	void m_clear ();
	void m_build (const int depth, const int index);

	/**
	 * バウンディングボックスが与えられた場合に、分割軸を求める.
	 */
	int m_select_axis (const sxsdk::vec3 &bbMin, const sxsdk::vec3 &bbMax);

	/**
	 * 指定の頂点を内包する末端のノードを検索.
	 */
	int m_search_leaf_node (const sxsdk::vec3 &v);

	void m_search_vertices_loop (const int index, const sxsdk::vec3 &v, const float distance, std::vector<int> &indices);

public:
	CBSPSearch (const std::vector<sxsdk::vec3> &vertices);
	~CBSPSearch ();

	/**
	 * 頂点数を取得.
	 */
	int get_number_of_vertices () { return m_vertices.size(); }

	/**
	 * ノード数を取得.
	 */
	int get_number_of_nodes () { return m_nodes.size(); }

	/**
	 * 空間分割.
	 */
	void build ();

	/**
	 * バウンディングボックスを取得.
	 */
	void get_boundingbox (sxsdk::vec3 &bbMin, sxsdk::vec3 &bbMax) {
		bbMin = m_bbMin;
		bbMax = m_bbMax;
	}

	/**
	 * 空間分割情報を指定.
	 */
	void set_buildSetting (const int maxDepth, const int minVertices) {
		m_maxDepth    = maxDepth;
		m_minVertices = minVertices;
	}

	/**
	 * 指定の頂点位置に近接する頂点を検索.
	 */
	int search_vertices (const sxsdk::vec3 &v, const float distance, std::vector<int> &indices);

	/**
	 * 頂点の取得.
	 */
	inline sxsdk::vec3 &get_vertex (const int index) { return m_vertices[index]; }
};

#endif
