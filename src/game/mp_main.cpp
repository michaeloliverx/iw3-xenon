#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>

#include <xtl.h>
#include <xbox.h>
#include <xgraphics.h>

#include "mp_main.h"
#include "mp_structs.h"

#include "../detour.h"
#include "../filesystem.h"
#include "../xboxkrnl.h"

extern "C"
{
    void DbgPrint(const char *format, ...);
}

namespace
{
    uint32_t ShowKeyboard(const wchar_t *title, const wchar_t *description, const wchar_t *defaultText, std::string &result, size_t maxLength, uint32_t keyboardType)
    {
        size_t realMaxLength = maxLength + 1;
        XOVERLAPPED overlapped = {};

        std::vector<wchar_t> wideBuffer(realMaxLength);
        std::vector<char> buffer(realMaxLength);

        XShowKeyboardUI(0, keyboardType, defaultText, title, description, wideBuffer.data(), realMaxLength, &overlapped);

        while (!XHasOverlappedIoCompleted(&overlapped))
            Sleep(100);

        if (XGetOverlappedResult(&overlapped, nullptr, TRUE) == ERROR_SUCCESS)
        {
            wcstombs_s(nullptr, buffer.data(), realMaxLength, wideBuffer.data(), realMaxLength * sizeof(wchar_t));
            result = buffer.data();
            return ERROR_SUCCESS;
        }

        return ERROR_CANCELLED;
    }
}

std::vector<uint8_t> Xbox360ConvertToLinearTexture(const std::vector<uint8_t> &data, int pixelWidth, int pixelHeight, GPUTEXTUREFORMAT textureFormat)
{
    std::vector<uint8_t> destData(data.size());
    uint32_t blockPixelSize;
    uint32_t texelBytePitch;

    switch (textureFormat)
    {
    case GPUTEXTUREFORMAT_8: // LinearPaletteIndex8bpp:
        blockPixelSize = 1;
        texelBytePitch = 1;
        break;
    case GPUTEXTUREFORMAT_8_8:
        blockPixelSize = 1;
        texelBytePitch = 2;
        break;
    case GPUTEXTUREFORMAT_8_8_8_8: // {b8,g8,r8,ap8}
        blockPixelSize = 1;
        texelBytePitch = 4;
        break;
    case GPUTEXTUREFORMAT_DXT1: // Bc1Dxt1
        blockPixelSize = 4;
        texelBytePitch = 8;
        break;
    case GPUTEXTUREFORMAT_DXT2_3: // Bc2Dxt2 & Bc2Dxt3
    case GPUTEXTUREFORMAT_DXT4_5: // Bc3Dxt4 & Bc3Dxt5
    case GPUTEXTUREFORMAT_DXN:
        blockPixelSize = 4;
        texelBytePitch = 16;
        break;
    default:
        throw std::invalid_argument("Bad texture type!");
    }

    // Width and height in number of blocks.
    // So a 256x128 DXT1 image would be 64x32 in 4x4 blocks.
    uint32_t widthInBlocks = pixelWidth / blockPixelSize;
    uint32_t heightInBlocks = pixelHeight / blockPixelSize;

    // This loops in terms of the swizzled source.
    for (uint32_t j = 0; j < heightInBlocks; j++)
    {
        for (uint32_t i = 0; i < widthInBlocks; i++)
        {
            uint32_t blockOffset = j * widthInBlocks + i;
            uint32_t x = XGAddress2DTiledX(blockOffset, widthInBlocks, texelBytePitch);
            uint32_t y = XGAddress2DTiledY(blockOffset, widthInBlocks, texelBytePitch);
            uint32_t srcByteOffset = j * widthInBlocks * texelBytePitch + i * texelBytePitch;
            uint32_t destByteOffset = y * widthInBlocks * texelBytePitch + x * texelBytePitch;

            if (destByteOffset + texelBytePitch > destData.size())
                continue;
            memcpy(&destData[destByteOffset], &data[srcByteOffset], texelBytePitch);
        }
    }

    return destData;
}

// DDS Constants
const uint32_t DDS_MAGIC = 0x20534444; // 'DDS ' in hex (must be little-endian)
const uint32_t DDS_HEADER_SIZE = 124;
const uint32_t DDS_PIXEL_FORMAT_SIZE = 32;
const uint32_t DDSD_CAPS = 0x1;
const uint32_t DDSD_HEIGHT = 0x2;
const uint32_t DDSD_WIDTH = 0x4;
const uint32_t DDSD_PIXELFORMAT = 0x1000;
const uint32_t DDSD_LINEARSIZE = 0x80000;
const uint32_t DDPF_FOURCC = 0x4;
const uint32_t DDPF_RGB = 0x40;
const uint32_t DDPF_ALPHAPIXELS = 0x1;
const uint32_t DDSCAPS_TEXTURE = 0x1000;
const uint32_t DDSCAPS_MIPMAP = 0x400000;
const uint32_t DDPF_LUMINANCE = 0x20000;

// DDS Pixel Formats (FourCC Codes)
const uint32_t DXT1_FOURCC = 0x31545844; // 'DXT1'
const uint32_t DXT3_FOURCC = 0x33545844; // 'DXT3'
const uint32_t DXT5_FOURCC = 0x35545844; // 'DXT5'
const uint32_t DXN_FOURCC = 0x32495441;  // 'ATI2' (DXN / BC5)

// Additional DDS Cubemap Flags
const uint32_t DDSCAPS2_CUBEMAP = 0x200;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEX = 0x400;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000;

// DDS Header Structure (with inline endian swapping)
struct DDSHeader
{
    uint32_t magic;
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];
    struct
    {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t rgbBitCount;
        uint32_t rBitMask;
        uint32_t gBitMask;
        uint32_t bBitMask;
        uint32_t aBitMask;
    } pixelFormat;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

static_assert(sizeof(DDSHeader) == 128, "");

struct DDSImage
{
    DDSHeader header;
    std::vector<uint8_t> data;
};

// Function to swap all necessary fields from little-endian to big-endian
void SwapDDSHeaderEndian(DDSHeader &header)
{
    header.magic = _byteswap_ulong(header.magic);
    header.size = _byteswap_ulong(header.size);
    header.flags = _byteswap_ulong(header.flags);
    header.height = _byteswap_ulong(header.height);
    header.width = _byteswap_ulong(header.width);
    header.pitchOrLinearSize = _byteswap_ulong(header.pitchOrLinearSize);
    header.depth = _byteswap_ulong(header.depth);
    header.mipMapCount = _byteswap_ulong(header.mipMapCount);

    for (int i = 0; i < 11; i++)
        header.reserved1[i] = _byteswap_ulong(header.reserved1[i]);

    header.pixelFormat.size = _byteswap_ulong(header.pixelFormat.size);
    header.pixelFormat.flags = _byteswap_ulong(header.pixelFormat.flags);
    header.pixelFormat.fourCC = _byteswap_ulong(header.pixelFormat.fourCC);
    header.pixelFormat.rgbBitCount = _byteswap_ulong(header.pixelFormat.rgbBitCount);
    header.pixelFormat.rBitMask = _byteswap_ulong(header.pixelFormat.rBitMask);
    header.pixelFormat.gBitMask = _byteswap_ulong(header.pixelFormat.gBitMask);
    header.pixelFormat.bBitMask = _byteswap_ulong(header.pixelFormat.bBitMask);
    header.pixelFormat.aBitMask = _byteswap_ulong(header.pixelFormat.aBitMask);

    header.caps = _byteswap_ulong(header.caps);
    header.caps2 = _byteswap_ulong(header.caps2);
    header.caps3 = _byteswap_ulong(header.caps3);
    header.caps4 = _byteswap_ulong(header.caps4);
    header.reserved2 = _byteswap_ulong(header.reserved2);
}

DDSImage ReadDDSFile(const std::string &filepath)
{
    DDSImage ddsImage;
    std::ifstream file(filepath, std::ios::binary);

    if (!file.is_open())
    {
        DbgPrint("ERROR: Unable to open file: %s\n", filepath.c_str());
        return ddsImage; // Return empty DDSImage
    }

    // Read DDS header (raw, little-endian)
    file.read(reinterpret_cast<char *>(&ddsImage.header), sizeof(DDSHeader));

    // Swap only the magic number to big-endian for proper validation
    uint32_t magicSwapped = _byteswap_ulong(ddsImage.header.magic);

    if (magicSwapped != 0x20534444) // 'DDS ' in big-endian
    {
        DbgPrint("ERROR: Invalid DDS file: %s\n", filepath.c_str());
        file.close();
        return ddsImage;
    }

    // Swap header fields to big-endian for Xbox 360
    SwapDDSHeaderEndian(ddsImage.header);

    // Move to end of file to get total file size
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();

    // Ensure fileSize is valid before proceeding
    if (fileSize == std::streampos(-1))
    {
        DbgPrint("ERROR: Failed to determine file size.\n");
        file.close();
        return ddsImage;
    }

    // Move back to after the header
    file.seekg(sizeof(DDSHeader), std::ios::beg);

    // Compute data size safely
    size_t dataSize = static_cast<size_t>(fileSize) - sizeof(DDSHeader);

    // Read image data
    ddsImage.data.resize(dataSize);
    file.read(reinterpret_cast<char *>(ddsImage.data.data()), dataSize);

    file.close();

    // Debug output
    DbgPrint("INFO: DDS file '%s' loaded successfully.\n", filepath.c_str());
    DbgPrint("      Resolution: %ux%u\n", ddsImage.header.width, ddsImage.header.height);
    DbgPrint("      MipMaps: %u\n", ddsImage.header.mipMapCount);
    DbgPrint("      Data Size: %u bytes\n", static_cast<unsigned int>(dataSize));

    return ddsImage;
}

std::vector<uint8_t> Xbox360ConvertToTiledTexture(const std::vector<uint8_t> &linearData, int pixelWidth, int pixelHeight, GPUTEXTUREFORMAT textureFormat)
{
    std::vector<uint8_t> tiledData(linearData.size());
    uint32_t blockPixelSize;
    uint32_t texelBytePitch;

    switch (textureFormat)
    {
    case GPUTEXTUREFORMAT_8: // LinearPaletteIndex8bpp:
        blockPixelSize = 1;
        texelBytePitch = 1;
        break;
    case GPUTEXTUREFORMAT_8_8:
        blockPixelSize = 1;
        texelBytePitch = 2;
        break;
    case GPUTEXTUREFORMAT_8_8_8_8: // {b8,g8,r8,ap8}
        blockPixelSize = 1;
        texelBytePitch = 4;
        break;
    case GPUTEXTUREFORMAT_DXT1: // Bc1Dxt1
        blockPixelSize = 4;
        texelBytePitch = 8;
        break;
    case GPUTEXTUREFORMAT_DXT2_3: // Bc2Dxt2 & Bc2Dxt3
    case GPUTEXTUREFORMAT_DXT4_5: // Bc3Dxt4 & Bc3Dxt5
    case GPUTEXTUREFORMAT_DXN:
        blockPixelSize = 4;
        texelBytePitch = 16;
        break;
    default:
        throw std::invalid_argument("Bad texture type!");
    }

    uint32_t widthInBlocks = pixelWidth / blockPixelSize;
    uint32_t heightInBlocks = pixelHeight / blockPixelSize;

    // The function should operate in reverse of `Xbox360ConvertToLinearTexture`
    for (uint32_t y = 0; y < heightInBlocks; y++)
    {
        for (uint32_t x = 0; x < widthInBlocks; x++)
        {
            uint32_t blockOffset = y * widthInBlocks + x;

            // Get the *tiled* coordinates from a linear block offset
            uint32_t tiledX = XGAddress2DTiledX(blockOffset, widthInBlocks, texelBytePitch);
            uint32_t tiledY = XGAddress2DTiledY(blockOffset, widthInBlocks, texelBytePitch);

            // Compute the correct offsets (swap src and dest logic)
            uint32_t srcByteOffset = tiledY * widthInBlocks * texelBytePitch + tiledX * texelBytePitch;
            uint32_t destByteOffset = y * widthInBlocks * texelBytePitch + x * texelBytePitch;

            if (srcByteOffset + texelBytePitch > linearData.size() || destByteOffset + texelBytePitch > tiledData.size())
                continue;

            memcpy(&tiledData[destByteOffset], &linearData[srcByteOffset], texelBytePitch);
        }
    }

    return tiledData;
}

#define INVALID_FILE_ATTRIBUTES -1

bool FileExists(const char *filepath)
{
    DWORD attributes = GetFileAttributesA(filepath);

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return false;
    }

    return true;
}

std::vector<std::string> ListFilesInDirectory(const std::string &directory)
{
    std::vector<std::string> filenames;
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((directory + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        DbgPrint("ERROR: Directory '%s' does not exist or cannot be accessed.\n", directory.c_str());
        return filenames; // Return empty vector
    }

    do
    {
        // Ignore "." and ".." and only include regular files (not directories)
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::string filename(findFileData.cFileName);
            filenames.push_back(filename);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);

    return filenames;
}

namespace mp
{
    // Functions
    Cbuf_AddText_t Cbuf_AddText = reinterpret_cast<Cbuf_AddText_t>(0x82239FD0);

    CL_ConsolePrint_t CL_ConsolePrint = reinterpret_cast<CL_ConsolePrint_t>(0x822E4D18);
    CL_GamepadButtonEvent_t CL_GamepadButtonEvent = reinterpret_cast<CL_GamepadButtonEvent_t>(0x822DD1E8);

    Cmd_AddCommandInternal_t Cmd_AddCommandInternal = reinterpret_cast<Cmd_AddCommandInternal_t>(0x8223ADE0);

    Com_Printf_t Com_Printf = reinterpret_cast<Com_Printf_t>(0x82237000);
    Com_PrintError_t Com_PrintError = reinterpret_cast<Com_PrintError_t>(0x82235C50);
    Com_PrintWarning_t Com_PrintWarning = reinterpret_cast<Com_PrintWarning_t>(0x822356B8);

    DB_EnumXAssets_FastFile_t DB_EnumXAssets_FastFile = reinterpret_cast<DB_EnumXAssets_FastFile_t>(0x8229ED48);
    DB_FindXAssetHeader_t DB_FindXAssetHeader = reinterpret_cast<DB_FindXAssetHeader_t>(0x822A0298);
    DB_GetAllXAssetOfType_FastFile_t DB_GetAllXAssetOfType_FastFile = reinterpret_cast<DB_GetAllXAssetOfType_FastFile_t>(0x8229E8E0);

    Load_MapEntsPtr_t Load_MapEntsPtr = reinterpret_cast<Load_MapEntsPtr_t>(0x822A9648);

    R_DownsampleMipMapBilinear_t R_DownsampleMipMapBilinear = reinterpret_cast<R_DownsampleMipMapBilinear_t>(0x82133800);
    R_GetImageList_t R_GetImageList = reinterpret_cast<R_GetImageList_t>(0x82152A58);
    R_StreamLoadFileSynchronously_t R_StreamLoadFileSynchronously = reinterpret_cast<R_StreamLoadFileSynchronously_t>(0x82151510);

    Scr_ReadFile_FastFile_t Scr_ReadFile_FastFile = reinterpret_cast<Scr_ReadFile_FastFile_t>(0x82221220);

    // Variables
    auto cmd_functions = reinterpret_cast<cmd_function_s *>(0x82A2335C);

    Detour CL_ConsolePrint_Detour;

    void CL_ConsolePrint_Hook(int localClientNum, int channel, const char *txt, int duration, int pixelWidth, int flags)
    {
        CL_ConsolePrint_Detour.GetOriginal<decltype(CL_ConsolePrint)>()(localClientNum, channel, txt, duration, pixelWidth, flags);
        xbox::DbgPrint("CL_ConsolePrint txt=%s \n", txt);
    }

    Detour CL_GamepadButtonEvent_Detour;

    void CL_GamepadButtonEvent_Hook(int localClientNum, int controllerIndex, int key, int down, unsigned int time)
    {
        // xbox::DbgPrint("CL_GamepadButtonEvent localClientNum=%d controllerIndex=%d key=%d down=%d time=%d\n", localClientNum, controllerIndex, key, down, time);
        CL_GamepadButtonEvent_Detour.GetOriginal<decltype(CL_GamepadButtonEvent)>()(localClientNum, controllerIndex, key, down, time);

        if (key == K_BUTTON_RSTICK && down)
        {
            std::string value;
            auto result = ShowKeyboard(L"Enter command", L"Description", L"", value, 100, 0);
            if (result == ERROR_SUCCESS)
            {
                xbox::DbgPrint("ShowKeyboard result: %s\n", value.c_str());
                Cbuf_AddText(0, value.c_str());
            }
            else
            {
                xbox::DbgPrint("ShowKeyboard cancelled.\n");
            }
        }
    }

    Detour Load_MapEntsPtr_Detour;

    void Load_MapEntsPtr_Hook()
    {
        // TODO: don't write null byte to file
        // and add null byte to entityString when reading from file

        xbox::DbgPrint("Load_MapEntsPtr_Hook\n");

        // TODO: write comment what this is ***
        // Get pointer to pointer stored at 0x82475914
        MapEnts **varMapEntsPtr = *(MapEnts ***)0x82475914;

        Load_MapEntsPtr_Detour.GetOriginal<decltype(&Load_MapEntsPtr_Hook)>()();

        // Validate pointer before dereferencing
        if (varMapEntsPtr && *varMapEntsPtr)
        {
            MapEnts *mapEnts = *varMapEntsPtr;

            // Write stock map ents to disk
            std::string file_path = "game:\\dump\\";
            file_path += mapEnts->name;
            file_path += ".ents";                                        //  iw4x naming convention
            std::replace(file_path.begin(), file_path.end(), '/', '\\'); // Replace forward slashes with backslashes
            filesystem::write_file_to_disk(file_path.c_str(), mapEnts->entityString, mapEnts->numEntityChars);

            // Load map ents from file
            // Path to check for existing entity file
            std::string raw_file_path = "game:\\raw\\";
            raw_file_path += mapEnts->name;
            raw_file_path += ".ents";                                            // IW4x naming convention
            std::replace(raw_file_path.begin(), raw_file_path.end(), '/', '\\'); // Replace forward slashes with backslashes

            // If the file exists, replace entityString
            if (filesystem::file_exists(raw_file_path))
            {
                xbox::DbgPrint("Found entity file: %s\n", raw_file_path.c_str());
                std::string new_entity_string = filesystem::read_file_to_string(raw_file_path);
                if (!new_entity_string.empty())
                {
                    // Allocate new memory and copy the data
                    size_t new_size = new_entity_string.size() + 1; // Include null terminator
                    char *new_memory = static_cast<char *>(malloc(new_size));

                    if (new_memory)
                    {
                        memcpy(new_memory, new_entity_string.c_str(), new_size); // Copy with null terminator

                        // Update the entityString pointer to point to the new memory
                        mapEnts->entityString = new_memory;

                        // // Update numEntityChars
                        // mapEnts->numEntityChars = static_cast<int>(new_entity_string.size());	// unnecessary

                        xbox::DbgPrint("Replaced entityString from file: %s\n", raw_file_path.c_str());
                    }
                    else
                    {
                        xbox::DbgPrint("Failed to allocate memory for entityString replacement.\n");
                    }
                }
            }
        }
        else
        {
            xbox::DbgPrint("Hooked Load_MapEntsPtr: varMapEntsPtr is NULL or invalid.\n");
        }
    }

    Detour R_StreamLoadFileSynchronously_Detour;

    int R_StreamLoadFileSynchronously_Hook(const char *filename, unsigned int bytesToRead, unsigned __int8 *outData)
    {
        if (strncmp(filename, "D:\\", 3) == 0) // Check if path starts with "D:\"
        {
            char newPath[MAX_PATH];
            _snprintf(newPath, sizeof(newPath), "game:\\raw\\%s", filename + 3); // Change "D:\" to "game:\raw\"

            // Try loading from the modified path first
            if (R_StreamLoadFileSynchronously_Detour.GetOriginal<decltype(R_StreamLoadFileSynchronously)>()(newPath, bytesToRead, outData))
            {
                return 1; // Success with the new path
            }
        }

        // Fallback to original path if modified path failed
        return R_StreamLoadFileSynchronously_Detour.GetOriginal<decltype(R_StreamLoadFileSynchronously)>()(filename, bytesToRead, outData);
    }

    Detour Scr_ReadFile_FastFile_Detour;

    char *Scr_ReadFile_FastFile_Hook(const char *filename, const char *extFilename, const char *codePos, bool archive)
    {
        xbox::DbgPrint("Scr_ReadFile_FastFile_Hook extFilename=%s \n", extFilename);

        std::string raw_file_path = "game:\\raw\\";
        raw_file_path += extFilename;
        std::replace(raw_file_path.begin(), raw_file_path.end(), '/', '\\'); // Replace forward slashes with backslashes
        if (filesystem::file_exists(raw_file_path))
        {
            xbox::DbgPrint("Found raw file: %s\n", raw_file_path.c_str());
            // return ReadFileContents(raw_file_path);
            std::string new_contents = filesystem::read_file_to_string(raw_file_path);
            if (!new_contents.empty())
            {

                // Allocate new memory and copy the data
                size_t new_size = new_contents.size() + 1; // Include null terminator
                char *new_memory = static_cast<char *>(malloc(new_size));

                if (new_memory)
                {
                    memcpy(new_memory, new_contents.c_str(), new_size); // Copy with null terminator

                    xbox::DbgPrint("Replaced contents from file: %s\n", raw_file_path.c_str());
                    return new_memory;
                }
                else
                {
                    xbox::DbgPrint("Failed to allocate memory for contents replacement.\n");
                }
            }
        }

        return Scr_ReadFile_FastFile_Detour.GetOriginal<decltype(&Scr_ReadFile_FastFile_Hook)>()(filename, extFilename, codePos, archive);
    }

    const unsigned int MAX_RAWFILES = 2048;
    struct RawFileList
    {
        unsigned int count;
        RawFile *files[MAX_RAWFILES];
    };

    void R_AddRawFileToList(void *asset, void *inData)
    {
        RawFileList *rawFileList = reinterpret_cast<RawFileList *>(inData);
        RawFile *rawFile = reinterpret_cast<RawFile *>(asset);

        if (!rawFile)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "R_AddRawFileToList: Null RawFile!\n");
            return;
        }

        if (rawFileList->count >= MAX_RAWFILES)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "R_AddRawFileToList: RawFileList is full!\n");
            return;
        }

        rawFileList->files[rawFileList->count++] = rawFile;
    }

    void R_GetRawFileList(RawFileList *rawFileList)
    {
        rawFileList->count = 0;
        DB_EnumXAssets_FastFile(ASSET_TYPE_RAWFILE, R_AddRawFileToList, rawFileList, true);
    }

    void Cmd_rawfilesdump()
    {
        RawFileList rawFileList;
        R_GetRawFileList(&rawFileList);

        Com_Printf(CON_CHANNEL_CONSOLEONLY, "Dumping %d raw files to `raw\\` %d\n", rawFileList.count);

        for (unsigned int i = 0; i < rawFileList.count; i++)
        {
            auto rawfile = rawFileList.files[i];
            std::string asset_name = rawfile->name;
            std::replace(asset_name.begin(), asset_name.end(), '/', '\\'); // Replace forward slashes with backslashes
            filesystem::write_file_to_disk(("game:\\dump\\" + asset_name).c_str(), rawfile->buffer, rawfile->len);
        }
    }

    void Image_DbgPrint(const GfxImage *image)
    {
        const int format = image->texture.basemap->Format.DataFormat;
        char *format_str;
        switch (format)
        {
        case GPUTEXTUREFORMAT_DXT1:
            format_str = "DXT1";
            break;
        case GPUTEXTUREFORMAT_DXT2_3:
            format_str = "DXT2_3";
            break;
        case GPUTEXTUREFORMAT_DXT4_5:
            format_str = "DXT4_5";
            break;
        case GPUTEXTUREFORMAT_DXN:
            format_str = "DXN";
            break;
        case GPUTEXTUREFORMAT_8:
            format_str = "8";
            break;
        case GPUTEXTUREFORMAT_8_8:
            format_str = "8_8";
            break;
        case GPUTEXTUREFORMAT_8_8_8_8:
            format_str = "8_8_8_8";
            break;
        default:
            format_str = "UNKNOWN";
            break;
        }

        XGTEXTURE_DESC SourceDesc;
        XGGetTextureDesc(image->texture.basemap, 0, &SourceDesc);
        BOOL IsBorderTexture = XGIsBorderTexture(image->texture.basemap);
        UINT MipTailBaseLevel = XGGetMipTailBaseLevel(SourceDesc.Width, SourceDesc.Height, IsBorderTexture);

        // SourceDesc.BitsPerPixel;
        // SourceDesc.BytesPerBlock;

        UINT MipLevelCount = image->texture.basemap->GetLevelCount();

        UINT BaseSize;
        XGGetTextureLayout(image->texture.basemap, 0, &BaseSize, 0, 0, 0, 0, 0, 0, 0, 0);

        Com_Printf(CON_CHANNEL_CONSOLEONLY, "Image_DbgPrint: Dumping image Name='%s', Type=%d, Dimensions=%dx%d, MipLevels=%d, MipTailBaseLevel=%d, Format=%s, BitsPerPixel=%d, BytesPerBlock=%d, Endian=%d, BaseSize=%d\n",
                   image->name,
                   image->mapType,
                   image->width,
                   image->height,
                   MipLevelCount,
                   MipTailBaseLevel,
                   format_str,
                   SourceDesc.BitsPerPixel,
                   SourceDesc.BytesPerBlock,
                   image->texture.basemap->Format.Endian,
                   BaseSize);
    }

    void Image_Dump(const GfxImage *image)
    {
        // TODO: cleanup empty files if failed

        Com_Printf(CON_CHANNEL_CONSOLEONLY, "Image_Dump: Dumping image '%s'\n", image->name);

        if (!image)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Null GfxImage!\n");
            return;
        }

        if (!image->pixels || image->baseSize == 0)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Image '%s' has no valid pixel data!\n", image->name);
            return;
        }

        if (image->mapType != MAPTYPE_2D && image->mapType != MAPTYPE_CUBE)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Unsupported map type %d!\n", image->mapType);
            return;
        }

        UINT BaseSize;
        XGGetTextureLayout(image->texture.basemap, 0, &BaseSize, 0, 0, 0, 0, 0, 0, 0, 0);

        DDSHeader header;
        memset(&header, 0, sizeof(DDSHeader));

        header.magic = _byteswap_ulong(DDS_MAGIC);
        header.size = _byteswap_ulong(DDS_HEADER_SIZE);
        header.flags = _byteswap_ulong(DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE);
        header.height = _byteswap_ulong(image->height);
        header.width = _byteswap_ulong(image->width);
        header.depth = _byteswap_ulong(image->depth);
        header.mipMapCount = _byteswap_ulong(image->texture.basemap->GetLevelCount());

        auto format = image->texture.basemap->Format.DataFormat;
        switch (format)
        {
        case GPUTEXTUREFORMAT_DXT1:
            header.pixelFormat.fourCC = _byteswap_ulong(DXT1_FOURCC);
            header.pitchOrLinearSize = BaseSize;
            break;
        case GPUTEXTUREFORMAT_DXT2_3:
            header.pixelFormat.fourCC = _byteswap_ulong(DXT3_FOURCC);
            header.pitchOrLinearSize = BaseSize;
            break;
        case GPUTEXTUREFORMAT_DXT4_5:
            header.pixelFormat.fourCC = _byteswap_ulong(DXT5_FOURCC);
            header.pitchOrLinearSize = BaseSize;
            break;
        case GPUTEXTUREFORMAT_DXN:
            header.pixelFormat.fourCC = _byteswap_ulong(DXN_FOURCC);
            header.pitchOrLinearSize = BaseSize;
            break;
        case GPUTEXTUREFORMAT_8:
            header.pixelFormat.flags = _byteswap_ulong(DDPF_LUMINANCE);
            header.pixelFormat.rgbBitCount = _byteswap_ulong(8);
            header.pixelFormat.rBitMask = _byteswap_ulong(0x000000FF);
            header.pixelFormat.gBitMask = 0;
            header.pixelFormat.bBitMask = 0;
            header.pixelFormat.aBitMask = 0;
            header.pitchOrLinearSize = BaseSize;
            break;
        case GPUTEXTUREFORMAT_8_8:
            header.pixelFormat.flags = _byteswap_ulong(DDPF_LUMINANCE | DDPF_ALPHAPIXELS);
            header.pixelFormat.rgbBitCount = _byteswap_ulong(16);
            header.pixelFormat.rBitMask = _byteswap_ulong(0x000000FF);
            header.pixelFormat.gBitMask = _byteswap_ulong(0x0000FF00);
            header.pixelFormat.bBitMask = 0;
            header.pixelFormat.aBitMask = 0;
            header.pitchOrLinearSize = BaseSize;
            break;
        case GPUTEXTUREFORMAT_8_8_8_8:
            header.pixelFormat.flags = _byteswap_ulong(DDPF_RGB | DDPF_ALPHAPIXELS);
            header.pixelFormat.rgbBitCount = _byteswap_ulong(32);
            header.pixelFormat.rBitMask = _byteswap_ulong(0x00FF0000);
            header.pixelFormat.gBitMask = _byteswap_ulong(0x0000FF00);
            header.pixelFormat.bBitMask = _byteswap_ulong(0x000000FF);
            header.pixelFormat.aBitMask = _byteswap_ulong(0xFF000000);
            header.pitchOrLinearSize = BaseSize;
            break;
        default:
            Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Unsupported texture format %d!\n", format);
            return;
        }

        // Set texture capabilities
        header.caps = _byteswap_ulong(DDSCAPS_TEXTURE | DDSCAPS_MIPMAP);

        // Handle Cubemaps
        if (image->mapType == mp::MAPTYPE_CUBE)
        {
            header.caps2 = _byteswap_ulong(DDSCAPS2_CUBEMAP |
                                           DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX |
                                           DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY |
                                           DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ);
        }

        std::string filename = "game:\\dump\\images\\";
        std::string sanitized_name = image->name;

        // Remove invalid characters
        sanitized_name.erase(std::remove_if(sanitized_name.begin(), sanitized_name.end(), [](char c)
                                            { return c == '*'; }),
                             sanitized_name.end());

        filename += sanitized_name + ".dds";

        std::ofstream file(filename, std::ios::binary);
        if (!file)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Failed to open file: %s\n", filename.c_str());
            return;
        }

        if (image->mapType == MAPTYPE_CUBE)
        {
            file.write(reinterpret_cast<const char *>(&header), sizeof(DDSHeader));

            unsigned int face_size = 0;

            switch (image->texture.basemap->Format.DataFormat)
            {
            case GPUTEXTUREFORMAT_DXT1:
                face_size = (image->width / 4) * (image->height / 4) * 8;
                break;
            case GPUTEXTUREFORMAT_8_8_8_8:
                face_size = image->width * image->height * 4;
                break;
            default:
                Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Unsupported cube map format %d!\n", image->texture.basemap->Format.DataFormat);
                return;
            }

            // TODO: handle mip levels per face for cubemaps
            for (int i = 0; i < 6; i++)
            {
                unsigned char *face_pixels = image->pixels + (i * face_size); // Offset for each face

                // Swap bytes before unswizzling
                std::vector<uint8_t> swappedFace(face_size);
                for (size_t j = 0; j < face_size; j += 2)
                {
                    swappedFace[j] = face_pixels[j + 1];
                    swappedFace[j + 1] = face_pixels[j];
                }

                // Convert swizzled texture to linear layout
                std::vector<uint8_t> linearFace = Xbox360ConvertToLinearTexture(
                    swappedFace, image->width, image->height, static_cast<GPUTEXTUREFORMAT>(image->texture.basemap->Format.DataFormat));

                file.write(reinterpret_cast<const char *>(linearFace.data()), linearFace.size());
            }

            file.close();
        }
        else if (image->mapType == MAPTYPE_2D)
        {
            // TODO: write mip levels
            file.write(reinterpret_cast<const char *>(&header), sizeof(DDSHeader));

            std::vector<uint8_t> pixelData(image->pixels, image->pixels + image->baseSize);

            switch (image->texture.basemap->Format.Endian)
            {
            case GPUENDIAN_8IN16:
                XGEndianSwapMemory(pixelData.data(), pixelData.data(), XGENDIAN_8IN16, 2, pixelData.size() / 2);
                break;
            case GPUENDIAN_8IN32:
                XGEndianSwapMemory(pixelData.data(), pixelData.data(), XGENDIAN_8IN32, 4, pixelData.size() / 4);
                break;
            case GPUENDIAN_16IN32:
                XGEndianSwapMemory(pixelData.data(), pixelData.data(), XGENDIAN_16IN32, 4, pixelData.size() / 4);
                break;
            default:
                break;
            }
            auto linearData = Xbox360ConvertToLinearTexture(pixelData, image->width, image->height, static_cast<GPUTEXTUREFORMAT>(image->texture.basemap->Format.DataFormat));
            file.write(reinterpret_cast<const char *>(linearData.data()), linearData.size());

            file.close();
        }
        else
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Unsupported map type %d!\n", image->mapType);
            return;
        }
    }

    void Cmd_imagedump()
    {
        ImageList imageList;
        R_GetImageList(&imageList);

        // images bundled in xex
        // auto g_imageProgs = reinterpret_cast<GfxImage *>(0x84FEA6D0);
        // for (unsigned int i = 0; i < 10; i++)
        // {
        //     imageList.image[imageList.count++] = &g_imageProgs[i];
        // }

        Com_Printf(CON_CHANNEL_CONSOLEONLY, "Dumping %d images to 'raw\\images\\' %d\n", imageList.count);

        CreateDirectoryA("game:\\dump", 0);
        CreateDirectoryA("game:\\dump\\images", 0);

        for (unsigned int i = 0; i < imageList.count; i++)
        {
            auto image = imageList.image[i];
            // xbox::DbgPrint(
            //     "Image %d: Name='%s', Type=%d, Dimensions=%dx%dx%d, MipLevels=%d, Format=%d, CardMemory=%d bytes, BaseSize=%d, Streaming=%d, DelayLoad=%d, Semantic=%d, StreamSlot=%d\n",
            //     i,
            //     image->name,
            //     image->mapType,
            //     image->width,
            //     image->height,
            //     image->depth, // Added depth for 3D textures
            //     image->texture.basemap->Format.MaxMipLevel + 1,
            //     image->texture.basemap->Format.DataFormat,
            //     image->cardMemory.platform[1], // Total memory allocation for the texture
            //     image->baseSize,               // Base memory size (for non-streaming textures)
            //     image->streaming,              // Whether the image is streamed
            //     image->delayLoadPixels,        // Whether pixel data is delayed in loading
            //     image->semantic,               // Purpose of the texture in the engine
            //     image->streamSlot              // Streaming slot ID
            // );
            Image_DbgPrint(image);

            Image_Dump(image);
        }
    }

    void Image_Replace_2D(GfxImage *image, const DDSImage &ddsImage)
    {
        if (image->mapType != MAPTYPE_2D)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' is not a 2D map!\n", image->name);
            return;
        }

        XGTEXTURE_DESC SourceDesc;
        XGGetTextureDesc(image->texture.basemap, 0, &SourceDesc);
        BOOL IsBorderTexture = XGIsBorderTexture(image->texture.basemap);
        UINT MipTailBaseLevel = XGGetMipTailBaseLevel(SourceDesc.Width, SourceDesc.Height, IsBorderTexture);

        UINT offset = 0; // Track position in DDS data

        // Determine block size based on format
        UINT blockSize = 0;
        switch (image->texture.basemap->Format.DataFormat)
        {
        case GPUTEXTUREFORMAT_DXT1: // DXT1
            blockSize = 8;
            break;
        case GPUTEXTUREFORMAT_DXT4_5: // DXT5
            blockSize = 16;
            break;
        default:
            Com_PrintError(CON_CHANNEL_ERROR, "Unsupported texture format for image '%s'!\n", image->name);
            return;
        }

        for (UINT i = 0; i < MipTailBaseLevel; i++)
        {
            // Compute width/height for this mip level
            UINT width = max(1U, ddsImage.header.width >> i);
            UINT height = max(1U, ddsImage.header.height >> i);

            // Ensure width/height are at least 4x4 for block compression
            UINT blockWidth = (width + 3) / 4;
            UINT blockHeight = (height + 3) / 4;
            UINT CurrentLevelSize = blockWidth * blockHeight * blockSize; // Corrected block size for DXT1/DXT5

            DbgPrint("  Mip Level %u: %ux%u, Size=%u\n", i, width, height, CurrentLevelSize);

            // Ensure we're not reading out of bounds
            if (offset + CurrentLevelSize > ddsImage.data.size())
            {
                DbgPrint("  [ERROR] Mip Level %u exceeds DDS data size! Skipping...\n", i);
                break;
            }

            std::vector<uint8_t> tiledData = Xbox360ConvertToTiledTexture(
                std::vector<uint8_t>(ddsImage.data.begin() + offset, ddsImage.data.begin() + offset + CurrentLevelSize),
                width, height,
                static_cast<GPUTEXTUREFORMAT>(image->texture.basemap->Format.DataFormat));

            switch (image->texture.basemap->Format.Endian)
            {
            case GPUENDIAN_8IN16:
                XGEndianSwapMemory(tiledData.data(), tiledData.data(), XGENDIAN_8IN16, 2, tiledData.size() / 2);
                break;
            case GPUENDIAN_8IN32:
                XGEndianSwapMemory(tiledData.data(), tiledData.data(), XGENDIAN_8IN32, 4, tiledData.size() / 4);
                break;
            case GPUENDIAN_16IN32:
                XGEndianSwapMemory(tiledData.data(), tiledData.data(), XGENDIAN_16IN32, 4, tiledData.size() / 4);
                break;
            default:
                break;
            }

            memcpy(image->pixels + offset, tiledData.data(), tiledData.size());

            offset += CurrentLevelSize;
        }
    }

    void Image_Replace_Cube(GfxImage *image, const DDSImage &ddsImage)
    {
        if (image->mapType != MAPTYPE_CUBE)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' is not a cube map!\n", image->name);
            return;
        }

        const GPUTEXTUREFORMAT format = static_cast<GPUTEXTUREFORMAT>(image->texture.basemap->Format.DataFormat);

        // Check can we get the base size here and /6 for face size

        unsigned int face_size = 0;
        switch (format)
        {
        case GPUTEXTUREFORMAT_DXT1:
            face_size = (image->width / 4) * (image->height / 4) * 8;
            break;
        case GPUTEXTUREFORMAT_8_8_8_8:
            face_size = image->width * image->height * 4;
            break;
        default:
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' has unsupported format %d!\n", image->name, format);
            return;
        }

        for (int i = 0; i < 6; i++)
        {
            const unsigned char *face_pixels = ddsImage.data.data() + (i * face_size);

            // Convert the DDS image to tiled texture format
            std::vector<uint8_t> tiledData = Xbox360ConvertToTiledTexture(
                std::vector<uint8_t>(face_pixels, face_pixels + face_size),
                image->width, image->height, format);

            switch (image->texture.basemap->Format.Endian)
            {
            case GPUENDIAN_8IN16:
                XGEndianSwapMemory(tiledData.data(), tiledData.data(), XGENDIAN_8IN16, 2, tiledData.size() / 2);
                break;
            case GPUENDIAN_8IN32:
                XGEndianSwapMemory(tiledData.data(), tiledData.data(), XGENDIAN_8IN32, 4, tiledData.size() / 4);
                break;
            case GPUENDIAN_16IN32:
                XGEndianSwapMemory(tiledData.data(), tiledData.data(), XGENDIAN_16IN32, 4, tiledData.size() / 4);
                break;
            default:
                break;
            }

            // Copy the data to the image
            memcpy(image->pixels + (i * face_size), tiledData.data(), face_size);
        }
    }

    void Image_Replace(GfxImage *image)
    {
        const std::string replacement_base_dir = "game:\\raw\\images";
        const std::string replacement_path = replacement_base_dir + "\\" + image->name + ".dds";

        if (!FileExists(replacement_path.c_str()))
        {
            Com_PrintError(CON_CHANNEL_ERROR, "File does not exist: %s\n", replacement_path.c_str());
            return;
        }

        DDSImage ddsImage = ReadDDSFile(replacement_path.c_str());
        if (ddsImage.data.empty())
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Failed to load DDS file: %s\n", replacement_path.c_str());
            return;
        }

        if (image->width != ddsImage.header.width || image->height != ddsImage.header.height)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' dimensions do not match DDS file: %s\n", image->name, replacement_path.c_str());
            return;
        }

        GPUTEXTUREFORMAT ddsFormat;
        switch (ddsImage.header.pixelFormat.fourCC)
        {
        case DXT1_FOURCC:
            ddsFormat = GPUTEXTUREFORMAT_DXT1;
            break;
        case DXT3_FOURCC:
            ddsFormat = GPUTEXTUREFORMAT_DXT2_3;
            break;
        case DXT5_FOURCC:
            ddsFormat = GPUTEXTUREFORMAT_DXT4_5;
            break;
        case DXN_FOURCC:
            ddsFormat = GPUTEXTUREFORMAT_DXN;
            break;
        default:
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' has an unsupported DDS format: 0x%X\n", image->name, ddsImage.header.pixelFormat.fourCC);
            return;
        }

        if (static_cast<uint32_t>(image->texture.basemap->Format.DataFormat) != static_cast<uint32_t>(ddsFormat))
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' format does not match DDS file: Expected %d, Got %d\n",
                           image->name, static_cast<uint32_t>(image->texture.basemap->Format.DataFormat),
                           static_cast<uint32_t>(ddsFormat));
            return;
        }

        if (image->mapType == MAPTYPE_2D)
        {
            Image_Replace_2D(image, ddsImage);
        }
        else if (image->mapType == MAPTYPE_CUBE)
        {
            Image_Replace_Cube(image, ddsImage);
        }
        else
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' is not a 2D or cube map!\n", image->name);
            return;
        }
    }

    void Cmd_imageload_f()
    {
        const UINT MAX_IMAGES = 2048;
        XAssetHeader assets[MAX_IMAGES];
        UINT count = DB_GetAllXAssetOfType_FastFile(ASSET_TYPE_IMAGE, assets, MAX_IMAGES);
        DbgPrint("Cmd_imageload2_f: Found %d images\n", count);
        for (UINT i = 0; i < count; i++)
        {
            GfxImage *image = assets[i].image;
            // debug image metadata print out all
            Image_Replace(image);
        }
    }

    void init()
    {
        xbox::DbgPrint("Initializing MP\n");

        CL_ConsolePrint_Detour = Detour(CL_ConsolePrint, CL_ConsolePrint_Hook);
        CL_ConsolePrint_Detour.Install();

        CL_GamepadButtonEvent_Detour = Detour(CL_GamepadButtonEvent, CL_GamepadButtonEvent_Hook);
        CL_GamepadButtonEvent_Detour.Install();

        Load_MapEntsPtr_Detour = Detour(Load_MapEntsPtr, Load_MapEntsPtr_Hook);
        Load_MapEntsPtr_Detour.Install();

        R_StreamLoadFileSynchronously_Detour = Detour(R_StreamLoadFileSynchronously, R_StreamLoadFileSynchronously_Hook);
        R_StreamLoadFileSynchronously_Detour.Install();

        Scr_ReadFile_FastFile_Detour = Detour(Scr_ReadFile_FastFile, Scr_ReadFile_FastFile_Hook);
        Scr_ReadFile_FastFile_Detour.Install();

        cmd_function_s *rawfilesdump_VAR = new cmd_function_s;
        Cmd_AddCommandInternal("rawfiledump", Cmd_rawfilesdump, rawfilesdump_VAR);

        cmd_function_s *imagedump_VAR = new cmd_function_s;
        Cmd_AddCommandInternal("imagedump", Cmd_imagedump, imagedump_VAR);

        cmd_function_s *imageload_f_VAR = new cmd_function_s;
        Cmd_AddCommandInternal("imageload", Cmd_imageload_f, imageload_f_VAR);
    }
}
