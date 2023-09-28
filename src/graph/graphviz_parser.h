#ifndef GRAPH_GRAPHVIZ_PARSER
#define GRAPH_GRAPHVIZ_PARSER

#include <graphviz/cgraph.h>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <utility>
#include "../entities/arrow.h"
#include "../utils/transforms.h"

namespace atk {
    struct GraphVizModel {

        std::map<std::string, std::map<std::string, std::string>> nodes;
        std::map<std::pair<std::string, std::string>, std::map<std::string, std::string>> edges;

        static GraphVizModel read_from_file(const std::string& path) {
            std::ifstream graph_file(path);
            std::stringstream buffer;
            buffer << graph_file.rdbuf();
            graph_file.close();

            return GraphVizModel(buffer.str());
        }

        explicit GraphVizModel(const std::string& contents) {
            Agraph_t  *g;
            if (!(g = agmemread(contents.c_str()))) {
                throw std::runtime_error("Failed to read graph file.");
            }

            std::map<Agnode_t*, std::string> node_ptr_to_label;

            // iterate over the nodes
            for (Agnode_t* n = agfstnode(g); n; n = agnxtnode(g, n)) {
                node_ptr_to_label[n] = agnameof(n);
                this->nodes[node_ptr_to_label[n]] = parse_node_attributes(g, n);
            }

            for (Agnode_t* n = agfstnode(g); n; n = agnxtnode(g, n)) {

                std::string in_label = node_ptr_to_label[n];

                Agedge_t* e = nullptr;
                for (e = agfstout(g,n); e; e = agnxtout(g,e)) {

                    std::string out_label = node_ptr_to_label[e->node];

                    auto edge = std::make_pair(out_label, in_label);

                    this->edges[edge] = parse_edge_attributes(g, e);
                }
            }

            agclose(g);
        }

    private:
        static std::map<std::string, std::string> parse_node_attributes(Agraph_t* g, Agnode_t* n) {
            std::map<std::string, std::string> values;

            Agsym_t* sym = nullptr;
            while ((sym = agnxtattr(g, AGNODE, sym))) {
                Agsym_t* node_sym = agattrsym(n, sym->name);
                if (node_sym) {
                    values[sym->name] = agget(n, sym->name);
                }
            }

            return values;
        }

        static std::map<std::string, std::string> parse_edge_attributes(Agraph_t* g, Agedge_t* e) {
            std::map<std::string, std::string> values;

            Agsym_t* sym = nullptr;
            while ((sym = agnxtattr(g, AGEDGE, sym))) {
                Agsym_t* node_sym = agattrsym(e, sym->name);
                if (node_sym) {
                    values[sym->name] = agget(e, sym->name);
                }
            }

            return values;
        }
    };

    class GraphSceneNodeFactory {
    private:
        std::shared_ptr<ShaderCache> shader_cache;

    public:

        GraphSceneNodeFactory(std::shared_ptr<ShaderCache>  shader_cache) : shader_cache(std::move(shader_cache)) {

        }

    private:

        [[nodiscard]] std::pair<float, float> parse_coord_string(const std::string& coord_string) {
            std::vector<std::string> strs;
            boost::split(strs, coord_string, boost::is_any_of(","));

            float x = strtof(strs[0].c_str(), nullptr);
            float y = strtof(strs[1].c_str(), nullptr);

            return std::make_pair(x, y);
        }

        std::shared_ptr<SceneNode> create_for_node(const std::string& name, const GraphVizModel& model) {
            auto position = parse_coord_string(model.nodes.at(name).at("pos"));

            auto result = std::make_shared<SceneNode>(std::make_unique<Dot>(3, shader_cache));
            result->get_drawable_as<Dot>()->set_fill_color(constants::color::SolarizedDark::base3);
            TransformUtils::set_translation_part(result->transform(), position.first, position.second);
            return result;
        }

        static std::shared_ptr<SceneNode> create_for_edge(const std::string& from,
                                                   const std::string& to,
                                                   const GraphVizModel& model,
                                                   std::shared_ptr<SceneNode>& nodes) {

            return std::make_shared<SceneNode>(std::make_unique<Arrow>(
                    nodes,
                    nodes->get(from),
                    nodes->get(to)));
        }

    public:

        std::shared_ptr<SceneNode> from_model(const GraphVizModel& model) {
            auto result = std::make_shared<SceneNode>();
            auto nodes = result->add("nodes");
            auto edges = result->add("edges");
            for (const auto& name_to_props : model.nodes) {
                auto name = name_to_props.first;
                nodes->add(name, create_for_node(name, model));
            }

            for (const auto& edge_to_props : model.edges) {
                auto edge = edge_to_props.first;
                std::stringstream ss;
                ss << edge.first << "->" << edge.second;
                edges->add(ss.str(), create_for_edge(edge.first, edge.second, model, nodes));
            }

            return result;
        }
    };
}

#endif