#include "imuread.h"

static void quad_to_rotation(const libcalib::Quaternion_t *quat, float (& rmatrix)[9])
{
	float qw = quat->q0;
	float qx = quat->q1;
	float qy = quat->q2;
	float qz = quat->q3;
	rmatrix[0] = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
	rmatrix[1] = 2.0f * qx * qy - 2.0f * qz * qw;
	rmatrix[2] = 2.0f * qx * qz + 2.0f * qy * qw;
	rmatrix[3] = 2.0f * qx * qy + 2.0f * qz * qw;
	rmatrix[4] = 1.0f  - 2.0f * qx * qx - 2.0f * qz * qz;
	rmatrix[5] = 2.0f * qy * qz - 2.0f * qx * qw;
	rmatrix[6] = 2.0f * qx * qz - 2.0f * qy * qw;
	rmatrix[7] = 2.0f * qy * qz + 2.0f * qx * qw;
	rmatrix[8] = 1.0f  - 2.0f * qx * qx - 2.0f * qy * qy;
}

static void rotate(const libcalib::Point_t *in, libcalib::Point_t *out, const float (& rmatrix)[9])
{
	out->x = in->x * rmatrix[0] + in->y * rmatrix[1] + in->z * rmatrix[2];
	out->y = in->x * rmatrix[3] + in->y * rmatrix[4] + in->z * rmatrix[5];
	out->z = in->x * rmatrix[6] + in->y * rmatrix[7] + in->z * rmatrix[8];
}



static GLuint spherelist;
static GLuint spherelowreslist;

void display_callback()
{
	calib.m_magcal.ensure_quality();

	float xscale = 0.05;
	float yscale = 0.05;
	float zscale = 0.05;
	float xoff = 0.0;
	float yoff = 0.0;
	float zoff = -7.0;

	libcalib::Quaternion_t orientation = calib.m_current_orientation;
	
	// TODO: this almost but doesn't perfectly seems to get the
	//  real & screen axes in sync....
	orientation.q3 *= -1.0f;

	float rotation[9];
	quad_to_rotation(&orientation, rotation);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1, 0, 0);	// set current color to red
	glLoadIdentity();
	
	for (int i = 0; i < calib.m_magcal.m_cSamp; ++i)
	{
		const auto & samp = calib.m_magcal.m_aSamp[i];
		const libcalib::Point_t * pBc = &samp.m_pntCal;
		libcalib::Point_t draw;

		rotate(pBc, &draw, rotation);

		glPushMatrix();
		glTranslatef(
			draw.x * xscale + xoff,
			draw.z * yscale + yoff,
			draw.y * zscale + zoff
		);
		if (draw.y >= 0.0f) {
			glCallList(spherelist);
		} else {
			glCallList(spherelowreslist);
		}
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


static const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
static const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

static const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
static const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
static const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat high_shininess[] = { 100.0f };

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
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

	sphere = gluNewQuadric();
	gluQuadricDrawStyle(sphere, GLU_FILL);
	gluQuadricNormals(sphere, GLU_SMOOTH);
	spherelist = glGenLists(1);
	glNewList(spherelist, GL_COMPILE);
	gluSphere(sphere, 0.08, 16, 14);
	glEndList();
	spherelowreslist = glGenLists(1);
	glNewList(spherelowreslist, GL_COMPILE);
	gluSphere(sphere, 0.08, 12, 10);
	glEndList();
	gluDeleteQuadric(sphere);
}



