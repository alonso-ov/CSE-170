#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include "shader.h"
#include "shaderprogram.h"

#include <math.h>
#include <vector>

/*=================================================================================================
	DOMAIN
=================================================================================================*/

// Window dimensions
const int InitWindowWidth  = 800;
const int InitWindowHeight = 800;
int WindowWidth  = InitWindowWidth;
int WindowHeight = InitWindowHeight;

// Last mouse cursor position
int LastMousePosX = 0;
int LastMousePosY = 0;

// Arrays that track which keys are currently pressed
bool key_states[256];
bool key_special_states[256];
bool mouse_states[8];

// Other parameters
bool draw_wireframe = false;

/*=================================================================================================
	SHADERS & TRANSFORMATIONS
=================================================================================================*/

ShaderProgram PassthroughShader;
ShaderProgram PerspectiveShader;

glm::mat4 PerspProjectionMatrix( 1.0f );
glm::mat4 PerspViewMatrix( 1.0f );
glm::mat4 PerspModelMatrix( 1.0f );

float perspZoom = 1.0f, perspSensitivity = 0.35f;
float perspRotationX = 0.0f, perspRotationY = 0.0f;

/*=================================================================================================
	OBJECTS
=================================================================================================*/

GLuint axis_VAO;
GLuint axis_VBO[2];

// torus VAO and VBO
GLuint torus_VAO;
GLuint torus_VBO[3];

//normal VAO and VBO
GLuint outlines_VAO;
GLuint outlines_VBO[2];


float axis_vertices[] = {
	//x axis
	-1.0f,  0.0f,  0.0f,
	1.0f,  0.0f,  0.0f,
	//y axis
	0.0f, -1.0f,  0.0f,
	0.0f,  1.0f,  0.0f,
	//z axis
	0.0f,  0.0f, -1.0f,
	0.0f,  0.0f,  1.0f,
};

float axis_colors[] = {
	//x axis
	1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f,
	//y axis
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	//z axis
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f
};


#define PI	3.14159265358979323846

// vector of floats that stores all verticies of the torus
std::vector <float> vect;

// vector of floats that stores all the normals of each vertex
std::vector <float> norms;

// vector that stores all outlines of normals of each vertex
std::vector <float> n_outlines;

// global R, r, and n with default values
float n = 11.0f;
float R = 0.5f;
float r = 0.25f;

bool isFlat = true;

void FlatShading(float A[3], float B[3], float C[3], float D[3]) {

	// our three normals for each triangle
	glm::vec3 U, V, normTemp;

	// top right triangle normal

	//A - B
	U[0] = A[0] - C[0];
	U[1] = A[1] - C[1];
	U[2] = A[2] - C[2];

	//B - C
	V[0] = C[0] - B[0];
	V[1] = C[1] - B[1];
	V[2] = C[2] - B[2];


	normTemp = glm::cross(U, V);

	for (int i = 0; i < 3; i++) {
		norms.push_back(normTemp[0]);
		norms.push_back(normTemp[1]);
		norms.push_back(normTemp[2]);
		norms.push_back(1.0f);
	}

	//convert normals into unit vectors
	float magnitude = sqrt(pow(normTemp[0], 2) + pow(normTemp[1], 2) + pow(normTemp[2], 2));

	normTemp[0] /= magnitude;
	normTemp[1] /= magnitude;
	normTemp[2] /= magnitude;

	//scale down the unit vector
	normTemp[0] *= 0.05;
	normTemp[1] *= 0.05;
	normTemp[2] *= 0.05;

	//calculate norm outline
	for (int i = 0; i < 3; i++) {
		n_outlines.push_back(A[i]);
	}
	n_outlines.push_back(1.0f);

	for (int i = 0; i < 3; i++) {
		n_outlines.push_back(A[i] + normTemp[i]);
	}
	n_outlines.push_back(1.0f);

	//calculate norm outline
	for (int i = 0; i < 3; i++) {
		n_outlines.push_back(B[i]);
	}
	n_outlines.push_back(1.0f);

	for (int i = 0; i < 3; i++) {
		n_outlines.push_back(B[i] + normTemp[i]);
	}
	n_outlines.push_back(1.0f);

	//calculate norm outline
	for (int i = 0; i < 3; i++) {
		n_outlines.push_back(C[i]);
	}
	n_outlines.push_back(1.0f);

	for (int i = 0; i < 3; i++) {
		n_outlines.push_back(C[i] + normTemp[i]);
	}
	n_outlines.push_back(1.0f);

	// bottom left triangle normal

	// A - C
	U[0] = A[0] - D[0];
	U[1] = A[1] - D[1];
	U[2] = A[2] - D[2];

	// C - D
	V[0] = D[0] - C[0];
	V[1] = D[1] - C[1];
	V[2] = D[2] - C[2];


	normTemp = glm::cross(U, V);

	for (int i = 0; i < 3; i++) {
		norms.push_back(normTemp[0]);
		norms.push_back(normTemp[1]);
		norms.push_back(normTemp[2]);
		norms.push_back(1.0f);
	}

}

void SmoothShading(float A[3], float B[3], float C[3], float D[3]) {
	norms.push_back(0.0f);
	n_outlines.push_back(0.0f);
}

// generates vertex values for all triangles of the torus
void ComputerTorusVerticies(float R, float r, float n) {

	// our four verticies of our square
	float A[3], B[3], C[3], D[3];

	// defines increment of each square
	float increment = (2 * PI) / n;

	float preTheta = 0.0f, theta = increment, phi = increment;

	// rotation around the axis
	for (float i = 0; i < n; i += 1) {

		// rotation around tube
		for (float j = 0; j < n; j += 1) {

			//VERTICIES

			// top left vertex
			A[0] = (R + r * cos(theta)) * cos(phi + increment);
			A[1] = (R + r * cos(theta)) * sin(phi + increment);
			A[2] = r * sin(theta);

			//top right vertex
			B[0] = (R + r * cos(preTheta)) * cos(phi + increment);
			B[1] = (R + r * cos(preTheta)) * sin(phi + increment);
			B[2] = r * sin(preTheta);

			// lower right vertex
			C[0] = (R + r * cos(preTheta)) * cos(phi);
			C[1] = (R + r * cos(preTheta)) * sin(phi);
			C[2] = r * sin(preTheta);

			// lower left vertex
			D[0] = (R + r * cos(theta)) * cos(phi);
			D[1] = (R + r * cos(theta)) * sin(phi);
			D[2] = r * sin(theta);


			// top right triangle

			for (int i = 0; i < 3; i++)
				vect.push_back(C[i]);
			vect.push_back(1.0f);

			for (int i = 0; i < 3; i++)
				vect.push_back(B[i]);
			vect.push_back(1.0f);

			for (int i = 0; i < 3; i++)
				vect.push_back(A[i]);
			vect.push_back(1.0f);

			// bottom left Triangle
			for (int i = 0; i < 3; i++)
				vect.push_back(A[i]);
			vect.push_back(1.0f);

			for (int i = 0; i < 3; i++)
				vect.push_back(D[i]);
			vect.push_back(1.0f);

			for (int i = 0; i < 3; i++)
				vect.push_back(C[i]);
			vect.push_back(1.0f);

			//END OF VERTICIES


			// flat or smooth shading
			if (isFlat  == true) {
				FlatShading(A, B, C, D);
			}
			else {
				SmoothShading(A, B, C, D);
			}
		
			// next phi value
			phi += increment;
		}
		// store previous theta value
		preTheta = theta;

		// next theta value
		theta += increment;
	}
};

/*=================================================================================================
	HELPER FUNCTIONS
=================================================================================================*/

void window_to_scene( int wx, int wy, float& sx, float& sy )
{
	sx = ( 2.0f * (float)wx / WindowWidth ) - 1.0f;
	sy = 1.0f - ( 2.0f * (float)wy / WindowHeight );
}

/*=================================================================================================
	SHADERS
=================================================================================================*/

void CreateTransformationMatrices( void )
{
	// PROJECTION MATRIX  <- used to project 3D point onto the screen
	PerspProjectionMatrix = glm::perspective<float>( glm::radians( 60.0f ), (float)WindowWidth / (float)WindowHeight, 0.01f, 1000.0f );

	// VIEW MATRIX <- used to transform the model's verticies from world-space into view-space
	glm::vec3 eye   ( 0.0, 0.0, 2.0 );
	glm::vec3 center( 0.0, 0.0, 0.0 );
	glm::vec3 up    ( 0.0, 1.0, 0.0 );

	PerspViewMatrix = glm::lookAt( eye, center, up );

	// MODEL MATRIX <- contains every translation, rotation, or scaling applied to an object 
	PerspModelMatrix = glm::mat4( 1.0 );
	PerspModelMatrix = glm::rotate( PerspModelMatrix, glm::radians( perspRotationX ), glm::vec3( 1.0, 0.0, 0.0 ) );
	PerspModelMatrix = glm::rotate( PerspModelMatrix, glm::radians( perspRotationY ), glm::vec3( 0.0, 1.0, 0.0 ) );
	PerspModelMatrix = glm::scale( PerspModelMatrix, glm::vec3( perspZoom ) );
}

void CreateShaders( void )
{
	// Renders without any transformations
	PassthroughShader.Create( "./shaders/simple.vert", "./shaders/simple.frag" );

	// Renders using perspective projection
	PerspectiveShader.Create( "./shaders/persp.vert", "./shaders/persp.frag" );

	// Renders using prespective lighting projection
	// Renders using prespective lighting projection
	PerspectiveShader.Create("./shaders/persplight.vert", "./shaders/persplight.frag");
}

/*=================================================================================================
	BUFFERS
=================================================================================================*/

void CreateAxisBuffers( void )
{

	glGenVertexArrays( 1, &axis_VAO );
	glBindVertexArray( axis_VAO );

	glGenBuffers( 2, &axis_VBO[0] );

	glBindBuffer( GL_ARRAY_BUFFER, axis_VBO[0] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( axis_vertices ), axis_vertices, GL_STATIC_DRAW );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), (void*)0 );
	glEnableVertexAttribArray( 0 );

	glBindBuffer( GL_ARRAY_BUFFER, axis_VBO[1] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( axis_colors ), axis_colors, GL_STATIC_DRAW );
	glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)0 );
	glEnableVertexAttribArray( 1 );

	glBindVertexArray( 0 );
}


void CreateNormsOutlineBuffer(void) {

	std::vector <float> norms_color;

	for (int i = 0; i < n_outlines.size() / 4; i++) {
		norms_color.push_back(0.0f);
		norms_color.push_back(1.0f);
		norms_color.push_back(0.0f);
		norms_color.push_back(1.0f);
	}

	glGenVertexArrays(1, &outlines_VAO);
	glBindVertexArray(outlines_VAO);

	glGenBuffers(2, &outlines_VBO[0]);

	glBindBuffer(GL_ARRAY_BUFFER, outlines_VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(n_outlines[0]) * n_outlines.size(), &n_outlines[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, outlines_VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(norms_color[0]) * norms_color.size(), &norms_color[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}


// call verticies function and send verticies to buffer
void CreateTorusBuffer( void ) {

	// clear old verticies
	vect.clear();
	norms.clear();
	n_outlines.clear();
	
	// compute verticies
	ComputerTorusVerticies(R, r, n);


	std::vector <float> torus_colors;

	//make all verticies blue
	for (int i = 0; i < vect.size()/4; i++) {
		torus_colors.push_back(0.0f);
		torus_colors.push_back(0.0f);
		torus_colors.push_back(1.0f);
		torus_colors.push_back(1.0f);
	}

	glGenVertexArrays(1, &torus_VAO);
	glBindVertexArray(torus_VAO);

	glGenBuffers(3, &torus_VBO[0]);

	// verticies
	glBindBuffer(GL_ARRAY_BUFFER, torus_VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vect[0]) * vect.size(), &vect[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// colors
	glBindBuffer(GL_ARRAY_BUFFER, torus_VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(torus_colors[0]) * torus_colors.size(), &torus_colors[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	// normal vectors
	glBindBuffer(GL_ARRAY_BUFFER, torus_VBO[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(norms[0]) * norms.size(), &norms[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	CreateNormsOutlineBuffer();
}

/*=================================================================================================
	CALLBACKS
=================================================================================================*/

//-----------------------------------------------------------------------------
// CALLBACK DOCUMENTATION
// https://www.opengl.org/resources/libraries/glut/spec3/node45.html
// http://freeglut.sourceforge.net/docs/api.php#WindowCallback
//-----------------------------------------------------------------------------

void idle_func()
{
	//uncomment below to repeatedly draw new frames
	glutPostRedisplay();
}

void reshape_func( int width, int height )
{
	WindowWidth  = width;
	WindowHeight = height;

	glViewport( 0, 0, width, height );
	glutPostRedisplay();
}

float toggleOutlines = 1.0f;

void keyboard_func( unsigned char key, int x, int y )
{
	key_states[ key ] = true;

	switch( key )
	{
		case 'w':
		{
			draw_wireframe = !draw_wireframe;
			if( draw_wireframe == true )
				std::cout << "Wireframes on.\n";
			else
				std::cout << "Wireframes off.\n";
			break;
		}

		/*
		* All values are adjusted with a difference of 1.0f
		* and then call CreateTorusBuffer() to generate
		* torus based off new values
		*/


		// increases n
		case 'q':
		{
			n += 1.0f;
			CreateTorusBuffer();
			break;
		}

		// decreases n
		case 'a':
		{
			n -= 1.0f;
			CreateTorusBuffer();
			break;
		}

		// increases r
		case 'e':
		{
			r += 0.1f;
			CreateTorusBuffer();
			break;
		}

		// decreases r
		case 'd':
		{
			r -= 0.1f;
			CreateTorusBuffer();
			break;
		}

		// increases R
		case 'r':
		{
			R += 0.1f;
			CreateTorusBuffer();
			break;
		}

		// decreases R
		case 'f':
		{
			R -= 0.1f;
			CreateTorusBuffer();
			break;
		}

		//enable flat shading
		case 'z':
			isFlat = true;
			CreateTorusBuffer();
			break;

		//enable smooth shading
		case 'x':
			isFlat = false;
			CreateTorusBuffer();
			break;

		//toggle normal vector outlines
		case 'c':
			toggleOutlines *= -1.0f;
			CreateTorusBuffer();
			break;


		// Exit on escape key press
		case '\x1B':
		{
			exit( EXIT_SUCCESS );
			break;
		}
	}
}

void key_released( unsigned char key, int x, int y )
{
	key_states[ key ] = false;
}

void key_special_pressed( int key, int x, int y )
{
	key_special_states[ key ] = true;
}

void key_special_released( int key, int x, int y )
{
	key_special_states[ key ] = false;
}

void mouse_func( int button, int state, int x, int y )
{
	// Key 0: left button
	// Key 1: middle button
	// Key 2: right button
	// Key 3: scroll up
	// Key 4: scroll down

	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	if( button == 3 )
	{
		perspZoom += 0.03f;
	}
	else if( button == 4 )
	{
		if( perspZoom - 0.03f > 0.0f )
			perspZoom -= 0.03f;
	}

	mouse_states[ button ] = ( state == GLUT_DOWN );

	LastMousePosX = x;
	LastMousePosY = y;
}

void passive_motion_func( int x, int y )
{
	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	LastMousePosX = x;
	LastMousePosY = y;
}

void active_motion_func( int x, int y )
{
	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	if( mouse_states[0] == true )
	{
		perspRotationY += ( x - LastMousePosX ) * perspSensitivity;
		perspRotationX += ( y - LastMousePosY ) * perspSensitivity;
	}

	LastMousePosX = x;
	LastMousePosY = y;
}

/*=================================================================================================
	RENDERING
=================================================================================================*/

void display_func( void )

	/*
	* Drawing the modern way
	* 
	* 1.	write the shaders
	* 2.	initalize the shaders
	*		-send shader programs to GPU (& compile)
	* 3.	organize geometry data in buffers and send
	*		to the GPU
	*		-create needed buffer objects(VBO, VAO, etc)
	* 4.	Draw
	*		-Enable the shader programs and buffers
	*			+everything is already in the graphics card
	*		-Send the draw command
	* 
	* + VAO is a buffer of memory that the gpu can access
	*		-vertex array object
	*		-VBO is vertex buffer object
	* 
	*/
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


	CreateTransformationMatrices();

	PerspectiveShader.Use();
	PerspectiveShader.SetUniform( "projectionMatrix", glm::value_ptr( PerspProjectionMatrix ), 4, GL_FALSE, 1 );
	PerspectiveShader.SetUniform( "viewMatrix", glm::value_ptr( PerspViewMatrix ), 4, GL_FALSE, 1 );
	PerspectiveShader.SetUniform( "modelMatrix", glm::value_ptr( PerspModelMatrix ), 4, GL_FALSE, 1 );


	if( draw_wireframe == true )
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );


	glBindVertexArray( axis_VAO );
	glDrawArrays( GL_LINES, 0, 6);
	glBindVertexArray( 0 );


	glBindVertexArray(torus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, vect.size() / 4);
	glBindVertexArray(0);

	//shows outlines if toggleOutlines is positive
	if (toggleOutlines > 0 && isFlat == true) {
		glBindVertexArray(outlines_VAO);
		glDrawArrays(GL_LINES, 0, n_outlines.size() / 4);
		glBindVertexArray(0);
	}


	if( draw_wireframe == true )
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );


	glutSwapBuffers();
}

/*=================================================================================================
	INIT
=================================================================================================*/

void init( void )
{
	// Print some info
	std::cout << "Vendor:         " << glGetString( GL_VENDOR   ) << "\n";
	std::cout << "Renderer:       " << glGetString( GL_RENDERER ) << "\n";
	std::cout << "OpenGL Version: " << glGetString( GL_VERSION  ) << "\n";
	std::cout << "GLSL Version:   " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n\n";

	// Set OpenGL settings
	glClearColor( 1.0f, 1.0f, 1.0f, 0.0f ); // background color
	glEnable( GL_DEPTH_TEST ); // enable depth test
	glEnable( GL_CULL_FACE ); // enable back-face culling

	// Create shaders
	CreateShaders();

	// Create buffers
	CreateAxisBuffers();
	CreateTorusBuffer();

	std::cout << "Use q and a to control n variable" << std::endl;
	std::cout << "Use e and d to control r variable" << std::endl;
	std::cout << "Use r and f to control R variable" << std::endl;

	std::cout << std::endl;

	std::cout << "Finished initializing...\n\n";
}

/*=================================================================================================
	MAIN
=================================================================================================*/

int main( int argc, char** argv )
{
	glutInit( &argc, argv );

	glutInitWindowPosition( 100, 100 );
	glutInitWindowSize( InitWindowWidth, InitWindowHeight );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );

	glutCreateWindow( "CSE-170 Computer Graphics" );

	// Initialize GLEW
	GLenum ret = glewInit();
	if( ret != GLEW_OK ) {
		std::cerr << "GLEW initialization error." << std::endl;
		glewGetErrorString( ret );
		return -1;
	}

	glutDisplayFunc( display_func );
	glutIdleFunc( idle_func );
	glutReshapeFunc( reshape_func );
	glutKeyboardFunc( keyboard_func );
	glutKeyboardUpFunc( key_released );
	glutSpecialFunc( key_special_pressed );
	glutSpecialUpFunc( key_special_released );
	glutMouseFunc( mouse_func );
	glutMotionFunc( active_motion_func );
	glutPassiveMotionFunc( passive_motion_func );

	init();

	glutMainLoop();

	return EXIT_SUCCESS;
}