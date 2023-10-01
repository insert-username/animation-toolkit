#include <SFML/Graphics.hpp>

#include <math.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <algorithm>
#include <optional>

#include <ffnx/drplan/plans/canonical_top_down/PebbleGame2D.h>

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
#include "graph/pebblegame.h"
#include "utils/common_manipulations.h"
#include "utils/color.h"

#include <filesystem>

namespace fs = std::filesystem;

using Graph = atk::GraphVizFlowGraphFactory::ffnx_graph;
using Cluster = ffnx::cluster::Cluster<Graph>;
using Move = ffnx::pebblegame::PebbleGame2D<Graph>::Move;
using PG2D = ffnx::pebblegame::PebbleGame2D<Graph>;

template<typename T>
using sptr = std::shared_ptr<T>;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        throw std::runtime_error("Expected two arguments");
    }

    atk::GraphVizModel template_graph_model = atk::GraphVizModel::read_from_file(argv[1]);
    std::shared_ptr<Graph> graph = atk::GraphVizFlowGraphFactory::get_graph(template_graph_model);

    auto cluster = Cluster::Builder::of_graph(graph);
    auto pebblegame = std::make_shared<ffnx::pebblegame::PebbleGame2D<Graph>>(cluster);

    auto shader_cache = std::make_shared<atk::ShaderCache>();
    atk::GraphSceneNodeFactory graph_factory(shader_cache);
    auto template_graph = graph_factory.from_model(template_graph_model);
    auto game_graph = graph_factory.from_model(template_graph_model);
    template_graph->visit_recursive([](std::shared_ptr<atk::SceneNode> s){
        auto a = s->try_get_drawable_as<atk::Arrow>();
        if (a.has_value()) {
            a.value()->set_draw_head(false);
        }
    });

    game_graph->get("edges")->clear();

    auto scene = std::make_shared<atk::SceneNode>();

    // arranging scene
    {
        scene->add("template_graph", template_graph)->set_origin_to_midpoint();
        auto wb = scene->get("template_graph")->world_bounds_recursive();
        scene->add("game_graph", game_graph);
        scene->get("game_graph")->translate_to_world_coordinate(wb.left + wb.width + 10.0f, 0);
        scene->set_origin_to_midpoint();
        atk::CommonManipulations::set_built(scene);
    }

    int window_width = 800;
    int window_height = 800;

    auto renderer = std::make_shared<atk::WindowRenderer>(window_width, window_height,
                                                          atk::constants::color::SolarizedDark::base03,
                                                          false);
    scene->translate_to_world_coordinate(window_width * 0.5f, window_height * 0.5f);

    auto timer = atk::SFMLClockTimer();
    timer.set_scale(std::stof(argv[2]));
    auto timeline = std::make_shared<atk::Timeline>();
    atk::Director director(scene, timeline, renderer);

    auto scene_graph_pebblegame = atk::SceneGraphPebbleGame(pebblegame, cluster, game_graph);

    pebblegame->run([&](std::shared_ptr<Move> move){
        std::cout << "Move" << std::endl;

        scene_graph_pebblegame.update(director, *timeline, shader_cache, move);

        director.play(timer);

        auto edge_being_added = move->edge_being_added;

        std::string input_edge_to_highlight;
        if (edge_being_added.has_value()) {
            auto v0 = (*graph)[edge_being_added.value().first];
            auto v1 = (*graph)[edge_being_added.value().second];

            input_edge_to_highlight = (std::stringstream() << v0 << "->" << v1).str();
        }

        auto default_col = atk::constants::color::SolarizedDark::base3;
        auto dfs_col = atk::constants::color::SolarizedDark::magenta;
        auto add_col = atk::constants::color::SolarizedDark::magenta;

        for (const auto &kv : template_graph->get("edges")->children()) {
            auto target_col = (kv.first == input_edge_to_highlight) ? add_col : default_col;
            auto arrow = template_graph->get("edges")->get(kv.first)->get_drawable_as<atk::Arrow>();
            auto start_col = arrow->get_fill_color();

            if (start_col != target_col) {
                timeline->add(
                        std::make_unique<atk::FireOnceScheduler>(0, 0.5),
                        std::make_unique<atk::InterpolatedAnimation>(
                                atk::InterpolatedAnimation::ease_out_interpolation(),
                                [arrow, start_col, target_col](float v) {
                                    arrow->set_fill_color(atk::ColorUtils::lerp(v, start_col, target_col));
                                }));
            }
        }

        director.play(timer);
    });

    std::cout << "Done" << std::endl;

    director.play_forever(timer );

    return 0;
}
