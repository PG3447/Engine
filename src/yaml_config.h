#ifndef YAML_CONFIG_H
#define YAML_CONFIG_H

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <string>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>

// Serializacja yaml dla GLM vec3
// moznaby ewentualnie tez dla vec4 lub innych jesli potrzeba bedzie
namespace YAML {
    template<>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }
            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };
}

class YamlConfig {
private:
    YAML::Node rootNode;
    std::string filepath;

public:
    YamlConfig() {
        rootNode = YAML::Node(YAML::NodeType::Map);
    }
    ~YamlConfig() = default;

    bool load(const std::string& path) {
        filepath = path;
        try {
            rootNode = YAML::LoadFile(filepath);

            if (rootNode.IsNull()) {
                rootNode = YAML::Node(YAML::NodeType::Map);
            }

            spdlog::info("YAML zaladowany: {}", filepath);
            return true;
        }
        catch (const YAML::Exception& e) {
            spdlog::error("Blad zaladowywania YAML ({}): {}", filepath, e.what());
            return false;
        }
    }

    bool save(const std::string& path = "") {
        std::string savePath = path.empty() ? filepath : path;
        if (savePath.empty()) {
            spdlog::error("Brak sciezki do YAML");
            return false;
        }

        std::ofstream fout(savePath);
        if (!fout.is_open()) {
            spdlog::error("Nie mozna otworzyc pliku do zapisu: {}", savePath);
            return false;
        }

        YAML::Emitter emitter;
        emitter << rootNode;
        fout << emitter.c_str();

        spdlog::info("Zapisano YAML: {}", savePath);
        return true;
    }

    // glebokie szukanie z wartoscia domyslna jesli nie znajdzie
    // np.
    // get<int>({ "Window", "Resolution", "Width" }, 1280);
    // dla 
    // 
    // Window:
    //      Resolution:
    //          Width: 1920
    //          Height : 1080
    
    template <typename T>
    T get(const std::vector<std::string>& keys, const T& defaultValue = T()) {
        if (keys.empty()) return defaultValue;

        YAML::Node currentNode = rootNode;
        for (const auto& key : keys) {
            if (!currentNode || !currentNode[key]) {
                return defaultValue;
            }

            currentNode.reset(currentNode[key]);
        }

        return currentNode.as<T>(defaultValue);
    }

    template <typename T>
    void set(const std::vector<std::string>& keys, const T& value) {
        if (keys.empty()) return;

        YAML::Node currentNode = rootNode;
        for (size_t i = 0; i < keys.size() - 1; ++i) {
            if (!currentNode[keys[i]] || currentNode[keys[i]].IsNull()) {
                currentNode[keys[i]] = YAML::Node(YAML::NodeType::Map);
            }
            currentNode.reset(currentNode[keys[i]]);
        }
        currentNode[keys.back()] = value;
    }

    YAML::Node& getRoot() {
        return rootNode;
    }
};

#endif