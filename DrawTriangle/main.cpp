//
//  main.cpp
//  DrawTriangle
//
//  Created by nami on 2018/06/22.
//  Copyright © 2018年 Kousaku Namikawa. All rights reserved.
//

#define GLFW_INCLUDE_VULKAN

using namespace std;

// 3DCG ライブラリ
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// errorやlog用
#include <iostream>
#include <stdexcept>
// lambda function用　主にresorce management
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
// EXIT_SUCCESSとEXIT_FAILUREを提供する
#include <cstdlib>


const int WIDTH = 800;
const int HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const bool enableValidationLayers = true;

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;
    
    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

// スワップチェーンについて調べる必要があること
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities; // 何枚貯めて置けるかと画像のサイズ、それぞれのmin/max
    vector<VkSurfaceFormatKHR> formats; // ピクセルフォーマット、色関連
    vector<VkPresentModeKHR> presentModes; // どんな風に変えるか
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0; // bindingの中のインデックス
        bindingDescription.stride = sizeof(Vertex); // 何バイトが1頂点/インスタンス分か
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 頂点ごとかインスタンスごとに読み込むデータを次にするかを決める
        
        return bindingDescription;
    }
    
    static array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
        
        attributeDescriptions[0].binding = 0; // bindingのインデックス
        attributeDescriptions[0].location = 0; // shaderのロケーション
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // format
        attributeDescriptions[0].offset = offsetof(Vertex, pos); // o
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        
        return attributeDescriptions;
    }
};

const vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.5f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.5f, 0.5f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
};

void error_callback(int error, const char* description) {
    puts(description);
}

static vector<char> readFile(const string& filename) {
    ifstream file(filename, ios::ate | ios::binary);
    
    if (!file.is_open()) {
        throw runtime_error("failed to open file!");
    }
    
    size_t fileSize = (size_t) file.tellg();
    vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    
    file.close();
    
    return buffer;
}

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
    
private:
    GLFWwindow* window;
    
    VkInstance instance;
    VkDebugReportCallbackEXT callback;
    VkSurfaceKHR surface;
    
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    VkSwapchainKHR swapChain;
    vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    
    vector<VkImageView> swapChainImageViews;
    
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    
    VkPipeline graphicsPipeline;
    
    vector<VkFramebuffer> swapChainFramebuffers;
    
    VkCommandPool commandPool;
    vector<VkCommandBuffer> commandBuffers;
    
    vector<VkSemaphore> imageAvailableSemaphores;
    vector<VkSemaphore> renderFinishedSemaphores;
    vector<VkFence> inFlightFences;
    size_t currentFrame = 0;
    
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    
    void initWindow() {
        if (!glfwVulkanSupported()) {
            cout << "vulkan loader not found!" << endl;
        }
        
        glfwInit();
        glfwGetTime();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        window = glfwCreateWindow(800, 600, "Vulkan Test!!", nullptr, nullptr);
    }
    void initVulkan() {
        glfwSetErrorCallback(error_callback);
        createInstance();
        setupDebugCallback();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createCommandBuffers();
        createSyncObjects();
    }
    
    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        
        // ウィンドウを閉じたときに、非同期処理なので、同期して終了するために必要
        vkDeviceWaitIdle(device);
    }
    
    void cleanup() {
        cleanupSwapChain();
        
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        
        vkDestroyCommandPool(device, commandPool, nullptr);
        
        vkDestroyDevice(device, nullptr);
        
        if (enableValidationLayers) {
            DestroyDebugReportCallbackEXT(instance, callback, nullptr);
        }
        
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        
        glfwDestroyWindow(window);
        
        glfwTerminate();
    }
    
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw runtime_error("validation layers requested, but not avaliable!");
        }
        
        // application情報
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        
        // global extentions, validationlayerに関する情報
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        
        // 検証レイヤーを通しつつ、extension関連の初期化
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        // ここでようやくインスタンスを作る
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw runtime_error("failed to create instance!");
        }
    }
    
    void setupDebugCallback() {
        if (!enableValidationLayers) return;
        
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = debugCallback;
        
        if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
            throw runtime_error("failed to set up debug callback!");
        }
    }
    
    void pickPhysicalDevice() {
        // 使用する物理デバイスを選択する
        
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        
        if (deviceCount == 0) {
            throw runtime_error("failed to find GPUs with Vulkan support!");
        }
        
        vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        
        
        // 標準的なデバイスの選定
//        for (const auto& device : devices) {
//            if (isDeviceSuitable(device)) {
//                physicalDevice = device;
//                break;
//            }
//        }
        
        // 例えば一番高い性能のGPUを使いたい場合
        multimap<int, VkPhysicalDevice> candidates;
        
        for (const auto& device : devices) {
            int score = rateDeviceSuitability(device);
            candidates.insert(make_pair(score, device));
        }
        
        // 一番高いscoreを持つGPUセットする
        if (candidates.rbegin()->first > 0) {
            physicalDevice = candidates.rbegin()->second;
        } else {
            throw runtime_error("failed to find a suitable GPU!");
        }
        
        
        if (physicalDevice == VK_NULL_HANDLE) {
            throw runtime_error("failed to find a suitable GPU!");
        }
        // 例えば一番高い性能のGPUを使いたい場合 終わり
    }
    
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        
        vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};
        
        float queuePriority = 1.0f;
        for (int queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        VkPhysicalDeviceFeatures deviceFeatures = {};
        
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        
        createInfo.pEnabledFeatures = &deviceFeatures;
        
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }
        
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw runtime_error("failed to create logical device!");
        }
        
        vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
    }
    
    void createSurface() {
        // glfwの簡単にウィンドウを作ってくれるやつ
        cout << glfwCreateWindowSurface(instance, window, nullptr, &surface) << endl;
        
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw runtime_error("failed to create window surface!");
        }
    }
    
    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
        
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
        
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        
        // スワップチェーン作るための細かい設定たち
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        // どのように複数のキューでスワップチェーンを使用するかの設定
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphicsFamily, (uint32_t) indices.presentFamily};
        
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        
        // ウィンドウを透過して他のウィンドウをアルファブレンディングで表示するかどうか
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        
        createInfo.presentMode = presentMode;
        // 他のウィンドウが前にきたときに、そのウィンドウのピクセルの色を知りたいときなどにVK_FALSEにして使う(TRUEの場合は前面のウィンドウを無視する)
        createInfo.clipped = VK_TRUE;
        // スクリーンのリサイズ時などにスワップチェーンで作られるべきバッファが作成されなかったときに、古いものへの参照を指定することができる（後のチャプターで解説）
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }
    
    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            
            // スウィズル演算子の設定
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            
            // ミップマップや複数レイヤーの設定
            // もしVRゴーグルなどに出すときは、複数のレイヤーをもつスワップチェーンを作成して、それぞれのレイヤーに左目のビューと右目のビューを表す複数のImageViewを作成する
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            
            // ImageView作る
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw runtime_error("failed to create image views!");
            }
        }
        
    }
    
    void createRenderPass() {
        // レンダリング時に使用されるフレームバッファの数、そしてそれらのフォーマットやデプスバッファの有無、さらにそれらがどのようにレンダリング中に操作されるかを作成する
        // attachmentは一つ一つのフレームバッファに関する設定
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        
        // レンダリング前と後でデータをどのように処理するか(colorとdepth)
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
        // 上のステンシル版
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // レンダーパスが始まる前
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // レンダーパスが終了したときに自動的に遷移するレイアウト
        
        // subpass : レンダーパスの中で、ポストプロセッシングなどの前のフレームの情報が残されたままだと都合がいい場合に、それらの処理を一つにまとめることができる
        // subpassは後でパフォーマンスがいいように順番を並べ替えることができる
        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0; // subpassのインデックス（順番）
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        // Vulkanは将来的に計算用のsubpassも追加される予定なので、このサブパスがグラフィック用であることを明示する必要がある
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef; // shaderからlayout(location = 0) out vec4 outColorのように, シェーダー側のlocationとsubpassのAttachmentのインデックスが一致している
        
        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &colorAttachment;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        
        // Subpass同士の依存関係を示す（例：　このSubpassの次にこのSubpassを実行したいなど？）, 常にsrc<dstでないといけない
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // この定数は指定したSubpassたちの前に実行される暗黙のサブパス(内部システムのもの？)
        dependency.dstSubpass = 0;
        
        // その操作が発生する段階
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // スワップチェーンがイメージを読み込むのを待つ
                                                                                 // 待機する操作
        dependency.srcAccessMask = 0; // この場合は特に操作を指定していなくても、カラーアタッチメントのステージを待てば大丈夫
        
        // このSubpass内のどの操作を次の操作が待てばいいかを指定する
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //この場合、カラーアタッチメントのステージで
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // 読み書きが終われば次の処理は走っても大丈夫
        
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &dependency;
        
        if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw runtime_error("failed to create render pass!");
        }
        
    }
    
    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");
        
        cout << "vertShaderFile size : " << vertShaderCode.size() << "bytes" << endl;
        cout << "fragShaderFile size : " << fragShaderCode.size() << "bytes" << endl;
        
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        
        // shaderModuleの作成（この時点ではただのバッファのラッパで、お互いのリンクや何のシェーダかは決められていない）
        vertShaderModule = createShaderModule(vertShaderCode);
        fragShaderModule = createShaderModule(fragShaderCode);
        
        // shader stageの作成
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main"; // 最初に呼び出す関数の名前を指定
    //        vertShaderStageInfo.pSpecializationInfo = // シェーダに定数を渡すことができる、参考:https://blogs.igalia.com/itoral/2018/03/20/improving-shader-performance-with-vulkans-specialization-constants/
        
        // レンダリングパイプラインに関する設定
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        // (プログラマブル部分の）レンダリングパイプラインの作成
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
        
        // 頂点情報を取得
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        
        // 頂点シェーダーに渡す情報の設定
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1; // 頂点シェーダへの頂点データの数
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // 頂点データのポインタ
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); // attributeの数
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // attributeを表すVkVertexInputAttributeDescriptionのポインタ
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // openglでいうGL_PRIMITIVE_MODE
        inputAssembly.primitiveRestartEnable = VK_FALSE; // triangleとLineに分割できる???
        
        // viewport設定
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        // フレームバッファのトリミング範囲の設定
        VkRect2D scissor = {};
        scissor.offset = {0,0};
        scissor.extent = swapChainExtent;
        
        // viewportとトリミング範囲の設定（それぞれ複数ずつ使用することも可能だが、その場合論理デバイスで設定する必要がある）
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;
        
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable = VK_FALSE; // シャドウマップなどを作る際にクリッピング空間外のオブジェクトを破棄するのではなく、クランプしたい場合にVK_TRUEで使うことができる。（論理デバイスでGPU機能を有効化する必要がある）
        rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // ラスタライザをスキップするかどうか、基本的にスキップするとフレームバッファに書き込まれない
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // ポリゴンの塗りつぶし設定(FILLとLINEとPOINTがある）
        rasterizationStateCreateInfo.lineWidth = 1.0f; // 1.0より太い太さを設定する場合、論理デバイスでGPU機能を有効化する必要がある
        
        // カリング周り
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        
        // 深度値のバイアスを設定する。シャドウマップなどで、奥に行く時の補間方法などを細かく指定したい場合に使える
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
//        rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
//        rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
//        rasterizationStateCreateInfo.depthViasSlopeFactor = 0.0f;
        
        // マルチサンプリング（単純に大きい解像度でレンダリングしてから縮小をかけるより大幅にコストを下げることができる）、論理デバイスで要有効化
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//        multisampleStateCreateInfo.minSampleShading = 1.0f;
//        multisampleStateCreateInfo.pSampleMask = nullptr;
//        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
//        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
        
        // depth test & stencil test
//        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
//        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        
        // 各フレームバッファにブレンディングの設定を付加
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        
        // 例えばこんな感じ
//        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
//        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
//        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
//        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        
        // アルファブレンディング
//        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
//        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
        colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
        
        // 一部のパイプラインの設定は、動的に変更することができる、その場合Dynamic stateとしてあらかじめ登録しておく必要がある
//        VkDynamicState dynamicStates[] = {
//            VK_DYNAMIC_STATE_VIEWPORT,
//            VK_DYNAMIC_STATE_LINE_WIDTH
//        }
//        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
//        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//        dynamicStateCreateInfo.dynamicStateCount = 2;
//        dynamicStateCreateInfo.pDynamicStates = dynamicStates;
        
        // uniform変数
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
//        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
//        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
        
        if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw runtime_error("failed to create pipeline layout!");
        }
        
        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStages;
        
        // 今まで作ってきたやつをVkGraphicsPipelineCreateInfoにぶち込む
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
//        pipelineCreateInfo.pDepthStencilState = nullptr;
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
//        pipelineCreateInfo.pDynamicState = nullptr;
        
        pipelineCreateInfo.layout = pipelineLayout;
        
        pipelineCreateInfo.renderPass = renderPass; // このパイプラインを他のレンダーパスで使用することもできる
        pipelineCreateInfo.subpass = 0; // 使用するsubpassのインデックス
        
        // Vulkanでは既存のパイプラインから新しいパイプラインを作ることができ、そのときに別のパイプラインとの関連性を指定できる
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
//        pipelineCreateInfo.basePipelineIndex = -1;
        
        // 本当は一回のコールで複数のパイプラインを同時に作れるように設計されている
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw runtime_error("failed to create graphics pipeline!");
        }
        
        
        
        
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
    
    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };
            
            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.pAttachments = attachments;
            framebufferCreateInfo.width = swapChainExtent.width;
            framebufferCreateInfo.height = swapChainExtent.height;
            framebufferCreateInfo.layers = 1;
            
            if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw runtime_error("failed to create framebuffer!");
            }
        }
    }
    
    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
        poolCreateInfo.flags = 0;
        
        if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw runtime_error("failed to create command pool!");
        }
    }
    
    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());
        
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // コマンドの強さ（Primaryなら直接実行キューとして渡せるが、他のキューから参照できなく、Secondaryなら直接実行キューには渡せないが、他のキューから実行できる）
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
        
        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw runtime_error("failed to allocate command buffers!");
        }
        
        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // コマンドバッファの使用方法
            beginInfo.pInheritanceInfo = nullptr;
            
            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw runtime_error("failed to begin recording command buffer!");
            }
            
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            
            renderPassInfo.renderArea.offset = {0,0};
            renderPassInfo.renderArea.extent = swapChainExtent;
            
            VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;
            
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            
            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            // 二番目はオフセット、三番目はバインディングの数
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            
            vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0); //一番目から, commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance
            
            vkCmdEndRenderPass(commandBuffers[i]);
            
            if (vkEndCommandBuffer(commandBuffers[i]) !=  VK_SUCCESS) {
                throw runtime_error("failed to record command buffer!");
            }
        }
        
        
    }
    
    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, numeric_limits<uint64_t>::max());
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        
        uint32_t imageIndex;
        
        // 一番目から device, swapchain, イメージが使用可能になるまでのタイムアウト（ナノ秒、最大値なら無効）, semaphore | or & fence, アクセスすべきスワップチェーンのイメージのインデックス
        VkResult result = vkAcquireNextImageKHR(device, swapChain, numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw runtime_error("failed to acquire swap chain image!");
        }
        
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]}; // 実行が開始される前にどのSemaphoresを待つか
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // 実行が開始される前にどのStageを待つか
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        // コマンドバッファの実行が終了したときに通知したいSemaphoreを指定する
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw runtime_error("failed to submit draw command buffer!");
        }
        
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        
        // 複数のスワップチェーンがあった場合に、プレゼンテーションが成功した場合にそれぞれのスワップチェーンを確認することができる
        presentInfo.pResults = nullptr;
        
        // イメージをスワップチェーンに格納する
        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        
        vkQueueWaitIdle(presentQueue);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw runtime_error("failed to present swap chain image!");
        }
        
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    
    void createSyncObjects() {
        // semaphoreでGPUとGPUの同期を行い、fenceでGPUとCPUの同期をとっている
        
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw runtime_error("failed to create semaphores!");
            }
        }

    }
    
    void cleanupSwapChain() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        
        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }
    
    // ウィンドウのリサイズ時にスワップチェーンを作り直す
    void recreateSwapChain() {
        vkDeviceWaitIdle(device);
        
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }
    
    void createVertexBuffer() {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; //bufferのusage ^で複数指定可能
        
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
            throw runtime_error("failed to create vertex buffer!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // CPU側から書き込めるメモリを指定
        
        if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
            throw runtime_error("failed to allocate vertex buffer memory!");
        }
        
        vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0); // 実際にメモリを有効化 四番目のパラメーターはメモリ領域内のオフセット
        
        void* data;
        // オフセットとサイズを指定して作ったメモリをCPU側からアクセス可能なメモリにマッピングして使用可能にする
        // サイズをVK_WHOLE_SIZEにすれば全てのメモリをマップすることもできる
        // 最後から二番目の変数はflagsを指定できる（現在のAPIではまだ使用不可）
        vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferInfo.size); // 頂点データのメモリをコピーしていれる
        vkUnmapMemory(device, vertexBufferMemory);
        
    }
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        // propertiesでメモリに対して要求する機能を指定
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        
        throw runtime_error("failed to find suitable memory type!");
    }
    
    VkShaderModule createShaderModule(const vector<char>& code) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw runtime_error("failed to create shader module!");
        }
        
        return shaderModule;
    }
    
    // 例えば使用できるデバイスの中で一番高いものを使用したい場合
    int rateDeviceSuitability(VkPhysicalDevice device) {
        // 基本的な名前・タイプ・サポートされているVulkanのバージョンなどを取得することができる
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        // VR向けのマルチビューポートや圧縮テクスチャ、64bit浮動小数点などのオプション機能
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        
        // 前略
        
        int score = 0;
        
        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }
        
        // Maximum possible GPUs have a significant performance advantage
        score += deviceProperties.limits.maxImageDimension2D;
        
        // Application can't function without geometry shaders
//        if (!deviceFeatures.geometryShader) {
//            return 0;
//        }
        
        QueueFamilyIndices indices = findQueueFamilies(device);
        
        if (indices.isComplete()) {
            runtime_error("device does not support graphic API!");
        }
        
        if (checkDeviceExtensionSupport(device)) {
            runtime_error("device extension does not supported");
        }
        
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        if (!swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty()) {
            runtime_error("Swap chain support is not sufficient");
        }
        
        cout << "device name : " << deviceProperties.deviceName << endl;
        cout << "\t" << "device type : " << deviceProperties.deviceType << endl;
        cout << "\t" << "device score : " << score << endl;
        
        return score;
        
    }
    
    
    // デバイスの拡張機能(スワップチェーンなど)をサポートしているかを確認
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        
        vector<VkExtensionProperties> avaliableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, avaliableExtensions.data());
        
        set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        
        for (const auto& extension : avaliableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        
        return requiredExtensions.empty();
    }
    
    // バッファの画像サイズの選択
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };
            
//            VkExtent2D actualExtent = {WIDTH, HEIGHT};
            
            actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));
            
            return actualExtent;
        }
    }
    
    // どのように画像差し替えるか
    VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR> avaliablePresentModes) {
        // 基本的にはVertidcal Sync（正確には違う？）で
        VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
        
        // できればトリプルバッファリングでやりたい
        for (const auto& avaliablePresentMode : avaliablePresentModes) {
            if (avaliablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return avaliablePresentMode;
            } else if (avaliablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                // 最悪これ使うしかない
                bestMode = avaliablePresentMode;
            }
        }
        return bestMode;
    }
    
    // バッファのフォーマット
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& avaliableFormats) {
        // もし、使えるフォーマットが使いたいフォーマットだけならそれ使う
        if (avaliableFormats.size() == 1 && avaliableFormats[0].format == VK_FORMAT_UNDEFINED) {
            return {VK_FORMAT_B8G8R8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        }
        
        // 使いたいフォーマットがあるか探す
        for (const auto& avaliableFormat : avaliableFormats) {
            if (avaliableFormat.format == VK_FORMAT_B8G8R8_UNORM && avaliableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return avaliableFormat;
            }
        }
        
        // いいのがなかったら一番目使っとけ
        return avaliableFormats[0];
    }
    
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        
        // キャパビリティ
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        
        // フォーマット・カラー
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device ,surface, &formatCount, nullptr);
        
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }
        
        // プレゼンテーションモード
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        
        return details;
    }
    
    
    bool isDeviceSuitable(VkPhysicalDevice device) {
        // この関数で、GPUなどが実際に使いたい機能を使えるかを判断する
        
        // 基本的な名前・タイプ・サポートされているVulkanのバージョンなどを取得することができる
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        // VR向けのマルチビューポートや圧縮テクスチャ、64bit浮動小数点などのオプション機能
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // 例えば、geometry shaderを使いたい場合は以下のようにする
//        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
        
//        return true;
        
        QueueFamilyIndices indices = findQueueFamilies(device);
        
        return indices.isComplete();
    }
    
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        
        vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            
            if (queueFamily.queueCount  > 0 && presentSupport) {
                indices.presentFamily = i;
            }
            
            if (indices.isComplete()) {
                break;
            }
            
            ++i;
        }
        
        return indices;
    }
    
    vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
        
        return extensions;
    }
    
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
        vector<VkLayerProperties> avaliableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());
        
        for (const char* layerName : validationLayers) {
            bool layerFound = false;
            cout << "layerName : " << endl;
            for (const auto& layerProperties : avaliableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            
            if(!layerFound) return false;
        }
        
        return true;
    }
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userDate) {
        cerr << "validation layer: " << msg << endl;
        
        return VK_FALSE;
    }
};

int main(int argc, const char * argv[]) {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
