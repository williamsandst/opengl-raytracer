#include "Renderer.h"

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::updateDeltatime()
{
	float currentFrame = SDL_GetTicks();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

void Renderer::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	camera.calculateProjectionMatrix();
	camera.calculateViewMatrix();
	camera.getCornerRays();

	switch (renderType)
	{
		case PathTracer: drawPathTracer(); break;
		case Rasterizer: drawRasterizer(); break;
	}

	SDL_GL_SwapWindow(window);
}

void Renderer::drawRasterizer()
{
	rasterizerShader.use();

	for (auto& it : geometryVBOs) { //Draw chunks
		glm::mat4 matrix = glm::mat4(camera.getProjectionMatrix() * camera.getViewMatrix() * it.translation);
		rasterizerShader.setMat4("matrix", matrix);
		glBindVertexArray(it.VAO);
		glDrawArrays(GL_TRIANGLES, 0, it.size);
	}
}

void Renderer::initRasterizer()
{
	rasterizerShader = Shader("shaders/geometry.vert", "shaders/geometry.frag");
}

void Renderer::drawPathTracer()
{
	pathTracerComputeShader.use();

	//Set corner ray uniforms to give the program the view
	pathTracerComputeShader.setVec3("eye", camera.getPosition());
	pathTracerComputeShader.setVec3("ray00", camera.ray00);
	pathTracerComputeShader.setVec3("ray10", camera.ray10);
	pathTracerComputeShader.setVec3("ray01", camera.ray01);
	pathTracerComputeShader.setVec3("ray11", camera.ray11);

	 // launch compute shaders!
	glDispatchCompute((GLuint)screenWidth, (GLuint)screenHeight, 1);
	
	// make sure writing to image has finished before read
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glActiveTexture(GL_TEXTURE0);
	screenTextureShader.use();
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

void Renderer::init()
{
	camera = Camera(windowWidth, windowHeight);

	initSDL();
	initOpenGL();

	switch (renderType)
	{
	case PathTracer: initPathTracer(); break;
	case Rasterizer: initRasterizer(); break;
	}
}

void Renderer::initOpenGL()
{
	glewExperimental = GL_TRUE;
	glewInit();
	
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.3f, 0.4f, 1.0f);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	
	//Wireframes
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	glViewport(0, 0, windowWidth, windowHeight);
}

void Renderer::initSDL()
{
	if (SDL_Init(SDL_INIT_VIDEO), 0) {
		std::cout << "SDL could not initialize! SDL_Error:" << SDL_GetError() << std::endl;
	}

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	screenHeight = DM.h;
	screenWidth = DM.w;

	window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	glContext = SDL_GL_CreateContext(window);

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_ShowCursor(0);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetSwapInterval(1);
}

void Renderer::resizeWindow(int width, int height)
{
	SDL_SetWindowSize(window, width, height);
	int newScreenPosX = screenWidth / 2 - width / 2;
	int newScreenPosY = screenHeight / 2 - height / 2; 
	SDL_SetWindowPosition(window, newScreenPosX, newScreenPosY);
}

void Renderer::initPathTracer()
{
	//Screen Texture
	//stbi_set_flip_vertically_on_load(true);
	screenTextureShader = Shader("shaders/screentexture.vert", "shaders/screentexture.frag");
	pathTracerComputeShader = ComputeShader("shaders/pathtracer.comp");

	//Compute shader
	// dimensions of the image
	glGenTextures(1, &textureOutput);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureOutput);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT,
		NULL);
	glBindImageTexture(0, textureOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

void Renderer::close()
{
	SDL_Quit();
}

void Renderer::loadVBOs(std::vector<Mesh>& meshes)
{
	for (auto &mesh : meshes)
	{
		geometryVBOs.push_back(GeometryVBO(mesh.pos, mesh.vertices));
	}
}
