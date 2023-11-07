// Include standard headers


// Include GLEW
#include <glad/glad.h>

// Include GLFW
#include <GLFW/glfw3.h>
#include <stdexcept>

#include "common/shader.hpp"
//#include <common/texture.hpp>
#include "common/controls.hpp"
#include "pyoUtils.hpp"
#include "pyo_rawobj.hpp"
#include "world.hpp"
#include "pyo_stddef.h"
#include "gl_classes.hpp"

//static_assert(std::is_trivially_destructible<std::vector<int>>());
constexpr float PIPE_DIAMETER = 0.15f;
constexpr float BALL_DIAMETER = 0.3f;
constexpr float BALL_RADIUS = BALL_DIAMETER / 2.0f;
constexpr int WINDOW_WIDTH = 1024;
constexpr int WINDOW_HEIGHT = 768;

namespace GLLoc {
	enum _GLLoc : GLint {
		vertexPosition_modelspace,
		vertexNormal_modelspace,
		M,
		M_1,
		M_2,
		M_3
	};

	using GLLoc = _GLLoc;
}
class GLEWTrap {
public:
	GLEWTrap() {
		// Initialize GLEW
		//glewExperimental = true; // Needed for core profile
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			throw std::runtime_error("Failed to initialize GLAD");
		}
	}
};
class GLFWTrap {
public:
	GLFWTrap() {
		if (!glfwInit())
		{
			throw std::runtime_error("Failed to initialize GLFW");
		}
	}
	~GLFWTrap() {
		glfwTerminate();
	}

};

class Window {
public:
	GLFWwindow* window;

	Window(void* container) {
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		
		// Open a window and create its OpenGL context
		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "gl_pipes", NULL, NULL);
		if (window == NULL)
			throw std::runtime_error("Failed to open GLFW window.If you have an Intel GPU, they are not 3.3 compatible.Try the 2.1 version of the tutorials.");
		
		glfwMakeContextCurrent(window);
		glfwSetWindowUserPointer(window, container);
	}

	~Window() {
		glfwDestroyWindow(window);
	}


	

	operator GLFWwindow* () {
		return window;
	}
};


struct StaticMeshes {
	GLBuffers<2> buffers;

	size_t verticies_size;
	size_t uvs_size;
	size_t normals_size;

	size_t vertices_offset;
	size_t normals_offset;
	size_t uvs_offset;

	size_t VBO_size;
	size_t IBO_size;

	size_t numElements;
	std::vector<size_t> numSubElements;
	std::vector<size_t> subOffsets;

	VertexArrays<1> vertex_arrays;

	constexpr GLuint vertex_array() {
		return vertex_arrays.vertex_arrays[0];
	}

	constexpr GLuint VBO() const {
		return buffers.buffers[0];
	}

	constexpr GLuint IBO() const {
		return buffers.buffers[1];
	}

	StaticMeshes() : buffers{}{
		
		RawobjectReader tube_mesh{ "tubes.jpraw" };
		assert(tube_mesh.sizeof_index_t == 2);
		assert(tube_mesh.sizeof_element_t == 4);
		assert(tube_mesh.header.indexed());
		assert(tube_mesh.header.hasNormal());
		//assert(tube_mesh.header.hasUV());


		verticies_size = tube_mesh.offsets.verticies_end - tube_mesh.offsets.verticies_start;
		//uvs_size = tube_mesh.offsets.uvs_end - tube_mesh.offsets.uvs_start;
		normals_size = tube_mesh.offsets.normals_end - tube_mesh.offsets.normals_start;

		vertices_offset = 0;
		normals_offset = tube_mesh.offsets.normals_start - tube_mesh.offsets.verticies_start;
		//uvs_offset = tube_mesh.offsets.uvs_start - tube_mesh.offsets.verticies_start;


		VBO_size = tube_mesh.arrays_end - tube_mesh.arrays_start;
		IBO_size = tube_mesh.index_end - tube_mesh.index_start;

		numElements = (tube_mesh.counts.triangles);

		glNamedBufferStorage(VBO(), VBO_size, nullptr, GL_MAP_WRITE_BIT);
		glNamedBufferStorage(IBO(), IBO_size, nullptr, GL_MAP_WRITE_BIT);

		GLMapedBuffer MVBO(VBO(), 0, VBO_size, GL_MAP_WRITE_BIT);
		GLMapedBuffer MIBO(IBO(), 0, IBO_size, GL_MAP_WRITE_BIT);
		//GLMapedBuffer MInstanceBuffer();

		tube_mesh.file.tseek((long)tube_mesh.index_start, SEEK_SET);
		tube_mesh.file.tread(MIBO.data, 1, IBO_size);


		tube_mesh.file.tseek((long)tube_mesh.arrays_start, SEEK_SET);
		tube_mesh.file.tread(MVBO.data, 1, VBO_size);
		
		//numSubElements.resize(tube_mesh.header.obj_count);
		for (auto count : tube_mesh.obj_counts) {
			numSubElements.push_back(count.triangles);
		}

		for (const auto& offset : tube_mesh.sub_offsets) {
			subOffsets.push_back(offset.triangles_start - tube_mesh.index_start);
		}

		glBindVertexArray(vertex_array());
		glBindBuffer(GL_ARRAY_BUFFER, VBO());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO());



		glEnableVertexAttribArray(GLLoc::vertexPosition_modelspace);
		glEnableVertexAttribArray(GLLoc::vertexNormal_modelspace);

		glEnableVertexAttribArray(GLLoc::M);
		glEnableVertexAttribArray(GLLoc::M_1);
		glEnableVertexAttribArray(GLLoc::M_2);
		glEnableVertexAttribArray(GLLoc::M_3);

		//glEnableVertexAttribArray(2);

		// 1st attribute mapped_buffer : verticies
		glVertexAttribPointer(
			GLLoc::vertexPosition_modelspace, // attribute
			3,								  // size
			GL_FLOAT,						  // type
			GL_FALSE,						  // normalized?
			0,								  // stride
			(void*)vertices_offset            // array mapped_buffer offset
		);

		// 2rd attribute mapped_buffer : normals
		glVertexAttribPointer(
			GLLoc::vertexNormal_modelspace,   // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)normals_offset             // array mapped_buffer offset
		);

		// Set the format for the model matricies loc
		// can't use glVertexAttribPointer since this loc's mapped_buffer will be regularly swapped out
		glVertexAttribFormat(GLLoc::M, 4, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(GLLoc::M_1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4);
		glVertexAttribFormat(GLLoc::M_2, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 8);
		glVertexAttribFormat(GLLoc::M_3, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 12);
		
		// A mat4 takes up 4 bining locations
		glVertexAttribBinding(GLLoc::M, GLLoc::M);
		glVertexAttribBinding(GLLoc::M_1, GLLoc::M);
		glVertexAttribBinding(GLLoc::M_2, GLLoc::M);
		glVertexAttribBinding(GLLoc::M_3, GLLoc::M);

		// The model matrix is a per instance attribute
		glVertexBindingDivisor(GLLoc::M, 1);
	}


	~StaticMeshes() {
	}
};

struct Uniforms {
	
	GLuint PerspectiveID;
	GLuint ViewMatrixID;
	GLuint ColorID;

	// Get a handle for our "myTextureSampler" uniform
	//GLuint TextureID;

	// Get a handle for our "LightPosition" uniform
	GLuint LightID;

	Uniforms(GLuint programID) {
		PerspectiveID = glGetUniformLocation(programID, "P");
		ViewMatrixID = glGetUniformLocation(programID, "V");
		ColorID = glGetUniformLocation(programID, "pipe_color");


		// Get a handle for our "myTextureSampler" uniform
		//TextureID = glGetUniformLocation(programID, "myTextureSampler");

		// Get a handle for our "LightPosition" uniform
		LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	}
};

struct Program {
	GLuint programID;
	Uniforms uniforms;
	Program() : programID(LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader")), uniforms(programID) {

	}

	void use() {
		glUseProgram(programID);
	}
};
/*
struct Texture {
	GLuint texture;

	Texture() : texture(loadDDS("uvtemplate.DDS")){
	}

	~Texture() {
		glDeleteTextures(1, &texture);
	}
};
*/

struct PipeRenderData {
	GLPersistentBufferDoubleStack<glm::mat4> stack_buffer;

	glm::vec3 direction;
	glm::vec3 start;
	glm::vec3 end;

	bool initialized;
	float length;
	PipeRenderData(size_t buffer_size) : stack_buffer{ (GLsizeiptr)buffer_size },
		initialized{ false } {
	}
	void rotateTowardsDirection(glm::mat4& M) {
		glm::vec3 rot{ direction.z, direction.y, direction.x };
		M = glm::rotate(M, glm::radians<float>(90), rot);
	}
	void rotateTowardsDir(glm::mat4& M, Direction dir) {
		switch (dir) {
		case Direction::North:
		case Direction::South:
			M = glm::rotate(M, glm::radians<float>(90), glm::vec3{ 1, 0, 0 });
			break;
		case Direction::East:
		case Direction::West:
			M = glm::rotate(M, glm::radians<float>(90), glm::vec3{ 0, 0, 1 });
			break;
		case Direction::Up:
		case Direction::Down:
			break;

		default:
			unreachable();
		}
	}

	GLsizeiptr pipes_size() const noexcept {
		return stack_buffer.left_size();
	}

	GLsizeiptr balls_size() const noexcept{
		return stack_buffer.right_size();
	}
	GLsizeiptr pipe_offset() const noexcept {
		return 0;
	}
	GLsizeiptr ball_offset() const noexcept {
		return stack_buffer.right_offset() *  sizeof(glm::mat4);
	}

	void addPipeFromBall(glm::vec3 ballCenter) {
		glm::vec3 ballEdge_r = ballCenter + direction * BALL_RADIUS;
		start = ballEdge_r;
		end = ballCenter + direction - direction * BALL_RADIUS;
		length = 1.0f - BALL_DIAMETER;


		glm::vec3 center = start + (end - start) / 2.0f;
		float scale = length / PIPE_DIAMETER;

		glm::mat4 M = glm::translate(glm::identity<glm::mat4>(), center);
		rotateTowardsDirection(M);
		M = glm::scale(M, glm::vec3{ 1,  scale, 1 });
		// a pipe originating from a ball is always a new pipe
		stack_buffer.push_left(M);
	}
	void addBend(Direction newDirection, glm::vec3 ballCenter2, glm::vec3 cur_node) {
		glm::vec3 ballCenter = end + direction * BALL_RADIUS;

		addBall(ballCenter);
		direction = dirAsVec(newDirection);
		addPipeFromBall(ballCenter);
	}
	void kill() {
		glm::vec3 ballCenter = end + direction * BALL_RADIUS;
		addBall(ballCenter);
	}
	void grow(glm::vec3 node) {
		length += 1;
		end += direction;
		float scale = length / (float)(PIPE_DIAMETER);
		glm::vec3 center = start + (end - start) / 2.0f;
		glm::mat4 M = glm::translate(glm::identity<glm::mat4>(), center);
		rotateTowardsDirection(M);
		M = glm::scale(M, glm::vec3{ 1,  scale, 1 });

		stack_buffer.update_left(M);			

	}
	void addFirstPipe(glm::vec3 center, glm::vec3 ballCenter, Direction dir) {
		initialized = true;
		direction = dirAsVec(dir);
		addPipeFromBall(ballCenter);
	}

	void addBall(glm::vec3 center) {
		glm::mat4 M = glm::translate(glm::identity<glm::mat4>(), center);
		stack_buffer.push_right(M);
	}

	const GLBuffer& buffer() {
		return stack_buffer.buffer();
	}

};
class App {
public:
	static constexpr size_t BUFFER_INIT_SIZE = 128;
	PipeUpdateData update_data;
	GLFWTrap glfw_trap;
	Window window;
	GLEWTrap glew_trap;
	Camera<relative_this(App, camera)> camera;
	Program program;
	//Texture texture;
	StaticMeshes meshes;
	World world{ 20, 20, 20, 4 };
	std::vector< PipeRenderData> pipe_render_data;
	std::vector<glm::mat4> pipe_data;

	void setupGLRenderState() {
		// Dark blue background
		glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		// Cull backfaces
		glEnable(GL_CULL_FACE);
	}
	App() : glfw_trap{}, window{ this }, glew_trap{}, camera{ window }, program{}, meshes{ }, pipe_data{ 100 }{
		// Preallocate space (emplace_back would trigger resize otherwise), since GLMappedBuffer and 
		// GLBuffers can't be coppied, and C++ lack destructive-move semantics
		pipe_render_data.reserve(world.max_pipes);

		glfwSetWindowUserPointer(window, this);

		setupGLRenderState();
	}
	void update_world() {
		if (!world.is_gen_complete()) {
			for (int i = 0; i < world.pipe_count(); ++i) {
				if (!world.is_pipe_alive(i))
					continue;
				world.pipe_update(update_data, i);

				switch (update_data.type) {
				case PipeUpdataType::NOP:
					break;
				case PipeUpdataType::PIPE_STRAIGHT:{
					PipeStraightData& straightData = update_data.data.pipeStraightData;
					pipe_render_data[i].grow(straightData.current_node);
					break;
				}
				case PipeUpdataType::PIPE_BEND: {
					PipeBendData& bendData = update_data.data.pipeBendData;
					pipe_render_data[i].addBend(bendData.current_dir, bendData.last_node, bendData.current_node);
					break;
				}
				case PipeUpdataType::FIRST_PIPE: {
					std::cout << "FIRST PIPE" << std::endl;
					PipeStraightData& straightData = update_data.data.pipeStraightData;
					pipe_render_data[i].addFirstPipe(straightData.current_node, straightData.last_node, straightData.current_dir);
					break;
				}
				case PipeUpdataType::DIED: {
					printf("Dead Pipe\n");
					pipe_render_data[i].kill();
					break;
				}
				default:
					unreachable();

				}
			}
		}

		if (world.pipe_count() < world.max_pipes){//}&& world.chance(world.new_pipe_chance)) {
			world.new_pipe(update_data);
			if (update_data.type == PipeUpdataType::NEW) {
				NewPipeData& newData = update_data.data.newPipeData;
				pipe_render_data.emplace_back(BUFFER_INIT_SIZE);
				pipe_render_data.back().addBall(newData.start_node);
			}
			else {
				unreachable();
			}
		}
	}
	void run() {
		program.use();
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, texture.texture);
		//glUniform1i(program.uniforms.TextureID, 0);

		// set the light position
		glm::vec3 lightPos = glm::vec3(10, 10,10);
		glUniform3f(program.uniforms.LightID, lightPos.x, lightPos.y, lightPos.z);

		double prevTime = glfwGetTime();
		do {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			camera.update();
			
			glUniformMatrix4fv(program.uniforms.PerspectiveID, 1, GL_FALSE, &camera.projectionMatrix[0][0]);
			glUniformMatrix4fv(program.uniforms.ViewMatrixID, 1, GL_FALSE, &camera.viewMatrix[0][0]);

			// check if the world should be updated
			double curTime = glfwGetTime();
			if (curTime - prevTime >= .1L) {
				prevTime = curTime;
				if(camera.toggles[0] == false)
					update_world();
			}

			// Draw each pipe
			for (int pipe_id = 0; pipe_id < pipe_render_data.size(); ++pipe_id) {
				PipeRenderData& prd = pipe_render_data[pipe_id];
				// Bind the correct color
				const glm::vec3& color = world.colors[pipe_id];
				glUniform3f(program.uniforms.ColorID, color.x, color.y, color.z);

				// Draw the tubes
				if (prd.pipes_size()) {
					glBindVertexBuffer(GLLoc::M, prd.buffer(), 0, sizeof(float) * 16);
					glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)(meshes.numSubElements[1] * 3), GL_UNSIGNED_SHORT, (void*)meshes.subOffsets[1], prd.pipes_size());
				} 
				// Draw the balls
				if (prd.balls_size()) {
					glBindVertexBuffer(GLLoc::M, prd.buffer(), prd.ball_offset(), sizeof(float) * 16);
					glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)(meshes.numSubElements[0] * 3), GL_UNSIGNED_SHORT, (void*)meshes.subOffsets[0], prd.balls_size());
				}
			}

			glfwSwapBuffers(window);
			glfwPollEvents();

		} // Check if the ESC key was pressed or the window was closed
		while (!camera.triggers[0] &&
			glfwWindowShouldClose(window) == 0);
	}
};

int main() {

	try {
		App app;
		app.run();
	}
	catch (std::exception& e) {
		fprintf(stderr, e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;

}