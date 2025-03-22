#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>

#include <xtl.h>
#include <xbox.h>
#include <xgraphics.h>

#include "mp_main.h"
#include "mp_structs.h"

#include "../detour.h"
#include "../filesystem.h"
#include "../xboxkrnl.h"

// Structure to hold data for the active keyboard request
struct KeyboardRequest
{
    wchar_t *resultText;
    XOVERLAPPED overlapped;
    std::function<void(bool, const wchar_t *)> callback;
    DWORD maxLength;
    bool isActive;

    KeyboardRequest() : resultText(nullptr), callback(nullptr), maxLength(0), isActive(false)
    {
        ZeroMemory(&overlapped, sizeof(XOVERLAPPED));
    }

    ~KeyboardRequest()
    {
        if (resultText)
        {
            delete[] resultText;
            resultText = nullptr;
        }
    }
};

// Global singleton for the keyboard request
static KeyboardRequest g_keyboardRequest;

// Function to show keyboard UI without blocking
bool ShowKeyboardAsync(
    DWORD dwUserIndex,
    const wchar_t *defaultText,
    const wchar_t *titleText,
    const wchar_t *descriptionText,
    DWORD maxTextLength,
    DWORD keyboardType,
    std::function<void(bool, const wchar_t *)> callback)
{
    // If there's already an active keyboard request, don't allow another one
    if (g_keyboardRequest.isActive)
    {
        xbox::DbgPrint("Keyboard UI is already active, ignoring request\n");
        return false;
    }

    // Clean up any previous request
    if (g_keyboardRequest.resultText)
    {
        delete[] g_keyboardRequest.resultText;
        g_keyboardRequest.resultText = nullptr;
    }

    // Allocate new result text buffer (add 1 for null terminator)
    DWORD bufferSize = maxTextLength + 1;
    g_keyboardRequest.resultText = new wchar_t[bufferSize];
    ZeroMemory(g_keyboardRequest.resultText, bufferSize * sizeof(wchar_t));
    g_keyboardRequest.maxLength = bufferSize;
    g_keyboardRequest.callback = callback;

    // Zero out the overlapped structure
    ZeroMemory(&g_keyboardRequest.overlapped, sizeof(XOVERLAPPED));

    // Set up the keyboard UI with overlapped to make it non-blocking
    HRESULT hr = XShowKeyboardUI(
        dwUserIndex,
        keyboardType,
        defaultText,
        titleText,
        descriptionText,
        g_keyboardRequest.resultText,
        bufferSize,
        &g_keyboardRequest.overlapped);

    if (FAILED(hr))
    {
        // Handle error
        xbox::DbgPrint("Failed to show keyboard UI: 0x%08X\n", hr);
        delete[] g_keyboardRequest.resultText;
        g_keyboardRequest.resultText = nullptr;
        return false;
    }

    // Mark as active
    g_keyboardRequest.isActive = true;
    xbox::DbgPrint("Keyboard UI requested, continuing with game loop...\n");
    return true;
}

// Function to call in your game loop to check for keyboard completion
void CheckKeyboardCompletion()
{
    // If no active keyboard request, nothing to do
    if (!g_keyboardRequest.isActive)
    {
        return;
    }

    // Check if the operation has completed
    if (XHasOverlappedIoCompleted(&g_keyboardRequest.overlapped))
    {
        DWORD result;
        XGetOverlappedResult(&g_keyboardRequest.overlapped, &result, TRUE);

        xbox::DbgPrint("Keyboard operation completed with result: 0x%08X\n", result);

        // Operation completed
        bool success = (result == ERROR_SUCCESS);

        // Call the user callback with the result
        if (g_keyboardRequest.callback)
        {
            if (success)
            {
                xbox::DbgPrint("Keyboard text entry successful\n");
                g_keyboardRequest.callback(true, g_keyboardRequest.resultText);
            }
            else
            {
                xbox::DbgPrint("Keyboard operation failed or canceled\n");
                g_keyboardRequest.callback(false, nullptr);
            }
        }

        // Clean up
        delete[] g_keyboardRequest.resultText;
        g_keyboardRequest.resultText = nullptr;
        g_keyboardRequest.isActive = false;
    }
    // If not completed, we'll check again next frame
}

void GPUEndianSwapTexture(std::vector<uint8_t> &pixelData, GPUENDIAN endianType)
{
    switch (endianType)
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
    }
}

// TODO: MAKEFOURCC('D', 'X', 'T', '1');
// DDS Constants
const uint32_t DDS_MAGIC = MAKEFOURCC('D', 'D', 'S', ' ');
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
const uint32_t DXT1_FOURCC = MAKEFOURCC('D', 'X', 'T', '1');
const uint32_t DXT3_FOURCC = MAKEFOURCC('D', 'X', 'T', '3');
const uint32_t DXT5_FOURCC = MAKEFOURCC('D', 'X', 'T', '5');
const uint32_t DXN_FOURCC = MAKEFOURCC('A', 'T', 'I', '2'); // (DXN / BC5)

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
        xbox::DbgPrint("ERROR: Unable to open file: %s\n", filepath.c_str());
        return ddsImage; // Return empty DDSImage
    }

    // Read DDS header (raw, little-endian)
    file.read(reinterpret_cast<char *>(&ddsImage.header), sizeof(DDSHeader));

    // Swap only the magic number to big-endian for proper validation
    uint32_t magicSwapped = _byteswap_ulong(ddsImage.header.magic);

    if (magicSwapped != 0x20534444) // 'DDS ' in big-endian
    {
        xbox::DbgPrint("ERROR: Invalid DDS file: %s\n", filepath.c_str());
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
        xbox::DbgPrint("ERROR: Failed to determine file size.\n");
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
    xbox::DbgPrint("INFO: DDS file '%s' loaded successfully.\n", filepath.c_str());
    xbox::DbgPrint("      Resolution: %ux%u\n", ddsImage.header.width, ddsImage.header.height);
    xbox::DbgPrint("      MipMaps: %u\n", ddsImage.header.mipMapCount);
    xbox::DbgPrint("      Data Size: %u bytes\n", static_cast<unsigned int>(dataSize));

    return ddsImage;
}

namespace mp
{
    // Functions
    Cbuf_AddText_t Cbuf_AddText = reinterpret_cast<Cbuf_AddText_t>(0x82239FD0);
    Cbuf_ExecuteBuffer_t Cbuf_ExecuteBuffer = reinterpret_cast<Cbuf_ExecuteBuffer_t>(0x8223AAE8);

    CG_DrawActive_t CG_DrawActive = reinterpret_cast<CG_DrawActive_t>(0x8231E6E0);
    CG_GameMessage_t CG_GameMessage = reinterpret_cast<CG_GameMessage_t>(0x8230AAF0);
    CG_GetPredictedPlayerState_t CG_GetPredictedPlayerState = reinterpret_cast<CG_GetPredictedPlayerState_t>(0x82309120);
    CG_RegisterGraphics_t CG_RegisterGraphics = reinterpret_cast<CG_RegisterGraphics_t>(0x8230D858);

    CL_ConsolePrint_t CL_ConsolePrint = reinterpret_cast<CL_ConsolePrint_t>(0x822E4D18);
    CL_GamepadButtonEvent_t CL_GamepadButtonEvent = reinterpret_cast<CL_GamepadButtonEvent_t>(0x822DD1E8);

    ClientCommand_t ClientCommand = reinterpret_cast<ClientCommand_t>(0x8227DCF0);

    Cmd_AddCommandInternal_t Cmd_AddCommandInternal = reinterpret_cast<Cmd_AddCommandInternal_t>(0x8223ADE0);
    Cmd_ExecFromFastFile_t Cmd_ExecFromFastFile = reinterpret_cast<Cmd_ExecFromFastFile_t>(0x8223AF40);
    Cmd_ExecuteSingleCommand_t Cmd_ExecuteSingleCommand = reinterpret_cast<Cmd_ExecuteSingleCommand_t>(0x8223A7A0);

    CheatsOk_t CheatsOk = reinterpret_cast<CheatsOk_t>(0x8227BF40);

    Com_Printf_t Com_Printf = reinterpret_cast<Com_Printf_t>(0x82237000);
    Com_PrintError_t Com_PrintError = reinterpret_cast<Com_PrintError_t>(0x82235C50);
    Com_PrintWarning_t Com_PrintWarning = reinterpret_cast<Com_PrintWarning_t>(0x822356B8);

    DB_EnumXAssets_FastFile_t DB_EnumXAssets_FastFile = reinterpret_cast<DB_EnumXAssets_FastFile_t>(0x8229ED48);
    DB_FindXAssetEntry_t DB_FindXAssetEntry = reinterpret_cast<DB_FindXAssetEntry_t>(0x8229EB98);
    DB_FindXAssetHeader_t DB_FindXAssetHeader = reinterpret_cast<DB_FindXAssetHeader_t>(0x822A0298);
    DB_GetAllXAssetOfType_FastFile_t DB_GetAllXAssetOfType_FastFile = reinterpret_cast<DB_GetAllXAssetOfType_FastFile_t>(0x8229E8E0);

    Dvar_GetBool_t Dvar_GetBool = reinterpret_cast<Dvar_GetBool_t>(0x821D15D8);
    Dvar_RegisterBool_t Dvar_RegisterBool = reinterpret_cast<Dvar_RegisterBool_t>(0x821D5180);
    Dvar_RegisterColor_t Dvar_RegisterColor = reinterpret_cast<Dvar_RegisterColor_t>(0x821D4D98);
    Dvar_RegisterEnum_t Dvar_RegisterEnum = reinterpret_cast<Dvar_RegisterEnum_t>(0x821D4F88);
    Dvar_RegisterInt_t Dvar_RegisterInt = reinterpret_cast<Dvar_RegisterInt_t>(0x821D5138);

    G_SetAngle_t G_SetAngle = reinterpret_cast<G_SetAngle_t>(0x8224AA98);
    G_SetOrigin_t G_SetOrigin = reinterpret_cast<G_SetOrigin_t>(0x8224AAF0);

    I_strnicmp_t I_strnicmp = reinterpret_cast<I_strnicmp_t>(0x821CDA98);

    Load_MapEntsPtr_t Load_MapEntsPtr = reinterpret_cast<Load_MapEntsPtr_t>(0x822A9648);

    PM_FoliageSounds_t PM_FoliageSounds = reinterpret_cast<PM_FoliageSounds_t>(0x82335E90);

    R_AddCmdDrawText_t R_AddCmdDrawText = (R_AddCmdDrawText_t)0x8216C0B8;
    R_DrawAllDynEnt_t R_DrawAllDynEnt = reinterpret_cast<R_DrawAllDynEnt_t>(0x8215FF98);
    R_GetImageList_t R_GetImageList = reinterpret_cast<R_GetImageList_t>(0x82152A58);
    R_RegisterFont_t R_RegisterFont = reinterpret_cast<R_RegisterFont_t>(0x8216EC00);
    R_StreamLoadFileSynchronously_t R_StreamLoadFileSynchronously = reinterpret_cast<R_StreamLoadFileSynchronously_t>(0x82151510);

    SetClientViewAngle_t SetClientViewAngle = reinterpret_cast<SetClientViewAngle_t>(0x82284C60);

    SCR_DrawSmallStringExt_t SCR_DrawSmallStringExt = reinterpret_cast<SCR_DrawSmallStringExt_t>(0x822C9B88);

    Scr_ReadFile_FastFile_t Scr_ReadFile_FastFile = reinterpret_cast<Scr_ReadFile_FastFile_t>(0x82221220);

    SV_Cmd_ArgvBuffer_t SV_Cmd_ArgvBuffer = reinterpret_cast<SV_Cmd_ArgvBuffer_t>(0x82239F48);
    SV_GameSendServerCommand_t SV_GameSendServerCommand = reinterpret_cast<SV_GameSendServerCommand_t>(0x82204BB8);
    SV_SendServerCommand_t SV_SendServerCommand = reinterpret_cast<SV_SendServerCommand_t>(0x821FFE30);

    Sys_SnapVector_t Sys_SnapVector = reinterpret_cast<Sys_SnapVector_t>(0x821A3BD0);

    TeleportPlayer_t TeleportPlayer = reinterpret_cast<TeleportPlayer_t>(0x8226F408);

    UI_DrawBuildNumber_t UI_DrawBuildNumber = reinterpret_cast<UI_DrawBuildNumber_t>(0x821EBB30);
    UI_DrawText_t UI_DrawText = reinterpret_cast<UI_DrawText_t>(0x821EB858);
    UI_Refresh_t UI_Refresh = reinterpret_cast<UI_Refresh_t>(0x821F2F28);

    va_t va = reinterpret_cast<va_t>(0x821CD858);

    // Variables
    auto clientUIActives = reinterpret_cast<clientUIActive_t *>(0x82435A10);
    auto cmd_functions = reinterpret_cast<cmd_function_s *>(0x82A2335C);
    auto g_entities = reinterpret_cast<gentity_s *>(0x8287CD08);

    ScreenPlacement &scrPlaceFullUnsafe = *reinterpret_cast<ScreenPlacement *>(0x8246F468);

    Detour CL_ConsolePrint_Detour;

    void CL_ConsolePrint_Hook(int localClientNum, int channel, const char *txt, int duration, int pixelWidth, int flags)
    {
        CL_ConsolePrint_Detour.GetOriginal<decltype(CL_ConsolePrint)>()(localClientNum, channel, txt, duration, pixelWidth, flags);
        xbox::DbgPrint("CL_ConsolePrint txt=%s \n", txt);
    }

    void Cmd_cmdinput_f()
    {
        bool success = ShowKeyboardAsync(
            0,                         // First controller
            L"",                       // Default text
            L"Enter Text",             // Title
            L"Please enter some text", // Description
            256,                       // Max length
            VKBD_DEFAULT,              // Keyboard type
            [](bool success, const wchar_t *text)
            {
                if (success && text)
                {
                    // Get the required buffer size
                    size_t wideLength = wcslen(text);
                    size_t mbBufferSize = wideLength * 4 + 1; // 4 bytes per character worst case

                    // Create buffer and convert
                    std::vector<char> mbBuffer(mbBufferSize);
                    wcstombs_s(nullptr, mbBuffer.data(), mbBufferSize, text, wideLength);

                    // Create string from the buffer
                    std::string result = mbBuffer.data();

                    Cbuf_AddText(0, result.c_str());
                }
                else
                {
                    xbox::DbgPrint("Keyboard operation failed or was canceled\n");
                }
            });

        if (!success)
        {
            xbox::DbgPrint("Failed to open keyboard UI\n");
        }
    }

    Detour CL_GamepadButtonEvent_Detour;

    void CL_GamepadButtonEvent_Hook(int localClientNum, int controllerIndex, int key, int down, unsigned int time)
    {
        CL_GamepadButtonEvent_Detour.GetOriginal<decltype(CL_GamepadButtonEvent)>()(localClientNum, controllerIndex, key, down, time);

        // Check if the client is disconnected (main menu)
        if (clientUIActives[localClientNum].connectionState == CA_DISCONNECTED)
        {
            if (key == K_BUTTON_RSTICK && down)
            {
                Cmd_cmdinput_f();
            }
        }
    }

    void Cmd_UFO_f(gentity_s *ent)
    {
        if (!CheatsOk(ent))
            return;

        gclient_s *client = ent->client;

        bool enableUFO = !client->ufo;
        client->ufo = enableUFO;

        const char *message = enableUFO ? "GAME_UFOON" : "GAME_UFOOFF";

        // Format the command string (note: 101 is ASCII for 'e')
        const char *commandString = va("%c \"%s\"", 101, message);

        int entityIndex = ent - g_entities;

        if (entityIndex == -1)
            SV_SendServerCommand(0, SV_CMD_CAN_IGNORE, "%s", commandString);
        else
            SV_GameSendServerCommand(entityIndex, SV_CMD_CAN_IGNORE, commandString);
    }

    void Cmd_Noclip_f(gentity_s *ent)
    {
        if (!CheatsOk(ent))
            return;

        gclient_s *client = ent->client;

        bool enableNoclip = !client->noclip;
        client->noclip = enableNoclip;

        const char *message = enableNoclip ? "GAME_NOCLIPON" : "GAME_NOCLIPOFF";

        // Format the command string (note: 101 is ASCII for 'e')
        const char *commandString = va("%c \"%s\"", 101, message);

        int entityIndex = ent - g_entities;

        if (entityIndex == -1)
            SV_SendServerCommand(0, SV_CMD_CAN_IGNORE, "%s", commandString);
        else
            SV_GameSendServerCommand(entityIndex, SV_CMD_CAN_IGNORE, commandString);
    }

    void Cmd_God_f(gentity_s *ent)
    {
        if (!CheatsOk(ent))
            return;

        // Toggle god mode flag (bit 0)
        ent->flags ^= 1;

        bool godModeEnabled = (ent->flags & 1) != 0;
        const char *message = godModeEnabled ? "GAME_GODMODE_ON" : "GAME_GODMODE_OFF";

        // Format command string (101 is ASCII for 'e')
        const char *commandString = va("%c \"%s\"", 101, message);

        int entityIndex = ent - g_entities;

        if (entityIndex == -1)
            SV_SendServerCommand(0, SV_CMD_CAN_IGNORE, "%s", commandString);
        else
            SV_GameSendServerCommand(entityIndex, SV_CMD_CAN_IGNORE, commandString);
    }

    struct SavedPos
    {
        float origin[3];
        float viewangles[3];
    };

    SavedPos savedPos = {0, 0, 0, 0, 0, 0};

    void Cmd_savepos_f(gentity_s *ent)
    {

        if (!CheatsOk(ent))
            return;

        // int entityIndex = ent - g_entities;

        savedPos.origin[0] = ent->r.currentOrigin[0];
        savedPos.origin[1] = ent->r.currentOrigin[1];
        savedPos.origin[2] = ent->r.currentOrigin[2];

        savedPos.viewangles[0] = ent->client->ps.viewangles[0];
        savedPos.viewangles[1] = ent->client->ps.viewangles[1];
        savedPos.viewangles[2] = ent->client->ps.viewangles[2];
    }

    void Cmd_loadpos_f(gentity_s *ent)
    {

        if (!CheatsOk(ent))
            return;

        int entityIndex = ent - g_entities;

        if (savedPos.origin[0] == 0 && savedPos.origin[1] == 0 && savedPos.origin[2] == 0)
        {
            const char *commandString = va("e \"%s\"", "No saved position");
            SV_GameSendServerCommand(entityIndex, SV_CMD_CAN_IGNORE, commandString);
            return;
        }

        // Clear velocity
        ent->client->ps.velocity[0] = 0;
        ent->client->ps.velocity[1] = 0;
        ent->client->ps.velocity[2] = 0;

        TeleportPlayer(ent, savedPos.origin, savedPos.viewangles);
    }

    Detour ClientCommand_Detour;

    void ClientCommand_Hook(int clientNum)
    {
        gentity_s *ent = &g_entities[clientNum];

        char cmd[1032];
        SV_Cmd_ArgvBuffer(0, cmd, 1024);

        if (I_strnicmp(cmd, "noclip", 6) == 0)
            Cmd_Noclip_f(ent);
        else if (I_strnicmp(cmd, "ufo", 3) == 0)
            Cmd_UFO_f(ent);
        else if (I_strnicmp(cmd, "god", 3) == 0)
            Cmd_God_f(ent);
        else if (I_strnicmp(cmd, "savepos", 7) == 0)
            Cmd_savepos_f(ent);
        else if (I_strnicmp(cmd, "loadpos", 7) == 0)
            Cmd_loadpos_f(ent);
        else
            ClientCommand_Detour.GetOriginal<decltype(ClientCommand)>()(clientNum);
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

    std::string extractFilename(const char *filename)
    {
        std::string path(filename);

        // Find last backslash '\' or forward slash '/'
        size_t lastSlash = path.find_last_of("\\/");
        size_t start = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;

        // Find last dot '.' (extension separator)
        size_t lastDot = path.find_last_of('.');
        size_t end = (lastDot == std::string::npos || lastDot < start) ? path.length() : lastDot;

        return path.substr(start, end - start);
    }

    bool R_StreamLoadHighMipReplacement(const char *filename, unsigned int bytesToRead, unsigned __int8 *outData)
    {
        std::string asset_name = extractFilename(filename);
        auto asset = DB_FindXAssetEntry(ASSET_TYPE_IMAGE, asset_name.c_str());

        if (!asset)
        {
            return false;
        }

        auto image = asset->entry.asset.header.image;

        std::string replacement_path = "game:\\raw\\highmip\\" + asset_name + ".dds";
        std::ifstream file(replacement_path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            return false;
        }

        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (file_size - 0x80 != bytesToRead) // 0x80 is the size of the DDS header
        {
            Com_PrintError(CON_CHANNEL_ERROR, "R_StreamLoadHighMipReplacement: File size mismatch: %s\n", replacement_path);
            return false;
        }

        std::vector<unsigned char> ddsHeader(0x80);
        file.read(reinterpret_cast<char *>(ddsHeader.data()), 0x80);

        // TODO: check if file is DDS and has correct format and dimensions

        std::vector<unsigned char> buffer;
        buffer.resize(static_cast<size_t>(bytesToRead));
        file.read(reinterpret_cast<char *>(buffer.data()), bytesToRead);

        GPUEndianSwapTexture(buffer, static_cast<GPUENDIAN>(image->texture.basemap->Format.Endian));

        XGTEXTURE_DESC textureDesc;
        XGGetTextureDesc(image->texture.basemap, 0, &textureDesc);

        // High mip are 2x the size of the original image
        auto width = image->width * 2;
        auto height = image->height * 2;
        auto rowPitch = textureDesc.RowPitch * 2;

        XGTileTextureLevel(
            width,
            height,
            0,
            image->texture.basemap->Format.DataFormat,
            XGTILE_NONPACKED, // Use non-packed mode (likely required for this texture)
            outData,          // Destination (tiled GPU memory for Base)
            nullptr,          // No offset (tile the whole image)
            buffer.data(),    // Source mip level data
            rowPitch,         // Row pitch of source image (should match DDS format)
            nullptr           // No subrectangle (tile the full image)
        );

        return true;
    }

    Detour R_StreamLoadFileSynchronously_Detour;

    int R_StreamLoadFileSynchronously_Hook(const char *filename, unsigned int bytesToRead, unsigned __int8 *outData)
    {
        if (R_StreamLoadHighMipReplacement(filename, bytesToRead, outData))
        {
            return 1;
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
            unsigned int rowPitch = 0;
            const GPUTEXTUREFORMAT format = static_cast<GPUTEXTUREFORMAT>(image->texture.basemap->Format.DataFormat);

            switch (format)
            {
            case GPUTEXTUREFORMAT_DXT1:
                face_size = (image->width / 4) * (image->height / 4) * 8;
                rowPitch = (image->width / 4) * 8; // 8 bytes per 4x4 block
                break;
            case GPUTEXTUREFORMAT_8_8_8_8:
                face_size = image->width * image->height * 4;
                rowPitch = image->width * 4; // 4 bytes per pixel
                break;
            default:
                Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Unsupported cube map format %d!\n", format);
                return;
            }

            // TODO: handle mip levels per face for cubemaps
            for (int i = 0; i < 6; i++)
            {
                unsigned char *face_pixels = image->pixels + (i * face_size); // Offset for each face

                std::vector<uint8_t> swappedFace(face_pixels, face_pixels + face_size);
                GPUEndianSwapTexture(swappedFace, static_cast<GPUENDIAN>(image->texture.basemap->Format.Endian));

                // Create buffer for linear texture data
                std::vector<uint8_t> linearFace(face_size);

                // Convert tiled texture to linear layout using XGUntileTextureLevel
                XGUntileTextureLevel(
                    image->width,               // Width
                    image->height,              // Height
                    0,                          // Level (base level)
                    static_cast<DWORD>(format), // GpuFormat
                    0,                          // Flags (no special flags)
                    linearFace.data(),          // pDestination (linear output)
                    rowPitch,                   // RowPitch
                    nullptr,                    // pPoint (no offset)
                    swappedFace.data(),         // pSource (tiled input)
                    nullptr                     // pRect (entire texture)
                );

                file.write(reinterpret_cast<const char *>(linearFace.data()), linearFace.size());
            }

            file.close();
        }
        else if (image->mapType == MAPTYPE_2D)
        {
            // TODO: write mip levels
            file.write(reinterpret_cast<const char *>(&header), sizeof(DDSHeader));

            std::vector<uint8_t> pixelData(image->pixels, image->pixels + image->baseSize);

            GPUEndianSwapTexture(pixelData, static_cast<GPUENDIAN>(image->texture.basemap->Format.Endian));

            // Create a linear data buffer to hold the untiled texture
            std::vector<uint8_t> linearData(image->baseSize);

            // Calculate row pitch based on format
            UINT rowPitch;
            auto format = image->texture.basemap->Format.DataFormat;

            switch (format)
            {
            case GPUTEXTUREFORMAT_DXT1:
            case GPUTEXTUREFORMAT_DXT2_3:
            case GPUTEXTUREFORMAT_DXT4_5:
            case GPUTEXTUREFORMAT_DXN:
                // Block compressed formats use 4x4 blocks
                rowPitch = ((image->width + 3) / 4) * (format == GPUTEXTUREFORMAT_DXT1 ? 8 : 16);
                break;
            case GPUTEXTUREFORMAT_8:
                rowPitch = image->width;
                break;
            case GPUTEXTUREFORMAT_8_8:
                rowPitch = image->width * 2;
                break;
            case GPUTEXTUREFORMAT_8_8_8_8:
                rowPitch = image->width * 4;
                break;
            default:
                Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Unsupported texture format %d!\n", format);
                return;
            }

            xbox::DbgPrint("Image_Dump: rowPitch=%d\n", rowPitch);

            // Call XGUntileTextureLevel to convert the tiled texture to linear format
            XGUntileTextureLevel(
                image->width,               // Width
                image->height,              // Height
                0,                          // Level (base level 0)
                static_cast<DWORD>(format), // GpuFormat
                XGTILE_NONPACKED,           // Flags (no special flags)
                linearData.data(),          // pDestination
                rowPitch,                   // RowPitch (calculated based on format)
                nullptr,                    // pPoint (no offset)
                pixelData.data(),           // pSource
                nullptr                     // pRect (entire texture)
            );

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
        CreateDirectoryA("game:\\dump\\highmip", 0);

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

        auto highmips = filesystem::list_files_in_directory("D:\\highmip");
        for (size_t i = 0; i < highmips.size(); ++i)
        {
            const std::string &filepath = "D:\\highmip\\" + highmips[i];
            Com_Printf(CON_CHANNEL_CONSOLEONLY, "Dumping highmip file: %s\n", filepath.c_str());
            std::string assetName = extractFilename(filepath.c_str());
            auto asset = DB_FindXAssetEntry(ASSET_TYPE_IMAGE, assetName.c_str());
            if (!asset)
            {
                Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' not found in asset list!\n", assetName.c_str());
                continue;
            }

            auto image = asset->entry.asset.header.image;

            std::ifstream input_file(filepath, std::ios::binary | std::ios::ate); // Open file in binary mode and seek to end
            if (!input_file)
            {
                Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Failed to open file: %s\n", filepath.c_str());
                continue;
            }

            std::streamsize size = input_file.tellg();
            if (size < 0)
            {
                Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Failed to determine file size: %s\n", filepath.c_str());
                continue;
            }

            input_file.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(static_cast<size_t>(size));

            if (input_file.read(reinterpret_cast<char *>(buffer.data()), size))
            {
                Com_Printf(CON_CHANNEL_CONSOLEONLY, "Read %d bytes from file.\n", size);
            }
            else
            {
                Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Error reading file: %s\n", filepath.c_str());
                continue;
            }

            auto width = image->width * 2;
            auto height = image->height * 2;
            auto baseSize = width * height * 4;

            DDSHeader header;
            memset(&header, 0, sizeof(DDSHeader));

            header.magic = _byteswap_ulong(DDS_MAGIC);
            header.size = _byteswap_ulong(DDS_HEADER_SIZE);
            header.flags = _byteswap_ulong(DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE);
            header.width = _byteswap_ulong(width);
            header.height = _byteswap_ulong(height);
            header.depth = _byteswap_ulong(image->depth);
            header.mipMapCount = _byteswap_ulong(1);
            header.caps = _byteswap_ulong(DDSCAPS_TEXTURE);
            header.pitchOrLinearSize = baseSize;

            auto format = image->texture.basemap->Format.DataFormat;
            switch (format)
            {
            case GPUTEXTUREFORMAT_DXT1:
                header.pixelFormat.fourCC = _byteswap_ulong(DXT1_FOURCC);

                break;
            case GPUTEXTUREFORMAT_DXT2_3:
                header.pixelFormat.fourCC = _byteswap_ulong(DXT3_FOURCC);

                break;
            case GPUTEXTUREFORMAT_DXT4_5:
                header.pixelFormat.fourCC = _byteswap_ulong(DXT5_FOURCC);

                break;
            case GPUTEXTUREFORMAT_DXN:
                header.pixelFormat.fourCC = _byteswap_ulong(DXN_FOURCC);

                break;
            case GPUTEXTUREFORMAT_8:
                header.pixelFormat.flags = _byteswap_ulong(DDPF_LUMINANCE);
                header.pixelFormat.rgbBitCount = _byteswap_ulong(8);
                header.pixelFormat.rBitMask = _byteswap_ulong(0x000000FF);
                header.pixelFormat.gBitMask = 0;
                header.pixelFormat.bBitMask = 0;
                header.pixelFormat.aBitMask = 0;

                break;
            case GPUTEXTUREFORMAT_8_8:
                header.pixelFormat.flags = _byteswap_ulong(DDPF_LUMINANCE | DDPF_ALPHAPIXELS);
                header.pixelFormat.rgbBitCount = _byteswap_ulong(16);
                header.pixelFormat.rBitMask = _byteswap_ulong(0x000000FF);
                header.pixelFormat.gBitMask = _byteswap_ulong(0x0000FF00);
                header.pixelFormat.bBitMask = 0;
                header.pixelFormat.aBitMask = 0;

                break;
            case GPUTEXTUREFORMAT_8_8_8_8:
                header.pixelFormat.flags = _byteswap_ulong(DDPF_RGB | DDPF_ALPHAPIXELS);
                header.pixelFormat.rgbBitCount = _byteswap_ulong(32);
                header.pixelFormat.rBitMask = _byteswap_ulong(0x00FF0000);
                header.pixelFormat.gBitMask = _byteswap_ulong(0x0000FF00);
                header.pixelFormat.bBitMask = _byteswap_ulong(0x000000FF);
                header.pixelFormat.aBitMask = _byteswap_ulong(0xFF000000);

                break;
            default:
                Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Unsupported texture format %d!\n", format);
                return;
            }

            // TODO: add sanity checks for format, size, etc.
            // TODO: handle filenames with unsupported characters for Windows

            auto output_filepath = "game:\\dump\\highmip\\" + assetName + ".dds";

            std::ofstream output_file(output_filepath, std::ios::binary);
            if (!output_file)
            {
                Com_PrintError(CON_CHANNEL_ERROR, "Image_Dump: Failed to open file: %s\n", output_filepath.c_str());
                return;
            }

            output_file.write(reinterpret_cast<const char *>(&header), sizeof(DDSHeader));

            GPUEndianSwapTexture(buffer, static_cast<GPUENDIAN>(image->texture.basemap->Format.Endian));

            // Calculate row pitch based on format
            UINT rowPitch;

            switch (format)
            {
            case GPUTEXTUREFORMAT_DXT1:
                rowPitch = (width / 4) * 8; // 8 bytes per 4x4 block
                break;
            case GPUTEXTUREFORMAT_DXT2_3:
            case GPUTEXTUREFORMAT_DXT4_5:
            case GPUTEXTUREFORMAT_DXN:
                rowPitch = (width / 4) * 16; // 16 bytes per 4x4 block
                break;
            case GPUTEXTUREFORMAT_8:
                rowPitch = width; // 1 byte per pixel
                break;
            case GPUTEXTUREFORMAT_8_8:
                rowPitch = width * 2; // 2 bytes per pixel
                break;
            case GPUTEXTUREFORMAT_8_8_8_8:
                rowPitch = width * 4; // 4 bytes per pixel
                break;
            default:
                rowPitch = width * 4; // Default to 4 bytes per pixel
                break;
            }

            // Create a buffer for linear texture data
            std::vector<uint8_t> linearData(buffer.size());
            std::vector<uint8_t> bufferAsUint8(buffer.begin(), buffer.end());

            // Convert tiled texture to linear layout
            XGUntileTextureLevel(
                width,                      // Width
                height,                     // Height
                0,                          // Level (base level)
                static_cast<DWORD>(format), // GpuFormat
                0,                          // Flags (no special flags)
                linearData.data(),          // pDestination (linear output)
                rowPitch,                   // RowPitch
                nullptr,                    // pPoint (no offset)
                bufferAsUint8.data(),       // pSource (tiled input)
                nullptr                     // pRect (entire texture)
            );

            output_file.write(reinterpret_cast<const char *>(linearData.data()), linearData.size());
            output_file.close();

            Com_Printf(CON_CHANNEL_CONSOLEONLY, "Dumped highmip file: %s\n", output_filepath.c_str());
        }
    }

    UINT CalculateMipLevelSize(UINT width, UINT height, UINT mipLevel, GPUTEXTUREFORMAT format)
    {
        // Calculate dimensions for the requested mip level
        UINT mipWidth = max(1, width >> mipLevel);
        UINT mipHeight = max(1, height >> mipLevel);

        // Calculate size based on format
        UINT blockSize;
        switch (format)
        {
        case GPUTEXTUREFORMAT_DXT1:
            blockSize = 8; // 8 bytes per 4x4 block
            break;
        case GPUTEXTUREFORMAT_DXT2_3:
            blockSize = 16; // 16 bytes per 4x4 block
            break;
        case GPUTEXTUREFORMAT_DXT4_5:
            blockSize = 16; // 16 bytes per 4x4 block
            break;
        case GPUTEXTUREFORMAT_DXN:
            blockSize = 16; // 16 bytes per 4x4 block (two 8-byte channels)
            break;
        default:
            xbox::DbgPrint("CalculateMipLevelSize: Unsupported format %d\n", format);
            return 0;
        }

        // For block-compressed formats, calculate number of blocks
        // Each block is 4x4 pixels, so we need to round up to nearest block
        UINT blocksWide = (mipWidth + 3) / 4;
        UINT blocksHigh = (mipHeight + 3) / 4;

        // Calculate total size in bytes
        UINT sizeInBytes = blocksWide * blocksHigh * blockSize;

        return sizeInBytes;
    }

    void Image_Replace_2D(GfxImage *image, const DDSImage &ddsImage)
    {
        if (image->mapType != MAPTYPE_2D)
        {
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' is not a 2D map!\n", image->name);
            return;
        }

        // Get base texture layout
        UINT baseAddress, baseSize, mipAddress, mipSize;

        XGGetTextureLayout(image->texture.basemap,
                           &baseAddress, &baseSize, 0, 0, 0,
                           &mipAddress, &mipSize, 0, 0, 0);

        XGTEXTURE_DESC TextureDesc;
        XGGetTextureDesc(image->texture.basemap, 0, &TextureDesc);

        UINT mipTailBaseLevel = XGGetMipTailBaseLevel(TextureDesc.Width, TextureDesc.Height, XGIsBorderTexture(image->texture.basemap));

        UINT ddsOffset = 0;

        for (UINT mipLevel = 0; mipLevel < mipTailBaseLevel; mipLevel++)
        {
            UINT widthInBlocks = max(1, TextureDesc.WidthInBlocks >> mipLevel);
            UINT rowPitch = widthInBlocks * TextureDesc.BytesPerBlock;
            // UINT levelSize = rowPitch * heightInBlocks;
            UINT ddsMipLevelSize = CalculateMipLevelSize(image->width, image->height, mipLevel, static_cast<GPUTEXTUREFORMAT>(image->texture.basemap->Format.DataFormat));

            if (ddsMipLevelSize == 0)
            {
                xbox::DbgPrint("  [ERROR] Unsupported format %d for mip level %u! Skipping...\n", image->texture.basemap->Format.DataFormat, mipLevel);
                break;
            }

            // Ensure we're not reading out of bounds
            if (ddsOffset + ddsMipLevelSize > ddsImage.data.size())
            {
                xbox::DbgPrint("  [ERROR] Mip Level %u exceeds DDS data size! Skipping...\n", mipLevel);
                break;
            }

            std::vector<uint8_t> levelData(ddsImage.data.begin() + ddsOffset, ddsImage.data.begin() + ddsOffset + ddsMipLevelSize);

            GPUEndianSwapTexture(levelData, static_cast<GPUENDIAN>(image->texture.basemap->Format.Endian));

            xbox::DbgPrint("Image_Replace_2D: Mip Level %d - Row Pitch=%u\n", mipLevel, rowPitch);

            UINT address = baseAddress;
            if (mipLevel > 0)
            {
                UINT mipLevelOffset = XGGetMipLevelOffset(image->texture.basemap, 0, mipLevel);
                address = mipAddress + mipLevelOffset;
            }

            xbox::DbgPrint("Image_Replace_2D: Writing mip level %d to address 0x%08X - levelSize=%u\n", mipLevel, address, ddsMipLevelSize);

            // // Write the base level
            XGTileTextureLevel(
                TextureDesc.Width,
                TextureDesc.Height,
                mipLevel,
                image->texture.basemap->Format.DataFormat,
                XGTILE_NONPACKED,                  // Use non-packed mode (likely required for this texture)
                reinterpret_cast<void *>(address), // Destination (tiled GPU memory for Base)
                nullptr,                           // No offset (tile the whole image)
                levelData.data(),                  // Source mip level data
                rowPitch,                          // Row pitch of source image (should match DDS format)
                nullptr                            // No subrectangle (tile the full image)
            );

            ddsOffset += ddsMipLevelSize;
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
        unsigned int rowPitch = 0;

        switch (format)
        {
        case GPUTEXTUREFORMAT_DXT1:
            face_size = (image->width / 4) * (image->height / 4) * 8;
            rowPitch = (image->width / 4) * 8; // 8 bytes per 4x4 block
            break;
        case GPUTEXTUREFORMAT_8_8_8_8:
            face_size = image->width * image->height * 4;
            rowPitch = image->width * 4; // 4 bytes per pixel
            break;
        default:
            Com_PrintError(CON_CHANNEL_ERROR, "Image '%s' has unsupported format %d!\n", image->name, format);
            return;
        }

        for (int i = 0; i < 6; i++)
        {
            const unsigned char *face_pixels = ddsImage.data.data() + (i * face_size);

            // Create a buffer for the tiled texture data
            std::vector<uint8_t> tiledData(face_size);

            // Convert the linear texture to tiled format using XGTileTextureLevel
            XGTileTextureLevel(
                image->width,               // Width
                image->height,              // Height
                0,                          // Level (base level)
                static_cast<DWORD>(format), // GpuFormat
                0,                          // Flags (no special flags)
                tiledData.data(),           // pDestination (tiled output)
                nullptr,                    // pPoint (no offset)
                face_pixels,                // pSource (linear input)
                rowPitch,                   // RowPitch
                nullptr                     // pRect (entire texture)
            );

            GPUEndianSwapTexture(tiledData, static_cast<GPUENDIAN>(image->texture.basemap->Format.Endian));

            // Copy the data to the image
            memcpy(image->pixels + (i * face_size), tiledData.data(), face_size);
        }
    }

    void Image_Replace(GfxImage *image)
    {
        const std::string replacement_base_dir = "game:\\raw\\images";
        const std::string replacement_path = replacement_base_dir + "\\" + image->name + ".dds";

        if (!filesystem::file_exists(replacement_path))
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

    void Load_images()
    {
        const UINT MAX_IMAGES = 2048;
        XAssetHeader assets[MAX_IMAGES];
        UINT count = DB_GetAllXAssetOfType_FastFile(ASSET_TYPE_IMAGE, assets, MAX_IMAGES);
        xbox::DbgPrint("Cmd_imageload2_f: Found %d images\n", count);
        for (UINT i = 0; i < count; i++)
        {
            GfxImage *image = assets[i].image;
            // debug image metadata print out all
            Image_Replace(image);
        }
    }

    Detour CG_RegisterGraphics_Detour;

    void CG_RegisterGraphics_Hook(int localClientNum, const char *mapname)
    {
        CG_RegisterGraphics_Detour.GetOriginal<decltype(CG_RegisterGraphics)>()(localClientNum, mapname);
        xbox::DbgPrint("CG_RegisterGraphics mapname=%s \n", mapname);
        Load_images();
    }

    Detour Cmd_ExecFromFastFile_Detour;

    bool Cmd_ExecFromFastFile_Hook(int localClientNum, int controllerIndex, const char *filename)
    {
        std::string file_path = "game:\\raw\\";
        file_path += filename;

        if (filesystem::file_exists(file_path))
        {
            std::string contents = filesystem::read_file_to_string(file_path);
            if (!contents.empty())
            {
                Com_Printf(CON_CHANNEL_SYSTEM, "execing %s from raw:\\\n", filename);
                Cbuf_ExecuteBuffer(localClientNum, controllerIndex, contents.c_str());
                return true;
            }
        }

        return Cmd_ExecFromFastFile_Detour.GetOriginal<decltype(Cmd_ExecFromFastFile)>()(localClientNum, controllerIndex, filename);
    }

    Detour R_DrawAllDynEnt_Detour;

    void R_DrawAllDynEnt_Hook(const GfxViewInfo *viewInfo)
    {
        if (Dvar_GetBool("r_drawDynEnts"))
        {
            R_DrawAllDynEnt_Detour.GetOriginal<decltype(R_DrawAllDynEnt)>()(viewInfo);
        }
    }

    dvar_s *pm_cj_hud_enable = nullptr;
    dvar_s *pm_cj_hud_color = nullptr;
    dvar_s *pm_cj_hud_x = nullptr;
    dvar_s *pm_cj_hud_y = nullptr;

    void DrawHudCJ()
    {
        auto ps = CG_GetPredictedPlayerState(0);
        float velocity_2d = sqrtf(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);

        char buff[128];
        sprintf_s(buff,
                  "s: %.2f\n"
                  "z: %.3f\n",
                  velocity_2d, ps->origin[2]);

        static Font_s *font = (Font_s *)R_RegisterFont("fonts/consoleFont");
        float color[4] = {
            pm_cj_hud_color->current.color[0],
            pm_cj_hud_color->current.color[1],
            pm_cj_hud_color->current.color[2],
            pm_cj_hud_color->current.color[3]};
        float x = pm_cj_hud_x->current.integer * scrPlaceFullUnsafe.scaleVirtualToFull[0];
        float y = pm_cj_hud_y->current.integer * scrPlaceFullUnsafe.scaleVirtualToFull[1];
        R_AddCmdDrawText(buff, 256, font, x, y, 1.0, 1.0, 0.0, color, 0);
    }

    Detour CG_DrawActive_Detour;

    void CG_DrawActive_Hook(int localClientNum)
    {
        CheckKeyboardCompletion();

        if (pm_cj_hud_enable->current.enabled)
        {
            DrawHudCJ();
        }

        CG_DrawActive_Detour.GetOriginal<decltype(CG_DrawActive)>()(localClientNum);
    }

    dvar_s *pm_fps_mode = nullptr;

    Detour Sys_SnapVector_Detour;

    void Sys_SnapVector_Hook(float *v)
    {
        if (pm_fps_mode->current.integer == PM_FPS_MODE_PC)
        {
            // Use __frnd for round-to-nearest-even behavior
            v[0] = (float)__frnd((double)v[0]);
            v[1] = (float)__frnd((double)v[1]);
            v[2] = (float)__frnd((double)v[2]);
        }
        else
        {
            Sys_SnapVector_Detour.GetOriginal<decltype(Sys_SnapVector)>()(v);
        }
    }

    dvar_s *bg_bobIdle = nullptr;

    BG_CalculateWeaponPosition_IdleAngles_t BG_CalculateWeaponPosition_IdleAngles = reinterpret_cast<BG_CalculateWeaponPosition_IdleAngles_t>(0x8232CA78);

    Detour BG_CalculateWeaponPosition_IdleAngles_Detour;

    void BG_CalculateWeaponPosition_IdleAngles_Hook(weaponState_t *ws, float *angles)
    {
        if (!bg_bobIdle->current.enabled)
        {
            return;
        }
        BG_CalculateWeaponPosition_IdleAngles_Detour.GetOriginal<decltype(BG_CalculateWeaponPosition_IdleAngles)>()(ws, angles);
    }

    BG_CalculateView_IdleAngles_t BG_CalculateView_IdleAngles = reinterpret_cast<BG_CalculateView_IdleAngles_t>(0x8232C840);

    Detour BG_CalculateView_IdleAngles_Detour;

    void BG_CalculateView_IdleAngles_Hook(viewState_t *vs, float *angles)
    {
        if (!bg_bobIdle->current.enabled)
        {
            return;
        }
        BG_CalculateView_IdleAngles_Detour.GetOriginal<decltype(BG_CalculateView_IdleAngles)>()(vs, angles);
    }

    void DrawBranding()
    {
        const char *branding = "IW3xe";
        const char *version = "0.1.0";
        const char *build = __DATE__ " " __TIME__;

        static Font_s *font = (Font_s *)R_RegisterFont("fonts/consoleFont");
        float color[4] = {1.0, 1.0, 1.0, 0.4};

        R_AddCmdDrawText(branding, 256, font, 10, 20, 1.0, 1.0, 0.0, color, 0);
        R_AddCmdDrawText(version, 256, font, 10, 30, 1.0, 1.0, 0.0, color, 0);
        R_AddCmdDrawText(build, 256, font, 10, 40, 1.0, 1.0, 0.0, color, 0);
    }

    Detour UI_DrawBuildNumber_Detour;

    void UI_DrawBuildNumber_Hook(const int localClientNum)
    {
        DrawBranding();
        CheckKeyboardCompletion();
    }

    void init()
    {
        xbox::DbgPrint("Initializing MP\n");

        auto ps_size = sizeof(playerState_s);
        xbox::DbgPrint("playerState_s size: %d\n", ps_size);

        UI_DrawBuildNumber_Detour = Detour(UI_DrawBuildNumber, UI_DrawBuildNumber_Hook);
        UI_DrawBuildNumber_Detour.Install();

        CG_DrawActive_Detour = Detour(CG_DrawActive, CG_DrawActive_Hook);
        CG_DrawActive_Detour.Install();

        ClientCommand_Detour = Detour(ClientCommand, ClientCommand_Hook);
        ClientCommand_Detour.Install();

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

        CG_RegisterGraphics_Detour = Detour(CG_RegisterGraphics, CG_RegisterGraphics_Hook);
        CG_RegisterGraphics_Detour.Install();

        Cmd_ExecFromFastFile_Detour = Detour(Cmd_ExecFromFastFile, Cmd_ExecFromFastFile_Hook);
        Cmd_ExecFromFastFile_Detour.Install();

        R_DrawAllDynEnt_Detour = Detour(R_DrawAllDynEnt, R_DrawAllDynEnt_Hook);
        R_DrawAllDynEnt_Detour.Install();

        Dvar_RegisterBool("r_drawDynEnts", true, 0, "Draw dynamic entities");

        pm_cj_hud_enable = Dvar_RegisterBool("pm_cj_hud_enable", false, 0, "Draw player speed and z origin");
        pm_cj_hud_color = Dvar_RegisterColor("pm_cj_hud_color", 1.0, 1.0, 1.0, 0.4, 0, "Draw player speed and z origin color");
        pm_cj_hud_x = Dvar_RegisterInt("pm_cj_hud_x", 0, 0, 640, 0, "Virtual screen x coordinate of the player speed and z origin");
        pm_cj_hud_y = Dvar_RegisterInt("pm_cj_hud_y", 470, 0, 480, 0, "Virtual screen y coordinate of the player speed and z origin");

        const char *fps_mode_values[] = {"console", "pc", nullptr};
        pm_fps_mode = Dvar_RegisterEnum("pm_fps_mode", fps_mode_values, PM_FPS_MODE_CONSOLE, 0, "FPS mode");

        Sys_SnapVector_Detour = Detour(Sys_SnapVector, Sys_SnapVector_Hook);
        Sys_SnapVector_Detour.Install();

        bg_bobIdle = Dvar_RegisterBool("bg_bobIdle", true, 0, "Idle gun sway");

        BG_CalculateWeaponPosition_IdleAngles_Detour = Detour(BG_CalculateWeaponPosition_IdleAngles, BG_CalculateWeaponPosition_IdleAngles_Hook);
        BG_CalculateWeaponPosition_IdleAngles_Detour.Install();

        BG_CalculateView_IdleAngles_Detour = Detour(BG_CalculateView_IdleAngles, BG_CalculateView_IdleAngles_Hook);
        BG_CalculateView_IdleAngles_Detour.Install();

        cmd_function_s *cmdinput_VAR = new cmd_function_s;
        Cmd_AddCommandInternal("cmdinput", Cmd_cmdinput_f, cmdinput_VAR);

        cmd_function_s *savepos_VAR = new cmd_function_s;
        Cmd_AddCommandInternal("savepos", nullptr, savepos_VAR);

        cmd_function_s *loadpos_VAR = new cmd_function_s;
        Cmd_AddCommandInternal("loadpos", nullptr, loadpos_VAR);
    }
}
