#include "gpu/rio_Texture.h"
#include "math/rio_Matrix.h"
#include "nn/ffl/FFLGender.h"
#include <nn/ffl/FFLResourceType.h>
#include <nn/ffl/detail/FFLiCharInfo.h>
#include <nn/ffl/FFLiCreateID.h>
#include <EnumStrings.h>
#include <Model.h>
#include <RootTask.h>

#include <filedevice/rio_FileDeviceMgr.h>
#include <gfx/rio_Projection.h>
#include <gfx/rio_Window.h>
#include <gfx/rio_Graphics.h>
#include <gpu/rio_RenderState.h>
#include <gpu/win/rio_Texture2DUtilWin.h>

#include <gfx/mdl/res/rio_ModelCacher.h>

#include <nn/ffl/FFLiSwapEndian.h>
#include <nn/ffl/detail/FFLiCrc.h>

#include <string>
#include <array>

RootTask::RootTask()
    : ITask("FFL Testing")
    , mInitialized(false)
    // disables occasionally drawing mii and makes non blocking
    , mpServerOnly(getenv("SERVER_ONLY"))
    , mpNoSpin(getenv("NO_SPIN"))
{
#ifdef RIO_USE_OSMESA // off screen rendering
    mpServerOnly = "1"; // force it truey
#endif
}

#if RIO_IS_WIN && defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#elif RIO_IS_WIN
#define closesocket close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef USE_SYSTEMD_SOCKET
#include <systemd/sd-daemon.h>
#endif
#endif // RIO_IS_WIN && defined(_WIN32)

#include <nn/ffl/FFLiMiiData.h>
#include <nn/ffl/FFLiMiiDataCore.h>
#include <nn/ffl/FFLiCreateID.h>

#if RIO_IS_WIN

// for opening ffsd folder
#include <filesystem>
#include <fstream>

// read Mii data from a folder
void RootTask::fillStoreDataArray_()
{
    // Path to the folder
    const std::string folderPath = "place_ffsd_files_here";
    // Check if the folder exists
    if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file()
                && entry.file_size() >= sizeof(charInfoStudio)
                && entry.path().filename().string().at(0) != '.')
            {
                // Read the file content
                std::ifstream file(entry.path(), std::ios::binary);
                if (file.is_open()) {
                    std::vector<char> data;
                    data.resize(entry.file_size());
                    file.read(&data[0], entry.file_size());
                    //if (file.gcount() == sizeof(FFLStoreData)) {
                        mStoreDataArray.push_back(data);
                    //}
                    file.close();
                }
            }
        }
        RIO_LOG("Loaded %zu FFSD files into mStoreDataArray\n", mStoreDataArray.size());
    }
}

int server_fd, new_socket;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);

// Setup socket to send data to
void RootTask::setupSocket_()
{
#ifdef USE_SYSTEMD_SOCKET
    if (getenv("LISTEN_FDS")) {
        // systemd socket activation detected
        int n_fds = sd_listen_fds(0);
        if (n_fds > 0) {
            server_fd = SD_LISTEN_FDS_START + 0; // Use only one socket
            mSocketIsListening = true;
            RIO_LOG("\033[1mUsing systemd socket activation, socket fd: %d\033[0m\n", server_fd);

            mpServerOnly = "1"; // force server only when using systemd socket
            return; // Exit the function as the socket is already set up
       }
    }
#endif

    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        perror("WSAStartup failed");
        exit(EXIT_FAILURE);
    }
    #endif // ifdef _WIN32

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    // Get port number from environment or use default
    char portReminder[] ="\033[2m(you can change the port with the PORT environment variable)\033[0m\n";
    const char* env_port = getenv("PORT");
    int port = 12346; // default port
    if (env_port) {
        port = atoi(env_port);
        portReminder[0] = '\0';
    }
    // Forcefully attaching socket to the port
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        RIO_LOG("\033[1m" \
        "TIP: Change the default port of 12346 with the PORT environment variable" \
        "\033[0m\n");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    } else {
        // Set socket to non-blocking mode
        char serverOnlyReminder[] = "\033[1mRemember to add the environment variable SERVER_ONLY=1 to hide the main window.\n\033[0m";
        // except if we are server only, in which case we WANT to block
        if (!mpServerOnly) {
#ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(server_fd, FIONBIO, &mode);
#else
            fcntl(server_fd, F_SETFL, O_NONBLOCK);
#endif
        } else {
            // don't show the reminder with server only
            serverOnlyReminder[0] = '\0';
        }

        mSocketIsListening = true;

        RIO_LOG("\033[1m" \
        "tcp server listening on port %d\033[0m\n" \
        "%s" \
        "%s",
        port, portReminder, serverOnlyReminder);
    }
}
#endif

void RootTask::prepare_()
{
    mInitialized = false;

    FFLInitDesc init_desc;
    init_desc.fontRegion = FFL_FONT_REGION_0;
    init_desc._c = false;
    init_desc._10 = true;

#ifndef TEST_FFL_DEFAULT_RESOURCE_LOADING

#if RIO_IS_CAFE
    FSInit();
#endif // RIO_IS_CAFE

    {
        std::string resPath;
        resPath.resize(256);
        // Middle (now being skipped bc it is not even used here)
        {
            FFLGetResourcePath(resPath.data(), 256, FFL_RESOURCE_TYPE_MIDDLE, false);
            {
                rio::FileDevice::LoadArg arg;
                arg.path = resPath;
                arg.alignment = 0x2000;

                u8* buffer = rio::FileDeviceMgr::instance()->getNativeFileDevice()->tryLoad(arg);
                if (buffer == nullptr)
                {
                    RIO_LOG("NativeFileDevice failed to load: %s\n", resPath.c_str());
                    RIO_LOG("Skipping loading FFL_RESOURCE_TYPE_MIDDLE\n");
                    // I added a line that skips the resource if the size is zero
                    mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] = 0;
                } else {
                    mResourceDesc.pData[FFL_RESOURCE_TYPE_MIDDLE] = buffer;
                    mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] = arg.read_size;
                }
            }
        }
        //mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] = 0;
        // High, load from FFL path or current working directory
        {
            // Two different paths
            std::array<std::string, 2> resPaths = {"", "./FFLResHigh.dat"};
            resPaths[0].resize(256);
            FFLGetResourcePath(resPaths[0].data(), 256, FFL_RESOURCE_TYPE_HIGH, false);

            bool resLoaded = false;

            // Try two different paths
            for (const auto& resPath : resPaths) {
                rio::FileDevice::LoadArg arg;
                arg.path = resPath;
                arg.alignment = 0x2000;

                u8* buffer = rio::FileDeviceMgr::instance()->getNativeFileDevice()->tryLoad(arg);
                if (buffer != nullptr) {
                    mResourceDesc.pData[FFL_RESOURCE_TYPE_HIGH] = buffer;
                    mResourceDesc.size[FFL_RESOURCE_TYPE_HIGH] = arg.read_size;
                    resLoaded = true;
                    // Break when one loads successfully
                    break;
                } else {
                    RIO_LOG("NativeFileDevice failed to load: %s\n", arg.path.c_str());
                }
            }

            if (!resLoaded) {
                RIO_LOG("Was not able to load high resource!!!\n");
                RIO_LOG("\033[1;31mThe FFLResHigh.dat needs to be present, or else this program won't work. It will probably crash right now.\033[0m\n");
                RIO_ASSERT(false);
            }
        }
    }

    FFLResult result = FFLInitResEx(&init_desc, &mResourceDesc);
#else
    FFLResult result = FFLInitResEx(&init_desc, nullptr);
#endif
    if (result != FFL_RESULT_OK)
    {
        RIO_LOG("FFLInitResEx() failed with result: %s\n", FFLResultToString(result));
        RIO_ASSERT(false);
        return;
    }

    RIO_ASSERT(FFLIsAvailable());

    FFLInitResGPUStep();

    initializeShaders_();

    // Get window instance
    const rio::Window* const window = rio::Window::instance();
    // Set projection matrix
    {
        // Calculate the aspect ratio based on the window dimensions
        f32 aspect = f32(window->getWidth()) / f32(window->getHeight());
        // Calculate the field of view (fovy) based on the given parameters
        f32 fovy;
        if (f32(window->getWidth()) < f32(window->getHeight())) {
            fovy = 2 * atan2f(43.2f / aspect, 500.0f);
        } else {
            fovy = 2 * atan2f(43.2f, 500.0f);
        }
        // C_MTXPerspective(Mtx44 m, f32 fovy, f32 aspect, f32 near, f32 far)
        // PerspectiveProjection(f32 near, f32 far, f32 fovy, f32 aspect)
        // RFLiMakeIcon: C_MTXPerspective(projMtx, fovy, aspect, 500.0f, 700.0f)
        // GetFaceMatrix: C_MTXPerspective(projMtx, 15.0, 1.0, 10.0, 1000.0);

        // Create perspective projection instance
        rio::PerspectiveProjection proj(
            500.0f, // Near
            1200.0f, // Far
            fovy,// * 1.6f, // fovy
            aspect // Aspect ratio
        );
        // The near and far values define the depth range of the view frustum (500.0f to 700.0f)

        // Calculate matrix
        mProjMtx = proj.getMatrix();

        rio::PerspectiveProjection projIconBody(
            10.0f,//10.0f,
            1000.0f,
            rio::Mathf::deg2rad(15.0f),
            1.0f
        );
        mProjMtxIconBody = new rio::BaseMtx44f(projIconBody.getMatrix());

    }

    #if RIO_IS_WIN
    fillStoreDataArray_();
    setupSocket_();
    // Set window aspect ratio, so that when resizing it will not change
    GLFWwindow* glfwWindow = rio::Window::instance()->getNativeWindow().getGLFWwindow();
    glfwSetWindowAspectRatio(glfwWindow, window->getWidth(), window->getHeight());
    #endif

    mMiiCounter = 0;
    createModel_();

    // load (just male for now) body model
    // the male and female bodies are like identical
    // from the torso up (only perspective we use)
    char key[] = "mii_static_body0";
    for (u32 i = 0; i < FFL_GENDER_MAX; i++)
    {
        RIO_LOG("loading model: %s\n", key);
        //bodyResModels[i] = rio::mdl::res::ModelCacher::instance()->loadModel(key, key); // "body0", "body1" per gender
        const rio::mdl::res::Model* resModel = rio::mdl::res::ModelCacher::instance()->loadModel(key, key);
        key[sizeof(key)-2]++; // iterate last character
        RIO_ASSERT(resModel);

        mpBodyModels[i] = new rio::mdl::Model(resModel);
    }
    //RIO_ASSERT(loadBodyGLTF(&gltfBodyModel, bodyGltfMaleFilename));
    //bodyShader.load("FFLBodyShader");

    mInitialized = true;
}

// amount of mii indexes to cycle through
// default source only has 6
// GetMiiDataNum()
int maxMiis = 6;

// create model for displaying on screen
void RootTask::createModel_() {
    FFLCharModelSource modelSource;

    // default model source if there is no socket
    #if RIO_IS_CAFE
        // use mii maker database on wii u
        modelSource.dataSource = FFL_DATA_SOURCE_OFFICIAL;
        // NOTE: will only use first 6 miis from mii maker database
    #else
    FFLiCharInfo charInfo;
    if (!mStoreDataArray.empty()) {
        // Use the custom Mii data array
        RenderRequest fakeRenderRequest;
        fakeRenderRequest.dataLength = static_cast<u16>(mStoreDataArray[mMiiCounter].size());
        fakeRenderRequest.verifyCRC16 = false;
        rio::MemUtil::copy(fakeRenderRequest.data, &mStoreDataArray[mMiiCounter][0], fakeRenderRequest.dataLength);
        if (pickupCharInfoFromRenderRequest(&charInfo, &fakeRenderRequest) != FFL_RESULT_OK)
        {
            RIO_LOG("pickupCharInfoFromRenderRequest failed on Mii counter: %d\n", mMiiCounter);
            mpModel = nullptr;
            mCounter = 0.0f;
            mMiiCounter = (mMiiCounter + 1) % maxMiis;
            return;
        }

        modelSource.index = 0;
        modelSource.dataSource = FFL_DATA_SOURCE_DIRECT_POINTER;
        modelSource.pBuffer = &charInfo;
        // limit current counter by the amount of custom miis
        maxMiis = static_cast<int>(mStoreDataArray.size());
    } else {
        // default mii source, otherwise known as guest miis
        modelSource.dataSource = FFL_DATA_SOURCE_DEFAULT;
        // guest miis are defined in FFLiDatabaseDefault.cpp
        // fetched from m_MiiDataOfficial, derived from the static array MII_DATA_CORE_RFL
    #endif
        modelSource.index = mMiiCounter;
        modelSource.pBuffer = NULL;
    #if !RIO_IS_CAFE
    }
    #endif

    // limit current counter by the amount
    // of max miis (6 for default/guest miis)
    mMiiCounter = (mMiiCounter + 1) % maxMiis;


    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = FFLResolution(768),
            .expressionFlag = 1 << 0,
            .modelFlag = 1 << 0 | 1 << 1 | 1 << 2,
            .resourceType = FFL_RESOURCE_TYPE_HIGH,
        },
        .source = modelSource
    };

    mpModel = new Model();
    ShaderType shaderType = SHADER_TYPE_WIIU;//(mMiiCounter-1) % (SHADER_TYPE_MAX);
    if (!mpModel->initialize(arg, *mpShaders[shaderType])) {
        delete mpModel;
        mpModel = nullptr;
    } /*else {
        mpModel->setScale({ 1.f, 1.f, 1.f });
        //mpModel->setScale({ 1 / 16.f, 1 / 16.f, 1 / 16.f });
    }*/
    mCounter = 0.0f;
}

FFLResult pickupCharInfoFromRenderRequest(FFLiCharInfo* pCharInfo, RenderRequest *buf) {
    MiiDataInputType inputType;

    switch (buf->dataLength) {
        /*case sizeof(FFLStoreData):
        case sizeof(FFLiMiiDataOfficial):
        case sizeof(FFLiMiiDataCore):
            inputType = INPUT_TYPE_FFL_MIIDATACORE;
            break;
        */
        case 76: // RFLStoreData, FFLiStoreDataRFL ....????? (idk if this exists)
            inputType = INPUT_TYPE_RFL_STOREDATA;
            break;
        case 74: // RFLCharData, FFLiMiiDataOfficialRFL
            inputType = INPUT_TYPE_RFL_CHARDATA;
            break;
        case sizeof(charInfo): // nx char info
            inputType = INPUT_TYPE_NX_CHARINFO;
            break;
        case sizeof(coreData):
        case 68://sizeof(storeData):
            inputType = INPUT_TYPE_NX_COREDATA;
            break;
        case sizeof(charInfoStudio): // studio raw
            inputType = INPUT_TYPE_STUDIO_RAW;
            break;
        case STUDIO_DATA_ENCODED_LENGTH: // studio encoded i think
            inputType = INPUT_TYPE_STUDIO_ENCODED;
            break;
        case sizeof(FFLiMiiDataCore):
        case sizeof(FFLiMiiDataOfficial): // creator name unused
            inputType = INPUT_TYPE_FFL_MIIDATACORE;
            break;
        case sizeof(FFLStoreData):
            inputType = INPUT_TYPE_FFL_STOREDATA;
            break;
        default:
            // uh oh, we can't detect it
            return FFL_RESULT_ERROR;
            break;
    }

    // create temporary charInfoNX for studio, coredata
    charInfo charInfoNX;

    switch (inputType) {
        case INPUT_TYPE_RFL_STOREDATA:
            // 76 = sizeof RFLStoreData
            if (buf->verifyCRC16 && !FFLiIsValidCRC16(buf->data, 76))
                return FFL_RESULT_FILE_INVALID;
            [[fallthrough]];
        case INPUT_TYPE_RFL_CHARDATA:
        {
            // it is NOT FFLiMiiDataCore, it may be RFL tho
            // cast to FFLiMiIDataCoreRFL to check create id
            FFLiMiiDataCoreRFL& charDataRFLData = reinterpret_cast<FFLiMiiDataCoreRFL&>(buf->data);
            FFLiMiiDataCoreRFL charDataRFL;
            rio::MemUtil::copy(&charDataRFL, &charDataRFLData, sizeof(FFLiMiiDataCoreRFL));
            // look at create id to run FFLiIsNTRMiiID
            // NOTE: FFLiMiiDataCoreRFL2CharInfo is SUPPOSED to check
            // whether it is an NTR mii id or not and store
            // the result as pCharInfo->birthPlatform = FFL_BIRTH_PLATFORM_NTR
            // HOWEVER, it clears out the create ID and runs the compare anyway
            // the create id is actually not the same size either


            // NOTE: NOT WORKING ATM
            /*FFLCreateID* createIDRFL = reinterpret_cast<FFLCreateID*>(charDataRFL.m_CreatorID);

            // test for if it is NTR data
            bool isNTR = FFLiIsNTRMiiID(createIDRFL);
            RIO_LOG("IS NTR??? %i\n", isNTR);
    */
            // NOTE: NFLCharData for DS can be little endian tho!!!!!!

#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
            // swap endian for rfl
            static const FFLiSwapEndianDesc SWAP_ENDIAN_DESC_RFL[] = {
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_Flag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 10 }, // m_Name
                { FFLI_SWAP_ENDIAN_TYPE_U8,  1 },  // m_Height
                { FFLI_SWAP_ENDIAN_TYPE_U8,  1 },  // m_Build
                { FFLI_SWAP_ENDIAN_TYPE_U8,  4 },  // m_CreatorID
                { FFLI_SWAP_ENDIAN_TYPE_U8,  4 },  // m_SystemID
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_FaceFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_HairFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 2 },  // m_EyebrowFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 2 },  // m_EyeFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_NoseFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_MouthFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_GlassFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_BeardFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 1 },  // m_MoleFlag
                { FFLI_SWAP_ENDIAN_TYPE_U16, 10 } // creator name
            };
            FFLiSwapEndianGroup(&charDataRFL, SWAP_ENDIAN_DESC_RFL, sizeof(SWAP_ENDIAN_DESC_RFL) / sizeof(FFLiSwapEndianDesc));
#endif
            /*if (pCharInfo->birthPlatform != FFL_BIRTH_PLATFORM_NTR) {
                // flip endian
            }
            isNTR = FFLiIsNTRMiiID(createIDRFL);
            RIO_LOG("IS NTR (2)??? %i\n", isNTR);
            */

            FFLiMiiDataCoreRFL2CharInfo(pCharInfo,
                charDataRFL,
                NULL, false
                //reinterpret_cast<u16*>(&buf->data[0x36]), true // creator name
                // name offset: https://github.com/SMGCommunity/Petari/blob/53fd4ff9db54cb1c91a96534dcae9f2c2ea426d1/libs/RVLFaceLib/include/RFLi_Types.h#L342
            );
            break;
        }
        case INPUT_TYPE_STUDIO_ENCODED:
        {
            // mii studio url data format is obfuscated
            // decodes it in a buffer
            char decodedData[STUDIO_DATA_ENCODED_LENGTH];
            rio::MemUtil::copy(decodedData, buf->data, STUDIO_DATA_ENCODED_LENGTH);
            studioURLObfuscationDecode(decodedData);
            studioToCharInfoNX(&charInfoNX, reinterpret_cast<::charInfoStudio*>(decodedData));
            charInfoNXToFFLiCharInfo(pCharInfo, &charInfoNX);
            break;
        }
        case INPUT_TYPE_STUDIO_RAW:
            // we may not need this if we decode from and to buf->data but that's confusing
            studioToCharInfoNX(&charInfoNX, reinterpret_cast<::charInfoStudio*>(buf->data));
            charInfoNXToFFLiCharInfo(pCharInfo, &charInfoNX);
            break;
        case INPUT_TYPE_NX_CHARINFO:
            charInfoNXToFFLiCharInfo(pCharInfo, reinterpret_cast<::charInfo*>(buf->data));
            break;
        case INPUT_TYPE_NX_COREDATA:
            coreDataToCharInfoNX(&charInfoNX, reinterpret_cast<::coreData*>(buf->data));
            charInfoNXToFFLiCharInfo(pCharInfo, &charInfoNX);
            break;
        case INPUT_TYPE_FFL_STOREDATA:
            // only verify CRC16, then fall through
            if (buf->verifyCRC16 && !FFLiIsValidCRC16(buf->data, sizeof(FFLiStoreDataCFL)))
                return FFL_RESULT_FILE_INVALID;
            [[fallthrough]];
        case INPUT_TYPE_FFL_MIIDATACORE:
            // TODO: SWAP ENDIAN ON WII U OR HANDLE BOTH KINDS
            FFLiMiiDataCore2CharInfo(pCharInfo,
                reinterpret_cast<FFLiMiiDataCore&>(buf->data),
            // const u16* pCreatorName, bool resetBirthday
            NULL, false);
            break;
        default: // unknown
            return FFL_RESULT_ERROR;
            break;
    }
    return FFL_RESULT_OK;
}

const std::string socketErrorPrefix = "ERROR: ";

// create model for render request
bool RootTask::createModel_(RenderRequest* buf, int socket_handle) {
    FFLiCharInfo charInfo;


    std::string errMsg;

    FFLResult pickupResult = pickupCharInfoFromRenderRequest(&charInfo, buf);

    if (pickupResult != FFL_RESULT_OK) {
        if (pickupResult == FFL_RESULT_FILE_INVALID) { // crc16 fail
            errMsg = "Data CRC16 verification failed.\n";
        } else {
            errMsg = "Unknown data type (pickupCharInfoFromRenderRequest failed)\n";
        }
        RIO_LOG("%s", errMsg.c_str());
        errMsg = socketErrorPrefix + errMsg;
        send(socket_handle, errMsg.c_str(), errMsg.length(), 0);
        return false;
    }

    // VERIFY CHARINFO
    if (buf->verifyCharInfo) {
        // get verify char info reason, don't verify name
        FFLiVerifyCharInfoReason verifyCharInfoReason =
            FFLiVerifyCharInfoWithReason(&charInfo, false);
        // I think I want to separate making the model
        // and picking up CharInfo from the request LATER
        // and then apply it when I do that

        if (verifyCharInfoReason != FFLI_VERIFY_CHAR_INFO_REASON_OK) {
            // CHARINFO IS INVALID, FAIL!
            errMsg = "FFLiVerifyCharInfoWithReason (data verification) failed: "
            + std::string(FFLiVerifyCharInfoReasonToString(verifyCharInfoReason))
            + "\n";
            RIO_LOG("%s", errMsg.c_str());
            errMsg = socketErrorPrefix + errMsg;
            send(socket_handle, errMsg.c_str(), errMsg.length(), 0);
            return false;
        }
/*
        if (FFLiIsNullMiiID(&charInfo.creatorID)) {
            errMsg = "FFLiIsNullMiiID returned true (this data will not work on a real console)\n";
            RIO_LOG("%s", errMsg.c_str());
            errMsg = socketErrorPrefix + errMsg;
            send(socket_handle, errMsg.c_str(), errMsg.length(), 0);
            return false;
        }
*/
    }

    // NOTE NOTE: MAKE SURE TO CHECK NULL MII ID TOO


    FFLCharModelSource modelSource;
    // needs to be initialized to zero
    modelSource.index = 0;
    // don't call PickupCharInfo or verify mii after we already did
    modelSource.dataSource = FFL_DATA_SOURCE_DIRECT_POINTER;
    //modelSource.dataSource = FFL_DATA_SOURCE_BUFFER; // aka CharInfo
    modelSource.pBuffer = &charInfo;

    const FFLExpressionFlag expressionFlag = (buf->expressionFlag != 0 ? buf->expressionFlag
        : static_cast<FFLExpressionFlag>(1) << buf->expression
        // % (FFL_EXPRESSION_MAX - 1),
    );

    FFLResolution texResolution;
    if (buf->texResolution < 0) { // if it is negative...
        texResolution = static_cast<FFLResolution>(
            static_cast<u32>(buf->texResolution * -1) // remove negative
            | FFL_RESOLUTION_MIP_MAP_ENABLE_MASK); // enable mipmap
    } else {
        texResolution = static_cast<FFLResolution>(buf->texResolution);
    }

    // otherwise just fall through and use default
    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = texResolution,
            .expressionFlag = expressionFlag,
            // model flag includes model type (required)
            // and flatten nose bit at pos 4
            .modelFlag = static_cast<u32>(buf->modelFlag),
            .resourceType = static_cast<FFLResourceType>(buf->resourceType),
        },
        .source = modelSource
    };

    mpModel = new Model();
    ShaderType whichShader = SHADER_TYPE_WIIU;
    if (buf->shaderType < SHADER_TYPE_MAX)
        whichShader = static_cast<ShaderType>(buf->shaderType);

    // this should happen after charinfo verification by the way
    if (buf->shaderType == SHADER_TYPE_MIITOMO)
        // miitomo erroneously makes glasses larger so we will too
        // NOTE: REMOVE LATER..??
        charInfo.parts.glassScale += 1;
    if (!mpModel->initialize(arg, *mpShaders[whichShader])) {
        errMsg = "FFLInitCharModelCPUStep FAILED while initializing model: "
        + std::string(FFLResultToString(mpModel->getInitializeCpuResult()))
        + "\n";
        RIO_LOG("%s", errMsg.c_str());
        errMsg = socketErrorPrefix + errMsg;
        send(socket_handle, errMsg.c_str(), errMsg.length(), 0);
        delete mpModel;
        mpModel = nullptr;
        return false;
    }

    mCounter = 0.0f;
    return true;
}

void copyAndSendRenderBufferToSocket(rio::RenderBuffer& renderBuffer, rio::Texture2D* texture, int socket, int ssaaFactor) {
    // does operations on the renderbuffer assuming it is already bound
#ifdef TRY_SCALING
    const u32 width = texture->getWidth() / ssaaFactor;
    const u32 height = texture->getHeight() / ssaaFactor;
#else
    const u32 width = texture->getWidth();
    const u32 height = texture->getHeight();
#endif
    // Read the rendered data into a buffer and save it to a file
    u32 bufferSize = rio::Texture2DUtil::calcImageSize(texture->getTextureFormat(), width, height);














#ifdef TRY_SCALING

    rio::RenderBuffer renderBufferDownsample;
    renderBufferDownsample.setSize(width, height);
    RIO_GL_CALL(glViewport(0, 0, width, height));
    //rio::Window::instance()->getNativeWindow()->getColorBufferTextureFormat();
    rio::Texture2D *renderTextureDownsampleColor = new rio::Texture2D(rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM, renderBufferDownsample.getSize().x, renderBufferDownsample.getSize().y, 1);
    rio::RenderTargetColor renderTargetDownsampleColor;
    renderTargetDownsampleColor.linkTexture2D(*renderTextureDownsampleColor);
    renderBufferDownsample.setRenderTargetColor(&renderTargetDownsampleColor);
    renderBufferDownsample.clear(rio::RenderBuffer::CLEAR_FLAG_COLOR_DEPTH_STENCIL, { 0.0f, 0.0f, 0.0f, 0.0f });
    renderBufferDownsample.bind();
    rio::RenderState render_state;
    render_state.setBlendEnable(true);

    // premultiplied alpha blending
    render_state.setBlendEquation(rio::Graphics::BLEND_FUNC_ADD);
    render_state.setBlendFactorSrcRGB(rio::Graphics::BLEND_MODE_ONE);
    render_state.setBlendFactorDstRGB(rio::Graphics::BLEND_MODE_ONE_MINUS_SRC_ALPHA);

    render_state.setBlendEquationAlpha(rio::Graphics::BLEND_FUNC_ADD);
    render_state.setBlendFactorSrcAlpha(rio::Graphics::BLEND_MODE_ONE_MINUS_DST_ALPHA);
    render_state.setBlendFactorDstAlpha(rio::Graphics::BLEND_MODE_ONE);
    render_state.apply();

    // Load and compile the downsampling shader
    rio::Shader downsampleShader;
    downsampleShader.load("TransparentAdjuster");
    downsampleShader.bind();

    // Bind the high-resolution texture
    rio::TextureSampler2D highResSampler;
    highResSampler.linkTexture2D(texture);
    highResSampler.tryBindFS(downsampleShader.getFragmentSamplerLocation("s_Tex"), 0);

    const float oneDivisionWidth = 1.0f / (width * ssaaFactor);
    const float oneDivisionHeight = 1.0f / (height * ssaaFactor);

    // Set shader uniforms if needed
    downsampleShader.setUniform(oneDivisionWidth, oneDivisionHeight, u32(-1), downsampleShader.getFragmentUniformLocation("u_OneDivisionResolution"));
    // Render a full-screen quad to apply the downsampling shader
    GLuint quadVAO, quadVBO;

    float quadVertices[] = {
#ifndef RIO_NO_CLIP_CONTROL
        -1.0f, 1.0f,  0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f,  -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,  0.0f, 0.0f,
        1.0f,  -1.0f, 1.0f, 1.0f,
        1.0f,  1.0f,  1.0f, 0.0f
#else
        -1.0f, -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f,  0.0f, 1.0f,
        1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f,
        1.0f,  -1.0f, 1.0f, 0.0f
#endif
    };

    RIO_GL_CALL(glGenVertexArrays(1, &quadVAO));
    RIO_GL_CALL(glGenBuffers(1, &quadVBO));
    RIO_GL_CALL(glBindVertexArray(quadVAO));
    RIO_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
    RIO_GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));
    RIO_GL_CALL(glEnableVertexAttribArray(0));
    RIO_GL_CALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0));
    RIO_GL_CALL(glEnableVertexAttribArray(1));
    RIO_GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))));
    RIO_GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 6));
    RIO_GL_CALL(glBindVertexArray(0));
    // Unbind the framebuffer
    RIO_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    // Clean up
    RIO_GL_CALL(glDeleteVertexArrays(1, &quadVAO));
    RIO_GL_CALL(glDeleteBuffers(1, &quadVBO));
    downsampleShader.unload();

    renderBufferDownsample.bind();




    //int bufferSize = renderRequest->resolution * renderRequest->resolution * 4;
/*
    // Create a regular framebuffer to resolve the MSAA buffer to
    GLuint resolveFBO, resolveColorRBO;
    glGenFramebuffers(1, &resolveFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

    // Create and attach a regular color renderbuffer
    glGenRenderbuffers(1, &resolveColorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, resolveColorRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, renderRequest->resolution, renderRequest->resolution);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, resolveColorRBO);


    // Blit (resolve) the multisampled framebuffer to the regular framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
    glBlitFramebuffer(0, 0, renderRequest->resolution, renderRequest->resolution,
                    0, 0, renderRequest->resolution, renderRequest->resolution,
                    GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

    RIO_GL_CALL(glReadPixels(0, 0, renderRequest->resolution, renderRequest->resolution, GL_RGBA, GL_UNSIGNED_BYTE, readBuffer));
*/

#endif

    //renderBufferDownsample.read(0, readBuffer, renderBufferDownsample.getSize().x, renderBufferDownsample.getSize().y, renderTextureDownsampleColor->getNativeTexture().surface.nativeFormat);

    rio::NativeTextureFormat nativeFormat = texture->getNativeTexture().surface.nativeFormat;

#if RIO_IS_WIN
/*
    glReadBuffer(GL_COLOR_ATTACHMENT0 + 0);
    glReadPixels(0, 0, width, height, nativeFormat.format, nativeFormat.type, readBuffer);
*/
    // map a pixel buffer object (DMA?)
    GLuint pbo;
    RIO_GL_CALL(glGenBuffers(1, &pbo));
    RIO_GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo));
    RIO_GL_CALL(glBufferData(GL_PIXEL_PACK_BUFFER, bufferSize, nullptr, GL_STREAM_READ));

    // Read pixels into PBO (asynchronously)
    RIO_GL_CALL(glReadPixels(0, 0, width, height, nativeFormat.format, nativeFormat.type, nullptr));

    // Map the PBO to read the data
    void* readBuffer;
    // opengl 2 and below compatible function
    //RIO_GL_CALL(readBuffer = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
    // this should be compatible with OpenGL 3.3 and OpenGL ES 3.0
    RIO_GL_CALL(readBuffer = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, bufferSize, GL_MAP_READ_BIT));

    // don't dereference a null pointer
    if (readBuffer) {
        // Process the data in readBuffer
        RIO_LOG("Rendered data read successfully from the buffer.\n");
        send(new_socket, reinterpret_cast<char*>(readBuffer), bufferSize, 0);
        RIO_GL_CALL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
    } else {
        RIO_ASSERT(!readBuffer && "why is readBuffer nullptr...???");
    }

    // Unbind the PBO
    RIO_GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
    RIO_GL_CALL(glDeleteBuffers(1, &pbo));
// NOTE: renderBuffer.read() DOES NOT WORK ON CAFE AS OF WRITING, NOR IS THIS MEANT TO RUN ON WII U AT ALL LOL
#else
    u8* readBuffer = new u8[bufferSize];
    //rio::MemUtil::set(readBuffer, 0xFF, bufferSize);
    // NOTE: UNTESTED
    renderBuffer.read(0, readBuffer, renderBuffer.getSize().x, renderBuffer.getSize().y, nativeFormat);
    send(new_socket, reinterpret_cast<char*>(readBuffer), bufferSize, 0);
    delete[] readBuffer;
#endif

    RIO_LOG("Wrote %d bytes out to socket.\n", bufferSize);
}

// scale vec3 for body
const rio::Vector3f calculateScaleFactors(f32 build, f32 height) {
    rio::Vector3f scaleFactors;
    // referenced in anonymous function in nn::mii::detail::VariableIconBodyImpl::CalculateWorldMatrix
    // also in ffl_app.rpx: FUN_020ec380 (FFLUtility), FUN_020737b8 (mii maker US)
#ifndef USE_HEIGHT_LIMIT_SCALE_FACTORS
    // ScaleApply?
    // 0.47 / 128.0 = 0.003671875
    scaleFactors.x = (build * (height * 0.003671875f + 0.4f)) / 128.0f +
                    // 0.23 / 128.0 = 0.001796875
                    height * 0.001796875f + 0.4f;
                    // 0.77 / 128.0 = 0.006015625
    scaleFactors.y = (height * 0.006015625f) + 0.5f;

    /* the following set is found in ffl_app.rpx (FFLUtility)
     * Q:/sugar/program/ffl_application/src/mii/body/Scale.cpp
     * when an input is set to 1 (enum ::mii::body::ScaleMode?)
     * it may be for limiting scale so that the pants don't show
     * this may be what is used in wii u mii maker bottom screen icons but otherwise the above factors seem more relevant
    */
#else
    // ScaleLimit?

    // NOTE: even in wii u mii maker this still shows a few
    // pixels of the pants, but here without proper body scaling
    // this won't actually let you get away w/o pants
    f32 heightFactor = height / 128.0f;
    scaleFactors.y = heightFactor * 0.55 + 0.6;
    scaleFactors.x = heightFactor * 0.3 + 0.6;
    scaleFactors.x = ((heightFactor * 0.6 + 0.8) - scaleFactors.x) *
                        (build / 128.0f) + scaleFactors.x;
#endif

    // z is always set to x for either set
    scaleFactors.z = scaleFactors.x;

    return scaleFactors;
}

void RootTask::drawMiiBodyREAL(const bool light_enable, FFLiCharInfo* charInfo, rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx, rio::BaseMtx44f& proj_mtx) {

    // SELECT BODY MODEL
    if (charInfo->gender > FFL_GENDER_MAX - 1)
        return;

    const rio::mdl::Model* model = mpBodyModels[charInfo->gender];

    const rio::mdl::Mesh* const meshes = model->meshes();

    // Render each mesh in order
    for (u32 i = 0; i < model->numMeshes(); i++)
    {
        const rio::mdl::Mesh& mesh = meshes[i];

        // BIND SHADER
        //bodyShader.bind();
        mpModel->getShader()->bindBodyShader(light_enable, charInfo);


        static const f32 bodyToHeadOffset = -93.92f;

        rio::Vector3f scaleFactors = calculateScaleFactors(static_cast<f32>(charInfo->build), static_cast<f32>(charInfo->height));
        // make new matrix for body
        rio::Matrix34f modelMtxBody = rio::Matrix34f::ident;//model_mtx;

        // apply scale factors before anything else
        modelMtxBody.applyScaleLocal(scaleFactors);
        // apply transformation for body head offset
        modelMtxBody.applyTranslationLocal({ 0.0f, bodyToHeadOffset, 0.0f });
        // apply original model matrix (rotation)
        modelMtxBody.setMul(model_mtx, modelMtxBody);

        mpModel->getShader()->setViewUniformBody(modelMtxBody, view_mtx, proj_mtx);

        // finally, the uniforms are all set

        rio::RenderState render_state;
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_BACK);
        render_state.applyCullingAndPolygonModeAndPolygonOffset();
        mesh.draw();
    }
    //bodyShader.unload();
}

// Convert degrees to radians
rio::Vector3f convertVec3iToRadians3f(const int16_t degrees[3]) {
    rio::Vector3f radians;
    radians.x = rio::Mathf::deg2rad(fmod(static_cast<f32>(degrees[0]), 360.0f));
    radians.y = rio::Mathf::deg2rad(fmod(static_cast<f32>(degrees[1]), 360.0f));
    radians.z = rio::Mathf::deg2rad(fmod(static_cast<f32>(degrees[2]), 360.0f));
    return radians;
}

// Calculate position based on spherical coordinates
rio::Vector3f calculateCameraOrbitPosition(f32 radius, const rio::Vector3f& radians) {
    rio::Vector3f position;
    position.x = radius * -std::sin(radians.y) * std::cos(radians.x);
    position.y = radius * std::sin(radians.x);
    position.z = radius * std::cos(radians.y) * std::cos(radians.x);
    return position;
}

// Calculate up vector based on z rotation
rio::Vector3f calculateUpVector(const rio::Vector3f& radians) {
    rio::Vector3f up;
    up.x = std::sin(radians.z);
    up.y = std::cos(radians.z);
    up.z = 0.0f;
    return up;
}

void RootTask::handleRenderRequest(char* buf) {
    if (mpModel == nullptr) {
        /*const char* errMsg = "mpModel == nullptr";
        send(new_socket, errMsg, strlen(errMsg), 0);
        */
        closesocket(new_socket);
        return;
    }

    // hopefully renderrequest is proper
    RenderRequest* renderRequest = reinterpret_cast<RenderRequest*>(buf);

    if (renderRequest->exportAsGLTF) {
#ifndef NO_GLTF
        handleGLTFRequest(renderRequest);
#endif
        closesocket(new_socket);
        return;
    }

    // mask ONLY - which was already initialized, so nothing else needs to happen
    if (renderRequest->drawStageMode == DRAW_STAGE_MODE_MASK_ONLY) {
        FFLiCharModel* pCharModel = reinterpret_cast<FFLiCharModel*>(mpModel->getCharModel());

        // select mask texture for current expression
        const FFLiRenderTexture* pRenderTexture = pCharModel->maskTextures.pRenderTextures[pCharModel->expression];
        RIO_ASSERT(pRenderTexture != nullptr);

        pRenderTexture->pRenderBuffer->bind();

        // NOTE the resolution of this is the texture resolution so that would have to match what the client expects
        copyAndSendRenderBufferToSocket(*pRenderTexture->pRenderBuffer, pRenderTexture->pTexture2D, new_socket, 1);

        closesocket(new_socket);
        return;
    }
    // switch between two projection matrxies
    rio::BaseMtx44f *projMtx;
    switch (renderRequest->viewType) {
        case VIEW_TYPE_FACE:
        case VIEW_TYPE_FACE_ONLY:
        {
            // if it has body then use the matrix we just defined
            projMtx = mProjMtxIconBody;

            //RIO_LOG("x = %i, y = %i, z = %i\n", renderRequest->cameraRotate.x, renderRequest->cameraRotate.y, renderRequest->cameraRotate.z);
            /*
            rio::Vec3f fCameraPosition = {
                fmod(static_cast<f32>(renderRequest->cameraRotate.x), 360),
                fmod(static_cast<f32>(renderRequest->cameraRotate.y), 360),
                fmod(static_cast<f32>(renderRequest->cameraRotate.z), 360),
            };*/

            // FFLMakeIconWithBody view uses 37.05f, 415.53f
            // below values are extracted from wii u mii maker
            mCamera.pos() = { 0.0f, 33.016785f, 411.181793f };
            mCamera.at() = { 0.0f, 33.016785f, 0.0f };
            mCamera.setUp({ 0.0f, 1.0f, 0.0f });
            break;
        }
        case VIEW_TYPE_NNMII_VARIABLEICONBODY_VIEW:
        {
            projMtx = mProjMtxIconBody;
            // nn::mii::VariableIconBody::StoreCameraMatrix values
            mCamera.pos() = { 0.0f, 37.0f, 380.0f };
            mCamera.at() = { 0.0f, 37.0f, 0.0f };
            mCamera.setUp({ 0.0f, 1.0f, 0.0f });
            break;
        }
        case VIEW_TYPE_ALL_BODY:
        {
            projMtx = mProjMtxIconBody;
            // NOTE: NOTE: TODO: NOT ACCURATE TO ANYTHING JUST GUESSING
            mCamera.pos() = { 0.0f, 9.0f, 900.0f };
            mCamera.at() = { 0.0f, 6.0f, 0.0f };
            mCamera.setUp({ 0.0f, 1.0f, 0.0f });
            break;
        }
        default:
        {
            // default, face only (FFLMakeIcon)
            // use default if request is head only
            projMtx = &mProjMtx;

            mCamera.at() = { 0.0f, 34.5f, 0.0f };
            mCamera.setUp({ 0.0f, 1.0f, 0.0f });

            mCamera.pos() = { 0.0f, 34.5f, 600.0f };
            break;
        }
    }


    // Update camera position
    const rio::Vector3f cameraPosInitial = mCamera.pos();
    const f32 radius = cameraPosInitial.z;

    const rio::Vector3f cameraRotate = convertVec3iToRadians3f(renderRequest->cameraRotate);
    const rio::Vector3f modelRotate = convertVec3iToRadians3f(renderRequest->modelRotate);

    rio::Vector3f position = calculateCameraOrbitPosition(radius, cameraRotate);
    position.y += cameraPosInitial.y;
    const rio::Vector3f upVector = calculateUpVector(cameraRotate);

    mCamera.pos() = position;
    mCamera.setUp(upVector);

    rio::Matrix34f view_mtx;
    mCamera.getMatrix(&view_mtx);

    rio::Matrix34f model_mtx = rio::Matrix34f::ident;
    //rio::Matrix34f rotationMtx;
    model_mtx.makeR(modelRotate); // NOTE: makes the model matrix, the rotation matrix directly

    //model_mtx.setMul(rio::Matrix34f::ident, rotationMtx);

    mpModel->setMtxRT(model_mtx);

    #ifdef RIO_NO_CLIP_CONTROL
        // render upside down so glReadPixels is right side up
        if (projMtx->m[1][1] > 0) // if it is not already negative
            projMtx->m[1][1] *= -1.f; // Flip Y axis
        // set front face culling from CCW to CW
        RIO_GL_CALL(glFrontFace(GL_CW));
    #endif

    mpModel->setLightEnable(renderRequest->lightEnable);

    // Create the render buffer with the desired size
    rio::RenderBuffer renderBuffer;
    //renderBuffer.setSize(renderRequest->resolution * 2, renderRequest->resolution * 2);    hasSocketRequest = false;

    int ssaaFactor =
#ifdef TRY_SCALING
    2;  // Super Sampling factor, e.g., 2 for 2x SSAA
#else
    1;
#endif
    int width = static_cast<int>(renderRequest->resolution) * ssaaFactor;
    int height = static_cast<int>(renderRequest->resolution) * ssaaFactor;

    renderBuffer.setSize(width, height);
    RIO_LOG("Render buffer created with size: %dx%d\n", renderBuffer.getSize().x, renderBuffer.getSize().y);

    //rio::Window::instance()->getNativeWindow()->getColorBufferTextureFormat();
    rio::Texture2D renderTextureColor(rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM, renderBuffer.getSize().x, renderBuffer.getSize().y, 1);
    rio::RenderTargetColor renderTargetColor;
    renderTargetColor.linkTexture2D(renderTextureColor);
    renderBuffer.setRenderTargetColor(&renderTargetColor);

    rio::Texture2D renderTextureDepth(rio::DEPTH_TEXTURE_FORMAT_R32_FLOAT, renderBuffer.getSize().x, renderBuffer.getSize().y, 1);
    rio::RenderTargetDepth renderTargetDepth;
    renderTargetDepth.linkTexture2D(renderTextureDepth);

    renderBuffer.setRenderTargetDepth(&renderTargetDepth);

/*
// Create multisampled framebuffer for head rendering
GLuint msaaFBO, msaaColorRBO, msaaDepthRBO;
GLsizei samples = 4; // You can adjust this value

RIO_GL_CALL(glGenFramebuffers(1, &msaaFBO));
RIO_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO));

RIO_GL_CALL(glGenRenderbuffers(1, &msaaColorRBO));,
RIO_GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, msaaColorRBO));
RIO_GL_CALL(glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, renderBuffer.getSize().x, renderBuffer.getSize().y));
RIO_GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRBO));

RIO_GL_CALL(glGenRenderbuffers(1, &msaaDepthRBO));
RIO_GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthRBO));
RIO_GL_CALL(glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT32F, renderBuffer.getSize().x, renderBuffer.getSize().y));
RIO_GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaaDepthRBO));

*/
    //const rio::Color4f clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    // NOTE: this calls glViewport
    // make background color float from RGBA bytes
    rio::Color4f fBackgroundColor = {
        static_cast<f32>(renderRequest->backgroundColor[0]) / 256,
        static_cast<f32>(renderRequest->backgroundColor[1]) / 256,
        static_cast<f32>(renderRequest->backgroundColor[2]) / 256,
        static_cast<f32>(renderRequest->backgroundColor[3]) / 256
    };
    renderBuffer.clear(rio::RenderBuffer::CLEAR_FLAG_COLOR_DEPTH_STENCIL, fBackgroundColor);

    // Bind the render buffer
    renderBuffer.bind();
/*        RIO_GL_CALL(glViewport(0, 0, renderRequest->resolution, renderRequest->resolution));

    // Bind MSAA framebuffer
    //RIO_GL_CALL(glViewport(0, 0, renderRequest->resolution, renderRequest->resolution));
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);

    // Clear the buffer
    glClearColor(renderRequest->backgroundColor.r, renderRequest->backgroundColor.g, renderRequest->backgroundColor.b, renderRequest->backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
*/
    RIO_LOG("Render buffer bound.\n");

    //if (gl_FragCoord.z > 0.98593) discard;
    // Enable depth testing
    /*glEnable(GL_DEPTH_TEST);
    // Set the depth function to discard fragments with depth values greater than the threshold
    glDepthFunc(GL_GREATER);
    glDepthRange(0.98593f, 0.f);
*/
    DrawStageMode drawStages = static_cast<DrawStageMode>(renderRequest->drawStageMode);

    // Render the first frame to the buffer
    if (drawStages == DRAW_STAGE_MODE_ALL || drawStages == DRAW_STAGE_MODE_OPA_ONLY)
        mpModel->drawOpa(view_mtx, *projMtx);
    RIO_LOG("drawOpa rendered to the buffer.\n");

    // draw body?
    if (renderRequest->viewType != VIEW_TYPE_FACE_ONLY
        && renderRequest->viewType != VIEW_TYPE_FACE_ONLY_FFLMAKEICON
    )
    {
        FFLiCharModel* pCharModel = reinterpret_cast<FFLiCharModel*>(mpModel->getCharModel());

        s32 originalFavoriteColor = pCharModel->charInfo.favoriteColor;
        if (renderRequest->clothesColor >= 0
            // verify favorite color is in range here bc it is NOT verified in drawMiiBodyREAL
            && renderRequest->clothesColor < FFL_FAVORITE_COLOR_MAX
        ) {
            // change favorite color after drawing opa
            pCharModel->charInfo.favoriteColor = renderRequest->clothesColor;
        }

        //drawMiiBodyFAKE(renderTextureColor, renderTextureDepth, favoriteColorIndex);
        drawMiiBodyREAL(mpModel->getLightEnable(), &pCharModel->charInfo, model_mtx, view_mtx, *projMtx);
        // restore original favorite color tho
        pCharModel->charInfo.favoriteColor = originalFavoriteColor;
    }

    renderBuffer.bind();

    // draw xlu mask only after body is drawn
    // in case there are elements of the mask that go in the body region
    if (drawStages == DRAW_STAGE_MODE_ALL || drawStages == DRAW_STAGE_MODE_XLU_ONLY)
        mpModel->drawXlu(view_mtx, *projMtx);
    RIO_LOG("drawXlu rendered to the buffer.\n");


    //copyAndSendRenderBufferToSocket(renderBuffer, &renderTextureColor, new_socket);

#ifdef ENABLE_BENCHMARK
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = std::chrono::high_resolution_clock::now();
#endif
    copyAndSendRenderBufferToSocket(renderBuffer, &renderTextureColor, new_socket, ssaaFactor);
#ifdef ENABLE_BENCHMARK
    end = std::chrono::high_resolution_clock::now();
    long long int duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    RIO_LOG("copyAndSendRenderBufferToSocket: %lld µs\n", duration);
#endif

    // Unbind the render buffer
    renderBuffer.getRenderTargetColor()->invalidateGPUCache();
    renderBuffer.getRenderTargetDepth()->invalidateGPUCache();
    renderTextureColor.setCompMap(0x00010205);

    //RIO_LOG("Render buffer unbound and GPU cache invalidated.\n");

    if (!mpServerOnly) {
        #ifdef RIO_NO_CLIP_CONTROL
            // set back front face culling
            RIO_GL_CALL(glFrontFace(GL_CCW));
            if (projMtx->m[1][1] < 0) // if it is negative
                projMtx->m[1][1] *= -1.f; // Flip Y axis back
        #endif
        rio::Window::instance()->makeContextCurrent();

        u32 width = rio::Window::instance()->getWidth();
        u32 height = rio::Window::instance()->getHeight();

        rio::Graphics::setViewport(0, 0, width, height);
        rio::Graphics::setScissor(0, 0, width, height);
        RIO_LOG("Viewport and scissor reset to window dimensions: %dx%d\n", width, height);
    }

    closesocket(new_socket);
}

void RootTask::calc_()
{
    if (!mInitialized)
        return;

    #if RIO_IS_WIN
    char buf[RENDERREQUEST_SIZE];

    bool hasSocketRequest = false;

    if (mSocketIsListening &&
        (new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) > 0) {
        int read_bytes =
            recv(new_socket, buf, RENDERREQUEST_SIZE, 0);

        if (read_bytes == RENDERREQUEST_SIZE) {
            delete mpModel;
            hasSocketRequest = true;

            RenderRequest* reqBuf = reinterpret_cast<RenderRequest*>(buf);

            if (!createModel_(reqBuf, new_socket)) {
                mpModel = nullptr;
                mCounter = 0.0f;
            };
        } else {
            RIO_LOG("got a request of length %i (should be %d), dropping\n", read_bytes, static_cast<u32>(RENDERREQUEST_SIZE));
            closesocket(new_socket);
        }
    } else {
        // otherwise just fall through and use default
        // when mii is directly in front of the camera
    #endif // RIO_IS_WIN
        if (!mpServerOnly && mCounter >= rio::Mathf::pi2())
        {
            delete mpModel;
            createModel_();
        }
    #if RIO_IS_WIN
    }
    #endif


    if (hasSocketRequest)
        return handleRenderRequest(buf);

    // Distance in the XZ-plane from the center to the camera position
    f32 radius = 600.0f;
    // Define a constant position in the 3D space for the center position of the camera
    static const rio::Vector3f CENTER_POS = { 0.0f, 34.5f, radius };
    static const rio::Vector3f target = { 0.0f, 34.5f, 0.0f };
    mCamera.at() = target;
    static const rio::Vector3f cameraUp = { 0.0f, 1.0f, 0.0f };
    mCamera.setUp(cameraUp);
    // Move the camera around the target clockwise
    // Define the radius of the orbit in the XZ-plane (distance from the target)
    rio::Matrix34f model_mtx = rio::Matrix34f::ident;
    if (!mpNoSpin && !mpServerOnly && !hasSocketRequest && mpModel != nullptr) {
        radius += 380;
        /*mCamera.pos().set(
            // Set the camera's position using the sin and cos functions to move it in a circle around the target
            std::sin(mCounter) * radius,
            CENTER_POS.y * std::sin(mCounter) * 7.5 - 30,
            // Add a minus sign to the cosine to spin CCW (same as SpinMii)
            std::cos(mCounter) * radius
        );
        */mCamera.pos() = { CENTER_POS.x, CENTER_POS.y, radius };

        model_mtx.makeR({
            0.0f,
            mCounter,
            0.0f
        });

        mpModel->setMtxRT(model_mtx);

        mCamera.at() = { target.x, target.y - 30, target.z };
    } else {
        mCamera.pos() = CENTER_POS;
    }
    // Increment the counter to gradually change the camera's position over time
    if (!mpServerOnly) {
        mCounter += 1.f / 60;
    }

    // Get the view matrix from the camera, which represents the camera's orientation and position in the world
    rio::BaseMtx34f view_mtx;
    mCamera.getMatrix(&view_mtx);

    if (!mpServerOnly) {
        rio::Window::instance()->clearColor(0.2f, 0.3f, 0.3f, 1.0f);
        rio::Window::instance()->clearDepthStencil();
        //rio::Window::instance()->setSwapInterval(0);  // disable v-sync
    }

    if (mpModel != nullptr) {
        mpModel->drawOpa(view_mtx, mProjMtx);
        drawMiiBodyREAL(mpModel->getLightEnable(), &reinterpret_cast<FFLiCharModel*>(mpModel->getCharModel())->charInfo, model_mtx, view_mtx, mProjMtx);
        mpModel->drawXlu(view_mtx, mProjMtx);
    }
}

#ifndef NO_GLTF

#include "GLTFExportCallback.h"
#include <sstream>

void RootTask::handleGLTFRequest(RenderRequest* renderRequest)
{
    // Initialize ExportShader
    GLTFExportCallback exportShader;

    exportShader.SetCharModel(mpModel->getCharModel());

    RIO_LOG("Created glTF export callback.\n");

    // Get the shader callback
    FFLShaderCallback callback = exportShader.GetShaderCallback();

    DrawStageMode drawStages = static_cast<DrawStageMode>(renderRequest->drawStageMode);

    if (renderRequest->drawStageMode == DRAW_STAGE_MODE_MASK_ONLY) {
        // only draw the xlu mask in this mode
        const FFLDrawParam* maskParam = FFLGetDrawParamXluMask(mpModel->getCharModel());
        exportShader.Draw(*maskParam);
    } else {
        if (drawStages == DRAW_STAGE_MODE_ALL || drawStages == DRAW_STAGE_MODE_OPA_ONLY)
            FFLDrawOpaWithCallback(mpModel->getCharModel(), &callback);

        if (drawStages == DRAW_STAGE_MODE_ALL || drawStages == DRAW_STAGE_MODE_XLU_ONLY)
            FFLDrawXluWithCallback(mpModel->getCharModel(), &callback);
    }
    RIO_LOG("Drawn model to glTF callback data.\n");
    /*
    const std::time_t now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "/dev/shm/%Y-%m-%d_%H-%M-%S.glb");
    std::string output = oss.str();
    RIO_LOG("outputting glb to: %s\n", output.c_str());
    exportShader.ExportModelToFile(output);
    */

    // Create a stream buffer to hold the GLTF model data
    std::ostringstream modelStream;

    // Export the GLTF model to the stream
    if (!exportShader.ExportModelToStream(&modelStream))
    {
        RIO_LOG("Failed to export GLTF model to stream.\n");
        return;
    }

    // Get the model data from the stream
    std::string modelData = modelStream.str();


    // Send the actual GLTF model data
    unsigned long totalSent = 0;
    const unsigned long fileSize = modelData.size();
    while (totalSent < fileSize)
    {
        long sent = send(new_socket, modelData.data() + totalSent, fileSize - totalSent, 0);
        if (sent < 0)
        {
            RIO_LOG("Failed to send GLTF data\n");
            return;
        }
        totalSent += sent;
    }
    RIO_LOG("Wrote %lu bytes out to socket.\n", fileSize);
}

#endif // NO_GLTF

void RootTask::exit_()
{
    if (!mInitialized)
        return;

    // we need to free stuff that's allocated in our program
    // ... which is usually all pointers

    delete mpModel; // FFLCharModel destruction must happen before FFLExit
    mpModel = nullptr;

    FFLExit();

    rio::MemUtil::free(mResourceDesc.pData[FFL_RESOURCE_TYPE_HIGH]);
    if (mResourceDesc.size[FFL_RESOURCE_TYPE_MIDDLE] != 0) {
      rio::MemUtil::free(mResourceDesc.pData[FFL_RESOURCE_TYPE_MIDDLE]);
    }

    delete mProjMtxIconBody;
    // delete all shaders that were initialized
    for (int type = 0; type < SHADER_TYPE_MAX; type++) {
        delete mpShaders[type];
    }
    // delete body models that were initialized earlier
    for (u32 i = 0; i < FFL_GENDER_MAX; i++) {
        delete mpBodyModels[i];
    }

    mInitialized = false;
}
