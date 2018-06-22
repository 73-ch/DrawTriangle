//
//  main.cpp
//  DrawTriangle
//
//  Created by nami on 2018/06/22.
//  Copyright © 2018年 Kousaku Namikawa. All rights reserved.
//

// errorやlog用
#include <iostream>
#include <stdexcept>
// lambda function用　主にresorce management
#include <functional>
// EXIT_SUCCESSとEXIT_FAILUREを提供する
#include <cstdlib>
// 3DCG ライブラリ
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }
    
private:
    void initVulkan() {
        
    }
    
    void mainLoop() {
        
    }
    
    void cleanup() {
        
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
