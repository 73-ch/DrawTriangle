//
//  main.cpp
//  DrawTriangle
//
//  Created by nami on 2018/06/22.
//  Copyright © 2018年 Kousaku Namikawa. All rights reserved.
//

#define GLFW_INCLUDE_VULKAN

// 3DCG ライブラリ
#include <vulkan/vulkan.h>
//#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// errorやlog用
#include <iostream>
#include <stdexcept>
// lambda function用　主にresorce management
#include <functional>
#include <vector>
#include <map>
#include <set>
// EXIT_SUCCESSとEXIT_FAILUREを提供する
#include <cstdlib>


const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

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

void error_callback(int error, const char* description) {
    puts(description);
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
    
    void initWindow() {
        if (!glfwVulkanSupported()) {
            std::cout << "vulkan loader not found!" << std::endl;
        }
        
        glfwInit();
        glfwGetTime();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(800, 600, "Vulkan Test!!", nullptr, nullptr);
    }
    void initVulkan() {
        glfwSetErrorCallback(error_callback);
        createInstance();
        setupDebugCallback();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
    }
    
    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }
    
    void cleanup() {
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
            throw std::runtime_error("validation layers requested, but not avaliable!");
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
            throw std::runtime_error("failed to create instance!");
        }
    }
    
    void setupDebugCallback() {
        if (!enableValidationLayers) return;
        
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = debugCallback;
        
        if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug callback!");
        }
    }
    
    void pickPhysicalDevice() {
        // 使用する物理デバイスを選択する
        
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        
        
        // 標準的なデバイスの選定
//        for (const auto& device : devices) {
//            if (isDeviceSuitable(device)) {
//                physicalDevice = device;
//                break;
//            }
//        }
        
        // 例えば一番高い性能のGPUを使いたい場合
        std::multimap<int, VkPhysicalDevice> candidates;
        
        for (const auto& device : devices) {
            int score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }
        
        // 一番高いscoreを持つGPUセットする
        if (candidates.rbegin()->first > 0) {
            physicalDevice = candidates.rbegin()->second;
        } else {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        
        
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        // 例えば一番高い性能のGPUを使いたい場合 終わり
    }
    
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};
        
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
        
        createInfo.enabledExtensionCount = 0;
        
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }
        
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }
        
        vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
    }
    
    void createSurface() {
        // glfwの簡単にウィンドウを作ってくれるやつ
        std::cout << glfwCreateWindowSurface(instance, window, nullptr, &surface) << std::endl;
        
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
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
            std::runtime_error("device doesn not support graphic API!");
        }
        
        std::cout << "device name : " << deviceProperties.deviceName << std::endl;
        std::cout << "\t" << "device type : " << deviceProperties.deviceType << std::endl;
        std::cout << "\t" << "device score : " << score << std::endl;
        
        return score;
        
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
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
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
    
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
        
        return extensions;
    }
    
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
        std::vector<VkLayerProperties> avaliableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());
        
        for (const char* layerName : validationLayers) {
            bool layerFound = false;
            std::cout << "layerName : " << std::endl;
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
        std::cerr << "validation layer: " << msg << std::endl;
        
        return VK_FALSE;
    }
};

int main(int argc, const char * argv[]) {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
