#include <SFML/Graphics.hpp>

#include <math.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <algorithm>
#include <optional>

#include "scene_graph.h"
#include "animation/animation.h"
#include "entities/grid.h"
#include "animation/timeline.h"
#include "animation/director.h"
#include "animation/sfml_clock_timer.h"
#include "rendering/renderer.h"
#include "entities/empty.h"
#include "entities/dot.h"
#include "graph/graphviz_parser.h"
#include "utils/common_manipulations.h"
#include "utils/color.h"

#include <filesystem>

namespace fs = std::filesystem;

std::vector<std::string> split_str(const std::string& str) {
    if (str.empty()) {
        return {};
    }

    std::vector<std::string> strs;
    boost::split(strs, str, boost::is_any_of(","));

    return strs;
}

std::vector<int> split_int_str(const std::string& str) {
    std::vector<std::string> strs = split_str(str);

    std::vector<int> result;
    for (const auto &s : strs) {
        result.push_back(stoi(s, nullptr));
    }

    return result;
}

void update_graph(atk::Director& director,
                  std::shared_ptr<atk::Timeline>& timeline,
                  std::shared_ptr<atk::SceneNode>& graph,
                  const atk::GraphVizModel& from,
                  const atk::GraphVizModel& to) {

    auto graph_nodes = graph->get("nodes");
    auto graph_edges = graph->get("edges");

    // determine the pebble destinations that need to be updated

    // if a node loses or recieves a pebble, the entire node's pebbles need to be arranged
    // the discard pile of pebbles also needs to be arranged??
    std::set<std::string> before_pebbles;
    std::set<std::string> after_pebbles;
    for (const auto& n : from.nodes) {
        for (const auto &s : split_str(n.second.at("xlabel"))) {
            before_pebbles.insert(s);
        }
    }
    for (const auto& n : to.nodes) {
        for (const auto &s : split_str(n.second.at("xlabel"))) {
            after_pebbles.insert(s);
        }
    }

    for (const auto& p : before_pebbles) {
        if (!after_pebbles.contains(p)) {
            director.unbuild(graph->get("pebbles")->get(p));
        }
    }

    using sn_ptr = std::shared_ptr<atk::SceneNode>;

    for (const auto &n : to.nodes) {
        // determine if the node state has changed, is arrangement required?
        sn_ptr node_ptr = graph_nodes->get(n.first);
        if (!n.second.at("xlabel").empty() && n.second.at("xlabel") != from.nodes.at(n.first).at("xlabel")) {
            std::vector<sn_ptr> to_arrange;
            for (const auto& s : split_str(n.second.at("xlabel"))) {
                to_arrange.push_back(graph->get("pebbles")->get(s));
            }
            director.arrange(node_ptr->get("pebble_target"),
                             to_arrange,
                             atk::Sequencer { 0, 0.5, 0.5 },
                             atk::Sequencer { 0, 0.5, 0.5 });
        }
    }


    // determine any edges that have been added
    for (const auto &e : to.edges) {
        std::string e_name = (std::stringstream() << e.first.first << "->" << e.first.second).str();
        std::string e_reversed_name = (std::stringstream() << e.first.second << "->" << e.first.first).str();

        if (graph_edges->contains(e_name)) {
            auto arrow = graph_edges->get(e_name)->try_get_drawable_as<atk::Arrow>().value();
            if (arrow->get_build_percent() != 1.0f) {
                std::cout << "    animating build " << e_name << std::endl;
                director.build(graph_edges->get(e_name));
            }

            bool should_highlight = e.second.contains("state") && e.second.at("state") == "dfs";
            auto default_col = atk::constants::color::SolarizedDark::base3;
            auto dfs_col = atk::constants::color::SolarizedDark::magenta;

            auto start_col = arrow->get_fill_color();
            auto target_col = should_highlight ? dfs_col : default_col;

            if (start_col != target_col) {
                timeline->add(
                        std::make_unique<atk::FireOnceScheduler>(0, 0.5),
                        std::make_unique<atk::InterpolatedAnimation>(
                                atk::InterpolatedAnimation::ease_out_interpolation(),
                                [arrow, start_col, target_col](float v) {
                                    arrow->set_fill_color(atk::ColorUtils::lerp(v, start_col, target_col));
                                }));
            }
        } else {
            std::cout << "    CREATING " << e_name << std::endl;
            graph_edges->add(e_name, std::make_unique<atk::Arrow>(
                    graph_edges,
                    graph_nodes->get(e.first.first),
                    graph_nodes->get(e.first.second)));
            atk::CommonManipulations::set_unbuilt(graph_edges->get(e_name));
            director.build(graph_edges->get(e_name));
        }

        if (graph_edges->contains(e_reversed_name)) {
            auto buildable = graph_edges->get(e_reversed_name)->try_get_drawable_as<atk::Buildable>().value();
            if (buildable->get_build_percent() != 0) {
                std::cout << "    animating hide " << e_reversed_name << std::endl;
                director.unbuild(graph_edges->get(e_reversed_name));
            }
        }
    }
}

void populate_pebbles(atk::Director& director,
                  std::shared_ptr<atk::ShaderCache> shader_cache,
                  std::shared_ptr<atk::SceneNode>& graph,
                  const atk::GraphVizModel& model) {

    auto pebbles = graph->get("pebbles");

    std::map<std::string, std::vector<std::string>> pebble_arrangements;

    for (const auto &n : model.nodes) {
        std::string node_name = n.first;
        pebble_arrangements[node_name] = std::vector<std::string>();

        if (n.second.contains("xlabel")) {
            auto pebble_indexes = split_int_str(n.second.at("xlabel"));

            for (auto pi : pebble_indexes) {
                pebbles->add(std::to_string(pi), std::make_unique<atk::Dot>(
                        4,
                        shader_cache,
                        atk::constants::color::SolarizedDark::red));
                pebble_arrangements[node_name].push_back(std::to_string(pi));
            }
        }
    }

    for (const auto& node_to_pebs : pebble_arrangements) {
        auto peb_target = graph->get("nodes")->get(node_to_pebs.first)->add("pebble_target");
        peb_target->transform().translate(0, -10);

        if (node_to_pebs.second.empty()) {
            continue;
        }

        std::vector<std::shared_ptr<atk::SceneNode>> pebs;
        for (const auto& p_name : node_to_pebs.second) {
            pebs.push_back(pebbles->get(p_name));
        }

        director.arrange(graph->get("nodes")->get(node_to_pebs.first)->get("pebble_target"),
                         pebs,
                         atk::Sequencer {0, 0.1f, 0},
                         atk::Sequencer {0, 0.1f, 0});
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        throw std::runtime_error("Expected two arguments");
    }

    std::string template_graph_file = argv[1];
    std::string step_graph_dir = argv[2];
    std::vector<std::string> step_graphs;
    for (const auto& f : fs::directory_iterator(step_graph_dir)) {
        if (f.exists() && f.is_regular_file()) {
            std::string path = f.path();

            if (boost::algorithm::ends_with(path, ".dot")) {
                step_graphs.push_back(f.path());
            }
        }

    }
    std::sort(step_graphs.begin(), step_graphs.end());

    atk::GraphVizModel template_graph_model = atk::GraphVizModel::read_from_file(template_graph_file);
    std::vector<atk::GraphVizModel> models;

    for (const auto& s : step_graphs) {
        models.push_back(atk::GraphVizModel::read_from_file(s));
    }

    auto shader_cache = std::make_shared<atk::ShaderCache>();
    atk::GraphSceneNodeFactory graph_factory(shader_cache);
    auto template_graph = graph_factory.from_model(template_graph_model);
    template_graph->visit_recursive([](std::shared_ptr<atk::SceneNode> s){
        auto a = s->try_get_drawable_as<atk::Arrow>();
        if (a.has_value()) {
            a.value()->set_draw_head(false);
        }
    });
    auto graph = graph_factory.from_model(models[0]);

    std::cout << "Creating scene: " << std::endl;
    auto scene = std::make_shared<atk::SceneNode>();
    //scene->print_bounds();

    std::cout << "Adding graph: " << std::endl;

    scene->add("graph", graph);
    //scene->print_bounds();

    std::cout << "Adding template graph: " << std::endl;

    scene->add("template_graph", template_graph)->transform()
        .translate(scene->get("graph")->world_bounds_recursive().width + 20, 0);
    //scene->print_bounds();

    std::cout << "Setting origin to midpoint: " << std::endl;


    scene->set_origin_to_midpoint();
    //scene->print_bounds();

    std::cout << "Translating to world 0, 0: " << std::endl;
    scene->translate_to_world_coordinate(0, 0);
    //scene->print_bounds();


    atk::CommonManipulations::set_unbuilt(scene);
    atk::CommonManipulations::set_built(scene->get("template_graph"));

    int window_width = 800;
    int window_height = 800;

    auto renderer = std::make_shared<atk::WindowRenderer>(window_width, window_height,
                                                          atk::constants::color::SolarizedDark::base03,
                                                          false);
    //scene->translate_to_world_coordinate(window_width * 0.5f, window_height * 0.5f);
    std::cout << "Translating to world " << window_width * 0.5f << ", " << window_height * 0.5f << std::endl;

    scene->translate_to_world_coordinate(window_width * 0.5f, window_height * 0.5f);
    //scene->print_bounds();

    auto timer = atk::SFMLClockTimer();
    timer.set_scale(1);
    auto timeline = std::make_shared<atk::Timeline>();
    atk::Director director(scene, timeline, renderer);

    director.build(graph->get("nodes"), atk::Sequencer{0.5f, 0.8f, 0.79f});
    director.play(timer);

    graph->add("pebbles");
    graph->add("pebbles_discard_target")->transform().translate(graph->world_bounds_recursive().width + 10, 0);

    populate_pebbles(director, shader_cache, graph, models[0]);
    atk::CommonManipulations::set_unbuilt(graph->get("pebbles"));
    director.play(timer);

    director.build(graph->get("edges"), atk::Sequencer{0.5f, 0.8f, 0.79f});
    director.play(timer);

    director.build(graph->get("pebbles"), atk::Sequencer{0.5f, 0.8f, 0.79f});
    director.play(timer);

    for (int i = 1; i < models.size(); i++) {
        std::cout << "Updating graph" << std::endl;
        update_graph(director, timeline, graph, models[i - 1], models[i]);
        std::cout << "starting animation" << std::endl;

        director.play(timer);
    }

    director.play_forever(timer);

    return 0;
}
