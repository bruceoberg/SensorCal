#include "imuread.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static glm::mat4 Mat4FromBasisVectors(glm::vec3 vecX, glm::vec3 vecY, glm::vec3 vecZ)
{
	// columns of a mat4 are vec4; w column is identity
	return glm::mat4(
		glm::vec4(vecX, 0.0f),
		glm::vec4(vecY, 0.0f),
		glm::vec4(vecZ, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);
}

constexpr glm::vec3 s_vecAxisX  = glm::vec3( 1.0f,  0.0f,  0.0f);
constexpr glm::vec3 s_vecAxisY  = glm::vec3( 0.0f,  1.0f,  0.0f);
constexpr glm::vec3 s_vecAxisZ  = glm::vec3( 0.0f,  0.0f,  1.0f);
constexpr glm::vec3 s_vecAxisNX = glm::vec3(-1.0f,  0.0f,  0.0f);
constexpr glm::vec3 s_vecAxisNY = glm::vec3( 0.0f, -1.0f,  0.0f);
constexpr glm::vec3 s_vecAxisNZ = glm::vec3( 0.0f,  0.0f, -1.0f);

static GLuint s_glistSphere;

namespace Light
{
	constexpr GLfloat s_rgbaAmbient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
	constexpr GLfloat s_rgbaDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr GLfloat s_rgbaSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr GLfloat s_pos[]          = { 2.0f, 5.0f, 5.0f, 0.0f };

	constexpr GLfloat s_matAmbient[]   = { 0.7f, 0.7f, 0.7f, 1.0f };
	constexpr GLfloat s_matDiffuse[]   = { 0.8f, 0.8f, 0.8f, 1.0f };
	constexpr GLfloat s_matSpecular[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr GLfloat s_matShininess[] = { 100.0f };
}

void display_callback()
{
	libcalib::Calibrator & calib = libcalib::Calibrator::Ensure();

	calib.m_fitter.EnsureQuality();

	// Build the combined modelview transform:
	//   matTransform = matTranslate * matScale * matSwizzle * matRotation
	//
	// matRotation  — AHRS orientation quaternion
	// matSwizzle     — remap sensor axes to GL view axes
	// matScale     — scale calibrated mag values into view
	// matTranslate — push the sphere cluster back into the frustum

	libcalib::SQuat orientation = calib.m_current_orientation;
	glm::quat qRot(orientation.q0, orientation.q1, orientation.q2, orientation.q3);
	glm::mat4 matRotation = glm::mat4_cast(qRot);

	// Remap sensor axes to GL view axes for Adafruit ISM330DHCX + LIS3MDL
	// (product 4569): sensor x → GL x, sensor y → GL -z, sensor z → GL y
	glm::mat4 matSwizzle = Mat4FromBasisVectors(s_vecAxisX, s_vecAxisNZ, s_vecAxisY);

	glm::mat4 matScale = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));
	glm::mat4 matTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -7.0f));

	glm::mat4 matTransform = matTranslate * matScale * matSwizzle * matRotation;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (calib.m_fitter.AreErrorsOk())
	{
		glColor3f(0, 1, 0);	// green
	}
	else
	{
		switch (calib.m_fitter.m_solver)
		{
		case libcalib::Sphere::CFitter::SOLVER_4Inv:
			glColor3f(1, 0, 1);	// magenta
			break;
		case libcalib::Sphere::CFitter::SOLVER_7Eig:
			glColor3f(0, 1, 1);	// cyan
			break;
		case libcalib::Sphere::CFitter::SOLVER_10Eig:
			glColor3f(1, 1, 0);	// yellow
			break;
		default:
			glColor3f(1, 0, 0);	// red
			break;
		}
	}

	// Transform points on the CPU so the GL modelview stays identity + translation.
	// This keeps sphere normals in eye space for consistent front-lighting.

	glLoadIdentity();

	for (int iSamp = 0; iSamp < calib.m_fitter.m_samps.CSamp(); ++iSamp)
	{
		const auto & samp = calib.m_fitter.m_samps.Samp(iSamp);
		glm::vec4 pos = matTransform * glm::vec4(samp.m_pntCal.x, samp.m_pntCal.y, samp.m_pntCal.z, 1.0f);

		glPushMatrix();
		glTranslatef(pos.x, pos.y, pos.z);
		glCallList(s_glistSphere);
		glPopMatrix();
	}
}

void resize_callback(int width, int height)
{
	const float ar = (float) width / (float) height;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity() ;
}


void visualize_init()
{
	GLUquadric *sphere;

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_LIGHT0);
	//glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  Light::s_rgbaAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  Light::s_rgbaDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, Light::s_rgbaSpecular);
	glLightfv(GL_LIGHT0, GL_POSITION, Light::s_pos);
	glMaterialfv(GL_FRONT, GL_AMBIENT,   Light::s_matAmbient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE,   Light::s_matDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR,  Light::s_matSpecular);
	glMaterialfv(GL_FRONT, GL_SHININESS, Light::s_matShininess);

	sphere = gluNewQuadric();
	gluQuadricDrawStyle(sphere, GLU_FILL);
	gluQuadricNormals(sphere, GLU_SMOOTH);
	s_glistSphere = glGenLists(1);
	glNewList(s_glistSphere, GL_COMPILE);
	gluSphere(sphere, 0.08, 16, 14);
	glEndList();
	gluDeleteQuadric(sphere);
}



