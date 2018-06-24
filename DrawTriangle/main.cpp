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
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// errorやlog用
#include <iostream>
#include <stdexcept>
// lambda function用　主にresorce management
#include <functional>
#include <vector>
// EXIT_SUCCESSとEXIT_FAILUREを提供する
#include <cstdlib>


const int WIDTH = 800;
const int HEIGHT = 600;

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
    
    void initWindow() {
        glfwInit();
        glfwGetTime();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(800, 600, "Vulkan Test!!", nullptr, nullptr);
    }
    void initVulkan() {
        createInstance();
    }
    
    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }
    
    void cleanup() {
        vkDestroyInstance(instance, nullptr);
        
        glfwDestroyWindow(window);
        
        glfwTerminate();
    }
    
    void createInstance() {
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
        
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        
        // glfwからデバイスごとに必要なextensionの情報をえて、createInfoに追加する
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        if (glfwExtensionCount) {
            std::cout << "requirement extensions:" << std::endl;
            std::cout << "\t" << **glfwExtensions << std::endl;
        }
        
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        
        createInfo.enabledLayerCount = 0;
        
        uint32_t extensionCount = 0;
        // 3つ目の変数をからにすると要素数を返す
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        // 先ほどの要素数でvectorを初期化して、サポートされている拡張機能とその詳細のリストを取得する
        
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        
        std::cout << "avaliable extensions:" << std::endl;
        
        for (const auto& extension : extensions) {
            std::cout << "\t" << extension.extensionName << std::endl;
        }
        

        // ここでようやくインスタンスを作る
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
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
