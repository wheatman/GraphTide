#include "nlohmann/json.hpp"
#include "parlay/parallel.h"
#include "parlay/primitives.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>

using json = nlohmann::json;
namespace fs = std::filesystem;

bool StartsWith(const std::string &str, const std::string &prefix) {
  if (prefix.length() > str.length())
    return false;
  return str.substr(0, prefix.length()) == prefix;
}

bool EndsWith(const std::string &str, const std::string &suffix) {
  if (suffix.length() > str.length())
    return false;
  return str.substr(str.length() - suffix.length(), suffix.length()) == suffix;
}

std::string ReadFileContent(const std::string &path) {
  std::ifstream in(path);
  in.seekg(0, std::ios::end); // Go to end of file
  size_t length = in.tellg(); // Get file size
  in.seekg(0, std::ios::beg); // Go back to beginning

  if (length > 0) {
    std::string contents;
    contents.resize(length);       // Pre-allocate memory
    in.read(&contents[0], length); // Read directly into string's buffer
    return contents;
  } else {
    return ""; // Handle empty file
  }
}

parlay::sequence<std::optional<std::pair<size_t, size_t>>>
ParsePaperFile(const std::string &path) {
  std::string s = ReadFileContent(path);
  auto ids = parlay::pack(
      parlay::delayed_tabulate(s.length(), [&](size_t i) { return i; }),
      parlay::delayed_tabulate(s.length(),
                               [&](size_t i) { return s[i] == '\n'; }));
  auto lines = parlay::delayed_tabulate(ids.size(), [&](size_t i) {
    size_t st = (i == 0) ? 0 : ids[i - 1] + 1;
    size_t ed = ids[i];
    return s.substr(st, ed - st);
  });
  parlay::sequence<std::optional<std::pair<size_t, size_t>>> a(lines.size());
  parlay::parallel_for(0, lines.size(), [&](size_t i) {
    json j = json::parse(lines[i]);
    if (j.contains("corpusid") && j["corpusid"].is_number() &&
        j.contains("year") && j["year"].is_number()) {
      a[i] = {j["corpusid"], j["year"]};
    } else {
      a[i] = {};
    }
  });
  return a;
}

parlay::sequence<std::optional<std::pair<size_t, size_t>>>
ParseCitationFile(const std::string &path) {
  std::string s = ReadFileContent(path);
  auto ids = parlay::pack(
      parlay::delayed_tabulate(s.length(), [&](size_t i) { return i; }),
      parlay::delayed_tabulate(s.length(),
                               [&](size_t i) { return s[i] == '\n'; }));
  auto lines = parlay::delayed_tabulate(ids.size(), [&](size_t i) {
    size_t st = (i == 0) ? 0 : ids[i - 1] + 1;
    size_t ed = ids[i];
    return s.substr(st, ed - st);
  });
  parlay::sequence<std::optional<std::pair<size_t, size_t>>> a(lines.size());
  parlay::parallel_for(0, lines.size(), [&](size_t i) {
    json j = json::parse(lines[i]);
    if (j.contains("citingcorpusid") && j["citingcorpusid"].is_number() &&
        j.contains("citedcorpusid") && j["citedcorpusid"].is_number()) {
      a[i] = {j["citingcorpusid"], j["citedcorpusid"]};
    } else {
      a[i] = {};
    }
  });
  return a;
}

int main(int argc, char **argv) {

  assert(argc == 2 && "Usage: parse <path>");
  std::string path = argv[1];

  parlay::sequence<parlay::sequence<std::optional<std::pair<size_t, size_t>>>>
      paper_lists;
  for (const auto &entry : fs::recursive_directory_iterator(
           path, fs::directory_options::skip_permission_denied)) {
    if (fs::is_regular_file(entry.status())) {
      std::string file_name = entry.path().filename().string();
      std::string file_path = entry.path().string();
      if (StartsWith(file_name, "paper_file") && !EndsWith(file_name, ".gz")) {
        std::cout << "Parsing file: " << file_name << std::endl;
        auto papers = ParsePaperFile(file_path);
        paper_lists.push_back(papers);
      }
    }
  }
  auto papers = parlay::flatten(paper_lists);
  std::unordered_map<size_t, size_t> paper_map;
  for (const auto &p : papers) {
    if (p.has_value()) {
      paper_map[p->first] = p->second;
    }
  }

  std::ofstream out(fs::path(path) / "semantic_scholar.csv");
  out << "source,dest,year\n";
  for (const auto &entry : fs::recursive_directory_iterator(
           path, fs::directory_options::skip_permission_denied)) {
    if (fs::is_regular_file(entry.status())) {
      std::string file_name = entry.path().filename().string();
      std::string file_path = entry.path().string();
      if (StartsWith(file_name, "citation_file") &&
          !EndsWith(file_name, ".gz")) {
        std::cout << "Parsing file: " << file_name << std::endl;
        auto citations = ParseCitationFile(file_path);
        auto years = parlay::tabulate(
            citations.size(), [&](size_t i) -> std::optional<size_t> {
              if (citations[i].has_value()) {
                auto it = paper_map.find(citations[i]->first);
                if (it != paper_map.end())
                  return it->second;
              }
              return {};
            });
        for (size_t i = 0; i < citations.size(); i++) {
          if (citations[i].has_value() && years[i].has_value()) {
            out << citations[i]->first << "," << citations[i]->second << ","
                << years[i].value() << "\n";
          }
        }
      }
    }
  }
  out.close();
  std::cout << "Parsing completed. Output written to semantic_scholar.csv\n";

  return 0;
}