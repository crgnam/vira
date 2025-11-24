#ifndef VIRA_RENDERING_VULKAN_WINDOW_HPP
#define VIRA_RENDERING_VULKAN_WINDOW_HPP

#include <string>

#include "vulkan/vulkan.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace vira::vulkan {
    /**
     * @class ViraWindow
     * @brief Window for on screen rendering. Mostly deprecated for offscreen rendering.
	 * 
 	 * Member Variables:
 	 * - int width: Width of the window in pixels.
	 * - int height: Height of the window in pixels.
	 * - bool framebufferResized: Flag indicating whether the framebuffer size has changed and the swapchain needs to be recreated.
	 * - bool middleButtonDown: Tracks whether the middle mouse button is currently pressed.
	 * - std::string windowName: Human-readable name/title of the window.
	 * - glm::vec2 scrollChange: Amount of scroll wheel movement since the last frame.
	 * - glm::vec2 prevScrollChange: Scroll change value from the previous frame (for comparison or smoothing).
	 * - GLFWwindow* window: Raw pointer to the underlying GLFW window handle.
     */	
	class ViraWindow {
	public:

		// Constructors / Destructor
		ViraWindow(int w, int h);
		ViraWindow(int w, int h, std::string name);
		~ViraWindow();

		// Delete Copy Methods
		ViraWindow(const ViraWindow*) = delete;
		ViraWindow* operator=(const ViraWindow*) = delete;

    	// Get/Set Flag methods
		bool shouldClose();		
		virtual vk::Extent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		GLFWwindow* getGLFWwindow() const { return window; };
		bool getMiddleButtonDown();
		glm::vec2 getScrollChange();
		bool wasWindowResized();
		void resetWindowResizedFlag();

		/**
		 * @brief Creates a window surface for on-screen rendering
		 */
		void createWindowSurface(vk::Instance &instance, vk::SurfaceKHR &surface);

		// Public Member Variables
		bool glfw_initialized = false;
		glm::vec2 cursorPreviousPosition{ 0,0 };

	protected:

		/**
		 * @brief Initializes the GLFW window for on-screen rendering
		 */
		void initWindow();

		// Callbacks
		static void glfwErrorCallback(int error, const char* description);
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
		static void mouseMiddleCallback(GLFWwindow* window, int button, int action, int mods);
		static void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

		// Member Variables
		int width;
		int height;
		bool framebufferResized = false;
		bool middleButtonDown = false;
		std::string windowName;
		glm::vec2 scrollChange{ 0.,0. };
		glm::vec2 prevScrollChange{ 0., 0. };
		GLFWwindow* window;
	};
}

#include "implementation/rendering/vulkan_private/window.ipp"

#endif