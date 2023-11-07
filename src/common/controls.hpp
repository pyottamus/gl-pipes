#ifndef CONTROLS_HPP
#define CONTROLS_HPP
#include <bitset>
#include <GLFW/glfw3.h>
#include "keymap.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

constexpr double PI = glm::pi<double>();
namespace _KeyBinds {
	enum KeyBinds {
		forward = 0,
		backward = 1,
		right = 2,
		left = 3,
	};
}
using KeyBinds = _KeyBinds::KeyBinds;

namespace _keymap_type {
	enum keymap_type : uint8_t {
		TRIGGER = 0,
		TIMED = 1,
		TOGGLE = 2
	};

}
constexpr uint8_t keymap_type_max = 2;
constexpr uint8_t keymap_type_bits = 2;
constexpr uint8_t keymap_type_mask = (255 >> (8 - keymap_type_bits)) << (8 - keymap_type_bits);

using keymap_type = _keymap_type::keymap_type;

struct keymap_info {
	uint8_t data;
	
	keymap_type getType() {
		return (keymap_type)((data) >> (8 - keymap_type_bits));
	}
	uint8_t getEventId() {
		return (data & (~keymap_type_mask)) - 1;
	}
	keymap_info() : data(0) {}
	keymap_info(keymap_type type, uint8_t id) : data((type << (8 - keymap_type_bits)) | (id + 1)) {
		int xl;
	}


};

template<typename relthis>
struct Camera {

	bool triggers[2]{ false };
	bool toggles[1]{ false };
	int window_width, window_height;
	double cursor_x, cursor_y;
	double next_cursor_x, next_cursor_y;

	std::bitset<4> key_states{ 0 };
	std::bitset<4> key_pressed{ 0 };
	std::bitset<4> key_released{ 0 };

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	// Initial position : on +Z
	glm::vec3 position = glm::vec3(-20, 10, -20);
	// Initial horizontal angle : toward -Z
	float horizontalAngle = PI/4.0f;
	// Initial vertical angle : none
	float verticalAngle = 0;
	// Initial Field of View
	float FoV = 45.0f;

	double prevTime;
	glm::vec3 direction;
	glm::vec3 right;
	glm::vec3 up;

	float speed = 3.0f; // 3 units / second
	double mouseSpeed = 0.0025f;


	



	keymap_info keyMap[190];
	

	Camera(GLFWwindow* window) : direction(
		cos(verticalAngle)* sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle)* cos(horizontalAngle)
	), right(
		sin(horizontalAngle - PI / 2.0f),
		0,
		cos(horizontalAngle - PI / 2.0f)
	), up{ glm::cross(right, direction) },
	   key_states{ 0 }
	 , prevTime{glfwGetTime()}
	{
		glfwGetWindowSize(window, &window_width, &window_height);
		cursor_x = ((double)window_width) / 2;
		cursor_y = ((double)window_height) / 2;
		next_cursor_x = cursor_x;
		next_cursor_y = cursor_y;

		glfwSetCursorPos(window, cursor_x, cursor_y);


		keyMap[PYO_KEY_W] = keymap_info(keymap_type::TIMED, (uint8_t)KeyBinds::forward);
		keyMap[PYO_KEY_S] = keymap_info(keymap_type::TIMED, (uint8_t)KeyBinds::backward);
		keyMap[PYO_KEY_D] = keymap_info(keymap_type::TIMED, (uint8_t)KeyBinds::right);
		keyMap[PYO_KEY_A] = keymap_info(keymap_type::TIMED, (uint8_t)KeyBinds::left);

		keyMap[PYO_KEY_ESCAPE] = keymap_info(keymap_type::TRIGGER, 0);
		keyMap[PYO_KEY_Q] = keymap_info(keymap_type::TRIGGER, 1);

		keyMap[PYO_KEY_P] = keymap_info(keymap_type::TOGGLE, 0);



		glfwSetKeyCallback(window, &Camera::key_callback_thunk);

		glfwSetWindowFocusCallback(window, &Camera::focus_callback_thunk);
		glfwSetCursorPosCallback(window, &Camera::cursor_position_callback_thunk);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	~Camera() {

	}

	void focus_callback(GLFWwindow* window, int focused) {
		if (focused == GLFW_TRUE) {
			glfwGetCursorPos(window, &cursor_x, &cursor_y);
			next_cursor_x = cursor_x;
			next_cursor_y = cursor_y;
		}
	}

	void update_pos_from_keys(double deltaTime) {

		float forward_dir = (float)((int)key_states[KeyBinds::forward] - (int)key_states[KeyBinds::backward]);
		float right_dir = (float)((int)key_states[KeyBinds::right] - (int)key_states[KeyBinds::left]);

		position += forward_dir * direction * (float)deltaTime;
		position += right_dir * right * (float)deltaTime;

	}


	void update_view_from_cursor_delta(double deltaTime, double deltaX, double deltaY) {
		horizontalAngle -= (float)(mouseSpeed * deltaX);
		verticalAngle -= (float)(mouseSpeed * deltaY);

		direction = glm::vec3(
			cos(verticalAngle) * sin(horizontalAngle),
			sin(verticalAngle),
			cos(verticalAngle) * cos(horizontalAngle)
			);
		right = glm::vec3(
			sin(horizontalAngle - PI / 2.0f),
			0,
			cos(horizontalAngle - PI / 2.0f)
		); 
		
		up = glm::cross(right, direction);
	}
	void handle_timed_key(GLFWwindow* window, keymap_info keydata, uint8_t key, bool action) {
		KeyBinds binding = (KeyBinds)(keydata.getEventId());

		bool state = key_states[binding];
		bool pressed = key_states[binding];

		if (action) { // GLFW_PRESS
			key_states[binding] = true;
			key_pressed[binding] = true;
			key_released[binding] = false;
		}
		else { // GLFW_RELEASE

			if (pressed) {
				key_released[binding] = true;
			}
			else {
				key_states[binding] = false;
			}
		}
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_UNKNOWN)
			return;

		//Filter out repeats
		if (action == GLFW_REPEAT)
			return;


		// reduce key range
		uint16_t hbit = key & 256;
		key -= 1 - (hbit >> 8);
		uint8_t ckey = key & 255;
		ckey |= hbit >> 1;
		ckey -= 31;

		bool baction = action == GLFW_PRESS;

		keymap_info keydata = keyMap[ckey];
	
		// filter out unbound keys
		if (keydata.data == 0)
			return;
		/* Logic map
		* unset, unpressed, unreleased      (000)->
		* Pressed : set, pressed, unreleased (110)
		* Released: ?

		* unset, unpressed, released   (001)-> ?
		* 
		* unset, pressed, unreleased   (010)-> ?
		* 
		* unset, pressed, released     (011)-> ?
		* 
		* set, unpressed, unreleased   (100)-> ?
		* Pressed : ?
		* Released: unset, unpressed, unreleased (000)
		* set, unpressed, released     (101)-> ?
		* 
		* set, pressed, unreleased     (110)-> ?
		* Pressed : ?
		* Released: set, pressed, released (111)
		* 
		* set, pressed, released       (111)-> ?
		* Pressed : set, pressed, unreleased (110)
		* Released: ?
		* 
		*/

		
		switch (keydata.getType()) {
		case keymap_type::TIMED:
			printf("TIMED\n");
			handle_timed_key(window, keydata, ckey, baction);
			break;
		case keymap_type::TRIGGER:
			printf("TRIGGER\n");
			if(baction)
				triggers[(keydata.getEventId())] = true;
			break;
		case keymap_type::TOGGLE:
			printf("TOGGLE\n");

			if (baction)
				toggles[(keydata.getEventId())] ^= true;
			break;
		}


	}
	
	void cursor_position_callback(GLFWwindow* window, double x, double y)
	{
		next_cursor_x = x;
		next_cursor_y = y;
	}
	static void focus_callback_thunk(GLFWwindow* window, int focused) {
		relthis::relthis(glfwGetWindowUserPointer(window))->focus_callback(window, focused);
	}
	static void key_callback_thunk(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		relthis::relthis(glfwGetWindowUserPointer(window))->key_callback(window, key, scancode, action, mods);
	}

	static void cursor_position_callback_thunk(GLFWwindow* window, double x, double y)
	{
		relthis::relthis(glfwGetWindowUserPointer(window))->cursor_position_callback(window, x, y);
	}




	void update() {
		const double curTime = glfwGetTime();
		const double deltaTime = curTime - prevTime;
		prevTime = curTime;

		double delta_cursor_x = (next_cursor_x - cursor_x) ;
		double delta_cursor_y = (next_cursor_y - cursor_y) ;

		cursor_x = next_cursor_x;
		cursor_y = next_cursor_y;

		update_view_from_cursor_delta(deltaTime, delta_cursor_x, delta_cursor_y);

		update_pos_from_keys(deltaTime);
		prevTime = curTime;
		if (key_released[KeyBinds::forward]) {
			key_states[KeyBinds::forward] = false;
		}
		if (key_released[KeyBinds::backward]) {
			key_states[KeyBinds::backward] = false;
		}
		if (key_released[KeyBinds::right]) {
			key_states[KeyBinds::right] = false;
		}
		if (key_released[KeyBinds::left]) {
			key_states[KeyBinds::left] = false;
		}


		
		key_released.reset();
		key_pressed.reset();
		// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
		projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 1000.0f);
		// Camera matrix
		viewMatrix = glm::lookAt(
			position,             // Camera is here
			position + direction, // and looks here : at the same position, plus "direction"
			up                    // Head is up (set to 0,-1,0 to look upside-down)
		);

		if (triggers[1]) {
			triggers[1] = false;
			std::cout << horizontalAngle << ", " << verticalAngle << std::endl;
		}
	}
};


#endif