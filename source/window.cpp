#include "vk.h"
#include "camera.h"

#include <GLFW/glfw3.h>

static Camera camera{};
static float last_mouse_x{};
static float last_mouse_y{};

static void poll_keyboard(GLFWwindow * window, float delta_time)
{
	const float sensitivity = 48.0f;

	if (glfwGetKey(window, GLFW_KEY_W))
	{
		camera.move_forward(sensitivity * delta_time);
	}
	else if (glfwGetKey(window, GLFW_KEY_S))
	{
		camera.move_backward(sensitivity * delta_time);
	}
	if (glfwGetKey(window, GLFW_KEY_A))
	{
		camera.move_left(sensitivity * delta_time);
	}
	else if (glfwGetKey(window, GLFW_KEY_D))
	{
		camera.move_right(sensitivity * delta_time);
	}
	if (glfwGetKey(window, GLFW_KEY_R))
	{
		camera.move_up(sensitivity * delta_time);
	}
	else if (glfwGetKey(window, GLFW_KEY_F))
	{
		camera.move_down(sensitivity * delta_time);
	}
}

static void mouse_callback(GLFWwindow * window, double pos_x, double pos_y)
{
	float offset_x = pos_x - last_mouse_x;
	float offset_y = last_mouse_y - pos_y;

	last_mouse_x = pos_x;
	last_mouse_y = pos_y;

	const float sensitivity = 0.16f;

	offset_x *= sensitivity;
	offset_y *= sensitivity;

	camera.aux.yaw += offset_x;
	camera.aux.pitch += offset_y;

	camera.update();
}

bool vk_window_create()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	state.window = glfwCreateWindow(1024, 768, "Epic Renderer Time", nullptr, nullptr);

	glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(state.window, mouse_callback);

	return true;
}

bool vk_window_destroy()
{
	glfwDestroyWindow(state.window);
	glfwTerminate();

	return true;
}

bool vk_window_update()
{
	poll_keyboard(state.window, 0.00016);
	camera.update();
	state.view = camera.view_matrix();

	return true;
}
