module;
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
} // namespace str
