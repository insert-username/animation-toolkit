#ifndef ENTITIES_ARROW_H
#define ENTITIES_ARROW_H

#include "buildable.h"
#include "../utils/bounds.h"
#include "../utils/proportional_quantity.h"

namespace atk {

    class Arrow: public sf::Drawable, public atk::LocalBoundable, public atk::Buildable {

        bool _draw_head = true;

        std::weak_ptr<SceneNode> head_target;
        std::weak_ptr<SceneNode> tail_target;
        std::weak_ptr<SceneNode> parent;

        float build_percent = 1.0f;

        sf::Color _fill_color = atk::constants::color::SolarizedDark::base3;

        ProportionalQuantity head_offset = ProportionalQuantity(std::nullopt, std::nullopt, 0.15f, 0);
        ProportionalQuantity tail_offset = ProportionalQuantity(std::nullopt, std::nullopt, 0.15f, 0);
        ProportionalQuantity thickness = ProportionalQuantity(3.0f, 3.0f, 1.0f, 0.0f);
        ProportionalQuantity head_length = ProportionalQuantity(std::nullopt, std::nullopt, 0.2f, 0.0f);
        ProportionalQuantity head_thickness = ProportionalQuantity(std::nullopt, std::nullopt, 0.1f, 0.0f);
        ProportionalQuantity head_undercut = ProportionalQuantity(std::nullopt, std::nullopt, 0.05f, 0.0f);


    public:
        Arrow(const std::shared_ptr<SceneNode>& parent,
                const std::shared_ptr<SceneNode>& head_target, const std::shared_ptr<SceneNode>& tail_target) :
            head_target(head_target),
            tail_target(tail_target),
            parent(parent) {

        }

        void set_draw_head(const bool& draw_head) {
            this->_draw_head = draw_head;
        }

        void set_fill_color(sf::Color fill_color) {
            _fill_color = fill_color;
        }

        sf::Color get_fill_color() {
            return _fill_color;
        }

        float get_build_percent() override {
            return this->build_percent;
        }

        void set_build_percent(const float &new_build_percent) override {
            this->build_percent = new_build_percent;
        }

        sf::FloatRect get_local_bounds() override {
            sf::VertexArray body;
            sf::Transform t;
            construct_body(body, t);

            return t.transformRect(body.getBounds());
        }

    protected:
        void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
            // arrow should always track targets
            sf::VertexArray body;
            sf::Transform t;
            construct_body(body, t);

            states.transform = states.transform * t;

            target.draw(body, states);
        }

    private:
        void construct_body(sf::VertexArray& body, sf::Transform& t) const {
            auto head_target_xy = head_target.lock()->local_to_world_transform().transformPoint(0, 0);
            auto tail_target_xy = tail_target.lock()->local_to_world_transform().transformPoint(0, 0);

            head_target_xy = parent.lock()->world_to_local_transform().transformPoint(head_target_xy);
            tail_target_xy = parent.lock()->world_to_local_transform().transformPoint(tail_target_xy);

            float x0 = tail_target_xy.x;
            float y0 = tail_target_xy.y;

            float x1 = head_target_xy.x;
            float y1 = head_target_xy.y;

            auto length = std::sqrt((x1 - x0)*(x1 - x0) + (y1 - y0)*(y1 - y0));
            auto head_dl = head_length.get_adjusted(length);

            auto half_line_thickness = 0.5f * thickness.get_adjusted(length);

            auto head_l_end = length - head_offset.get_adjusted(length);
            auto head_l_start = head_l_end - head_dl;
            auto tail_l_start = tail_offset.get_adjusted(length);
            auto head_half_thickness = 0.5f * head_thickness.get_adjusted(length);
            auto undercut = head_undercut.get_adjusted(length);

            if (_draw_head) {
                body = sf::VertexArray(sf::TriangleFan, 7);
                body[0] = sf::Vertex(sf::Vector2f(head_l_end, 0), _fill_color);
                body[1] = sf::Vertex(sf::Vector2f(head_l_start, -half_line_thickness - head_half_thickness), _fill_color);
                body[2] = sf::Vertex(sf::Vector2f(head_l_start + undercut, -half_line_thickness), _fill_color);
                body[3] = sf::Vertex(sf::Vector2f(tail_l_start, -half_line_thickness), _fill_color);
                body[4] = sf::Vertex(sf::Vector2f(tail_l_start, half_line_thickness), _fill_color);
                body[5] = sf::Vertex(sf::Vector2f(head_l_start + undercut, half_line_thickness), _fill_color);
                body[6] = sf::Vertex(sf::Vector2f(head_l_start, half_line_thickness + head_half_thickness), _fill_color);
            } else {
                body = sf::VertexArray(sf::TriangleStrip, 4);
                body[0] = sf::Vertex(sf::Vector2f(tail_l_start, -half_line_thickness), _fill_color);
                body[1] = sf::Vertex(sf::Vector2f(tail_l_start, half_line_thickness), _fill_color);
                body[2] = sf::Vertex(sf::Vector2f(head_l_end, -half_line_thickness), _fill_color);
                body[3] = sf::Vertex(sf::Vector2f(head_l_end, half_line_thickness), _fill_color);
            }

            float angle = std::atan2(y1 - y0, x1 - x0);
            float angle_deg = 180.0f * angle * (float)M_1_PI;
            t = sf::Transform::Identity;
            t.translate(x0, y0);
            t.rotate(angle_deg);
            t.scale(build_percent, build_percent);
        }
    };
}

#endif