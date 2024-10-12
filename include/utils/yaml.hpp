#ifndef UTILS_YAML_HPP_
#define UTILS_YAML_HPP_

// std
#include <iostream>
#include <string>
#include <type_traits>
// yaml
#include <yaml-cpp/yaml.h>

namespace utils {

class Yaml {
  public:
    Yaml() = delete;
    Yaml(Yaml &) = default;
    Yaml(Yaml &&) = default;
    ~Yaml() = default;

    explicit Yaml(const std::string &yaml_file);

    template <typename T> auto readValue(std::string_view key) const {
        std::stringstream ss(key.data());
        std::cout << __LINE__ << " | " << ss.str() << "\n";
        std::string segment;
        std::vector<std::string> segments{};
        while (std::getline(ss, segment, '.'))
            segments.push_back(segment);
        std::cout << __LINE__ << " | " << yaml_config_[segments[0]] << "\n";
        auto node = yaml_config_[segments[0]];
        for (int i = 1; i < segments.size(); ++i)
            node = node[segments[i]];
        return node.as<T>();
    }

    template <typename T, typename = std::enable_if<std::is_integral_v<T>>,
              typename VEC2D = std::vector<std::vector<T>>>
    auto readValue(std::string_view key, size_t size1, size_t size2) const {
        auto raw_data = this->readValue<std::vector<T>>(key);
        if (raw_data.size() != size1 * size2) {
            std::stringstream ss;
            ss << key << " dim is [" << size1 << ',' << size2
               << "], mismatch yaml config data size: " << raw_data.size()
               << " != " << size1 * size2 << " (size1 * size2)";
            throw std::runtime_error(ss.str());
        }
        VEC2D vec_2d;
        for (size_t i = 0; i < size1; ++i) {
            vec_2d.push_back(std::vector<T>{});
            for (size_t j = 0; j < size2; ++j)
                vec_2d.at(i).push_back(raw_data.at(i * size2 + j));
        }
        return vec_2d;
    }

    template <typename T> auto readValue(std::string_view key, T &data) const {
        data = yaml_config_[key].as<T>();
        return;
    }

  private:
    YAML::Node yaml_config_;
};

} // namespace utils

#endif // !UTILS_YAML_HPP_
