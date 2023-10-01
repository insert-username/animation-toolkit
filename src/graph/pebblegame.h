#ifndef GRAPH_PEBBLEGAME_H
#define GRAPH_PEBBLEGAME_H

#include <ffnx/drplan/plans/canonical_top_down/PebbleGame2D.h>

#include "./graphviz_parser.h"

#include "../animation/director.h"
#include "../utils/common_manipulations.h"
#include "../utils/color.h"

#include <memory>
#include <string>
#include "../scene_graph.h"

namespace atk {

    /**
     * Responsible for tracking the position of pebble nodes on a scene graph.
     */
    class SceneGraphPebbleGame {
    private:
        template<typename T>
        using sptr = std::shared_ptr<T>;

        using Graph = atk::GraphVizFlowGraphFactory::ffnx_graph;
        using Cluster = ffnx::cluster::Cluster<Graph>;
        using Move = ffnx::pebblegame::PebbleGame2D<Graph>::Move;
        using PG2D = ffnx::pebblegame::PebbleGame2D<Graph>;

        sptr<PG2D> pebblegame;
        sptr<SceneNode> scene_graph;
        sptr<Cluster> game_cluster;

    public:
        SceneGraphPebbleGame(sptr<PG2D> pebblegame,
                             sptr<Cluster> game_cluster,
                             sptr<SceneNode> scene_graph) :
            pebblegame(pebblegame),
            game_cluster(game_cluster),
            scene_graph(scene_graph) {

            init();
        }

        void update(atk::Director& director,
                    atk::Timeline& timeline,
                    const sptr<ShaderCache>& shader_cache,
                    const sptr<PG2D::Move>& move) {

            update_pebbles(director, shader_cache);
            update_edges(director, shader_cache);

            highlight_edges(director, timeline, shader_cache, move);
        }

    private:

        void highlight_edges(atk::Director& director,
                             atk::Timeline& timeline,
                             const sptr<ShaderCache>& shader_cache,
                             const sptr<PG2D::Move>& move) {

            std::set<std::string> dfs_edge_ids;
            for (auto &e : move->dfs_edges) {
                auto e_id = edge_to_scene_node_id(e, false);
                dfs_edge_ids.insert(e_id);
            }

            sf::Color base_color = atk::constants::color::SolarizedDark::base3;
            sf::Color dfs_color = atk::constants::color::SolarizedDark::magenta;


            for (const auto& kv : scene_graph->get("edges")->children()) {
                auto arrow = kv.second->get_drawable_as<Arrow>();

                auto current_color = arrow->get_fill_color();
                auto target_color = dfs_edge_ids.contains(kv.first) ? dfs_color : base_color;

                if (current_color != target_color) {
                    timeline.add(std::make_shared<FireOnceScheduler>(0, 0.5),
                            std::make_shared<InterpolatedAnimation>(
                                    InterpolatedAnimation::ease_in_out_interpolation(),
                                    [target_color, current_color, arrow](float v){
                                        arrow->set_fill_color(atk::ColorUtils::lerp(v, current_color, target_color));
                                    }));
                }
            }
        }

        void update_edges(atk::Director& director, const sptr<ShaderCache>& shader_cache) {
            auto game_edges = pebblegame->get_game_graph().graph().edges();

            // determine if any edges need to be added/destroyed
            for (const auto& e : game_edges) {
                auto external_edge = pebblegame->get_game_graph().external_edge(e);

                hide_inverted_edge_if_present(external_edge, director);
                show_edge_node(external_edge, director);
            }
        }

        void show_edge_node(const Graph::edge_descriptor& e, Director &director) {
            auto id = edge_to_scene_node_id(e, false);

            auto graph = game_cluster->graph().lock();

            auto v0_v1 = graph->vertices_for_edge(e);

            std::string v0 = (*graph)[v0_v1.first];
            std::string v1 = (*graph)[v0_v1.second];

            if (!scene_graph->get("edges")->contains(id)) {
                scene_graph->get("edges")->add(id, std::make_unique<Arrow>(
                        scene_graph->get("edges"),
                        scene_graph->get("nodes")->get(v1),
                        scene_graph->get("nodes")->get(v0)));
                atk::CommonManipulations::set_unbuilt(scene_graph->get("edges")->get(id));
            }

            auto node = scene_graph->get("edges")->get(id);

            if (node->get_drawable_as<Buildable>()->get_build_percent() != 1) {
                director.build(node);
            }
        }

        void hide_inverted_edge_if_present(const Graph::edge_descriptor& e, Director &director) {
            auto id = edge_to_scene_node_id(e, true);

            if (scene_graph->get("edges")->contains(id)) {
                auto node = scene_graph->get("edges")->get(id);
                if (node->get_drawable_as<Buildable>()->get_build_percent() != 0) {
                    director.unbuild(node);
                }
            }
        }

        [[nodiscard]] std::string edge_to_scene_node_id(const Graph::edge_descriptor& e, bool should_invert) const {
            auto graph = game_cluster->graph().lock();

            auto v0_v1 = graph->vertices_for_edge(e);

            std::string v0 = (*graph)[v0_v1.first];
            std::string v1 = (*graph)[v0_v1.second];

            if (should_invert) {
                std::swap(v0, v1);
            }

            return (std::stringstream() << v0 << "->" << v1).str();
        }

        void update_pebbles(atk::Director& director, const sptr<ShaderCache>& shader_cache) {
            // determine the set of nodes that require pebbles to be rearranged
            std::map<std::string, std::vector<std::string>> pebble_arrangements;
            std::set<std::string> pebbles_on_board;

            for (const auto &internal_vertex : pebblegame->get_game_graph().graph().vertices()) {
                auto external_vertex = pebblegame->get_game_graph().external_vert(internal_vertex);

                // determine the scene graph node:
                std::string node_id = (*game_cluster->graph().lock())[external_vertex];

                pebble_arrangements[node_id] = std::vector<std::string>();
                for (const auto &i : pebblegame->get_game_graph().get_vert_pebbles(internal_vertex).pebbles()) {
                    auto pebble_id = std::to_string(i);
                    pebble_arrangements[node_id].push_back(pebble_id);
                    pebbles_on_board.insert(pebble_id);
                }
            }

            // determine which pebbles have been removed from the board
            for (const auto &kv : scene_graph->get("pebbles")->children()) {
                if (!pebbles_on_board.contains(kv.first) && scene_graph->get("pebbles")->get(kv.first)->get_drawable_as<Buildable>()->get_build_percent() != 0) {
                    director.unbuild(scene_graph->get("pebbles")->get(kv.first), Sequencer{ 0, 0.5, 0.5 });
                }
            }

            for (const auto &kv : pebble_arrangements) {
                auto target_node = scene_graph->get("nodes")->get(kv.first);

                std::vector<sptr<atk::SceneNode>> pebbles_to_move;
                for (const auto& pid : kv.second) {
                    pebbles_to_move.push_back(this->get_pebble_node(director, shader_cache, pid));
                }

                // only queue animation if the current set of pebbles differs from the existing set on the node
                director.arrange(target_node->get("pebble_target"),
                                 pebbles_to_move,
                                 Sequencer { 0, 0.5, 0.5 },
                                 Sequencer { 0, 0.5, 0.5 });
            }
        }

        void init() {
            if (scene_graph->contains("pebbles")) {
                throw std::runtime_error("Scene node already initialied");
            }

            // create a node to hold the pebbles
            scene_graph->add("pebbles");

            // create nodes to serve as the targets for all pebbles
            for (auto &kv : scene_graph->get("nodes")->children()) {
                auto pt = scene_graph->get("nodes")->get(kv.first)->add("pebble_target");

                pt->transform().translate(0, -10);
            }
        }

        /**
         * @return the scene node associated with the specified pebble id. Creates one if not present.
         */
        sptr<SceneNode> get_pebble_node(Director& director, sptr<ShaderCache> shader_cache, const std::string& id) {
            if (scene_graph->get("pebbles")->contains(id)) {
                return scene_graph->get("pebbles")->get(id);
            }

            auto new_node = scene_graph->get("pebbles")->add(id, std::make_unique<atk::Dot>(3, shader_cache));
            director.build(new_node);

            return new_node;
        }
    };

}

#endif