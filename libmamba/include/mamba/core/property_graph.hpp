// Copyright (c) 2019, QuantStack and Mamba Contributors
//
// Distributed under the terms of the BSD 3-Clause License.
//
// The full license is in the file LICENSE, distributed with this software.

#ifndef MAMBA_PROPERTY_GRAPH_HPP
#define MAMBA_PROPERTY_GRAPH_HPP

#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <iostream>

#include "mamba/core/union_find.hpp"

namespace mamba
{
    // TODO move this to actual graph_util
    template <class T, class U>
    class MPropertyGraph
    {
    public:
        using node = T;
        using node_id = size_t;
        using node_list = std::vector<node>;
        using list = std::vector<node_id>;
        using edge_info = U;
        // they need to be sorted, otherwise we might not merge all
        // TODO change this afterwards
        using neighs = std::set<node_id>;
        using edge_list = std::vector<std::pair<node_id, edge_info>>;
        using adjacency_list = std::vector<edge_list>;
        using rev_adjacency_list = std::vector<neighs>;
        using cycle_list = std::vector<node_list>;
        using node_path = std::unordered_map<node_id, edge_list>;

        const node_list& get_node_list() const;
        const node& get_node(node_id id) const;
        const adjacency_list& get_adj_list() const;
        const edge_list& get_edge_list(node_id id) const;
        const neighs& get_rev_edge_list(node_id id) const;

        node_id add_node(const node& value);
        node_id add_node(node&& value);
        void add_edge(node_id from, node_id to, edge_info info);

        template <class V>
        void update_node(node_id id, V&& value);

        template <class Y>
        bool update_edge_if_present(node_id from, node_id to, Y&& info);

        list get_roots() const;
        node_path get_paths_from(node_id id) const;

        node_path get_parents_to_leaves() const;

    private:
        template <class V>
        node_id add_node_impl(V&& value);
        edge_list get_leaves(const std::pair<node_id, edge_info>& edge) const;

        node_list m_node_list;
        adjacency_list m_adjacency_list;
        rev_adjacency_list m_rev_adjacency_list;
        std::vector<int> m_levels;
    };

    /************************
     * graph implementation *
     ************************/

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_adj_list() const -> const adjacency_list&
    {
        return m_adjacency_list;
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_node_list() const -> const node_list&
    {
        return m_node_list;
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_node(node_id id) const -> const node&
    {
        return m_node_list[id];
    }


    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_edge_list(node_id id) const -> const edge_list&
    {
        return m_adjacency_list[id];
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_rev_edge_list(node_id id) const -> const neighs&
    {
        return m_rev_adjacency_list[id];
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_roots() const -> list
    {
        list roots;
        for (node_id i = 0; i < m_levels.size(); ++i)
        {
            if (m_levels[i] == 0)
            {
                roots.push_back(i);
            }
        }
        return roots;
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::add_node(const node& value) -> node_id
    {
        return add_node_impl(value);
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::add_node(node&& value) -> node_id
    {
        return add_node_impl(std::move(value));
    }

    template <class T, class U>
    inline void MPropertyGraph<T, U>::add_edge(node_id from, node_id to, edge_info info)
    {
        m_adjacency_list[from].push_back(std::make_pair(to, info));
        if (m_rev_adjacency_list.size() <= to)
        {
            m_rev_adjacency_list.resize(to + 1);
        }
        m_rev_adjacency_list[to].insert(from);
        // std::cerr << "rev " << to << " " << from << std::endl;
        ++m_levels[to];
    }


    template <class T, class U>
    template <class V>
    inline auto MPropertyGraph<T, U>::add_node_impl(V&& value) -> node_id
    {
        // std::cerr << "Adding new node with " << value << std::endl;
        m_node_list.push_back(std::forward<V>(value));
        m_adjacency_list.push_back(edge_list());
        m_levels.push_back(0);
        return m_node_list.size() - 1u;
    }


    template <class T, class U>
    template <class V>
    inline void MPropertyGraph<T, U>::update_node(node_id id, V&& value)
    {
        // std::cerr << "Updating node " << id << " with " << value << std::endl;
        m_node_list[id].add(value);
    }

    template <class T, class U>
    template <class Y>
    inline bool MPropertyGraph<T, U>::update_edge_if_present(node_id from, node_id to, Y&& value)
    {
        std::vector<std::pair<node_id, edge_info>>& edge_list = m_adjacency_list[from];
        for (auto it = edge_list.begin(); it != edge_list.end(); ++it)
        {
            if (it->first == to)
            {
                // std::cerr << "Updating " << from << "to " << it->first << " with " << value <<
                // std::endl;
                it->second.add(value);
                return true;
            }
        }
        return false;
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_paths_from(node_id id) const -> node_path
    {
        node_path paths_to_leaves;
        edge_list edges = get_edge_list(id);
        for (const auto& edge : edges)
        {
            std::cout << edge.first << " " << edge.second << std::endl;
            edge_list leaves = get_leaves(edge);
            // TODO: remove this -> we should not add it here
            paths_to_leaves[edge.first].push_back(edge);
            // TODO maybe we go to the same end result => unordered_set
            paths_to_leaves[edge.first].insert(
                paths_to_leaves[edge.first].end(), leaves.begin(), leaves.end());
        }
        return paths_to_leaves;
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_leaves(
        const std::pair<node_id, edge_info>& node_edge) const -> edge_list
    {
        node_id id = node_edge.first;
        edge_list edges = get_edge_list(id);
        edge_list path;
        if (edges.size() == 0)
        {
            path.push_back(node_edge);
            return path;
        }

        for (const auto& edge : edges)
        {
            edge_list leaves = get_leaves(edge);
            path.insert(path.end(), leaves.begin(), leaves.end());
        }
        return path;
    }

    template <class T, class U>
    inline auto MPropertyGraph<T, U>::get_parents_to_leaves() const -> node_path
    {
        // TODO - sanity check should only be a root
        std::vector<std::pair<node_id, edge_info>> roots;
        for (node_id i = 0; i < m_levels.size(); ++i)
        {
            if (m_levels[i] == 0)
            {
                for (const auto& edge : get_edge_list(i))
                {
                    roots.push_back(edge);
                }
            }
        }

        node_path roots_to_leaves;
        for (const auto& root : roots)
        {
            roots_to_leaves[root.first].push_back(root);
            edge_list edges = get_leaves(root);
            roots_to_leaves[root.first].insert(
                roots_to_leaves[root.first].end(), edges.begin(), edges.end());
        }
        return roots_to_leaves;
    }


}  // namespace mamba

#endif  // MAMBA_PROPERTY_GRAPH_HPP
