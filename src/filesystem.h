#include <cstdint>
#include <string>

namespace filesystem
{
    void create_nested_dirs(const char *path);
    int write_file_to_disk(const char *file_path, const char *data, size_t data_size);
    bool file_exists(const std::string &file_path);
    std::string read_file_to_string(const std::string &file_path);
}
