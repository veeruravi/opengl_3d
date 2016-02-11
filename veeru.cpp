#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;
	GLuint TextureBuffer;
	GLuint TextureID;

	GLenum PrimitiveMode; // GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY and GL_TRIANGLES_ADJACENCY
	GLenum FillMode; // GL_FILL, GL_LINE
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID; // For use with normal shader
	GLuint TexMatrixID; // For use with texture shader
} Matrices;

struct FTGLFont {
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	cout << "Compiling shader : " <<  vertex_file_path << endl;
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	cout << VertexShaderErrorMessage.data() << endl;

	// Compile Fragment Shader
	cout << "Compiling shader : " << fragment_file_path << endl;
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	cout << FragmentShaderErrorMessage.data() << endl;

	// Link the program
	cout << "Linking program" << endl;
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	cout << ProgramErrorMessage.data() << endl;

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	cout << "Error: " << description << endl;
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

	if (hue < 60)
		return glm::vec3(1,x,0);
	else if (hue < 120)
		return glm::vec3(x,1,0);
	else if (hue < 180)
		return glm::vec3(0,1,x);
	else if (hue < 240)
		return glm::vec3(0,x,1);
	else if (hue < 300)
		return glm::vec3(x,0,1);
	else
		return glm::vec3(1,0,x);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  1,                  // attribute 1. Color
						  3,                  // size (r,g,b)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;
	vao->TextureID = textureID;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
	glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  2,                  // attribute 2. Textures
						  2,                  // size (s,t)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

void draw3DTexturedObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Bind Textures using texture units
	glBindTexture(GL_TEXTURE_2D, vao->TextureID);

	// Enable Vertex Attribute 2 - Texture
	glEnableVertexAttribArray(2);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

	// Unbind Textures to be safe
	glBindTexture(GL_TEXTURE_2D, 0);
}

/* Create an OpenGL Texture from an image */
GLuint createTexture (const char* filename)
{
	GLuint TextureID;
	// Generate Texture Buffer
	glGenTextures(1, &TextureID);
	// All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// Set our texture parameters
	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering (interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load image and create OpenGL texture
	int twidth, theight;
	unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
	SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

	return TextureID;
}


/**************************
 * Customizable functions *
 **************************/

VAO *cube,*person_body,*water,*walls,*person_leg,*person_hand,*person_eye,*person_neck,*person_head,*person_hair;
double wall[5][4],no_of_walls=2;
/*
	0-x
	1-z
	2-length
	3-direction
*/
double gameover=0;
double camera_x_direction=1,camera_z_direction=1;
double key=2;
double top_view=0,reset_view=0;
double length_of_cube_base=25,length_of_base=30,width_of_base=30,height_of_base=5;
double heights[30][30],empty_cube[182][2],no_of_pits=1;
double obstacles[182][2],no_of_obstacles=1;
int width = 1000;
int height = 700;
double camera_angle=0,camera_speed=1,camera_y=0;
double camera_nx=0,camera_ny=0,camera_nz=0,normal_view=0;
double person_x=(length_of_cube_base*length_of_base-3*length_of_cube_base)/2,person_z=(length_of_cube_base*width_of_base-3*length_of_cube_base)/2,person_y=length_of_cube_base*3/2.0+(height_of_base-2)*length_of_cube_base,person_shift=5,fall_state=0;
double person_jump=0,head_view=0,jump_speed=0,jump_direction=1;
int a_pressed=0,d_pressed=0,up_pressed=0,down_pressed=0,right_pressed=0,left_pressed=0,w_pressed=0,s_pressed=0,g_pressed=0,f_pressed=0;
int l_pressed=0;
double person_hand_angle=0,hand_angle_speed=5;
void intialize_base()
{
	for (int i = 0; i < length_of_base;i++)
		for (int i1 = 0; i1 < width_of_base;i1++)
			heights[i][i1]=height_of_base;
	int x=length_of_base-1,i2=0;
	for (int i = 0; i < width_of_base;i++)
	{
		if (i!=15&&i!=16)
		{
			heights[x][i]=height_of_base+2;
			heights[0][i]=height_of_base+2;
		}
		//if(i!=15&&i!=14&&i!=13)
		heights[(x-1)/2][i]=height_of_base+1;	
	}
	for (int i = 0; i < length_of_base;i++)
	{
		heights[i][x]=height_of_base+2;
		heights[i][0]=height_of_base+2;
		//if(i!=15&&i!=14&&i!=13)
		heights[i][(x-1)/2]=height_of_base+2;
	}
	heights[28][24]=height_of_base+1;
	heights[27][24]=height_of_base+1;
	heights[26][24]=height_of_base+1;
	heights[25][24]=height_of_base+1;
	heights[25][23]=height_of_base+1;
	heights[25][22]=height_of_base+1;
	heights[25][21]=height_of_base+1;
	heights[25][20]=height_of_base+1;
	heights[26][20]=height_of_base+1;
	heights[27][20]=height_of_base+1;
	heights[27][21]=height_of_base+1;
	for (int i = 0; i < length_of_base;i++)
		for (int i1 = 0; i1 < width_of_base;i1++)
		{
			if(heights[i][i1]!=height_of_base)
				if ((i!=(x-1)/2 ||(i1!=13&&i1!=15) ) && ( i1!=(x-1)/2 || (i!=13&&i!=15) ))
				{
					obstacles[i2][0]=length_of_cube_base/2.0+(i-width_of_base/2.0)*length_of_cube_base;
					obstacles[i2][1]=length_of_cube_base/2.0+(i1-length_of_base/2.0)*length_of_cube_base;
					i2++;
				}
		}
	obstacles[i2][0]=length_of_cube_base/2.0+(15-width_of_base/2.0)*length_of_cube_base;
	obstacles[i2][1]=length_of_cube_base/2.0+((x-1)/2-length_of_base/2.0)*length_of_cube_base;
	i2++;
	obstacles[i2][0]=length_of_cube_base/2.0+((x-1)/2-width_of_base/2.0)*length_of_cube_base;
	obstacles[i2][1]=length_of_cube_base/2.0+(13-length_of_base/2.0)*length_of_cube_base;
	i2++;
	obstacles[i2][0]=length_of_cube_base/2.0+(13-width_of_base/2.0)*length_of_cube_base;
	obstacles[i2][1]=length_of_cube_base/2.0+((x-1)/2-length_of_base/2.0)*length_of_cube_base;
	i2++;
	obstacles[i2][0]=length_of_cube_base/2.0+((x-1)/2-width_of_base/2.0)*length_of_cube_base;
	obstacles[i2][1]=length_of_cube_base/2.0+(15-length_of_base/2.0)*length_of_cube_base;
	i2++;
	no_of_obstacles=i2;
	int k=0;
	for (int i = 2; i < 12;i++)
	{
		for (int l= 15;l< 29;l++)
		{
			if (i!=7&&l!=22)
			{
				heights[i][l]=0;
				empty_cube[k][0]=length_of_cube_base/2.0+(i-width_of_base/2.0)*length_of_cube_base;
				empty_cube[k][1]=length_of_cube_base/2.0+(l-length_of_base/2.0)*length_of_cube_base;
				k++;
			}
			if ((i==7||i==6||i==5)&&(l==22||l==23||l==21))
			{
				heights[i][l]=0;
				empty_cube[k][0]=length_of_cube_base/2.0+(i-width_of_base/2.0)*length_of_cube_base;
				empty_cube[k][1]=length_of_cube_base/2.0+(l-length_of_base/2.0)*length_of_cube_base;
				k++;
			}
		}
	}
	for (int i = 1; i <14;i++)
	{
		heights[i][4]=0;
		empty_cube[k][0]=length_of_cube_base/2.0+(i-width_of_base/2.0)*length_of_cube_base;
		empty_cube[k][1]=length_of_cube_base/2.0+(4-length_of_base/2.0)*length_of_cube_base;
		k++;
		heights[i][8]=0;
		empty_cube[k][0]=length_of_cube_base/2.0+(i-width_of_base/2.0)*length_of_cube_base;
		empty_cube[k][1]=length_of_cube_base/2.0+(8-length_of_base/2.0)*length_of_cube_base;
		k++;
		heights[i][12]=0;
		empty_cube[k][0]=length_of_cube_base/2.0+(i-width_of_base/2.0)*length_of_cube_base;
		empty_cube[k][1]=length_of_cube_base/2.0+(12-length_of_base/2.0)*length_of_cube_base;
		k++;
	}
	heights[20][21]=0;
	empty_cube[k][0]=length_of_cube_base/2.0+(20-width_of_base/2.0)*length_of_cube_base;
	empty_cube[k][1]=length_of_cube_base/2.0+(21-length_of_base/2.0)*length_of_cube_base;
	no_of_pits=k+1;
	wall[0][0]=-300;
	wall[0][1]=length_of_cube_base/2.0+(4-length_of_base/2.0)*length_of_cube_base;
	wall[0][2]=length_of_cube_base*2;
	wall[0][3]=1;
	wall[1][0]=-200;
	wall[1][1]=length_of_cube_base/2.0+(8-length_of_base/2.0)*length_of_cube_base;
	wall[1][2]=length_of_cube_base*2;
	wall[1][3]=1;
	wall[2][0]=-100;
	wall[2][1]=length_of_cube_base/2.0+(12-length_of_base/2.0)*length_of_cube_base;
	wall[2][2]=length_of_cube_base*2;
	wall[2][3]=1;
	no_of_walls=3;
}

float formatAngle(float A)
{
    if(A<0.0f)
        return A+360.0f;
    if(A>=360.0f)
        return A-360.0f;
    return A;
}
float D2R(float A)
{
    return (A*M_PI)/180.0f;
}

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Function is called first on GLFW_PRESS.

	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_D:
				d_pressed=0;
                break;
			case GLFW_KEY_A:
				a_pressed=0;
				break;
			case GLFW_KEY_RIGHT:
				right_pressed=0;
                break;
			case GLFW_KEY_LEFT:
				left_pressed=0;
                break;
			case GLFW_KEY_DOWN:
				down_pressed=0;
                break;
			case GLFW_KEY_UP:
				up_pressed=0;
                break;
            case GLFW_KEY_W:
            	w_pressed=0;
                break;
            case GLFW_KEY_S:
            	s_pressed=0;
                break;
			case GLFW_KEY_F:
            	f_pressed=0;
                break;
            case GLFW_KEY_G:
            	g_pressed=0;
                break;
            case GLFW_KEY_L:
            	l_pressed=0;
                break;
            default:
				break;
		}
	}
	else if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
				quit(window);
				break;
			case GLFW_KEY_D:
				d_pressed=1;
                break;
			case GLFW_KEY_A:
				a_pressed=1;
				break;
			case GLFW_KEY_RIGHT:
				right_pressed=1;
                break;
			case GLFW_KEY_L:
            	l_pressed=1;
                break;
			case GLFW_KEY_LEFT:
				left_pressed=1;
                break;
			case GLFW_KEY_DOWN:
				down_pressed=1;
                break;
			case GLFW_KEY_UP:
				up_pressed=1;
                break;
            case GLFW_KEY_T:
				top_view=1;				
				normal_view=0;
				head_view=0;
				camera_angle=0;
                break;
            case GLFW_KEY_R:
				normal_view=0;
				top_view=0;
				head_view=0;
				camera_angle=0;
                break;
            case GLFW_KEY_N:
            	head_view=0;
            	top_view=0;
				normal_view=1;
                break;
            case GLFW_KEY_H:
				head_view=1;
				normal_view=0;
				top_view=0;
                break;
            case GLFW_KEY_W:
            	w_pressed=1;
                break;
            case GLFW_KEY_S:
            	s_pressed=1;
                break;
            case GLFW_KEY_F:
            	f_pressed=1;
                break;
            case GLFW_KEY_G:
            	g_pressed=1;
                break;
            case GLFW_KEY_SPACE:
            	person_jump=1;
                break;
            default:
				break;
		}
	}
}
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
			quit(window);
			break;
		default:
			break;
	}
}
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			if (action == GLFW_RELEASE)
				//triangle_rot_dir *= -1;
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			if (action == GLFW_RELEASE) {
				//rectangle_rot_dir *= -1;
			}
			break;
		default:
			break;
	}
}
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	/* With Retina display on Mac OS X, GLFW's FramebufferSize
	 is different from WindowSize */
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 9000.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	// glMatrixMode (GL_PROJECTION);
	// glLoadIdentity ();
	//gluPerspective (fov, (GLfloat) width / (GLfloat) height, 0.1, 5000.0f);
	// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	 Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 5000.0f);

	// Ortho projection for 2D views
	//Matrices.projection = glm::ortho(-1*(width*2/3)*1.0f, (width*2/3)*1.0f,-1*(height*2/3)*1.0f, (height*2/3)*1.0f, -1000*1.0f, 5000*1.0f);
}
VAO* createSector(float R,int parts,double clr[6][3])
{
  float diff=360.0f/parts;
  float A1=formatAngle(-diff/2);
  float A2=formatAngle(diff/2);
  GLfloat vertex_buffer_data[]={0.0f,0.0f,0.0f,R*cos(D2R(A1)),R*sin(D2R(A1)),0.0f,R*cos(D2R(A2)),R*sin(D2R(A2)),0.0f};
  GLfloat color_buffer_data[]={clr[0][0],clr[0][1],clr[0][2],clr[1][0],clr[1][1],clr[1][2],clr[2][0],clr[2][1],clr[2][2]};
  return create3DObject(GL_TRIANGLES,3,vertex_buffer_data,color_buffer_data,GL_FILL);
}
VAO* createRectangle1(double length, double breadth, double clr[6][3])
{
  // GL3 accepts only Triangles. Quads are not supported
  const GLfloat vertex_buffer_data [] = {
    0,0,0, // vertex 1
    length,0,0, // vertex 2
    length,breadth,0, // vertex 3

    0, 0,0, // vertex 3
    0, breadth,0, // vertex 4
    length,breadth,0  // vertex 1
  };

  const GLfloat color_buffer_data [] = {
    clr[0][0],clr[0][1],clr[0][2], // color 1
    clr[1][0],clr[1][1],clr[1][2], // color 2
    clr[2][0],clr[2][1],clr[2][2], // color 3

    clr[3][0],clr[3][1],clr[3][2], // color 3
    clr[4][0],clr[4][1],clr[4][2], // color 4
    clr[5][0],clr[5][1],clr[5][2]  // color 1
  };
  return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

VAO* createCube(GLfloat clr[108],double L,double B,double H)
{
	
	GLfloat vertex_buffer_data [] = {
		//Front Face
		-L, -H,  B,
		L, -H,  B,
		L,  H,  B,
		L,  H,  B,
		-L,  H,  B,
		-L, -H,  B,
		//Back Face
		-L, -H, -B,
		L, -H, -B,
		L,  H, -B,
		L,  H, -B,
		-L,  H, -B,
		-L, -H, -B,
		//left Face
		-L, -H,  B,
		-L, -H, -B,
		-L,  H, -B,
		-L,  H, -B,
		-L,  H,  B,
		-L, -H,  B,
		//right Face
		L, -H,  B,
		L, -H, -B,
		L,  H, -B,
		L,  H, -B,
		L,  H,  B,
		L, -H,  B,
		//Top Face
		-L,  H,  B,
		-L,  H, -B,
		L,  H, -B,
		L,  H, -B,
		L,  H,  B,
		-L,  H,  B,
		//Bottom Face
		-L, -H,  B,
		-L, -H, -B,
		L, -H, -B,
		L, -H, -B,
		L, -H,  B,
		-L, -H,  B
	};
	/*const GLfloat color_buffer_data [] = 
	{
	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2], // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2], // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2]  // color 1
  	};*/
  	return create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, clr, GL_FILL);
}

VAO* createCube1(GLfloat clr[108],double L,double B,double H)
{
	
	GLfloat vertex_buffer_data [] = {
		//Front Face
		0, 0,  2*B,
		2*L, 0,  2*B,
		2*L,  2*H,  2*B,
		2*L,  2*H,  2*B,
		0,  2*H, 2*B,
		0, 0,  2*B,
		//Back Face
		0, 0, 0,
		2*L, 0, 0,
		2*L,  2*H, 0,
		2*L,  2*H, 0,
		0,  2*H, 0,
		0, 0, 0,
		//left Face
		0, 0,  2*B,
		0, 0, 0,
		0,  2*H, 0,
		0,  2*H, 0,
		0,  2*H,  2*B,
		0, 0,  2*B,
		//right Face
		2*L, 0,  2*B,
		2*L, 0, 0,
		2*L,  2*H, 0,
		2*L,  2*H, 0,
		2*L,  2*H,  2*B,
		2*L, 0,  2*B,
		//Top Face
		0,  2*H,  2*B,
		0,  2*H, 0,
		2*L,  2*H, 0,
		2*L,  2*H, 0,
		2*L,  2*H,  2*B,
		0,  2*H,  2*B,
		//Bottom Face
		0, 0,  2*B,
		0, 0, 0,
		2*L, 0, 0,
		2*L, 0, 0,
		2*L, 0,  2*B,
		0, 0,  2*B
	};
	/*const GLfloat color_buffer_data [] = 
	{
	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2], // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2], // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2],  // color 1

	    clr[0][0],clr[0][1],clr[0][2], // color 1
	    clr[1][0],clr[1][1],clr[1][2], // color 2
	    clr[2][0],clr[2][1],clr[2][2], // color 3

	    clr[3][0],clr[3][1],clr[3][2], // color 3
	    clr[4][0],clr[4][1],clr[4][2], // color 4
	    clr[5][0],clr[5][1],clr[5][2]  // color 1
  	};*/
  	return create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, clr, GL_FILL);
}


void drawobject(VAO* obj,glm::vec3 trans,float angle,glm::vec3 rotat)
{
	double x,y,z,x1=0,y1=0,z1=0;
	if (top_view==0)
	{
		x=300*cos(camera_angle*M_PI/180);
		z=-300*sin(camera_angle*M_PI/180);
		y=camera_y;
	}
	else
	{
		x=0;//300*cos(camera_angle*M_PI/180);
		y=400;
		z=1;//-300*sin(camera_angle*M_PI/180);
	}
	if (normal_view==1)
	{
		x=person_x+(50+camera_nx);//*camera_x_direction;
		y=person_y+100+camera_ny;
		z=person_z+camera_nz;
		x1=person_x;
		y1=person_y+10+length_of_cube_base;
		z1=person_z;
	}
	if(head_view==1)
	{
		x1=person_x+50*camera_x_direction*-1;
		y1=person_y;
		z1=person_z+50*camera_z_direction*-1;
		x=person_x+(length_of_cube_base)*camera_x_direction*-1;
		y=person_y+length_of_cube_base;
		z=person_z+length_of_cube_base*camera_z_direction*-1;
	}
    Matrices.view = glm::lookAt(glm::vec3(x,y,z), glm::vec3(x1,y1,z1), glm::vec3(0,1,0));
    glm::mat4 VP = Matrices.projection * Matrices.view;
    glm::mat4 MVP;  // MVP = Projection * View * Model
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translatemat = glm::translate(trans);
    glm::mat4 rotatemat = glm::rotate(D2R(formatAngle(angle)), rotat);
    Matrices.model *= (translatemat * rotatemat);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(obj);
}

void drawtext(char *s)
{
	static int fontScale = 1;
	float fontScaleValue = 0.75 + 0.25*sinf(fontScale*M_PI/180.0f);
	fontScale = (fontScale + 1) % 360;
	glm::vec3 fontColor = getRGBfromHue (fontScale);
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
	glm::mat4 MVP;
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateText = glm::translate(glm::vec3(-3,2,0));
	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText * scaleText);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	GL3Font.font->Render(s);
	
}
void draw ()
{
	static double prev_x=0,prev_y=length_of_cube_base*3/2.0+(height_of_base-2)*length_of_cube_base,prev_z=0;
	if (person_x>=(length_of_cube_base*width_of_base+length_of_cube_base)/2||person_x<=-1*((length_of_cube_base*width_of_base+length_of_cube_base)/2))
		fall_state=1;
	if (person_z>=(length_of_cube_base*length_of_base+length_of_cube_base)/2||person_z<=-1*((length_of_cube_base*length_of_base+length_of_cube_base)/2))
		fall_state=1;
	if(d_pressed==1)
	{
		camera_nz+=10;
		camera_angle+=camera_speed;
	}
	if(a_pressed==1)
	{
		camera_nz-=10;
		camera_angle-=camera_speed;
	}
	if(right_pressed==1)
	{
		camera_z_direction=1;
		person_z-=person_shift;
		camera_x_direction=0;
		person_hand_angle+=hand_angle_speed;
	}
	if(left_pressed==1)
	{
		camera_x_direction=0;
		camera_z_direction=-1;
		person_z+=person_shift;
		person_hand_angle+=hand_angle_speed;
	}
	if(down_pressed==1)
	{
		camera_z_direction=0;
		camera_x_direction=-1;
		person_x+=person_shift;
		person_hand_angle+=hand_angle_speed;
	}
	if(up_pressed==1)
	{
		camera_z_direction=0;
		camera_x_direction=1;
		person_x-=person_shift;
		person_hand_angle+=hand_angle_speed;
	}
	if (person_hand_angle>30)
		hand_angle_speed=-5;
	else if (person_hand_angle<-30)
		hand_angle_speed=5;
	if(w_pressed==1)
	{
		camera_y+=10;
		camera_nx+=10;
	}
	if (s_pressed==1)
	{
		camera_nx-=10;
		camera_y-=10;
	}
	if (g_pressed==1)
		camera_ny+=10;
	if (f_pressed==1)
		camera_ny-=10;
	if (l_pressed==1)
	{
		person_x=(length_of_cube_base*length_of_base-3*length_of_cube_base)/2;
		person_z=(length_of_cube_base*width_of_base-3*length_of_cube_base)/2;
		person_y=length_of_cube_base*3/2.0+(height_of_base-2)*length_of_cube_base;
		key=2;
		fall_state=0;
		gameover=0;
	}
	if(person_jump==1)
	{
		if (jump_direction==1)
			jump_speed+=1;
		if (jump_speed>length_of_cube_base||jump_direction==-1)
		{
			jump_speed-=1;
			jump_direction=-1;
		}
		if (jump_speed==0)
		{
			person_jump=0;
			jump_direction=1;
		}
	}
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (programID);
	glUseProgram(textureProgramID);
	glUseProgram(fontProgramID);
	//drawtext("hello");
	//for (int i = 0; i < 10; ++i)
	if(key>=1)
	{
		int x=length_of_base;
		heights[(x-2)/2][15]=height_of_base;
	}
	if (key>=2)
	{
		int x=length_of_base;
		heights[13][(x-2)/2]=height_of_base;
	}
	if (key>=3)
	{
		int x=length_of_base;
		heights[(x-2)/2][13]=height_of_base;
	}
	glUseProgram (programID);
	for (int i2 = 0; i2 <length_of_base;i2++)
		for (int i = 0; i <width_of_base;i++)
		{
			for (int i1 = 0; i1<heights[i2][i];i1++)
				drawobject(cube,glm::vec3(
					length_of_cube_base/2.0+(i2-length_of_base/2.0)*length_of_cube_base,
					length_of_cube_base/2.0+(i1-1)*length_of_cube_base,
					length_of_cube_base/2.0+((i-width_of_base/2.0)*length_of_cube_base)),0,glm::vec3(0,0,1));
			if (heights[i2][i]==0)
				for (int i1 = 0; i1 < height_of_base-1;i1++)
					drawobject(water,glm::vec3(
						length_of_cube_base/2.0+(i2-length_of_base/2.0)*length_of_cube_base,
						length_of_cube_base/2.0+(i1-1)*length_of_cube_base,
						length_of_cube_base/2.0+((i-width_of_base/2.0)*length_of_cube_base)),0,glm::vec3(0,0,1));
		}
		//drawobject(cube,glm::vec3(150+2*length_of_cube_base,440,0),0,glm::vec3(0,0,1));
	// cout<<person_x<<"	"<<person_z<<"	"<<empty_cube[0][0]<<"	"<<empty_cube[0][1]<<endl;
	double var1,var2;
	for (int i = 0; i < no_of_pits;i++)
	{
		var1=person_x-empty_cube[i][0];//-length_of_cube_base/2;
		var2=person_z-empty_cube[i][1];//+length_of_cube_base/2;
		if (person_y+jump_speed==length_of_cube_base*3/2.0+(height_of_base-2)*length_of_cube_base)
			if (var1<length_of_cube_base/2 && var1>-1*length_of_cube_base/2 && var2<length_of_cube_base/2 && var2>-1*length_of_cube_base/2)
			{
				cout<<i<<"	"<<person_x<<"	"<<person_z<<"	"<<empty_cube[i][0]<<"	"<<empty_cube[i][1]<<endl;
				fall_state=1;
			}
		// if (i==0)
		// 	cout<<i<<"	"<<person_x<<"	"<<person_z<<"	"<<empty_cube[i][0]<<"	"<<empty_cube[i][1]<<var1<<"	"<<var2<<endl;
		// if (var1>-6&&var1<6&&var2>=-6&&var2<6)
		// 	fall_state=-1;
	}
	for (int i = 0; i < no_of_obstacles-key;i++)
	{
		var1=person_x-obstacles[i][0];//-length_of_cube_base/2;
	 	var2=person_z-obstacles[i][1];//+length_of_cube_base/2;
	 	if (var1<5*length_of_cube_base/6.0 && var1>-5*length_of_cube_base/6.0 && var2<5*length_of_cube_base/6.0 && var2>-5*length_of_cube_base/6.0)
	 	{
	 		person_z=prev_z;
	 		person_y=prev_y;
	 		person_x=prev_x;
	 	}
	}
	//	cout<<person_x<<endl;
	if (person_x==287.5 && person_z==162.5)
		key=1;
	if (person_x==-337.5 && person_z==337.5)
		key=2;
	
	if (fall_state==1)
	{
		person_z=prev_z;
	 	person_y=prev_y;
	 	person_x=prev_x;
		person_y-=1;
	}
	if (gameover==0)
	{
		if (camera_x_direction==1||camera_x_direction==-1)
		{
			drawobject(person_leg,glm::vec3(person_x,person_y+jump_speed+10,person_z+6),person_hand_angle,glm::vec3(0,0,1));
			drawobject(person_leg,glm::vec3(person_x,person_y+jump_speed+10,person_z-6),-1*person_hand_angle,glm::vec3(0,0,1));
		}
		if (camera_z_direction==1||camera_z_direction==-1)
		{
			drawobject(person_leg,glm::vec3(person_x+6,person_y+jump_speed+10,person_z),person_hand_angle,glm::vec3(1,0,0));
			drawobject(person_leg,glm::vec3(person_x-6,person_y+jump_speed+10,person_z),-1*person_hand_angle,glm::vec3(1,0,0));
		}
		drawobject(person_body,glm::vec3(person_x,person_y+jump_speed+12+length_of_cube_base/3,person_z),0,glm::vec3(0,0,1));
		for (int i = 0; i < 360; ++i)
			drawobject(person_neck,glm::vec3(person_x,person_y+jump_speed+12+length_of_cube_base,person_z),i,glm::vec3(0,1,0));
		if (camera_x_direction==1||camera_x_direction==-1)
		{
			drawobject(person_head,glm::vec3(person_x,person_y+jump_speed+12+7+length_of_cube_base,person_z),0,glm::vec3(0,1,0));
			drawobject(person_hand,glm::vec3(person_x,person_y+jump_speed+30,person_z-18),person_hand_angle,glm::vec3(0,0,1));
			drawobject(person_hand,glm::vec3(person_x,person_y+jump_speed+30,person_z+12),-1*person_hand_angle,glm::vec3(0,0,1));
			var1=-10;
			if (camera_x_direction==-1)
				var1=10;
			for (int i = 0; i < 360;i++)
				drawobject(person_eye,glm::vec3(person_x+var1,person_y+jump_speed+12+6+length_of_cube_base,person_z-8),i,glm::vec3(1,0,0));
			for (int i = 0; i < 360;i++)
				drawobject(person_eye,glm::vec3(person_x+var1,person_y+jump_speed+12+6+length_of_cube_base,person_z+8),i,glm::vec3(1,0,0));
			var1=2;
			if (camera_x_direction==-1)
				var1=-2;
			drawobject(person_hair,glm::vec3(person_x+var1,person_y+jump_speed+12+7+6+length_of_cube_base,person_z),0,glm::vec3(0,1,0));
		}
		else
		{
			drawobject(person_head,glm::vec3(person_x,person_y+jump_speed+12+7+length_of_cube_base,person_z),90,glm::vec3(0,1,0));
			drawobject(person_hand,glm::vec3(person_x+12,person_y+jump_speed+30,person_z),person_hand_angle,glm::vec3(1,0,0));
			drawobject(person_hand,glm::vec3(person_x-18,person_y+jump_speed+30,person_z),-1*person_hand_angle,glm::vec3(1,0,0));
			var1=-10;
			if (camera_z_direction==-1)
				var1=10;
			for (int i = 0; i < 360;i++)
				drawobject(person_eye,glm::vec3(person_x-8,person_y+jump_speed+12+6+length_of_cube_base,person_z+var1),i,glm::vec3(0,0,1));
			for (int i = 0; i < 360;i++)
				drawobject(person_eye,glm::vec3(person_x+8,person_y+jump_speed+12+6+length_of_cube_base,person_z+var1),i,glm::vec3(0,0,1));
			var1=2;
			if (camera_z_direction==-1)
				var1=-2;
			drawobject(person_hair,glm::vec3(person_x,person_y+jump_speed+12+7+6+length_of_cube_base,person_z+var1),90,glm::vec3(0,1,0));
		}
	}
	for (int i = 0; i < no_of_walls;i++)
	{
		var1=person_x-wall[i][0];
		var2=person_z-wall[i][1];
		if (var1<0)
			var1*=-1;
		//var1-=(length_of_cube_base/3);//+length_of_cube_base/2);
		if (var2<0)
			var2*=-1;
		//var2-=(length_of_cube_base/3);//+length_of_cube_base/2);
		if ((var1>0&&var1<wall[i][2]+length_of_cube_base/3)&&(var2>0&&var2<length_of_cube_base))
		{
			cout<<"hello"<<endl;
			person_z=prev_z;
	 		person_y=prev_y;
	 		person_x=prev_x;
	 		if (var2>0&&var2<length_of_cube_base)
		 		wall[i][3]*=-1;		
		 	gameover=1;
		}
		drawobject(walls,glm::vec3(wall[i][0],length_of_cube_base*3/2.0+(height_of_base-2)*length_of_cube_base,wall[i][1]),0,glm::vec3(0,0,1));
		if (wall[i][3]==1)
			wall[i][0]+=5;
		else 
			wall[i][0]-=5;
		if (wall[i][0]+wall[i][2]+length_of_cube_base>0)
			wall[i][3]*=-1;
		else if (wall[i][0]<-300)
			wall[i][3]*=-1;
	}
	prev_x=person_x;
	prev_z=person_z;
	prev_y=person_y;
}
GLFWwindow* initGLFW (int width, int height)
{
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );

	/* --- register callbacks with GLFW --- */

	/* Register function to handle window resizes */
	/* With Retina display on Mac OS X GLFW's FramebufferSize
	 is different from WindowSize */
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);

	/* Register function to handle window close */
	glfwSetWindowCloseCallback(window, quit);

	/* Register function to handle keyboard input */
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

	/* Register function to handle mouse click */
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

	return window;
}
void initGL (GLFWwindow* window, int width, int height)
{
	intialize_base();
	glActiveTexture(GL_TEXTURE0);
	GLuint textureID = createTexture("beach2.png");
	if(textureID == 0 )
		cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;
	textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
	Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");
	programID = LoadShaders( "Sample_GL3.vert", "Sample_GL3.frag" );
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");
	reshapeWindow (window, width, height);
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);
	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
	const char* fontfile = "arial.ttf";
	GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering
	if(GL3Font.font->Error())
	{
		cout << "Error: Could not load font `" << fontfile << "'" << endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	GLfloat clr[108];
	for (int i = 0; i < 36;i++)
	{
		clr[3*i]=0.2;
		clr[3*i+1]=0.098;
		clr[3*i+2]=0;
		if (i<30&&i>=24)
		{
			clr[3*i]=0.474;
			clr[3*i+1]=1;
			clr[3*i+2]=0.301;
		}
		if (i<12&&i>=0)
		{
			clr[3*i]=0.301;
			clr[3*i+1]=0.152;
			clr[3*i+2]=0;
		}
	}	
	cube=createCube(clr,length_of_cube_base/2,length_of_cube_base/2,length_of_cube_base/2);
	for (int i = 0; i <108;i++)
	{
		clr[i]=0;
	}
	person_body=createCube(clr,length_of_cube_base/2,length_of_cube_base/2,length_of_cube_base/2);
	person_leg=createCube1(clr,4,4,-12);
	person_neck=createCube(clr,3,3,7);
	for (int i = 0; i <108;i++)
		clr[i]=0.5;
	person_head=createCube(clr,10,18,6);
	for (int i = 0; i <108;i++)
		clr[i]=0.7;
	person_hair=createCube(clr,11,22,4);
	for (int i = 0; i <108;i++)
		clr[i]=0;
	person_eye=createCube(clr,2,2,2);
	for (int i = 0; i <108;i++)
		clr[i]=1;
	person_hand=createCube1(clr,3,3,-10);
	for (int i = 0; i <36;i++)
	{
		clr[3*i]=0.501;
		clr[3*i+1]=1.0;
		clr[3*i+2]=0.831;
	}
	walls=createCube(clr,length_of_cube_base*2,length_of_cube_base/2,length_of_cube_base/2);
	water=createCube(clr,length_of_cube_base/2,length_of_cube_base/2,(length_of_cube_base*5)/6);
	double clr1[6][3];
	for (int i = 0; i < 6;i++)
	{
		for (int i1 = 0; i1 < 3; i1++)
		{
			clr1[i][i1]=0;
		}
	}
	fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
	GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
	fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
	fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
	fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
	GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
	GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");
	GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
	GL3Font.font->FaceSize(1);
	GL3Font.font->Depth(0);
	GL3Font.font->Outset(0, 0);
	GL3Font.font->CharMap(ft_encoding_unicode);
	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

	double last_update_time = glfwGetTime(), current_time;

	/* Draw in loop */
	while (!glfwWindowShouldClose(window)) {
		//cout<<person_x<<"	drawrawdraw";

		// OpenGL Draw commands
		draw();

		// Swap Frame Buffer in double buffering
		glfwSwapBuffers(window);

		// Poll for Keyboard and mouse events
		glfwPollEvents();

		// Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
		current_time = glfwGetTime(); // Time in seconds
		if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
			// do something every 0.5 seconds ..
			last_update_time = current_time;
		}
		if (person_y<=0)
			gameover=1;
		//cout<<person_x<<"	"<<person_z<<endl;
	}

	glfwTerminate();
	exit(EXIT_SUCCESS);
}
