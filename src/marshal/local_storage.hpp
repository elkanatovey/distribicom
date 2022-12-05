
#include <fstream>
#include <filesystem>

std::string load_from_file(const std::filesystem::path &filename);

void dump_to_file(const std::filesystem::path &filename, const std::string &content);
