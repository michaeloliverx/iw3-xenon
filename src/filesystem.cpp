#include <xtl.h>
#include <cstdint>
#include <string>
#include <direct.h>
#include <fstream>

#include "xboxkrnl.h"

namespace filesystem
{
    void create_nested_dirs(const char *path)
    {
        char temp_path[256];
        strncpy(temp_path, path, sizeof(temp_path));

        char *p = temp_path;
        while (*p)
        {
            if (*p == '\\' || *p == '/')
            {
                *p = '\0';

                // Create the directory if it doesn't exist
                _mkdir(temp_path);

                *p = '\\';
            }
            p++;
        }

        _mkdir(temp_path);
    }

    /**
     * Writes data to a file on disk.
     *
     * @param file_path The full path to the file to write.
     * @param data The data to write.
     * @param data_size The size of the data in bytes.
     * @return 1 if successful, 0 if failed.
     */
    int write_file_to_disk(const char *file_path, const char *data, size_t data_size)
    {
        if (!file_path || !data || data_size == 0)
        {
            xbox::DbgPrint("write_file_to_disk: Invalid parameters!\n");
            return 0;
        }

        // Ensure the directory exists
        char dir_path[256];
        strncpy(dir_path, file_path, sizeof(dir_path));
        char *last_slash = strrchr(dir_path, '\\');
        if (last_slash)
        {
            *last_slash = '\0';
            create_nested_dirs(dir_path);
        }

        // Write data to file
        FILE *file = fopen(file_path, "wb");
        if (file)
        {
            fwrite(data, 1, data_size, file);
            fclose(file);
            xbox::DbgPrint("Successfully wrote file: %s\n", file_path);
            return 1;
        }
        else
        {
            xbox::DbgPrint("Failed to write file: %s\n", file_path);
            return 0;
        }
    }

    // Function to check if a file exists
    bool file_exists(const std::string &file_path)
    {
        std::ifstream file(file_path.c_str());
        return file.good();
    }

    // Function to read a file's contents into a string
    std::string read_file_to_string(const std::string &file_path)
    {
        std::ifstream file(file_path, std::ios::binary);
        if (!file)
        {
            xbox::DbgPrint("read_file_to_string: Failed to open file: %s\n", file_path.c_str());
            return "";
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return content;
    }
}
