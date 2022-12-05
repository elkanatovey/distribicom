#include "local_storage.hpp"


std::string load_from_file(const std::filesystem::path &filename) {
    std::ifstream ifs(filename);

    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));

    ifs.close();
    return content;
}

void dump_to_file(const std::filesystem::path &filename, const std::string &content) {

    std::filesystem::create_directories(filename.parent_path());

    std::ofstream ofs(filename);
    ofs << content;
    ofs.close();
}
