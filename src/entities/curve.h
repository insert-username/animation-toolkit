#ifndef ENTITIES_CURVE_H
#define ENTITIES_CURVE_H

#include "buildable.h"
#include "../constants.h"
#include "../utils/bounds.h"
#include "shader_cache.h"

namespace atk {

    /**
     * Arbitrary curve sampled along some interval.
     */
    class Curve: public sf::Drawable, public Buildable, public LocalBoundable {
    private:
        static constexpr const char* VERTEX_SHADER_SRC = R"VERTEX_SHADER(
                                void main()
                                {
                                    // transform the vertex position
                                    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

                                    // transform the texture coordinates
                                    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

                                    // forward the vertex color
                                    gl_FrontColor = gl_Color;
                                })VERTEX_SHADER";

        static constexpr const char* FRAGMENT_SHADER_SRC = R"FRAGMENT_SHADER(
                                //uniform sampler2D texture;
                                uniform float buffer_percent;

                                void main()
                                {
                                    // lookup the pixel in the texture
                                    //vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);

                                    float opacity = gl_TexCoord[0].y;

                                    if (opacity <= buffer_percent) {
                                        opacity = opacity / buffer_percent;
                                    } else if (opacity >= (1.0 - buffer_percent)) {
                                        opacity = (1.0 - opacity);
                                        opacity = opacity / buffer_percent;
                                    } else {
                                        opacity = 1.0;
                                    }

                                    // multiply it by the color
                                    gl_FragColor = vec4(gl_Color.x, gl_Color.y, gl_Color.z, opacity); //gl_Color * pixel;
                                })FRAGMENT_SHADER";

        std::weak_ptr<ShaderCache> shader_cache;

        float u0;
        float u1;

        int sample_count;

        float half_thickness = 3;

        float build_percent = 1.0f;

        std::function<std::pair<float, float>(float)> sample;

        std::function<sf::Color(float)> color_sample;

        std::vector<sf::Vertex> verts;

    public:
        Curve(Curve &&to_move) noexcept :
                u0(to_move.u0),
                u1(to_move.u1),
                shader_cache(std::move(to_move.shader_cache)),
                sample(std::move(to_move.sample)),
                sample_count(to_move.sample_count),
                color_sample(to_move.color_sample) {

            verts = std::move(to_move.verts);
            sample_points = std::move(to_move.sample_points);
        }

        Curve(const float& u0,
                const float& u1,
                const std::shared_ptr<ShaderCache>& shader_cache,
                std::function<std::pair<float, float>(float)> sample,
                const int& sample_count,
                std::function<sf::Color(float)> color_sample = [](float u){ return atk::constants::color::SolarizedDark::base01; }) :
                u0(u0),
                u1(u1),
                sample(std::move(sample)),
                sample_count(sample_count),
                color_sample(std::move(color_sample)),
                shader_cache(shader_cache){

            resample();
        }

        sf::FloatRect get_local_bounds() override {
            resample();
            sf::FloatRect bounds(verts[0].position.x, verts[0].position.y, 0, 0);

            for (auto &v : verts) {
                auto v_bounds = sf::FloatRect(v.position.x, v.position.y, 0, 0);
                bounds = BoundsUtil::combine(bounds, v_bounds);
            }

            return bounds;
        }

        void set_thickness(const float& thickness) {
            half_thickness = thickness / 2;
        }

        float get_thickness() const {
            return half_thickness * 2;
        }

        void change_sample(std::function<std::pair<float, float>(float)> sample) {
            this->sample = std::move(sample);
            resample();
        }

        [[nodiscard]] const std::vector<sf::Vertex>& get_verts() const {
            return verts;
        }

        float get_build_percent() override {
            return build_percent;
        }

        void set_build_percent(const float &new_build_percent) override {
            this->build_percent = new_build_percent;
            resample();
        }

    public:
        void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
            sf::RenderStates states_with_shader(states);

            auto shader = shader_cache.lock()->get_shader(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);

            shader->setUniform("buffer_percent", 0.4f);
            
            states_with_shader.shader = shader.get();


            target.draw(&verts[0], verts.size(), sf::TriangleStrip, states_with_shader);
            return;

            sf::CircleShape circ(3);
            circ.setOrigin(3, 3);
            circ.setFillColor(sf::Color::Red);


            std::vector<sf::Vertex> line_verts;
            for (auto &v : verts) {
                line_verts.emplace_back(
                        v.position,
                        sf::Color::Blue);
            }

            target.draw(&line_verts[0], line_verts.size(), sf::LineStrip, states);

            for (const auto& v : verts) {
                circ.setPosition(v.position);
                target.draw(circ, states);
            }

            circ.setFillColor(sf::Color::Green);
            for (const auto &v : sample_points) {
                circ.setPosition(v.x, v.y);
                target.draw(circ, states);
            }
        }

    private:

        std::vector<sf::Vector2f> sample_points;
        void resample() {
            sample_points.clear();
            verts.clear();

            auto build_percent_adjusted_u1 = u0 + (u1 - u0) * build_percent;


            auto u_inc = (build_percent_adjusted_u1 - u0) / (float)sample_count;
            sample_points.clear();

            // first / last sample points, rotate vector to get normal

            auto s0 = sample(u0);
            sample_points.emplace_back(s0.first, s0.second);
            auto s1 = sample(u0 + u_inc);
            sample_points.emplace_back(s1.first, s1.second);

            auto delta = sub(sample_points[1], sample_points[0]);
            sf::Vector2f offset;
            sf::Color color;
            if (delta.x != 0 || delta.y != 0) {
                offset = scale(rotate(normalize(delta), M_PI_2), half_thickness);

                color = color_sample(u0);

                verts.emplace_back(sf::Vector2f(sf::Vector2f(s0.first, s0.second) - offset),
                                   color,
                                   sf::Vector2f(0, 0));
                verts.emplace_back(sf::Vector2f(sf::Vector2f(s0.first, s0.second) + offset),
                                   color,
                                   sf::Vector2f(0, 1));
            }

            for (int i = 2; i <= sample_count; i++) {
                float u = u0 + (float)i * u_inc;
                auto sample_point = sample(u);

                sample_points.emplace_back(sample_point.first, sample_point.second);

                sf::Vector2f xy_next = sample_points.back();
                sf::Vector2f xy = sample_points[i - 1];
                sf::Vector2f xy_prev = sample_points[i-2];

                sf::Vector2f to_prev = sub(xy_prev, xy);
                sf::Vector2f to_next = sub(xy_next, xy);

                //std::cout << "    to-prev: " << to_prev.x << ", " << to_prev.y << std::endl;
                //std::cout << "    to-next: " << to_next.x << ", " << to_next.y << std::endl;

                to_prev = rotate(to_prev, M_PI_2);
                to_next = rotate(to_next, -M_PI_2);

                sf::Vector2f norm = add(to_prev, to_next);

                if (norm.x == 0 && norm.y == 0) {
                    continue;
                }

                norm = normalize(norm);
                sf::Vector2f offset = scale(norm, half_thickness);

                // build the triangle strip from this
                auto color = color_sample(u);
                verts.emplace_back(
                        xy + offset,
                        color,
                        sf::Vector2f(0, 0));
                verts.emplace_back(
                        xy - offset,
                        color,
                        sf::Vector2f(0, 1));
            }

            delta = sub(sample_points.back(), sample_points[sample_points.size() - 2]);
            if (delta.x != 0 || delta.y != 0) {

                offset = scale(rotate(normalize(delta), M_PI_2), half_thickness);
                color = color_sample(u1);
                verts.emplace_back(sample_points.back() - offset, color, sf::Vector2f(0, 0));
                verts.emplace_back(sample_points.back() + offset, color, sf::Vector2f(0, 1));
            }

        }

        sf::Vector2f scale(const sf::Vector2f& input, const float& scale) {
            return sf::Vector2f(
                    input.x * scale,
                    input.y * scale
            );
        }

        sf::Vector2f sub(const sf::Vector2f& a, const sf::Vector2f& b) {
            return sf::Vector2f(
                    a.x - b.x,
                    a.y - b.y
            );
        }


        sf::Vector2f add(const sf::Vector2f& a, const sf::Vector2f& b) {
            return sf::Vector2f(
                    a.x + b.x,
                    a.y + b.y
            );
        }

        sf::Vector2f normalize(const sf::Vector2f& input) {
            if (input.x == 0 && input.y == 0) {
                throw std::runtime_error("Vector has zero length");
            }

            auto mag_sq = input.x * input.x + input.y * input.y;
            auto mag = std::sqrt(mag_sq);

            return sf::Vector2f(input.x / mag, input.y / mag);
        }

        sf::Vector2f rotate(const sf::Vector2f& input, const float& angle) {
            auto cos = std::cos(angle);
            auto sin = std::sin(angle);

            return sf::Vector2f(
                    cos * input.x - sin * input.y,
                    sin * input.x + cos * input.y);
        }


    };
}

#endif