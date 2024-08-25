module;
#include <map>
#include <set>
#include <string>

export module strset;

// "strset" conflicts in Windows
namespace str {
export class set {
  std::set<std::string> m_data{};

public:
  bool insert(std::string s) {
    auto [_, i] = m_data.insert(s);
    return i;
  }

  [[nodiscard]] auto begin() { return m_data.begin(); }
  [[nodiscard]] auto end() { return m_data.end(); }
};

export class map {
  std::map<std::string, uint64_t> m_data{};

public:
  constexpr auto & operator[](std::string key) { return m_data[key]; }
};
} // namespace str
