#ifndef ENTITIES_SHADER_CACHE_H
#define ENTITIES_SHADER_CACHE_H

#include <SFML/Graphics.hpp>
#include <memory>

namespace atk {

    /**
     *  Manages access to loaded shaders
     */
    class ShaderCache {
        using CacheEntry = std::pair<std::string, std::string>;

    private:
        std::map<CacheEntry, std::shared_ptr<sf::Shader>> shaders;

    public:
        std::shared_ptr<sf::Shader> get_shader(const std::string &vertex, const std::string &fragment) {
            CacheEntry cache_entry = std::make_pair(vertex, fragment);

            if (shaders.contains(cache_entry)) {
                return shaders[cache_entry];
            }

            auto result = std::make_shared<sf::Shader>();

            if (!result->loadFromMemory(vertex, sf::Shader::Vertex)) {
                throw std::runtime_error("Could not load vertex shader.");
            }

            if (!result->loadFromMemory(fragment, sf::Shader::Fragment)) {
                throw std::runtime_error("Could not load fragment shader.");
            }

            shaders[cache_entry] = result;
            return result;
        }
    };
}

#endif