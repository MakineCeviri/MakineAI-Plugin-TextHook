#pragma once

#include <string>
#include <unordered_map>
#include <fstream>

namespace texthook {

// Plugin settings — persisted as key=value in dataPath/settings.txt
class Settings {
public:
    void load(const std::string& path) {
        m_path = path;
        std::ifstream ifs(path);
        if (!ifs) return;
        std::string line;
        while (std::getline(ifs, line)) {
            auto eq = line.find('=');
            if (eq != std::string::npos)
                m_values[line.substr(0, eq)] = line.substr(eq + 1);
        }
    }

    void save() {
        if (m_path.empty()) return;
        std::ofstream ofs(m_path);
        for (const auto& [k, v] : m_values)
            ofs << k << "=" << v << "\n";
    }

    std::string get(const std::string& key, const std::string& def = "") const {
        auto it = m_values.find(key);
        return it != m_values.end() ? it->second : def;
    }

    void set(const std::string& key, const std::string& value) {
        m_values[key] = value;
    }

private:
    std::unordered_map<std::string, std::string> m_values;
    std::string m_path;
};

} // namespace texthook
