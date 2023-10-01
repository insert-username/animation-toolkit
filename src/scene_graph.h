#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include <SFML/Graphics.hpp>

#include <memory>
#include <map>
#include <string>
#include <sstream>
#include <stack>
#include "entities/local_boundable.h"
#include "utils/transforms.h"

namespace atk {

    class SceneNode : public std::enable_shared_from_this<SceneNode> {
    private:
        std::shared_ptr<sf::Drawable> sf_element = nullptr;

        // additional transfrom applied to the drawable, if possible
        sf::Transform _transform;

        /**
         * can be set to prioritize rendering.
         */
        int _z_order;

        std::map<std::string, std::shared_ptr<SceneNode>> _children;

        std::weak_ptr<SceneNode> _parent;

    public:
        explicit SceneNode(std::unique_ptr<sf::Drawable> drawable,
                  const int &z_order = 0) :
                _children(),
                _transform(sf::Transform::Identity),
                sf_element(std::move(drawable)),
                _z_order(z_order),
                _parent(std::weak_ptr<SceneNode>()){
        }

        explicit SceneNode(const int &z_order = 0) :
                _children(),
                _transform(sf::Transform::Identity),
                sf_element(nullptr),
                _z_order(z_order),
                _parent(std::weak_ptr<SceneNode>()) {
        }

        const std::map<std::string, std::shared_ptr<SceneNode>>& children() const {
            return _children;
        }

        std::weak_ptr<sf::Drawable> drawable() {
            return this->sf_element;
        }

        void clear() {
            for (auto& kv : this->_children) {
                kv.second->_parent = std::weak_ptr<SceneNode>();
            }

            this->_children.clear();
        }

        void remove(const std::string& id) {
            if (!this->_children.contains(id)) {
                throw std::runtime_error("specified id not present");
            }

            this->_children[id]->_parent = std::weak_ptr<SceneNode>();
            this->_children.erase(id);
        }

        /**
         * @return this node's drawable element if it is present and dynamic-castable to the specified type, otherwise
         * nullptr.
         */
        template <typename T>
        std::optional<std::shared_ptr<T>> try_get_drawable_as() {
            if (this->sf_element == nullptr) {
                return std::nullopt;
            } else {
                auto cast_val = std::dynamic_pointer_cast<T>(this->sf_element);
                if (cast_val != nullptr) {
                    return cast_val;
                } else {
                    return std::nullopt;
                }
            }
        }

        template <typename T>
        std::shared_ptr<T> get_drawable_as() {
            auto result = try_get_drawable_as<T>();

            if (!result.has_value()) {
                throw std::runtime_error("Could not return a drawable of requested type.");
            }

            return result.value();
        }

        /**
         * @return local boundary, relative to origin.
         */
        sf::FloatRect local_bounds() const {
            // getGlobalBounds is returned because we account for
            // the internal transformation of the shape
            if (this->sf_element == nullptr) {
                return {0, 0, 0, 0};
            }

            auto shape_ptr = std::dynamic_pointer_cast<sf::Shape>(this->sf_element);
            if (shape_ptr != nullptr) {
                return shape_ptr->getGlobalBounds();
            }

            auto sprite_ptr = std::dynamic_pointer_cast<sf::Sprite>(this->sf_element);
            if (sprite_ptr != nullptr) {
                return sprite_ptr->getGlobalBounds();
            }

            auto text_ptr = std::dynamic_pointer_cast<sf::Text>(this->sf_element);
            if (text_ptr != nullptr) {
                return text_ptr->getGlobalBounds();
            }

            auto boundable_ptr = std::dynamic_pointer_cast<atk::LocalBoundable>(this->sf_element);
            if (boundable_ptr != nullptr) {
                return boundable_ptr->get_local_bounds();
            }

            throw std::runtime_error("Object does not support bounds.");
        }

        /**
         * @return the bounding box, accounting for object transformation of this single node
         */
        sf::FloatRect world_bounds() const {
            sf::FloatRect local_bounds = this->local_bounds();
            return this->local_to_world_transform().transformRect(local_bounds);
        }

        /**
         * Updates the translation of this node such that it's current origin point will match the desired coordinate.
         */
        void translate_to_world_coordinate(const float& x, const float& y) {
            // the change to be applied to the current translation offset
            auto current_world_origin = local_to_world_transform().transformPoint(0, 0);
            float dx = x - current_world_origin.x;
            float dy = y - current_world_origin.y;

            _transform.translate(dx, dy);
            //TransformUtils::set_translation_part(
            //        _transform,
            //        current_translation.first + dx,
            //        current_translation.second + dy);

        }

        void set_origin_to_midpoint() {
            auto bounds = world_bounds_recursive();
            set_world_origin(bounds.left + 0.5f * bounds.width, bounds.top + 0.5f * bounds.height);

            auto new_bounds = world_bounds_recursive();
            auto world_origin = local_to_world_transform().transformPoint(0, 0);

            if (world_origin.x != new_bounds.left + new_bounds.width * 0.5f) {
                throw std::runtime_error("Bounds not set correctly");
            }

            if (world_origin.y != new_bounds.top + new_bounds.height * 0.5f) {
                throw std::runtime_error("Bounds not set correctly");
            }
        }

        void print_bounds(int depth = 0) {
            auto bounds = world_bounds_recursive();
            auto world_origin = local_to_world_transform().transformPoint(0, 0);

            std::cout << "Bounds: " << bounds.left << ", " << bounds.top << ", " << bounds.width << ", "
                      << bounds.height;
            std::cout << "  Origin: " << world_origin.x << ", " << world_origin.y;


            auto this_only_bounds = world_bounds();
            std::cout << "  This only Bounds: " << this_only_bounds.left << ", " << this_only_bounds.top << ", " << this_only_bounds.width << ", "
                      << this_only_bounds.height;


            auto local_bounds_val = local_bounds();
            std::cout << "  local Bounds: " << local_bounds_val.left << ", " << local_bounds_val.top << ", " << local_bounds_val.width << ", "
                      << local_bounds_val.height;


            std::cout << std::endl;

            for (auto &c : _children) {
                for (int i = 0; i <= depth; i++) {
                    std::cout << "    ";

                }


                std::cout << c.first << ": ";
                c.second->print_bounds(depth + 1);
            }
        }
        /**
         * Updates this SceneNode's transform such that the specified world position becomes its new local origin point.
         * All child nodes are similarly updated so that they maintain their current world transforms.
         *
         * the scenenode's world bounds will not change
         */
        void set_world_origin(const float& x, const float& y) {
            auto bounds_before = world_bounds_recursive();

            auto current_translation_offset = world_to_local_transform().transformPoint(x, y);

            // determine the required translation vector
            auto current_translation = TransformUtils::get_translation_part(_transform);

            // translation that must be applied to this node
            float dx = current_translation_offset.x - current_translation.first;
            float dy = current_translation_offset.y - current_translation.second;

            for (auto &c: _children) {
                c.second->transform().translate(-dx, -dy);
            }

            _transform.translate(dx, dy);

            auto bounds_after = world_bounds_recursive();

            if (bounds_before != bounds_after) {
                throw std::runtime_error("Internal error");
            }
        }

        sf::FloatRect world_bounds_recursive() const {
            auto this_bounds = world_bounds();
            for (const auto& c : _children) {
                auto child_bounds = c.second->world_bounds_recursive();

                float left = std::min(this_bounds.left, child_bounds.left);
                float top = std::min(this_bounds.top, child_bounds.top);

                float right = std::max(this_bounds.left + this_bounds.width, child_bounds.left + child_bounds.width);
                float bottom = std::max(this_bounds.top + this_bounds.height, child_bounds.top + child_bounds.height);

                float width = right - left;
                float height = bottom - top;

                this_bounds = sf::FloatRect(left, top, width, height);
            }

            return this_bounds;
        }

        [[nodiscard]] sf::Transform &transform() {
            return this->_transform;
        }

        sf::Transform local_to_local_transform(SceneNode& other) const {
            sf::Transform other_w_to_l = other.world_to_local_transform();
            sf::Transform l_to_w = local_to_world_transform();

            return other_w_to_l * l_to_w;
        }

        /**
         * @return a transform that brings the world coordinate to local coordinate 0, 0
         */
        sf::Transform world_to_local_transform() const {
            return local_to_world_transform().getInverse();
        }

        /**
         * @return transform that brings local coordinate 0, 0 to world position, applying all transforms of this node
         * and parents.
         */
        sf::Transform local_to_world_transform() const {
            std::stack<sf::Transform> transform_stack;

            transform_stack.push(this->_transform);

            auto parent = this->_parent;
            while (parent.lock()) {
                transform_stack.push(parent.lock()->_transform);
                parent = parent.lock()->_parent;
            }

            sf::Transform result = transform_stack.top();
            transform_stack.pop();
            while (!transform_stack.empty()) {
                result = transform_stack.top() * result;
                transform_stack.pop();
            }

            return result;
        }

        [[nodiscard]] int z_order() const {
            return _z_order;
        }

        void set_z_order(const int &z_order) {
            _z_order = z_order;
        }

        std::shared_ptr<SceneNode> add(const std::string &name) {
            if (_children.find(name) != _children.end()) {
                throw std::runtime_error(
                        (std::stringstream() << "Child with name " << name << " already present.").str());
            }

            _children[name] = std::make_shared<SceneNode>(_z_order);
            _children[name]->_parent = shared_from_this();
            return _children[name];
        }

        std::shared_ptr<SceneNode> add(const std::string &name, std::shared_ptr<SceneNode> node) {
            if (_children.find(name) != _children.end()) {
                throw std::runtime_error(
                        (std::stringstream() << "Child with name " << name << " already present.").str());
            }

            _children[name] = std::move(node); //std::make_shared<SceneNode>(std::move(drawable), _z_order);
            _children[name]->_parent = shared_from_this();
            return _children[name];
        }

        std::shared_ptr<SceneNode> add(const std::string &name, std::unique_ptr<sf::Drawable> drawable) {
            return add(name, std::make_unique<SceneNode>(std::move(drawable), _z_order));
        }

        bool contains(const std::string& name) const {
            return _children.contains(name);
        }

        std::shared_ptr<SceneNode> &get(const std::string &name) {
            if (_children.find(name) == _children.end()) {
                throw std::runtime_error((std::stringstream() << "Child with name " << name << " not present.").str());
            }

            return _children[name];
        }

        template<typename T>
        void modify(const std::function<void(T &)> &callback) {
            if (sf_element == nullptr) {
                throw std::runtime_error("Scene node does not have a drawable associated with it.");
            }

            callback(dynamic_cast<T &>(*sf_element));
        }

        template<typename T>
        T& get() {
            if (sf_element == nullptr) {
                throw std::runtime_error("Scene node does not have a drawable associated with it.");
            }

            return dynamic_cast<T &>(*sf_element);
        }

        template<typename T>
        void modify_recursive(const std::function<void(T &)> &callback) {
            if (sf_element == nullptr) {
                throw std::runtime_error("Scene node does not have a drawable associated with it.");
            }

            callback(dynamic_cast<T &>(*sf_element));

            for (auto &s: _children) {
                s.second->modify<T>(callback);
            }
        }

        void visit_recursive(const std::function<void(std::shared_ptr<SceneNode>)>& visitor) {
            visitor(shared_from_this());
            for (auto &s : _children) {
                s.second->visit_recursive(visitor);
            }
        }

        void render(const std::function<void(const sf::Drawable &, const sf::Transform &)> &visitor) {
            // will be sorted according to z order
            std::vector<std::shared_ptr<SceneNode>> nodes;

            std::stack<std::shared_ptr<SceneNode>> stack;
            stack.push(shared_from_this());
            while (!stack.empty()) {
                auto top = stack.top();
                nodes.push_back(top);
                stack.pop();

                if (top == nullptr) {
                    throw std::runtime_error("Internal error.");
                }

                for (auto &kv: top->_children) {
                    std::shared_ptr<SceneNode> next = kv.second;
                    stack.push(next);
                }
            }

            std::sort(nodes.begin(), nodes.end(), [](
                    const std::shared_ptr<SceneNode>& a,
                    const std::shared_ptr<SceneNode>& b) {
                return a->_z_order < b->_z_order;
            });

            for (const auto &s: nodes) {
                if (s->sf_element != nullptr) {
                    auto tr = s->local_to_world_transform();
                    auto tr_origin = tr.transformPoint(0, 0);
                    //std::cout << "Rendering node.. 0,0 goes to: " << tr_origin.x << ", " << tr_origin.y << std::endl;

                    visitor(*s->sf_element, s->local_to_world_transform());
                }
            }
        }

    private:


        void set_parent(const std::shared_ptr<SceneNode>& scene_node) {
            if (this->_parent.lock()) {
                throw std::runtime_error("Parent already set");
            }

            this->_parent = scene_node;
        }
    };
}

#endif