// Include standard headers


// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
#include <stdexcept>

#include "common/shader.hpp"
//#include <common/texture.hpp>
#include "common/controls.hpp"
#include "pyoUtils.hpp"
#include "pyo_rawobj.hpp"
#include "world.hpp"
#include <stddef.h>

constexpr float PIPE_SCALE = 0.15f;
constexpr float BALL_SCALE = 0.3f;


#ifndef unreachable
#ifdef __GNUC__ // GCC, Clang, ICC

#define unreachable() (__builtin_unreachable())

#else
#ifdef _MSC_VER // MSVC

#define unreachable() (__assume(false))

#else
#error "No deffinition for unreachable"
#endif
#endif
#endif
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
		glewExperimental = true; // Needed for core profile
		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("Failed to initialize GLEW");
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
GLFWwindow* window;

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
		window = glfwCreateWindow(1024, 768, "Tutorial 09 - Rendering several models", NULL, NULL);
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

template<size_t N>
struct GLBuffers {
	GLuint buffers[N];

	GLBuffers() {
		glCreateBuffers(N, buffers);
	}
	/*
	GLBuffers(const GLBuffers& other) = delete; // copy constructor
	GLBuffers& operator=(const GLBuffers& that) = delete;
	
	GLBuffers(GLBuffers&& other) {
	
	}
	GLBuffers& operator=(GLBuffers&& that) {

	}
	*/
	~GLBuffers() {
		glDeleteBuffers(N, buffers);
	}
};
class gl_error : std::exception {};

struct GLMapedBuffer {
	GLuint buffer;
	void* data;
	GLMapedBuffer(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) : buffer(buffer) {
		data = glMapNamedBufferRange(buffer, offset, length, access);
		if (data == NULL) {
			throw gl_error();
		}
	}
	~GLMapedBuffer() {
		glUnmapNamedBuffer(buffer);
	}
};

template<size_t N>
struct VertexArrays {
	GLuint vertex_arrays[N];

	VertexArrays() {
		glGenVertexArrays(N, vertex_arrays);
	}

	~VertexArrays() {
		glDeleteVertexArrays(N, vertex_arrays);
	}
};

struct StaticMeshes {
	GLBuffers<2> buffers;
	//size_t instance_count;

	size_t verticies_size;
	size_t uvs_size;
	size_t normals_size;

	size_t vertices_offset;
	size_t normals_offset;
	size_t uvs_offset;

	size_t VBO_size;
	size_t IBO_size;
	//size_t InstanceBuffer_size;

/*	struct InstanceBufferStorage {
		InstanceBufferStorage(GLuint buffer, size_t size) {
			glNamedBufferStorage(buffer, (GLsizeiptr )size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		}
	} instance_buffer_storage_helper;
	//GLMapedBuffer MInstanceBuffer;
	*/
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

	/*(constexpr GLuint InstanceBuffer() const {
		return buffers.buffers[2];
	}*/

	StaticMeshes() : buffers{}{
		
		RawobjectReader monkey{ "tubes.jpraw" };
		assert(monkey.sizeof_index_t == 2);
		assert(monkey.sizeof_element_t == 4);
		assert(monkey.header.indexed());
		assert(monkey.header.hasNormal());
		//assert(monkey.header.hasUV());


		verticies_size = monkey.offsets.verticies_end - monkey.offsets.verticies_start;
		//uvs_size = monkey.offsets.uvs_end - monkey.offsets.uvs_start;
		normals_size = monkey.offsets.normals_end - monkey.offsets.normals_start;

		vertices_offset = 0;
		normals_offset = monkey.offsets.normals_start - monkey.offsets.verticies_start;
		//uvs_offset = monkey.offsets.uvs_start - monkey.offsets.verticies_start;


		VBO_size = monkey.arrays_end - monkey.arrays_start;
		IBO_size = monkey.index_end - monkey.index_start;

		numElements = (monkey.counts.triangles);

		glNamedBufferStorage(VBO(), VBO_size, nullptr, GL_MAP_WRITE_BIT);
		glNamedBufferStorage(IBO(), IBO_size, nullptr, GL_MAP_WRITE_BIT);

		GLMapedBuffer MVBO(VBO(), 0, VBO_size, GL_MAP_WRITE_BIT);
		GLMapedBuffer MIBO(IBO(), 0, IBO_size, GL_MAP_WRITE_BIT);
		//GLMapedBuffer MInstanceBuffer();

		monkey.file.tseek((long)monkey.index_start, SEEK_SET);
		monkey.file.tread(MIBO.data, 1, IBO_size);


		monkey.file.tseek((long)monkey.arrays_start, SEEK_SET);
		monkey.file.tread(MVBO.data, 1, VBO_size);
		
		//numSubElements.resize(monkey.header.obj_count);
		for (auto count : monkey.obj_counts) {
			numSubElements.push_back(count.triangles);
		}

		for (const auto& offset : monkey.sub_offsets) {
			subOffsets.push_back(offset.triangles_start - monkey.index_start);
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

		// 1st attribute buffer : verticies
		glVertexAttribPointer(
			GLLoc::vertexPosition_modelspace, // attribute
			3,								  // size
			GL_FLOAT,						  // type
			GL_FALSE,						  // normalized?
			0,								  // stride
			(void*)vertices_offset            // array buffer offset
		);

		// 2rd attribute buffer : normals
		glVertexAttribPointer(
			GLLoc::vertexNormal_modelspace,   // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)normals_offset             // array buffer offset
		);

		//glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer());
		glVertexAttribFormat(GLLoc::M, 4, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(GLLoc::M_1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4);
		glVertexAttribFormat(GLLoc::M_2, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 8);
		glVertexAttribFormat(GLLoc::M_3, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 12);

		glVertexAttribBinding(GLLoc::M, GLLoc::M);
		glVertexAttribBinding(GLLoc::M_1, GLLoc::M);
		glVertexAttribBinding(GLLoc::M_2, GLLoc::M);
		glVertexAttribBinding(GLLoc::M_3, GLLoc::M);

		glVertexBindingDivisor(GLLoc::M, 1);

/*
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, (void*)(0));
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * 4));
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * 8));
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * 12));
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		/*
		// 3nd attribute buffer : colors
		glVertexAttribPointer(
			2,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)uvs_offset                 // array buffer offset
		);
		*/
	}


	~StaticMeshes() {
	}
};

struct Uniforms {
	
	GLuint PerspectiveID;
	GLuint ViewMatrixID;
	GLuint ColorID;

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID;

	// Get a handle for our "LightPosition" uniform
	GLuint LightID;

	Uniforms(GLuint programID) {
		PerspectiveID = glGetUniformLocation(programID, "P");
		ViewMatrixID = glGetUniformLocation(programID, "V");
		ColorID = glGetUniformLocation(programID, "pipe_color");


		// Get a handle for our "myTextureSampler" uniform
		TextureID = glGetUniformLocation(programID, "myTextureSampler");

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
struct BufferStorageHelper{
	BufferStorageHelper(GLuint buffer, size_t size) {
		glNamedBufferStorage(buffer, (GLsizeiptr)size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
	}
};

struct PipeRenderData {
	size_t numBalls = 0;
	size_t numPipes = 0;
	size_t buffer_size;
	GLBuffers<1> buffer;
	BufferStorageHelper _helper;
	GLMapedBuffer mbuffer;
	void* nextBall;
	void* nextPipe;
	PipeRenderData(size_t buffer_size) : buffer_size{ buffer_size }, _helper{ buffer.buffers[0], buffer_size * sizeof(glm::mat4)},
		mbuffer{ buffer.buffers[0], 0, (GLsizeiptr)(buffer_size*sizeof(glm::mat4)), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT} {
		nextPipe = mbuffer.data;
		nextBall = (char*)mbuffer.data + ((buffer_size - 1) * sizeof(glm::mat4));


	}

	
	void addPipe(glm::vec3 center, Direction dir) {
		if (nextBall < nextPipe) {
			// todo, reallocate
			throw std::runtime_error("out of buffer space");
		}
		glm::mat4 M = glm::translate(glm::identity<glm::mat4>(), center);

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
		M = glm::scale(M, glm::vec3{1,  6.6667f, 1 });


		memcpy(nextPipe, &M, sizeof(glm::mat4));
		glFlushMappedBufferRange(buffer.buffers[0], numPipes * sizeof(glm::mat4), sizeof(glm::mat4));
		++numPipes;
		nextPipe = (void*)((char*)nextPipe + sizeof(glm::mat4));
	}
	GLsizeiptr ball_offset() {
		return (buffer_size - numBalls) * sizeof(glm::mat4);
	}
	void addBend(glm::vec3 center, Direction start, Direction end) {

	}

	void addFirstPipe(glm::vec3 center, Direction dir) {

	}

	void addBall(glm::vec3 center) {
		if (nextBall < nextPipe) {
			// todo, reallocate
			throw std::runtime_error("out of buffer space");
		}
		glm::mat4 M = glm::translate(glm::identity<glm::mat4>(), center);
		memcpy(nextBall, &M, sizeof(glm::mat4));
		glFlushMappedBufferRange(buffer.buffers[0], (char*)nextBall - (char*)mbuffer.data, sizeof(glm::mat4));
		nextBall = (void*)((char*)nextBall - sizeof(glm::mat4));
		++numBalls;
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
	void setupInput() {
		// Ensure we can capture the escape key being pressed below
		//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
		// Hide the mouse and enable unlimited mouvement
		glfwSetWindowUserPointer(window, this);
		//glfwPollEvents();


	}

	void make_ball_joint(size_t pipe_id, glm::uvec3 node){
		/*
		PipeRenderData prd = pipe_render_data[pipe_id];
		size_t lower_bound;
		if (pipe_id == 0) {
			lower_bound = 1;
		}
		else {
			lower_bound = pipe_render_data[pipe_id - 1].base - (pipe_render_data[pipe_id - 1].numPipes + 1);
		}

		size_t tail = prd.base - prd.numBalls;
		if (tail == lower_bound) {
			/// todo, realloc
			throw std::runtime_error("can't allocate new pipe ball");
		}
		prd.numBalls += 1;
		tail -= 1;
		glm::mat4 pos = glm::translate(glm::mat4(), glm::vec3(node));
		memcpy((char*)meshes.MInstanceBuffer.data + (tail * sizeof(glm::mat4)), &pos, sizeof(glm::mat4));
		glFlushMappedBufferRange(meshes.InstanceBuffer(), (tail * sizeof(glm::mat4)), sizeof(glm::mat4));
		*/
	}
	void make_pipe_section(size_t pipe_id, Direction dir, glm::uvec3 node) {
		PipeRenderData prd = pipe_render_data[pipe_id];
		size_t upper_bound;
	}

	void setupGL() {
		// Dark blue background
		glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		// Cull triangles which normal is not towards the camera
		glEnable(GL_CULL_FACE);
	}
	App() : glfw_trap{}, window{ this }, glew_trap{}, camera{ window }, program{}, meshes{ }, pipe_data{ 100 }{
		// Initialise GLFW
		pipe_render_data.reserve(world.max_pipes);
		setupInput();
		setupGL();
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
					pipe_render_data[i].addPipe(straightData.current_node, straightData.current_dir);
					break;
				}
				case PipeUpdataType::PIPE_BEND: {
					PipeBendData& bendData = update_data.data.pipeBendData;
					pipe_render_data[i].addBall(bendData.last_node);
					pipe_render_data[i].addPipe(bendData.current_node, bendData.current_dir);
					break;
				}
				case PipeUpdataType::FIRST_PIPE: {
					PipeStraightData& straightData = update_data.data.pipeStraightData;
					pipe_render_data[i].addFirstPipe(straightData.current_node, straightData.current_dir);
					break;
				}
				default:
					unreachable();

				}
			}
		}

		if (world.pipe_count() < 2 &&  world.chance(world.new_pipe_chance)) {
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
		/*
		glm::mat4 pos[3];
		pos[0] = glm::mat4{ {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 1, 2, 1} };

		pos[1] = glm::mat4{ {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1} };
		pos[2] = glm::mat4{ {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 1, -2, 1} };
		pos[2] = glm::rotate(pos[2], glm::radians<float>(90), glm::vec3{ 1, 0, 0 });
		*/
		//memcpy(meshes.MInstanceBuffer.data, pos, 3 * sizeof(glm::mat4));
		//glFlushMappedBufferRange(meshes.InstanceBuffer(), 0, sizeof(glm::mat4));

		program.use();
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, texture.texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		//glUniform1i(program.uniforms.TextureID, 0);

		// set the light position
		glm::vec3 lightPos = glm::vec3(4, 4, 4);
		glUniform3f(program.uniforms.LightID, lightPos.x, lightPos.y, lightPos.z);
		//glBindVertexBuffer(2, prd.buffer.buffers[0], 0, sizeof(float) * 16);
		double prevTime = glfwGetTime();
		do {


			// Clear the screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			// Compute the MVP matrix from keyboard and mouse input
			
			camera.update();
			glm::mat4 ModelMatrix = glm::mat4(1.0);
			glm::mat4 MVP = camera.projectionMatrix * camera.viewMatrix * ModelMatrix;

			// in the "MVP" uniform
			glUniformMatrix4fv(program.uniforms.PerspectiveID, 1, GL_FALSE, &camera.projectionMatrix[0][0]);
			//glUniformMatrix4fv(program.uniforms.ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(program.uniforms.ViewMatrixID, 1, GL_FALSE, &camera.viewMatrix[0][0]);


			// Draw the triangles !
			//glDrawElementsInstancedBaseInstance(GL_TRIANGLES, (GLsizei)(meshes.numSubElements[0] * 3), GL_UNSIGNED_SHORT, (void*)meshes.subOffsets[0], 1, 0);

			double curTime = glfwGetTime();
			if (curTime - prevTime >= 1.0L) {
				prevTime = curTime;
				update_world();

			}
			for (int pipe_id = 0; pipe_id < pipe_render_data.size(); ++pipe_id) {
				PipeRenderData& prd = pipe_render_data[pipe_id];
				const glm::vec3& color = world.colors[pipe_id];
				glUniform3f(program.uniforms.ColorID, color.x, color.y, color.z);

				if (prd.numPipes) {
					glBindVertexBuffer(GLLoc::M, prd.buffer.buffers[0], 0, sizeof(float) * 16);
					glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)(meshes.numSubElements[1] * 3), GL_UNSIGNED_SHORT, (void*)meshes.subOffsets[1], prd.numPipes);
				} 
				
				if (prd.numBalls) {
					glBindVertexBuffer(GLLoc::M, prd.buffer.buffers[0], prd.ball_offset(), sizeof(float) * 16);
					glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)(meshes.numSubElements[0] * 3), GL_UNSIGNED_SHORT, (void*)meshes.subOffsets[0], prd.numBalls);
				}
			}
			//glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)(meshes.numSubElements[1] * 3), GL_UNSIGNED_SHORT, (void*)meshes.subOffsets[1], 10);

			// Swap buffers
			glfwSwapBuffers(window);
			glfwPollEvents();

		} // Check if the ESC key was pressed or the window was closed
		while (!camera.triggers[0] &&
			glfwWindowShouldClose(window) == 0);
	}
};

#include <filesystem>

int main() {

	std::cout << std::filesystem::current_path() <<std::endl;
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