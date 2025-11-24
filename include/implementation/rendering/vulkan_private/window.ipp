#include <stdexcept>

#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"

namespace vira::vulkan {

	void glfwErrorCallback(int error, const char* description) {
    	throw std::runtime_error(std::string("GLFW Error ") + std::to_string(error) + ": " + description);
	};


    // Constructors / Destructor
	ViraWindow::ViraWindow(int w, int h) : width{w}, height{h} , windowName{"TestWindow"} {};
   
	ViraWindow::ViraWindow(int w, int h, std::string name) : width{ w }, height{ h }, windowName{ name }
	{
		initWindow();
	};

	ViraWindow::~ViraWindow()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	};

	bool ViraWindow::shouldClose()
	{
		return glfwWindowShouldClose(window);
	};

	void ViraWindow::createWindowSurface(vk::Instance &instance, vk::SurfaceKHR &surface)
	{
        (void)instance; // TODO Consider removing

		//VkSurfaceKHR cSurface;
		// if (glfwCreateWindowSurface(instance, window, nullptr, &cSurface) != VK_SUCCESS) {
		// 	throw std::runtime_error("Failed to create window surface");
		// }
		// surface = vk::SurfaceKHR(cSurface);		
        surface = nullptr;
	};
 
    /**
     * @brief Initializes the GLFW window for on-screen rendering
     */
	void ViraWindow::initWindow()
	{
		// glfwSetErrorCallback(glfwErrorCallback);
	
		glfw_initialized = glfwInit();
		if (!glfw_initialized) {
			throw std::runtime_error("Failed to initialize GLFW");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		if (!window) {
        	throw std::runtime_error("Failed to create GLFW window");
    	}
		
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

		glfwSetMouseButtonCallback(window, mouseMiddleCallback);
		glfwSetScrollCallback(window, mouseScrollCallback);
	};

	bool ViraWindow::wasWindowResized()
	{
		return framebufferResized;
	};
	void ViraWindow::resetWindowResizedFlag()
	{
		framebufferResized = false;
	};

    bool ViraWindow::getMiddleButtonDown()
	{
		return middleButtonDown;
	};

	glm::vec2 ViraWindow::getScrollChange()
	{
		if (scrollChange == prevScrollChange) {
			scrollChange = { 0., 0. };
		}
		prevScrollChange = scrollChange;

		return scrollChange;
	};

	void ViraWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto viraWindow = reinterpret_cast<ViraWindow*>(glfwGetWindowUserPointer(window));
		viraWindow->framebufferResized = true;
		viraWindow->width = width;
		viraWindow->height = height;
	};

	void ViraWindow::mouseMiddleCallback(GLFWwindow* window, int button, int action, int mods)
	{
        (void)mods; // TODO Consider removing

		// Get the window:
		auto viraWindow = reinterpret_cast<ViraWindow*>(glfwGetWindowUserPointer(window));

		if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
			if (GLFW_PRESS == action) {
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				glm::vec2 cursorPreviousPosition((float)xpos, (float)ypos);
				viraWindow->cursorPreviousPosition = cursorPreviousPosition;
				viraWindow->middleButtonDown = true;
			}
			else if (GLFW_RELEASE == action) {
				viraWindow->middleButtonDown = false;
			}
		}
	};

	void ViraWindow::mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		auto viraWindow = reinterpret_cast<ViraWindow*>(glfwGetWindowUserPointer(window));
		viraWindow->scrollChange[0] = static_cast<float>(xoffset);
		viraWindow->scrollChange[1] = static_cast<float>(yoffset);
	};
}