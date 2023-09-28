#ifndef ENTITIES_DOT_H
#define ENTITIES_DOT_H

#include "buildable.h"
#include "../constants.h"

namespace atk {
    class Dot : public sf::Drawable, public Buildable, public LocalBoundable {
    private:
        static constexpr const char* VERTEX_SHADER_SRC = R"VERTEX_SHADER(
                                uniform float buffer_percent;
                                uniform vec4 outline_color;
                                uniform float outline_percent;

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
                                uniform vec4 outline_color;
                                uniform float outline_percent;

                                void main()
                                {
                                    float dist_to_edge = gl_TexCoord[0].y;

                                    vec4 core_color = (dist_to_edge > outline_percent) ? gl_Color : outline_color;

                                    float opacity = 0.0f;
                                    if (dist_to_edge <= buffer_percent) {
                                        opacity = dist_to_edge / buffer_percent;
                                    } else {
                                        opacity = 1.0;
                                    }

                                    // multiply it by the color
                                    gl_FragColor = vec4(core_color.x, core_color.y, core_color.z, opacity); //gl_Color * pixel;
                                })FRAGMENT_SHADER";


        float percent_complete = 1.0f;

        int num_points = 32;

        float radius;

        sf::VertexArray shape;

        sf::Color _fill_color;
        sf::Color _outline_color = atk::constants::color::SolarizedDark::green;

        float outline_percent = 0.3;

        std::weak_ptr<ShaderCache> shader_cache;

    public:
        explicit Dot(const float &radius,
                     const std::shared_ptr<ShaderCache>& shader_cache,
                     const sf::Color fill_color = atk::constants::color::SolarizedDark::magenta) :
            radius(radius),
            shader_cache(shader_cache),
            _fill_color(fill_color) {
            build_shape();
        }

        void set_fill_color(const sf::Color& new_color) {
            _fill_color = new_color;
        }

        float get_build_percent() override {
            return percent_complete;
        }

        void set_build_percent(const float &build_percent) override {
            percent_complete = build_percent;
            build_shape();
        }

        sf::FloatRect get_local_bounds() override {
            float actual_radius = radius * percent_complete;

            return sf::FloatRect(
                    -actual_radius,
                    -actual_radius,
                    2.0f * actual_radius,
                    2.0f * actual_radius);
        }

    protected:
        void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
            auto shader = shader_cache.lock()->get_shader(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);
            shader->setUniform("buffer_percent", 0.1f);
            shader->setUniform("outline_color", (sf::Glsl::Vec4)_outline_color);
            shader->setUniform("outline_percent", outline_percent);
            states.shader = shader.get();
            target.draw(shape, states);
        }

    private:
        void build_shape() {
            float actual_radius = radius * percent_complete;

            shape = sf::VertexArray(sf::TriangleFan, num_points + 2);

            auto tex_center = sf::Vector2f(0, 1);
            auto tex_edge = sf::Vector2f(0, 0);

            shape[0] = sf::Vertex(sf::Vector2f(0, 0), _fill_color, tex_center);

            float d_theta = (float)(2.0 * M_PI) / (float)num_points;
            for (int i = 0; i <= num_points; i++) {
                float x = actual_radius * std::cos(d_theta * (float)i);
                float y = actual_radius * std::sin(d_theta * (float)i);
                shape[i + 1] = sf::Vertex(sf::Vector2f(x, y), _fill_color, tex_edge);
            }
        }
    };
}

#endif