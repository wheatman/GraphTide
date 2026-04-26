#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum class DataType : uint8_t {
    NONE = 0,
    UINT32 = 12,
    FLOAT64 = 31,
};

enum class UpdateInfo : uint8_t {
    INSERT = 0,
    DELETE = 1,
};

struct GraphHeaderBinary {
    uint64_t num_vertices = 0;
    uint64_t num_unique_edges = 0;
    uint64_t total_updates = 0;
    uint8_t if_signal_graph = 0;
    uint8_t vertex_id_type = static_cast<uint8_t>(DataType::UINT32);
    uint8_t timestamp_type = static_cast<uint8_t>(DataType::UINT32);
    uint8_t if_directed = 1;
    uint8_t weight_type = static_cast<uint8_t>(DataType::NONE);
    uint8_t padding[3] = {0, 0, 0};
};

static_assert(sizeof(GraphHeaderBinary) == 32, "Graph header must be 32 bytes");

struct HeaderCounts {
    uint64_t num_vertices = 0;
    uint64_t num_unique_edges = 0;
    uint64_t total_updates = 0;
};

inline void trim(std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    if (start != 0 || end != s.size()) {
        s = s.substr(start, end - start);
    }
}

void parse_header_line(const std::string& line, HeaderCounts& counts) {
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, ',')) {
        trim(token);
        if (!token.empty()) tokens.push_back(token);
    }
    if (tokens.size() != 3) {
        throw std::runtime_error("Header line must contain exactly 3 values: " + line);
    }
    try {
        counts.num_vertices = std::stoull(tokens[0]);
        counts.num_unique_edges = std::stoull(tokens[1]);
        counts.total_updates = std::stoull(tokens[2]);
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to parse numeric values in header line: " + line);
    }
}

struct UpdateRow {
    uint32_t u = 0;
    uint32_t v = 0;
    uint32_t timestamp = 0;
    double weight = 0.0;
    UpdateInfo info = UpdateInfo::INSERT;
};

void parse_update_line(const std::string& line, UpdateRow& row, bool& is_modify) {
    is_modify = false;
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, ',')) {
        trim(token);
        if (!token.empty()) tokens.push_back(token);
    }
    if (tokens.empty()) {
        throw std::runtime_error("Encountered empty line.");
    }
    if (tokens.size() < 4) {
        throw std::runtime_error("Update line must contain at least 4 columns: " + line);
    }
    try {
        uint64_t u64 = std::stoull(tokens[0]);
        uint64_t v64 = std::stoull(tokens[1]);
        uint64_t ts64 = std::stoull(tokens[2]);
        if (u64 > std::numeric_limits<uint32_t>::max() ||
            v64 > std::numeric_limits<uint32_t>::max() ||
            ts64 > std::numeric_limits<uint32_t>::max()) {
            throw std::out_of_range("value too large for uint32_t");
        }
        row.u = static_cast<uint32_t>(u64);
        row.v = static_cast<uint32_t>(v64);
        row.timestamp = static_cast<uint32_t>(ts64);
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to parse numeric values in update line: " + line);
    }
    std::string op = tokens[3];
    for (char& c : op) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (op == "insert") {
        row.info = UpdateInfo::INSERT;
        if (tokens.size() >= 5) {
            try {
                row.weight = std::stod(tokens[4]);
            } catch (const std::exception&) {
                throw std::runtime_error("Failed to parse weight for insert line: " + line);
            }
        } else {
            row.weight = 1.0;
        }
        is_modify = false;
    } else if (op == "delete") {
        row.info = UpdateInfo::DELETE;
        row.weight = 0.0;
        is_modify = false;
    } else if (op == "modify" || op == "modification") {
        is_modify = true;
        return;
    } else {
        throw std::runtime_error("Unknown operation in update line: " + line);
    }
}

void usage() {
    std::cerr << "Usage: ./csv_to_bin updates.csv output.bin" << std::endl;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        usage();
        return 1;
    }
    try {
        const std::string input_csv = argv[1];
        const std::string output_bin = argv[2];

        std::ifstream csv(input_csv);
        if (!csv.is_open()) {
            std::cerr << "Failed to open input CSV: " << input_csv << std::endl;
            return 1;
        }

        std::string header_line;
        HeaderCounts counts;
        bool header_parsed = false;
        while (std::getline(csv, header_line)) {
            trim(header_line);
            if (header_line.empty()) continue;
            parse_header_line(header_line, counts);
            header_parsed = true;
            break;
        }
        if (!header_parsed) {
            throw std::runtime_error("Missing valid header in CSV.");
        }

        uint64_t actual_updates = 0;
        uint64_t skipped_modifications = 0;
        bool has_weights = false;
        bool weight_detected = false;
        std::string line;
        UpdateRow row;
        bool is_modify = false;
        std::streampos data_start = csv.tellg();
        while (std::getline(csv, line)) {
            trim(line);
            if (line.empty()) continue;
            parse_update_line(line, row, is_modify);
            if (is_modify) {
                skipped_modifications++;
                continue;
            }
            if (!weight_detected && row.info == UpdateInfo::INSERT) {
                std::stringstream ss(line);
                std::string token;
                int cols = 0;
                while (std::getline(ss, token, ',')) cols++;
                has_weights = (cols >= 5);
                weight_detected = true;
            }
            actual_updates++;
        }

        csv.clear();
        csv.seekg(data_start);

        std::ofstream bin(output_bin, std::ios::binary);
        if (!bin.is_open()) {
            std::cerr << "Failed to open output binary file: " << output_bin << std::endl;
            return 1;
        }

        GraphHeaderBinary header{};
        header.num_vertices = counts.num_vertices;
        header.num_unique_edges = counts.num_unique_edges;
        header.total_updates = actual_updates;
        header.weight_type = has_weights
            ? static_cast<uint8_t>(DataType::FLOAT64)
            : static_cast<uint8_t>(DataType::NONE);
        bin.write(reinterpret_cast<const char*>(&header), sizeof(header));

        std::cout << "Weight type: " << (has_weights ? "FLOAT64" : "NONE") << std::endl;

        csv.clear();
        csv.seekg(data_start);
        while (std::getline(csv, line)) {
            trim(line);
            if (line.empty()) continue;
            parse_update_line(line, row, is_modify);
            if (is_modify) continue;
            bin.write(reinterpret_cast<const char*>(&row.u), sizeof(row.u));
            bin.write(reinterpret_cast<const char*>(&row.v), sizeof(row.v));
            bin.write(reinterpret_cast<const char*>(&row.timestamp), sizeof(row.timestamp));
            if (has_weights) {
                bin.write(reinterpret_cast<const char*>(&row.weight), sizeof(row.weight));
            }
            uint8_t info = static_cast<uint8_t>(row.info);
            bin.write(reinterpret_cast<const char*>(&info), sizeof(info));
        }
        bin.close();

        std::cout << "Wrote " << actual_updates << " updates to " << output_bin
                  << " (skipped " << skipped_modifications << " modifications)." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
