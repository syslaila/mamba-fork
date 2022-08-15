// Copyright (c) 2019, QuantStack and Mamba Contributors
//
// Distributed under the terms of the BSD 3-Clause License.
//
// The full license is in the file LICENSE, distributed with this software.


#include "mamba/core/problems_explainer.hpp"

#include <tuple>

namespace mamba
{
    MProblemsExplainer::MProblemsExplainer(const MProblemsExplainer::graph& g,
                                           const MProblemsExplainer::adj_list& adj)
        : m_problems_graph(g)
        , m_conflicts_adj_list(adj)
    {
    }

    std::string MProblemsExplainer::explain()
    {
        node_path path = m_problems_graph.get_parents_to_leaves();

        std::unordered_map<std::string, std::vector<node_edge>> bluf_problems_packages;
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<node_edge>>>
            conflict_to_root_info;

        for (const auto& entry : path)
        {
            MGroupNode root_node = m_problems_graph.get_node(entry.first);
            // currently the vector only contains the root as a first entry & all the leaves
            const auto& root_info = entry.second[0];
            const auto& root_edge_info = root_info.second;
            std::cerr << "Root node" << root_info.first << " " << root_info.second << std::endl;
            for (auto it = entry.second.begin() + 1; it != entry.second.end(); it++)
            {
                MGroupNode conflict_node = m_problems_graph.get_node(it->first);
                auto conflict_name = conflict_node.get_name();
                conflict_to_root_info[conflict_name][join(it->second.m_deps, ", ")].push_back(
                    std::make_pair(root_node, root_edge_info));
                std::cerr << "conflict node" << conflict_node << std::endl;
                bluf_problems_packages[conflict_name].push_back(
                    std::make_pair(conflict_node, root_edge_info));
            }
        }

        std::stringstream sstr;
        for (const auto& entry : bluf_problems_packages)
        {
            auto conflict_name = entry.first;
            auto conflict_node = entry.second[0].first;  // only the first required
            std::vector<node_edge> conflict_to_root_deps = entry.second;
            std::unordered_set<std::string> s;
            for (const auto& conflict_to_dep : conflict_to_root_deps)
            {
                s.insert(conflict_to_dep.second.m_deps.begin(),
                         conflict_to_dep.second.m_deps.end());
            }
            sstr << "Requested packages " << explain(s) << std::endl
                 << "\tcannot be installed because they depend on";
            if (conflict_node.is_conflict())
            {
                sstr << " different versions of " << conflict_name << std::endl;
                std::unordered_set<std::string> conflicts;
                for (const auto& conflict_deps_to_root_info : conflict_to_root_info[conflict_name])
                {
                    for (const auto& root_infos : conflict_deps_to_root_info.second)
                    {
                        conflicts.insert(explain(root_infos, conflict_deps_to_root_info.first));
                    }
                }
                // TODO delimiter new line
                sstr << join(conflicts, "\t\t\n") << std::endl;
            }
            else
            {
                sstr << "\t " << explain_problem(conflict_node) << std::endl;
            }
        }
        return sstr.str();
    }

    std::string MProblemsExplainer::explain_problem(const MGroupNode& node) const
    {
        std::stringstream ss;
        if (!node.m_problem_type)
        {
            // TODO - raise exception - shouldn;t get here
            ss << node << " which is problematic";
            return ss.str();
        }
        std::string package_name = node.get_name();
        switch (node.m_problem_type.value())
        {
            case SOLVER_RULE_JOB_NOTHING_PROVIDES_DEP:
            case SOLVER_RULE_PKG_NOTHING_PROVIDES_DEP:
            case SOLVER_RULE_JOB_UNKNOWN_PACKAGE:
                ss << package_name << " which can't be found in the configured channels";
                break;
            case SOLVER_RULE_BEST:
                ss << package_name << " that can not be installed";
                break;
            case SOLVER_RULE_BLACK:
                ss << package_name << " that can only be installed by a direct request";
                break;
            case SOLVER_RULE_DISTUPGRADE:
                ss << package_name << " that does not belong to a distupgrade repository";
                break;
            case SOLVER_RULE_INFARCH:
                ss << package_name << " that has an inferior architecture";
                break;
            case SOLVER_RULE_UPDATE:
            case SOLVER_RULE_PKG_NOT_INSTALLABLE:
                ss << package_name << " that is disabled/has incompatible arch/is not installable";
                break;
            case SOLVER_RULE_STRICT_REPO_PRIORITY:
                ss << package_name << " that is excluded by strict repo priority";
                break;
            default:
                LOG_WARNING << "Shouldn't be here" << node.m_problem_type.value() << " " << node
                            << std::endl;
                ss << package_name << " which is problematic";
                break;
        }
        ss << std::endl;
        return ss.str();
    }

    std::string MProblemsExplainer::explain(
        const std::unordered_set<std::string>& requested_packages) const
    {
        return join(requested_packages, ",");
    }

    std::string MProblemsExplainer::explain(const node_edge& node_to_edge,
                                            std::string conflict_dep) const
    {
        std::stringstream sstr;
        MGroupNode group_node = node_to_edge.first;
        MGroupEdgeInfo group_node_edge = node_to_edge.second;
        sstr << group_node_edge << " versions: [" << join(group_node.m_pkg_versions, ", ")
             << "] depend on " << conflict_dep;
        return sstr.str();
    }
}
