#include "utils/yaml.hpp"

namespace utils {

Yaml::Yaml(const std::string &yaml_file)
    : yaml_config_(YAML::LoadFile(yaml_file)) {}

} // namespace utils
