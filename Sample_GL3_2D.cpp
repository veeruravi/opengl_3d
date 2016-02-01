#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
  glm::mat4 projection;
  glm::mat4 model;
  glm::mat4 view;
  GLuint MatrixID;
} Matrices;

GLuint programID;

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
  printf("Compiling shader : %s\n", vertex_file_path);
  char const * VertexSourcePointer = VertexShaderCode.c_str();
  glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
  glCompileShader(VertexShaderID);

  // Check Vertex Shader
  glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> VertexShaderErrorMessage(InfoLogLength);
  glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
  fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

  // Compile Fragment Shader
  printf("Compiling shader : %s\n", fragment_file_path);
  char const * FragmentSourcePointer = FragmentShaderCode.c_str();
  glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
  glCompileShader(FragmentShaderID);

  // Check Fragment Shader
  glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
  glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
  fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

  // Link the program
  fprintf(stdout, "Linking program\n");
  GLuint ProgramID = glCreateProgram();
  glAttachShader(ProgramID, VertexShaderID);
  glAttachShader(ProgramID, FragmentShaderID);
  glLinkProgram(ProgramID);

  // Check the program
  glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
  glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
  glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
  fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

  glDeleteShader(VertexShaderID);
  glDeleteShader(FragmentShaderID);

  return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

float sqr(float x)
{
  return x*x;
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

/**************************
 * Customizable functions *
 **************************/
float time_travel = 0;
float walls_position[300][2];
int array_postion = 0;
float initial_velocity =20;
float initial_velocity1 = initial_velocity;
float angle_thrown = M_PI/3;
float vertical_translation = -2;
float horizontal_translation = -3;
float final_velocity=0;
float final_velocity1=0;
float horizontal_translation1=0.8,vertical_translation1=-1.7,initial_velocity2=0,initial_velocity3=0,final_velocity2=0,final_velocity3,angle_thrown1,time_travel1=0;
float horizontal_translation2=-1.8,vertical_translation2=1.7,initial_velocity4=0,initial_velocity5=0,final_velocity4=0,final_velocity5,angle_thrown2,time_travel2=0;
float object_collision=0,object_collision1=0;
float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
float coefficient_of_elasticity = 0.8;
double xmousePos,ymousePos;
float tanker_angle= 0;
bool shoot =0;
float distance1;
float distance3;
float power=40;
float power1=0;
float additional_angle=0;
bool iscollide = 0,iscollide1=0;
int timetonextcollide = 0;
int flagfly=0,flagfly1=0;
int score=0;
int noofcollisions = 0,noofcollisions1=0;
float forerror=0;
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */ 
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_C:
                rectangle_rot_status = !rectangle_rot_status;
                break;
            case GLFW_KEY_P:
                triangle_rot_status = !triangle_rot_status;
                break;
            case GLFW_KEY_X:
                // do something ..
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
            default:
                break;
        }
    }
}

/* Executed floator character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
  switch (key) {
    case 'Q':
    case 'q':
            quit(window);
            break;
    case 'f':
            if(distance3+2+power1<22)
              power1+=1;
            break;
    case 's':
            if(distance3+2+power1>1)
              power1-=1;
            break;
    case ' ':
            shoot =1;
            angle_thrown = tanker_angle - M_PI/6;
            initial_velocity = power*cos(angle_thrown);
            initial_velocity1 = power*sin(angle_thrown);
            horizontal_translation = -3 - 0.1*cos(angle_thrown);
            vertical_translation = -2  - 0.65*sin(angle_thrown);
            time_travel = 0;
            break;
    case 'a':
            additional_angle +=M_PI/18;
            break;
    case 'b':
            additional_angle -=M_PI/18;
            break;
    default:
      break;
  }
}
float zoomX=8,zoomY=8;

void cbfun (GLFWwindow* window, double x,double y)
{
  cout << x << "<<<<" << y<< endl;
    if(y==-1)
    {
        zoomX+=1;
        zoomY+=1;
        cout << "::::::::" << endl;      
    }
    if(y==1)
    {
      zoomX-=1;
      zoomY-=1;
    }
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    if(button==3)
    {
        zoomX+=1;
        zoomY+=1;
        cout << "::::::::" << endl;      
    }
    if(button==4)
    {
      zoomX-=1;
      zoomY-=1;
    }
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS)
            {
              shoot =1;
              angle_thrown = tanker_angle - M_PI/6;
              initial_velocity = power*cos(angle_thrown);
              initial_velocity1 = power*sin(angle_thrown);
              horizontal_translation = -3 - 0.1*cos(angle_thrown);
              vertical_translation = -2  - 0.65*sin(angle_thrown);
              time_travel = 0;

              //time_travel=0;
            }
            if (action == GLFW_RELEASE)
                triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

  GLfloat fov = 90.0f;

  // sets the viewport of openGL renderer
  glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

  // set the projection matrix as perspective
  /* glMatrixMode (GL_PROJECTION);
     glLoadIdentity ();
     gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
  // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-zoomX/2.0f, zoomX/2.0f, -zoomY/2.0f, zoomY/2.0f, 0.1f, 500.0f);
}

VAO *triangle, *rectangle, *powerboxes, *triangle1, *scoresource, *tankercircle, *pig;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  static const GLfloat vertex_buffer_data [] = {
    0, 0,0, // vertex 0 
    0,0.1,0, // vertex 1
    0.1,0,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,1,1, // color 0
    1,1,1, // color 1
    1,1,1, // color 2
  };
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

void createpig ()
{
  static const GLfloat vertex_buffer_data [] = {
    0, 0,0, // vertex 0 
    0,0.05,0, // vertex 1
    0.05,0,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    0,0,0, // color 0
    0,0,0, // color 1
    0,0,0, // color 2
  };
  pig = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

void createtankercircle ()
{
  static const GLfloat vertex_buffer_data [] = {
    0, 0,0, // vertex 0 
    0,0.5,0, // vertex 1
    0.5,0,0, // vertex 2
  };
  static const GLfloat color_buffer_data [] = {
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,
  };
  tankercircle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}
void createTriangle1 ()
{
  static const GLfloat vertex_buffer_data [] = {
    0, 0,0, // vertex 0
    0,0.2,0, // vertex 1
    0.2,0,0, // vertex 2
  };
  static const GLfloat color_buffer_data [] = {
    1,1,0, // color 0
    1,1,0, // color 1
    1,1,0, // color 2
  };
  triangle1 = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}
void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.1,-0.6,0, // vertex 1
    0.1,-0.6,0, // vertex 2
    0.1, 0.6,0, // vertex 3

    0.1, 0.6,0, // vertex 3
    -0.1, 0.6,0, // vertex 4
    -0.1,-0.6,0  // vertex 1
  };
  static const GLfloat color_buffer_data [] = {
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,

    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,  
  };
  rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createPowerBoxes()
{
  static const GLfloat vertex_buffer_data [] = {
    -0.1,-0.1,0, // vertex 1
    0.1,-0.1,0, // vertex 2
    0.1, 0.1,0, // vertex 3

    0.1, 0.1,0, // vertex 3
    -0.1, 0.1,0, // vertex 4
    -0.1,-0.1,0  // vertex 1
  };
  static const GLfloat color_buffer_data [] = {
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,

    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,  
  };
  powerboxes = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}


void scorerectangle()
{
  static const GLfloat vertex_buffer_data [] = {
    -0.05,-0.05,0, // vertex 1
    0.05,-0.05,0, // vertex 2
    0.05, 0.05,0, // vertex 3

    0.05, 0.05,0, // vertex 3
    -0.05, 0.05,0, // vertex 4
    -0.05,-0.05,0  // vertex 1
  };
  static const GLfloat color_buffer_data [] = {
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,

    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,
    152/255.0, 205/255.0, 152/255.0,  
  };
  scoresource = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

void drawing_walls(float x_centre,float y_centre,VAO* obj)
{
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane 
  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP;  // MVP = Projection * View * Model
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangle = glm::translate (glm::vec3(x_centre,y_centre, 0));        // glTranslatef
  Matrices.model *= translateRectangle;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(obj); 
}
void wall_collision(float x_centre,float y_centre,VAO* obj)
{
  drawing_walls(x_centre,y_centre,obj);
  for(int ii=0;ii<=5;ii+=1)
  {
    walls_position[array_postion][0]=x_centre;
    walls_position[array_postion][1]=-0.5+0.2*ii+y_centre;
    array_postion++;
  }
  for(int iii=0;iii<array_postion;iii++)
  {
    distance1 = sqrt((sqr(walls_position[iii][0]-horizontal_translation)) + (sqr(walls_position[iii][1]-vertical_translation)));
    if(distance1<=0.2)
    {
      initial_velocity *= -1*0.8;
      if(horizontal_translation<x_centre && ((y_centre-0.55)<vertical_translation) &&  vertical_translation<(y_centre+0.55))
      {
        horizontal_translation = -0.2+x_centre;
        // cout << "2" << endl;
      }
      else if(horizontal_translation>x_centre && ((x_centre-0.55)<vertical_translation<(x_centre+0.55)))
      {
        horizontal_translation = x_centre + 0.2;
        // cout << "3" << endl;
      }
      else
      {
        
        if(horizontal_translation<3)
        {
          initial_velocity1 *= 0.5;
          vertical_translation = y_centre + 0.9;
          initial_velocity *= -1*coefficient_of_elasticity;
        }
        // cout << initial_velocity << " " <<  initial_velocity1 << endl;
        time_travel = 0;
        // cout << "1" << endl; 
               // cout << y_centre-0.6  << " " <<  vertical_translation << " " << y_centre+0.6 << endl;
        // cout << horizontal_translation << " " << x_centre << endl;
      }
    } 
  }
}
float ar[8],br[10];
void bird_collision(float horizontal_translation,float vertical_translation,float horizontal_translation1,float vertical_translation1,float initial_velocity,float initial_velocity1,float initial_velocity2,float initial_velocity3,float time_travel,float time_travel1)
{
  // float distance4 = sqrt((sqr(walls_position[iii][0]-horizontal_translation1)) + (sqr(walls_position[iii][1]-vertical_translation1)));
  if((horizontal_translation>0.4 && horizontal_translation<0.9) && (vertical_translation<-1.9))
  {
    initial_velocity *= -1*0.8;
    horizontal_translation = 0.4;
  }
  if((horizontal_translation1>0.4 && horizontal_translation1<0.9) && (vertical_translation1<-1.8))
  {
    initial_velocity2 *= -1*0.8;
    horizontal_translation1 = 0.3;
  }
  if((horizontal_translation>0.9 && horizontal_translation<1.2) && (vertical_translation<-1.9))
  {
    initial_velocity *= -1*0.8;
    horizontal_translation = 1.2;
  }
  if((horizontal_translation1>0.9 && horizontal_translation1<1.3) && (vertical_translation1<-1.8))
  {
    initial_velocity2 *= -1*0.8;
    horizontal_translation1 = 1.3;
  }
  if(vertical_translation<-1.6 && (horizontal_translation>0.5 && horizontal_translation<1.1))
  {
    time_travel = 0;
    // cout<<initial_velocity1<<" "<<final_velocity1<<endl;
    initial_velocity1 *= coefficient_of_elasticity;
    vertical_translation = -1.7;
    if(initial_velocity1<0)
    {
      initial_velocity1 *= -1;
    }
  }
  br[0]=horizontal_translation;
  br[1]=vertical_translation;
  br[2]=horizontal_translation1;
  br[3]=vertical_translation1;
  br[4]=initial_velocity;
  br[5]=initial_velocity1;
  br[6]=initial_velocity2;
  br[7]=initial_velocity3;
  br[8]=time_travel;
  br[9]=time_travel1;
}

void drawCircle(VAO* obj,float horizontal_translation,float vertical_translation)
{
 for(int i=0;i<360;i++)
  {
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP;  // MVP = Projection * View * Model
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateTriangle = glm::translate (glm::vec3(horizontal_translation,vertical_translation, 0.0f)); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(i*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  Matrices.model *= triangleTransform; 
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(obj);  
  }
}

void bullet(VAO *obj,float horizontal_translation,float vertical_translation,float time_travel,float angle_thrown,int flagg,float initial_velocity,float initial_velocity1,float final_velocity,float final_velocity1)
{
  // cout << initial_velocity << " " << initial_velocity1 << endl;
  // 
  drawCircle(obj,horizontal_translation,vertical_translation);
  horizontal_translation += initial_velocity*cos(angle_thrown)*0.005;
  vertical_translation += initial_velocity1*sin(angle_thrown)*0.005 - (time_travel*time_travel);
  final_velocity = sqrt(sqr(initial_velocity) - ((horizontal_translation+3)*8*0.001));
  final_velocity1 = initial_velocity1 - time_travel*89;
  // cout << final_velocity1 << " " << initial_velocity1 << " " <<  vertical_translation << endl;
  if(vertical_translation<-3.7)
  {
    time_travel = 0;
    // cout<<initial_velocity1<<" "<<final_velocity1<<endl;
    if(flagg==1)
      initial_velocity1 *= coefficient_of_elasticity;
    vertical_translation = -3.7;
    if(initial_velocity1<0)
    {
      initial_velocity1 *= -1;
    }
    // cout << initial_velocity1 << "  " << angle_thrown << endl;
  }
  // if(vertical_translation>1.8 && (horizontal_translation>-1.4 && horizontal_translation<0.4))
  // {
  //   initial_velocity1 = initial_velocity1/8;
  //   // time_travel = 0;
  //   // time_travel=0; 
  //   vertical_translation=1.75;
  // }
  if(horizontal_translation>3.7)
  {
    initial_velocity *= -1;
    horizontal_translation = 3.7;
  }
  if(flagg==1 || final_velocity>2 || final_velocity1<-2)
    time_travel += 0.01;
  // cout<< time_travel << endl;
  if((horizontal_translation>4 || horizontal_translation<-4) && (flagg==1))
  {
    shoot=0;
    horizontal_translation = -3 - 0.1*cos(angle_thrown);
    vertical_translation = -2  - 0.65*sin(angle_thrown);
    time_travel = 0;
  }
  ar[0]=horizontal_translation;
  ar[1]=vertical_translation;
  ar[2]=time_travel;
  ar[3]=angle_thrown;
  ar[4]=initial_velocity;
  ar[5]=initial_velocity1;
  ar[6]=final_velocity;
  ar[7]=final_velocity1;
}

void specialbullet(VAO *obj,float horizontal_translation,float vertical_translation,float time_travel,float angle_thrown,int flagg,float initial_velocity,float initial_velocity1,float final_velocity,float final_velocity1,int iscollide,int flagfly)
{
  drawCircle(obj,horizontal_translation,vertical_translation);
  horizontal_translation += initial_velocity*cos(angle_thrown)*0.005;
  vertical_translation += initial_velocity1*sin(angle_thrown)*0.005 - (time_travel*time_travel);
  // cout << vertical_translation << " " << time_travel<< endl;
  final_velocity1 = initial_velocity1 - time_travel*89;
  final_velocity = sqrt(sqr(initial_velocity) - ((horizontal_translation+3)*8*0.001));
  if(horizontal_translation>3.6)
  {
    initial_velocity *= -1;
    horizontal_translation = 3.6 ;
    // cout << "???????" << endl;
  }
  if(vertical_translation<-3.6)
  {
    time_travel = 0;
    initial_velocity1 = -final_velocity1*coefficient_of_elasticity;
    initial_velocity1 *=0.8;
    vertical_translation = -3.5;
    if(initial_velocity1<0)
    {
      initial_velocity *= -1;
    }
  }
  if(iscollide==1 || flagfly==1)
  {
    // cout << iscollide <<"$$$$$$$$$" << flagfly <<  endl;
    time_travel+=0.01;
  }
  forerror=time_travel;
  // cout << initial_velocity <<  " " << angle_thrown << " " << horizontal_translation <<  endl;
  br[0]=horizontal_translation;
  br[1]=vertical_translation;
  br[2]=time_travel;
  br[4]=initial_velocity;
  br[5]=initial_velocity1;
  br[6]=final_velocity;
  br[7]=final_velocity1;
  // cout << " )))) " << br[2] << endl;
}

void draw ()
{
  
  array_postion = 0;
  // cout << horizontal_translation1 << " " << vertical_translation1 << endl;
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);
Matrices.projection = glm::ortho(-zoomX/2.0f, zoomX/2.0f, -zoomY/2.0f, zoomY/2.0f, 0.1f, 500.0f);
  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  // drawCircle(triangle,horizontal_translation1,vertical_translation1);
    drawCircle(tankercircle,-3,-2.6);
  if(shoot==1)
  {
    bullet(triangle,horizontal_translation,vertical_translation,time_travel,angle_thrown,1,initial_velocity,initial_velocity1,final_velocity,final_velocity1);
    horizontal_translation=ar[0];
    vertical_translation=ar[1];
    time_travel=ar[2];
    angle_thrown=ar[3];
    initial_velocity = ar[4];
    initial_velocity1= ar[5];
    final_velocity=ar[6];
    final_velocity1=ar[7];
    // initial_velocity = 40*cos(angle_thrown);
    // initial_velocity1 = 40*sin(angle_thrown);
  }
  
  if(iscollide==1)
  {
    flagfly = 1;
    timetonextcollide++;
  }  
  if(timetonextcollide==100)
  {
    iscollide=0;
    iscollide1=0;
    timetonextcollide=0;
  }
  if(iscollide1==1)
  {
    flagfly1 = 1;
    timetonextcollide++;
  }  
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane 
  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP;  // MVP = Projection * View * Model
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangle = glm::translate (glm::vec3(-3, -2, 0));        // glTranslatef
  glm::mat4 translateRectangle1 = glm::translate (glm::vec3(0, -0.6, 0)); 
  glm::mat4 translateRectangle2 = glm::translate (glm::vec3(0, 0.6, 0)); 
  glm::mat4 rotateRectangle = glm::rotate((float)(-90+ tanker_angle), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  // translateRectangle = glm::translate (glm::vec3(-3, -2, 0));        // glTranslatef
  Matrices.model *= (translateRectangle * translateRectangle1 * rotateRectangle * translateRectangle2); 
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(rectangle);
  // cout << horizontal_translation2 << " " << vertical_translation2 << " " << time_travel2<< endl;
  object_collision = sqrt(sqr(horizontal_translation- horizontal_translation1)+ sqr(vertical_translation- vertical_translation1));
  if(noofcollisions>2)
  {
    object_collision=100;
  }
  // cout << object_collision << " &"<< endl;
  if(object_collision<=0.4 && iscollide==0)
  {
    initial_velocity2 = (2*initial_velocity)/3.5;
    initial_velocity3 = (2*initial_velocity1)/3.5;
    // cout << initial_velocity << " " << initial_velocity1 << " " << final_velocity << " " << final_velocity1 << endl;
    // initial_velocity2 = (2.0*initial_velocity)/3.0 + (final_velocity)/3.0;
    // initial_velocity3 = (2.0*initial_velocity1)/3.0 + (final_velocity1)/3.0;
    // initial_velocity *= 1/2.0;
    // cout << final_velocity2 << " " << final_velocity3 << endl;
    initial_velocity = -(initial_velocity)/1.5;
     // + (4.0*final_velocity2)/3.0;
    initial_velocity1 = -(initial_velocity1)/1.5 ;
    // + (4.0*final_velocity3)/3.0;
    time_travel=0;
    if(score<9)
    {
      score++;
      // cout << "[[[" << score << endl;
    }
    noofcollisions++;
    // cout << initial_velocity << " " << initial_velocity1 << endl;

    // cout << initial_velocity3 << endl;
    angle_thrown1 = M_PI/4;
  //   angle_thrown1 = angle_thrown;
  //   initial_velocity *= -1;
    // cout << "########" << endl;
    iscollide =1;
    // horizontal_translation -=0.4;
  }

  // Increment angles
  // wall_collision(1,-3.2,rectangle);
  // wall_collision(0.8,-3.2,rectangle);
  // wall_collision(0.6,-3.2,rectangle);
  if(shoot==1)
    drawCircle(triangle,horizontal_translation,vertical_translation);
  // cout << horizontal_translation1 << vertical_translation1 << time_travel1 << angle_thrown1 << endl;
  if(noofcollisions<=2)
  {
    specialbullet(triangle1,horizontal_translation1,vertical_translation1,time_travel1,angle_thrown1,0,initial_velocity2,initial_velocity3,final_velocity2,final_velocity3,iscollide,flagfly);
    horizontal_translation1=br[0];
    vertical_translation1=br[1];
    time_travel1=br[2];
    // angle_thrown1=br[3];
    initial_velocity2=br[4];
    initial_velocity3=br[5];
    final_velocity2=br[6];
    final_velocity3=br[7];
}

  for(int iiii=0;iiii<10;iiii++)
  {
    drawing_walls(0.6,-3.8+0.2*iiii,powerboxes);
    drawing_walls(0.8,-3.8+0.2*iiii,powerboxes);
    drawing_walls(1,-3.8+0.2*iiii,powerboxes);
  }
  // wall_collision(1,0);
  bird_collision(horizontal_translation,vertical_translation,horizontal_translation1,vertical_translation1,initial_velocity,initial_velocity1,initial_velocity2,initial_velocity3,time_travel,time_travel1);
  horizontal_translation=br[0];
  vertical_translation=br[1];
  horizontal_translation1=br[2];
  vertical_translation1=br[3];
  initial_velocity=br[4];
  initial_velocity1=br[5];
  initial_velocity2=br[6];
  initial_velocity3=br[7];
  time_travel=br[8];
  time_travel1=br[9];

  for(int iiii=0;iiii<40;iiii++)
  {  
    drawing_walls(3.9,3.9-iiii*0.2,powerboxes);
  }
  int qwerty=distance3+2+power1;
  for(int iiii=0;iiii<(qwerty);iiii++)
  {
    drawing_walls(-3.8+0.3*iiii,3.7,powerboxes);
  }
  // for(int iiii=0;iiii<6;iiii++)
  // {
  //   drawing_walls(-1.4+0.2*iiii,2,powerboxes);
  // }
  for(int iiii=0;iiii<39;iiii++)
  {
    drawing_walls(-3.9+0.2*iiii,-3.9,powerboxes);
  }

//////////////////////////////////////////
// object_collision1 = sqrt(sqr(horizontal_translation- horizontal_translation2)+ sqr(vertical_translation- vertical_translation2));
//   if(noofcollisions1>1)
//   {
//     object_collision1=100;
//   }
//   // cout << object_collision << " &"<< endl;
//   if(object_collision1<=0.4 && iscollide1==0)
//   {
//     initial_velocity4 = (2*initial_velocity)/3.5;
//     initial_velocity5 = (2*initial_velocity1)/3.5;
//     initial_velocity = -(initial_velocity)/1.5;
//     initial_velocity1 = -(initial_velocity1)/1.5 ;
//     time_travel=0;
//     if(score<9)
//     {
//       score++;
//     }
//     noofcollisions1++;
//     angle_thrown2 = M_PI/4;
//     cout << "########" << endl;
//     iscollide1 =1;
//   }
  // cout << time_travel2 << "&&&&" << br[2] << endl;
////////////////////////////////

object_collision1 = sqrt(sqr(horizontal_translation- horizontal_translation2)+ sqr(vertical_translation- vertical_translation2));
if(noofcollisions1>2)
{
  object_collision1=100;
}

if(object_collision1<=0.4 && iscollide1==0)
  {
    initial_velocity4 = (2*initial_velocity)/3.5;
    initial_velocity5 = (2*initial_velocity1)/3.5;
    initial_velocity = -(initial_velocity)/1.5;
    initial_velocity1 = -(initial_velocity1)/1.5 ;
    time_travel=0;
    if(score<9)
    {
      score++;
    }
    noofcollisions1++;
        angle_thrown2 = M_PI/4;
    // cout << "########" << endl;
    iscollide1 =1;
    // horizontal_translation -=0.4;
  }

  // cout << "UUUUUUUU" << endl;
if(noofcollisions1<=2)
{
    specialbullet(triangle1,horizontal_translation2,vertical_translation2,time_travel2,angle_thrown2,0,initial_velocity4,initial_velocity5,final_velocity4,final_velocity5,iscollide1,flagfly1);
    horizontal_translation2=br[0];
    vertical_translation2=br[1];
    time_travel2=br[2];
        // cout << time_travel2 << "+++++" << br[2] << endl;

    // angle_thrown1=br[3];
    initial_velocity4=br[4];
    initial_velocity5=br[5];
    final_velocity4=br[6];
    final_velocity5=br[7];
}
// time_travel2=forerror;

bird_collision(horizontal_translation,vertical_translation,horizontal_translation2,vertical_translation2,initial_velocity,initial_velocity1,initial_velocity4,initial_velocity5,time_travel,time_travel2);
  horizontal_translation=br[0];
  vertical_translation=br[1];
  horizontal_translation2=br[2];
  vertical_translation2=br[3];
  initial_velocity=br[4];
  initial_velocity1=br[5];
  initial_velocity4=br[6];
  initial_velocity5=br[7];
  time_travel=br[8];
  time_travel2=br[9];

// cout << initial_velocity4 << " " <<  initial_velocity5 <<  " " << object_collision1 << " " << time_travel2 <<  endl;
/////////////////////////////////////////



///////////////////////////score
  for(int iiii=0;(iiii<6) && (score==0 || score==1 || score==2 || score==3 || score==7 || score==8 || score==9 || score==4);iiii++)
    drawing_walls(3,3.6-0.12*iiii,scoresource);

  for(int iiii=0;(iiii<6) && (score==0 || score==1 || score==3 || score==4 || score==5 || score==6 || score==7 || score==8 || score==9);iiii++)
      drawing_walls(3,2.87-0.12*iiii,scoresource);

  for(int iiii=0;(iiii<6) && (score==0 || score==4 || score==5 || score==6 || score==8 || score==9);iiii++)
    drawing_walls(2.28,3.6-0.12*iiii,scoresource);

  for(int iiii=0;(iiii<7) && (score==0 || score==2 || score==6 || score==8);iiii++)
    drawing_walls(2.28,2.87-0.12*iiii,scoresource);

  for(int iiii=0;(iiii<7) && (score==0 || score==2 || score==3 || score==5 || score==6 || score==7 || score==8 || score==9);iiii++)
    drawing_walls(3-0.12*iiii,3.6,scoresource);

  for(int iiii=0;(iiii<7) && (score==0 || score==2 || score==3 || score==5 || score==6 || score==8);iiii++)
    drawing_walls(3-0.12*iiii,2.15,scoresource);

  for(int iiii=0;(iiii<7) && (score==2 || score==3 || score==4 || score==5 || score==6 || score==8 || score==9);iiii++)
    drawing_walls(3-0.12*iiii,2.90,scoresource);
//////////////////////
  if(noofcollisions==1 || noofcollisions==0 || noofcollisions==2)
    drawCircle(pig,horizontal_translation1+0.12*cos(M_PI/4),vertical_translation1+0.12*sin(M_PI/4));
  if(noofcollisions==0 || noofcollisions==1)
    drawCircle(pig,horizontal_translation1+0.12*cos((3*M_PI)/4),vertical_translation1+0.12*sin((3*M_PI)/4));
  if(noofcollisions1==1 || noofcollisions1==0 || noofcollisions1==2)
    drawCircle(pig,horizontal_translation2+0.12*cos(M_PI/4),vertical_translation2+0.12*sin(M_PI/4));
  if(noofcollisions1==0 || noofcollisions1==1)
    drawCircle(pig,horizontal_translation2+0.12*cos((3*M_PI)/4),vertical_translation2+0.12*sin((3*M_PI)/4));
  // cout << iscollide << " " << flagfly << endl;
  // cout << initial_velocity2 << " "  << initial_velocity3 << endl;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
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
    glfwSetScrollCallback (window,cbfun);

    return window;
}
void initGL (GLFWwindow* window, int width, int height)
{
  createTriangle();
  createRectangle();
  createPowerBoxes();
  createTriangle1();
  scorerectangle();
  createtankercircle();
  createpig();
  // Create and compile our GLSL program from the shaders
  programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
  // Get a handle for our "MVP" uniform
  Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

  
  reshapeWindow (window, width, height);

    // Background color of the scene
  glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
  glClearDepth (1.0f);

  glEnable (GL_DEPTH_TEST);
  glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
  int width = 600;
  int height = 600;

    GLFWwindow* window = initGLFW(width, height);

  initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw();
        glfwGetCursorPos(window,&xmousePos,&ymousePos);
        if(xmousePos<500)
        {
          tanker_angle = atan2(500 - ymousePos,xmousePos-70) + M_PI/7 + additional_angle;
          distance3 = sqr(-3 - ((xmousePos*8)/600)) + sqr(-2 - (((600-ymousePos)*8)/600));
          // cout << distance3 <<  " " << -3 - ((xmousePos*8)/600) << " " << -2 - ((ymousePos*8)/600) << endl;
          distance3 -= 40;
          distance3 /= 15;
          distance3 = floor(distance3);
          power = 30+2*distance3+power1;
          if(tanker_angle>M_PI/1.5)
          {
            if(additional_angle>0)
              additional_angle-=M_PI/18;
            else
              tanker_angle = M_PI/1.5;

          }
          if(tanker_angle<M_PI/6.5)
          {
            if(additional_angle<0)
            {
              additional_angle+=M_PI/18;
            }
            else
            {
              tanker_angle = M_PI/6.5;
            }
          }
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}