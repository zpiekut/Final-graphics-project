/**************************************************************************
File: glmain.c
Does: basic lighting and modeling for cs1566 hw4 Modeler
Author: Steven Lauck, based on some hwa
Date: 01/08/09
**************************************************************************/

#include "glmain.h"
#define my_assert(X,Y) ((X)?(void) 0:(printf("error:%s in %s at %d", Y, __FILE__, __LINE__), myabort()))

#define min(a,b) ((a) < (b)? a:b)
#define FALSE 0 
#define TRUE  1
#define MAX_LIGHTS  8
#define NUM_OBJECTS 8

typedef struct _Object {
  int sid;

  // example object storage arrays for vertex and normals
  GLfloat vertices_cube_smart[8][4];
  GLfloat normals_cube_smart[8][3];

  //house vertex and normals
  GLfloat vertices_house[9][4];
  GLfloat normals_house[9][3];

  //sphere vertex and normals
  GLfloat vertices_sph[50][50][4];
  GLfloat normals_sph[50][50][3];

  //cylinders
  GLfloat vertices_cyl[50][50][4];
  GLfloat normals_cyl[50][50][3];

  //cones
  GLfloat vertices_con[50][50][4];
  GLfloat normals_con[50][50][3];

  //torus
  GLfloat vertices_tor[50][50][4];
  GLfloat normals_tor[50][50][3];
 
  GLfloat shine;
  GLfloat emi[4];
  GLfloat amb[4];
  GLfloat diff[4];
  GLfloat spec[4];

  GLfloat translate[4];
  GLfloat scale[4];
  GLfloat rotate[4];

}OBJECT;

typedef struct _CAM{
  GLfloat pos[4];
  GLfloat at[4];
  GLfloat up[4];

  GLfloat dir[4];
}CAM;

typedef struct _LITE{
  GLfloat amb[4];
  GLfloat diff[4];
  GLfloat spec[4];
  GLfloat pos[4];
  GLfloat dir[3];
  GLfloat angle;
}LITE;

GLfloat colors [][3] = {
  {0.0, 0.0, 0.0},  /* black   */
  {1.0, 0.0, 0.0},  /* red     */
  {1.0, 1.0, 0.0},  /* yellow  */
  {1.0, 0.0, 1.0},  /* magenta */
  {0.0, 1.0, 0.0},  /* green   */
  {0.0, 1.0, 1.0},  /* cyan    */
  {0.0, 0.0, 1.0},  /* blue    */
  {0.5, 0.5, 0.5},  /* 50%grey */
  {1.0, 1.0, 1.0}   /* white   */
};

GLfloat vertices_axes[][4] = {
	{0.0, 0.0, 0.0, 1.0},  /* origin */ 
	{5.0, 0.0, 0.0, 1.0},  /* maxx */ 
	{0.0, 5.0, 0.0, 1.0}, /* maxy */ 
	{0.0, 0.0, 5.0, 1.0}  /* maxz */ 

};

void make_cube_smart(OBJECT *po, double size );
void make_house(OBJECT *po);
void make_sphere(OBJECT *po);
void make_cylinder(OBJECT *po);
void make_cone(OBJECT *po);
void make_torus(OBJECT *po);

void real_translation(OBJECT *po, GLfloat x, GLfloat y, GLfloat z);
void real_scaling(OBJECT *po, GLfloat sx, GLfloat sy, GLfloat sz);
void real_rotation(OBJECT *po, GLfloat deg, GLfloat x, GLfloat y, GLfloat z);

OBJECT my_objects[NUM_OBJECTS];
LITE my_lights[MAX_LIGHTS];
int num_objects;
int  num_lights;

// camera variables
CAM my_cam;
GLfloat camx, camy, camz;
GLfloat atx, aty, atz;
GLfloat upx, upy, upz;

//TRUE or FALSE
int firstPersonView;

int crt_render_mode;
int crt_shape, crt_rs, crt_vs;
int crt_transform;
int height = 40;

int jumpamount = 0;
double yaccel = 0;

void myabort(void) {
  abort();
  exit(1); /* exit so g++ knows we don't return. */
} 

int main(int argc, char** argv)
{ 
  setbuf(stdout, NULL);   /* for writing to stdout asap */
  glutInit(&argc, argv);

  my_setup(argc, argv);  
  glut_setup();
  gl_setup();

  glutMainLoop();
  return(0);
}


void glut_setup (){

  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
  
  glutInitWindowSize(700,700);
  glutInitWindowPosition(20,20);
  glutCreateWindow("CS1566 Project 4");

  /* set up callback functions */
  glutDisplayFunc(my_display);
  glutReshapeFunc(my_reshape);
  glutMouseFunc(my_mouse);
  glutMotionFunc(my_mouse_drag);
  glutKeyboardFunc(my_keyboard);
  glutSpecialFunc(my_arrows);
  glutIdleFunc( my_idle );	

  return;
}

void gl_setup(void) {
	float FogColor[3]={0.75f,0.75f,0.75f};
  // enable depth handling (z-buffer)
  glEnable(GL_DEPTH_TEST);

  // enable auto normalize
  glEnable(GL_NORMALIZE);

  // define the background color 
  glClearColor(0.75f,0.75f,0.75f,1);

  glMatrixMode(GL_PROJECTION) ;
  glLoadIdentity() ;
  gluPerspective( 40, 1.0, 1, 200.0);
  glMatrixMode(GL_MODELVIEW) ;
  glLoadIdentity() ;  // init modelview to identity

  //fog stuff
  glEnable(GL_FOG);
  glFogfv(GL_FOG_COLOR,FogColor); // Set the fog color
  glFogf(GL_FOG_DENSITY,0.1f);

  // toggle to smooth shading (instead of flat)
  glShadeModel(GL_SMOOTH); 
  lighting_setup();



  return ;
}

void my_setup(int argc, char **argv){

  firstPersonView = 1;
  num_objects = num_lights = 0;

  // initialize global shape defaults and mode for drawing
  crt_render_mode = GL_POLYGON;
  crt_shape = 0;

  crt_rs = 40;
  crt_vs = 40; 
 
  //If you want to allow the user to type in the spec file
  //then modify the following code.
  //Otherwise, the program will attempt to load the file as specified
  //on the command line:
  //EX: ./glmain spec3
  my_assert(argc >1, "need to supply a spec file");
  read_spec(argv[1]);
  return;
}

void parse_floats(char *buffer, GLfloat nums[]) {
  int i;
  char *ps;

  ps = strtok(buffer, " ");
  for (i=0; ps; i++) {
    nums[i] = atof(ps);
    ps = strtok(NULL, " ");
    //printf("read %f ",nums[i]);
  }

}

void parse_obj(char *buffer){
  OBJECT *po;
  char *pshape, *pshine, *pemi, *pamb, *pdiff, *pspec, *ptranslate, *pscale, *protate;



  my_assert ((num_objects < NUM_OBJECTS), "too many objects");
  po = &my_objects[num_objects++];

  pshape  = strtok(buffer, " ");
  //printf("pshape is %s\n",pshape);

  ptranslate    = strtok(NULL, "()");  strtok(NULL, "()");
  pscale        = strtok(NULL, "()");  strtok(NULL, "()"); 
  protate       = strtok(NULL, "()");  strtok(NULL, "()");  

  pshine  = strtok(NULL, "()");strtok(NULL, "()");
  //printf("pshine is %s\n",pshine);
 
  pemi    = strtok(NULL, "()");  strtok(NULL, "()"); 
  pamb    = strtok(NULL, "()");  strtok(NULL, "()"); 
  pdiff   = strtok(NULL, "()");  strtok(NULL, "()"); 
  pspec   = strtok(NULL, "()");  strtok(NULL, "()"); 


  po->sid  = atoi(pshape);
  po->shine = atof(pshine);

  parse_floats(ptranslate, po->translate);
  parse_floats(pscale, po->scale);
  parse_floats(protate, po->rotate);

  parse_floats(pemi, po->emi);
  parse_floats(pamb, po->amb);
  parse_floats(pdiff, po->diff);
  parse_floats(pspec, po->spec);

  // use switch to create your objects, cube given as example
  switch (po->sid){
  case 1: //cube
    make_cube_smart(po, 1);
	break;
  case 2: //sphere
	  make_sphere(po);
	  break;
  case 3: //cylinder
	  make_cylinder(po);
	  break;
  case 4: //cone
	  make_cone(po);
	  break;
  case 5: //torus
	  make_torus(po);
	  break;
  }
  
  // scale, rotate, translate using your real tranformations from assignment 3 depending on input from spec file
  
  real_scaling(po, po->scale[0], po->scale[1], po->scale[2]);  
  real_rotation(po, po->rotate[0], 1, 0, 0);
  real_rotation(po, po->rotate[1], 0, 1, 0);
  real_rotation(po, po->rotate[2], 0, 0, 1);
  real_translation(po, po->translate[0], po->translate[1], po->translate[2]);
  
  printf("read object\n");
}

void parse_camera(char *buffer){
  CAM *pc;
  char *ppos, *plook, *pup;

  pc = &my_cam;

  strtok(buffer, "()");
  ppos  = strtok(NULL, "()");  strtok(NULL, "()"); 
  plook  = strtok(NULL, "()");  strtok(NULL, "()"); 
  pup  = strtok(NULL, "()");  strtok(NULL, "()"); 

  parse_floats(ppos, pc->pos);
  parse_floats(plook, pc->at);
  parse_floats(pup, pc->up);

  pc->at[0] += pc->pos[0];
  pc->at[1] += pc->pos[1];
  pc->at[2] += pc->pos[2];

  pc->dir[0] = pc->at[0] - pc->pos[0];
  pc->dir[1] = pc->at[1] - pc->pos[1];
  pc->dir[2] = pc->at[2] - pc->pos[2];
  normalize(pc->dir);
  printf("read camera\n");
}

void parse_light(char *buffer){
  LITE *pl;
  char *pamb, *pdiff, *pspec, *ppos, *pdir, *pang;
  my_assert ((num_lights < MAX_LIGHTS), "too many lights");
  pl = &my_lights[++num_lights];

  strtok(buffer, "()");
  pamb  = strtok(NULL, "()");  strtok(NULL, "()"); 
  pdiff = strtok(NULL, "()");  strtok(NULL, "()"); 
  pspec = strtok(NULL, "()");  strtok(NULL, "()"); 
  ppos  = strtok(NULL, "()");  strtok(NULL, "()"); 
  pdir  = strtok(NULL, "()");  strtok(NULL, "()"); 
  pang  = strtok(NULL, "()");

  parse_floats(pamb, pl->amb);
  parse_floats(pdiff, pl->diff);
  parse_floats(pspec, pl->spec);
  parse_floats(ppos, pl->pos);
  if (pdir) {
    parse_floats(pdir, pl->dir);
    pl->angle = atof(pang);
    //printf("angle %f\n", pl->angle);
  }
  else
    pl->dir[0]= pl->dir[1]= pl->dir[2] =0;
  printf("read light\n");

}

/* assuming the spec is going to be properly written
   not error-checking here */
void read_spec(char *fname) {
  char buffer[300];
  FILE *fp;

  fp = fopen(fname, "r");
  my_assert(fp, "can't open spec");
  while(!feof(fp)){
    fgets(buffer, 300, fp);
    //printf("read line: %s\n", buffer);
    switch (buffer[0]) {
    case '#':
      break;
    case '1': //cube
   	  //read in the cube
	  parse_obj(buffer);
 	  break;
	//etc

    case 'l':
      parse_light(buffer);
      break;

    case 'c':
      parse_camera(buffer);
      break;

    default:
      break;
    }
  }
}

void lighting_setup () {
  int i;
  GLfloat globalAmb[]     = {.1, .1, .1, .1};

  // create flashlight
  GLfloat amb[] = {0.2, 0.2, 0.2, 1.0};
  GLfloat dif[] = {0.8, 0.8, 0.8, 1.0};
  GLfloat spec[] = {5.0, 5.0, 5.0, 1.0};

  //enable lighting
  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);

  // reflective propoerites -- global ambiant light
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

  // this was for the flashlights

 // glLightfv(GL_LIGHT0, GL_POSITION, my_cam.at);
 // glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, my_cam.dir);

 // glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
 // glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
 // glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
 // glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 30.0);

 // glEnable(GL_LIGHT0);

  // setup properties of lighting
  for (i=1; i<=num_lights; i++) {
    glEnable(GL_LIGHT0+i);
    glLightfv(GL_LIGHT0+i, GL_AMBIENT, my_lights[i].amb);
    glLightfv(GL_LIGHT0+i, GL_DIFFUSE, my_lights[i].diff);
    glLightfv(GL_LIGHT0+i, GL_SPECULAR, my_lights[i].spec);
    glLightfv(GL_LIGHT0+i, GL_POSITION, my_lights[i].pos);
    if ((my_lights[i].dir[0] > 0) ||  (my_lights[i].dir[1] > 0) ||  (my_lights[i].dir[2] > 0)) {
      glLightf(GL_LIGHT0+i, GL_SPOT_CUTOFF, my_lights[i].angle);
      glLightfv(GL_LIGHT0+i, GL_SPOT_DIRECTION, my_lights[i].dir);
    }
  }

}

void my_reshape(int w, int h) {
  // ensure a square view port
  glViewport(0,0,min(w,h),min(w,h)) ;
  return ;
}

//For the following 3 functions, do with them as you like.
//You will have to somehow rotate / translate / scale\
//the object as specified in the spec file.

void my_rev(GLfloat deg, GLfloat x, GLfloat y, GLfloat z) {
 GLfloat tempx = my_cam.pos[0];
	GLfloat tempy = my_cam.pos[1];
	GLfloat tempz = my_cam.pos[2];
	my_cam.pos[0] = 0;
	my_cam.pos[1] = 0;
	my_cam.pos[2] = 0;
	deg = ((M_PI*2)/180) * deg;	
	if(x == 1){
		GLfloat ypos = my_cam.at[1];
		GLfloat zpos = my_cam.at[2];
		my_cam.at[1] = ypos*cos(deg) - zpos*sin(deg);
		my_cam.at[2] = ypos*sin(deg) + zpos*cos(deg);

	}
	if(y == 1){
		GLfloat xpos = my_cam.at[0];
		GLfloat zpos = my_cam.at[2];
		my_cam.at[0] = xpos*cos(deg) - zpos*sin(deg);
		my_cam.at[2] = xpos*sin(deg) + zpos*cos(deg);
			
	}
	if(z == 1){
		GLfloat xpos = my_cam.at[0];
		GLfloat ypos = my_cam.at[1];
		my_cam.at[0] = xpos*cos(deg) - ypos*sin(deg); 
		my_cam.at[1] = xpos*sin(deg) + ypos*cos(deg);

	}
	my_cam.pos[0] = tempx;
	my_cam.pos[1] = tempy;
	my_cam.pos[2] = tempz;
}

void my_trans(GLfloat x, GLfloat y, GLfloat z) {
	my_cam.pos[0] += x;
	my_cam.at[0] += x;
	my_cam.pos[1] += y;
	my_cam.at[1] += y;
	my_cam.pos[2] += z;
	my_cam.at[2] += z;
}

void my_scale(GLfloat x, GLfloat y, GLfloat z) {

}

// reuse code from assignment 3 here to do transformations when reading spec
void real_translation(OBJECT *po, GLfloat x, GLfloat y, GLfloat z) {
	int i;
	int j;
	if (po->sid == 2)
	{
		for (i=0;i<9;i++)
		{
			po->vertices_house[i][0] += x;
			po->vertices_house[i][1] += y;
			po->vertices_house[i][2] += z;
		}
		return;
	}
	if (po->sid == 1)
	{
		for (i=0;i<8;i++)
		{
			po->vertices_cube_smart[i][0] += x;
			po->vertices_cube_smart[i][1] += y;
			po->vertices_cube_smart[i][2] += z;
		}
		return;
	}
	if (po->sid == 3)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				po->vertices_sph[i][j][0] += x;
				po->vertices_sph[i][j][1] += y;
				po->vertices_sph[i][j][2] += z;
			}
		}
		return;
	}
	if (po->sid == 4)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				po->vertices_cyl[i][j][0] += x;
				po->vertices_cyl[i][j][1] += y;
				po->vertices_cyl[i][j][2] += z;
			}
		}
		return;
	}
	if (po->sid == 6)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				po->vertices_tor[i][j][0] += x;
				po->vertices_tor[i][j][1] += y;
				po->vertices_tor[i][j][2] += z;
			}
		}
		return;
	}
	if (po->sid == 5)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				po->vertices_con[i][j][0] += x;
				po->vertices_con[i][j][1] += y;
				po->vertices_con[i][j][2] += z;
			}
		}
		return;
	}
}

void real_scaling(OBJECT *po, GLfloat sx, GLfloat sy, GLfloat sz) {
	int i;
	int j;
	if (po->sid == 2)
	{
		for (i=0;i<9;i++)
		{
			po->vertices_house[i][0] *= sx;
			po->vertices_house[i][1] *= sy;
			po->vertices_house[i][2] *= sz;
		}
		return;
	}
	if (po->sid == 1)
	{
		for (i=0;i<8;i++)
		{
			po->vertices_cube_smart[i][0] *= sx;
			po->vertices_cube_smart[i][1] *= sy;
			po->vertices_cube_smart[i][2] *= sz;
		}
		return;
	}
	if (po->sid == 3)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				po->vertices_sph[i][j][0] *= sx;
				po->vertices_sph[i][j][1] *= sy;
				po->vertices_sph[i][j][2] *= sz;
			}
		}
		return;
	}
	if (po->sid == 4)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				po->vertices_cyl[i][j][0] *= sx;
				po->vertices_cyl[i][j][1] *= sy;
				po->vertices_cyl[i][j][2] *= sz;
			}
		}
		return;
	}
	if (po->sid == 6)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				po->vertices_tor[i][j][0] *= sx;
				po->vertices_tor[i][j][1] *= sy;
				po->vertices_tor[i][j][2] *= sz;
			}
		}
		return;
	}
	if (po->sid == 5)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				po->vertices_con[i][j][0] *= sx;
				po->vertices_con[i][j][1] *= sy;
				po->vertices_con[i][j][2] *= sz;
			}
		}
		return;
	}
}

void real_rotation(OBJECT *po, GLfloat deg, GLfloat x, GLfloat y, GLfloat z) {
	int i;
	int j;
	double getx;
	double gety;
	double getz;
	if (po->sid == 2)
	{
		if (x == 1)
		{
			for (i=0;i<9;i++)
			{
				gety = po->vertices_house[i][1];
				getz = po->vertices_house[i][2];
				po->vertices_house[i][1] = (cos(deg) * gety) + (-sin(deg) * getz);
				po->vertices_house[i][2] = (sin(deg) * gety) + (cos(deg) * getz);
			}
		}
		else if (y == 1)
		{
			for (i=0;i<9;i++)
			{
			getx = po->vertices_house[i][0];
			getz = po->vertices_house[i][2];
			po->vertices_house[i][0] = (cos(deg) * getx) + (sin(deg) * getz);
			po->vertices_house[i][2] = (-sin(deg) * getx) + (cos(deg) * getz);
			}
		}
		else if (z == 1)
		{
			for (i=0;i<9;i++)
			{
			getx = po->vertices_house[i][0];
			gety = po->vertices_house[i][1];
			po->vertices_house[i][0] = (cos(deg) * getx) + (-sin(deg) * gety);
			po->vertices_house[i][1] = (sin(deg) * getx) + (cos(deg) * gety);
			}
		}
		return;
	}
	if (po->sid == 1)
	{
		for (i=0;i<9;i++)
		{
			if (x == 1)
			{
				gety = po->vertices_cube_smart[i][1];
				getz = po->vertices_cube_smart[i][2];
				po->vertices_cube_smart[i][1] = (cos(deg) * gety) + (-sin(deg) * getz);
				po->vertices_cube_smart[i][2] = (sin(deg) * gety) + (cos(deg) * getz);
			}
			else if (y == 1)
			{
				getx = po->vertices_cube_smart[i][0];
				getz = po->vertices_cube_smart[i][2];
				po->vertices_cube_smart[i][0] = (cos(deg) * getx) + (sin(deg) * getz);
				po->vertices_cube_smart[i][2] = (-sin(deg) * getx) + (cos(deg) * getz);
			}
			else if (z == 1)
			{
				getx = po->vertices_cube_smart[i][0];
				gety = po->vertices_cube_smart[i][1];
				po->vertices_cube_smart[i][0] = (cos(deg) * getx) + (-sin(deg) * gety);
				po->vertices_cube_smart[i][1] = (sin(deg) * getx) + (cos(deg) * gety);
			}
		}
		return;
	}
	if (po->sid == 3)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				if (x == 1)
				{
					gety = po->vertices_sph[i][j][1];
					getz = po->vertices_sph[i][j][2];
					po->vertices_sph[i][j][1] = (cos(deg) * gety) + (-sin(deg) * getz);
					po->vertices_sph[i][j][2] = (sin(deg) * gety) + (cos(deg) * getz);
				}
				else if (y == 1)
				{
					getx = po->vertices_sph[i][j][0];
					getz = po->vertices_sph[i][j][2];
					po->vertices_sph[i][j][0] = (cos(deg) * getx) + (sin(deg) * getz);
					po->vertices_sph[i][j][2] = (-sin(deg) * getx) + (cos(deg) * getz);
				}
				else if (z == 1)
				{
					getx = po->vertices_sph[i][j][0];
					gety = po->vertices_sph[i][j][1];
					po->vertices_sph[i][j][0] = (cos(deg) * getx) + (-sin(deg) * gety);
					po->vertices_sph[i][j][1] = (sin(deg) * getx) + (cos(deg) * gety);
				}
			}
		}
		return;
	}
	if (po->sid == 4)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				if (x == 1)
				{
					gety = po->vertices_cyl[i][j][1];
					getz = po->vertices_cyl[i][j][2];
					po->vertices_cyl[i][j][1] = (cos(deg) * gety) + (-sin(deg) * getz);
					po->vertices_cyl[i][j][2] = (sin(deg) * gety) + (cos(deg) * getz);
				}
				else if (y == 1)
				{
					getx = po->vertices_cyl[i][j][0];
					getz = po->vertices_cyl[i][j][2];
					po->vertices_cyl[i][j][0] = (cos(deg) * getx) + (sin(deg) * getz);
					po->vertices_cyl[i][j][2] = (-sin(deg) * getx) + (cos(deg) * getz);
				}
				else if (z == 1)
				{
					getx = po->vertices_cyl[i][j][0];
					gety = po->vertices_cyl[i][j][1];
					po->vertices_cyl[i][j][0] = (cos(deg) * getx) + (-sin(deg) * gety);
					po->vertices_cyl[i][j][1] = (sin(deg) * getx) + (cos(deg) * gety);
				}
			}
		}
		return;
	}
	if (po->sid == 5)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				if (x == 1)
				{
					gety = po->vertices_con[i][j][1];
					getz = po->vertices_con[i][j][2];
					po->vertices_con[i][j][1] = (cos(deg) * gety) + (-sin(deg) * getz);
					po->vertices_con[i][j][2] = (sin(deg) * gety) + (cos(deg) * getz);
				}
				else if (y == 1)
				{
					getx = po->vertices_con[i][j][0];
					getz = po->vertices_con[i][j][2];
					po->vertices_con[i][j][0] = (cos(deg) * getx) + (sin(deg) * getz);
					po->vertices_con[i][j][2] = (-sin(deg) * getx) + (cos(deg) * getz);
				}
				else if (z == 1)
				{
					getx = po->vertices_con[i][j][0];
					gety = po->vertices_con[i][j][1];
					po->vertices_con[i][j][0] = (cos(deg) * getx) + (-sin(deg) * gety);
					po->vertices_con[i][j][1] = (sin(deg) * getx) + (cos(deg) * gety);
				}
			}
		}
		return;
	}
	if (po->sid == 6)
	{
		for (i=0;i<50;i++)
		{
			for (j=0;j<50;j++)
			{
				if (x == 1)
				{
					gety = po->vertices_tor[i][j][1];
					getz = po->vertices_tor[i][j][2];
					po->vertices_tor[i][j][1] = (cos(deg) * gety) + (-sin(deg) * getz);
					po->vertices_tor[i][j][2] = (sin(deg) * gety) + (cos(deg) * getz);
				}
				else if (y == 1)
				{
					getx = po->vertices_tor[i][j][0];
					getz = po->vertices_tor[i][j][2];
					po->vertices_tor[i][j][0] = (cos(deg) * getx) + (sin(deg) * getz);
					po->vertices_tor[i][j][2] = (-sin(deg) * getx) + (cos(deg) * getz);
				}
				else if (z == 1)
				{
					getx = po->vertices_tor[i][j][0];
					gety = po->vertices_tor[i][j][1];
					po->vertices_tor[i][j][0] = (cos(deg) * getx) + (-sin(deg) * gety);
					po->vertices_tor[i][j][1] = (sin(deg) * getx) + (cos(deg) * gety);
				}
			}
		}
		return;
	}
}


void my_arrows(int key, int x, int y)
{
	switch( key ) {
   case GLUT_KEY_RIGHT:
    my_rev(-0.1,0,1,0);
    glutPostRedisplay();
    break;
   case GLUT_KEY_LEFT:
    my_rev(0.1, 0, 1, 0);
    glutPostRedisplay() ;
    break;
  case GLUT_KEY_UP:
    my_trans(0,0,-1);
    glutPostRedisplay();
    break;
  case GLUT_KEY_DOWN:
    my_trans(0,0,1);
    glutPostRedisplay();
    break;
  default: break;
	}
}

void startjump() 
{
	if (jumpamount < 2)
	{
		jumpamount++;
		yaccel = 30;
		my_cam.pos[1] += 0.0001;
	}
	
}
void my_keyboard( unsigned char key, int x, int y ) {
  
  switch( key ) {
  case ' ':
    startjump();
    glutPostRedisplay();
    break;
  case 'd':
    my_rev(-0.1,0,1,0);
    glutPostRedisplay();
    break;
  case 'a':
    my_rev(0.1, 0, 1, 0);
    glutPostRedisplay() ;
    break;
  case 'w':
    my_trans(0,0,-1);
    glutPostRedisplay();
    break;
  case 's':
    my_trans(0,0,1);
    glutPostRedisplay();
    break;

  case 'q': 
  case 'Q':
    exit(0) ;
    break ;	
  default: break;
  }
  
  return ;
}

void my_mouse_drag(int x, int y) {
}

int my_raytrace_cube(OBJECT *s, float *rayStart, float *rayDirection, float result[3])
{
	// If there is an intersection, this should set result to the closest one to the ray start.
	float t;
	if (rayDirection[0] != 0)
	{
		t = (-0.5-rayStart[0])/rayDirection[0];
		result[0] = rayStart[0] + (rayDirection[0] * t);
		result[1] = rayStart[1] + (rayDirection[1] * t);
		result[2] = rayStart[2] + (rayDirection[2] * t);
		if (result[1] <= 0.5 && result[1] >= -0.5 && result[2] <= 0.5 && result[2] >= -0.5)
			return 1;
		t = (-0.5+rayStart[0])/rayDirection[0];
		result[0] = rayStart[0] + (rayDirection[0] * t);
		result[1] = rayStart[1] + (rayDirection[1] * t);
		result[2] = rayStart[2] + (rayDirection[2] * t);
		if (result[0] <= 0.5 && result[0] >= -0.5 && result[2] <= 0.5 && result[2] >= -0.5)
			return 1;
	}
	if (rayDirection[1] != 0)
	{
		t = (-0.5-rayStart[1])/rayDirection[1];
		result[0] = rayStart[0] + (rayDirection[0] * t);
		result[1] = rayStart[1] + (rayDirection[1] * t);
		result[2] = rayStart[2] + (rayDirection[2] * t);
		if (result[0] <= 0.5 && result[0] >= -0.5 && result[2] <= 0.5 && result[2] >= -0.5)
			return 1;
		t = (-0.5+rayStart[1])/rayDirection[1];
		result[0] = rayStart[0] + (rayDirection[0] * t);
		result[1] = rayStart[1] + (rayDirection[1] * t);
		result[2] = rayStart[2] + (rayDirection[2] * t);
		if (result[0] <= 0.5 && result[0] >= -0.5 && result[2] <= 0.5 && result[2] >= -0.5)
			return 1;
	}
	if (rayDirection[2] != 0)
	{
		t = (-0.5-rayStart[2])/rayDirection[2];
		result[0] = rayStart[0] + (rayDirection[0] * t);
		result[1] = rayStart[1] + (rayDirection[1] * t);
		result[2] = rayStart[2] + (rayDirection[2] * t);
		if (result[0] <= 0.5 && result[0] >= -0.5 && result[1] <= 0.5 && result[1] >= -0.5)
			return 1;
		t = (-0.5+rayStart[2])/rayDirection[2];
		result[0] = rayStart[0] + (rayDirection[0] * t);
		result[1] = rayStart[1] + (rayDirection[1] * t);
		result[2] = rayStart[2] + (rayDirection[2] * t);
		if (result[0] <= 0.5 && result[0] >= -0.5 && result[1] <= 0.5 && result[1] >= -0.5)
			return 1;
	}
	// should return non-zero there is an intersection
	return 0;
}

int my_raytrace_sphere(OBJECT *s, float *rayStart, float *rayDirection, float result[3])
{
	float a = dotprod(rayDirection, rayDirection);
	float b = 2*dotprod(rayStart, rayDirection);
	float c = dotprod(rayStart, rayStart);

	float t1 = -b + sqrt(b*b-4*a*c) / (2*a);
	float t2 = -b - sqrt(b*b-4*a*c) / (2*a);

	result[0] = rayStart[0] + (rayDirection[0] * t1);
	result[1] = rayStart[1] + (rayDirection[1] * t1);
	result[2] = rayStart[2] + (rayDirection[2] * t1);
}

void my_raytrace(int mousex, int mousey)
{
	double modelViewMatrix[16];
	double projMatrix[16];
	int viewport[4];
	int foundIntersection = 0;
	int hit = 0;
	int i;
	double clickPoint[3];
	float intersectionPoint[3];
	float closestPoint[3];
	float rayStart[3];
	float rayDirection[3];
	float x;
	float y;
	float z;
	float dx;
	float dy;
	float dz;
	float t1;
	float t2;
	OBJECT *cur;

	// first we need to get the modelview matrix, the projection matrix, and the viewport
	glGetDoublev(GL_MODELVIEW_MATRIX, modelViewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	// gluUnProject with a Z value of 1 will find the point on the far clipping plane
	// corresponding the the mouse click. This is not the same as the vector
	// representing the click.
	gluUnProject(mousex, mousey, 1.0, modelViewMatrix, projMatrix, viewport, &clickPoint[0], &clickPoint[1], &clickPoint[2]);

	// Now we need a vector representing the click. It should start at the camera
	// position. We can subtract the click point, we will get the vector

	/* code for finding direction vector, set rayStart and rayDirection */
	x = clickPoint[0];
	y = clickPoint[1];
	z = clickPoint[2];
	dx = clickPoint[0] - my_cam.pos[0];
	if (dx != 0)
		dx = 1;
	else
		dx = 0;
	dy = clickPoint[1] - my_cam.pos[1];
	if (dy != 0)
		dy = 1;
	else
		dy = 0;
	dz = clickPoint[2] - my_cam.pos[2];
	if (dz != 0)
		dz = 1;
	else
		dz = 0;
	t1 = (((-2*x*dx) + (y*dy) + (z*dz)) + ((2*x*dx) + (y*dy) + (z*dz)) - (((4 * (dx*dx)) + (dy*dy) + (dz*dz)) * ((x*x) + (y*y) + (z*z) - 1))) / (2*((x*x) + (y*y) + (z*z) - 1));
	t2 = (((-2*x*dx) + (y*dy) + (z*dz)) - ((2*x*dx) + (y*dy) + (z*dz)) - (((4 * (dx*dx)) + (dy*dy) + (dz*dz)) * ((x*x) + (y*y) + (z*z) - 1))) / (2*((x*x) + (y*y) + (z*z) - 1));

	if (t2 < t1 && t2 > 0)
		t1 = t2;

	rayStart[0] = x + (dx * t1);
	rayStart[1] = y + (dy * t1);
	rayStart[2] = z + (dz * t1);

	rayDirection[0] = dx;
	rayDirection[1] = dy;
	rayDirection[2] = dz; 

//draw ray
  glLineWidth( 5.0 );

  glDisable(GL_LIGHTING);

  glBegin(GL_LINES); 
  {
		
    glColor3fv(colors[6]);
    glVertex3fv(&rayDirection[0]);
    glVertex3fv(&rayDirection[3]);

  }
  glEnd();
  glLineWidth( 1.0 );

  glEnable(GL_LIGHTING);
//end drawing ray

	// now go through the shapes and see if there is a hit
	for (i=0; i<num_objects; i++)
	{
		cur = my_objects + i;
		hit = 0;

		switch (cur->sid)
		{
		case 1:
			hit = my_raytrace_cube(cur, rayStart, rayDirection, intersectionPoint);
			break;
		case 2:
			hit = my_raytrace_sphere(cur, rayStart, rayDirection, intersectionPoint);
		default:
			break;
		}

		// found intersection
		if (hit)
		{
			if (foundIntersection)
			{
				// check to see if it is the closer than the closest one we found,
				// and if it is, store it in closestPoint
				float closest;
				float intersect;
				closest = closestPoint[0] + closestPoint[1] + closestPoint[2];
				intersect = intersectionPoint[0] + intersectionPoint[1] + intersectionPoint[2];
				if (intersect < closest)
				{
					closestPoint[0] = intersectionPoint[0];
					closestPoint[1] = intersectionPoint[1];
					closestPoint[2] = intersectionPoint[2];
				}
			}

			else
			{
				closestPoint[0] = intersectionPoint[0];
				closestPoint[1] = intersectionPoint[1];
				closestPoint[2] = intersectionPoint[2];
			}

			foundIntersection = 1;
	 }
	}

	if (foundIntersection)
	{
		printf("Intersected with object %s at (%f, %f, %f)\n", "object_name", closestPoint[0], closestPoint[1], closestPoint[2]);
	}
	else
	{
		printf("No intersection\n");
	}
}

void my_mouse(int button, int state, int mousex, int mousey) {

  switch( button ) {
	
  case GLUT_LEFT_BUTTON:
    if( state == GLUT_DOWN ) {
		
		my_raytrace(mousex, mousey);
    }
    
    if( state == GLUT_UP ) {
    }
    break ;

  case GLUT_RIGHT_BUTTON:
    if ( state == GLUT_DOWN ) {
    }
    
    if( state == GLUT_UP ) {
    }
    break ;
  }
  

  
  return ;
}

float dotprod(float v1[], float v2[]) {
  float tot = 0;
  int i;
  for (i=0; i<4; i++)
    tot += v1[i]*v2[i];
  return tot;
}


void normalize(GLfloat *p) { 
  double d=0.0;
  int i;
  for(i=0; i<3; i++) d+=p[i]*p[i];
  d=sqrt(d);
  if(d > 0.0) for(i=0; i<3; i++) p[i]/=d;
}


void cross(GLfloat *a, GLfloat *b, GLfloat *c, GLfloat *d) { 
  d[0]=(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
  d[1]=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
  d[2]=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
  normalize(d);
}

void print_matrix(float my_matrix[])
{ 
  int i, j;

  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) {
      printf ("%f ", my_matrix[i+j*4]);
    }
    printf ("\n");
  }
  printf ("\n");
}

void make_quad(GLfloat vertices[][3]) {
  glBegin(GL_POLYGON); 
  {
    glVertex3fv(vertices[0]);
    glVertex3fv(vertices[1]);
    glVertex3fv(vertices[2]);
    glVertex3fv(vertices[3]);
  }
  glEnd();
}

/*************************************
 FUNCTION: make_*; reuse your stitcher code here.
*************************************/
void make_cube_smart(OBJECT *po, double size){
  int i;  
  // compute verts on PI/4 angels for x y z, then -x y z  
  for(i = 0; i < 4; i++){
    po->vertices_cube_smart[i][0] = size*cos((M_PI/4));
    po->vertices_cube_smart[i][1] = -size*sin(i*(M_PI/2)+(M_PI/4));
    po->vertices_cube_smart[i][2] = size*cos(i*(M_PI/2)+(M_PI/4));
    po->vertices_cube_smart[i][3] = 1;
    // mirror on x axis
    po->vertices_cube_smart[i+4][0] = -size*cos((M_PI/4));
    po->vertices_cube_smart[i+4][1] = -size*sin(i*(M_PI/2)+(M_PI/4));
    po->vertices_cube_smart[i+4][2] = size*cos(i*(M_PI/2)+(M_PI/4));
    po->vertices_cube_smart[i+4][3] = 1;
  }

  //compute normals
    cross(po->vertices_cube_smart[0], po->vertices_cube_smart[1], po->vertices_cube_smart[2], po->normals_cube_smart[0]);
    cross(po->vertices_cube_smart[1], po->vertices_cube_smart[5], po->vertices_cube_smart[6], po->normals_cube_smart[1]);
    cross(po->vertices_cube_smart[5], po->vertices_cube_smart[4], po->vertices_cube_smart[7], po->normals_cube_smart[2]);
    cross(po->vertices_cube_smart[4], po->vertices_cube_smart[0], po->vertices_cube_smart[3], po->normals_cube_smart[3]);
}
void make_sphere(OBJECT *po)
{
	int i;
	int j;
	double theta;
	double phi;
	for (i=0;i<=crt_rs;i++)
	{
		for (j=0;j<=crt_vs;j++)
		{
			phi = ((M_PI * 2) / crt_rs) * j;
			theta = (i * (M_PI / crt_vs)) - (M_PI / 2);
			po->vertices_sph[i][j][0] = cos(phi) * cos(theta);
			po->vertices_sph[i][j][1] = sin(theta);
			po->vertices_sph[i][j][2] = sin(phi) * cos(theta);
			po->vertices_sph[i][j][3] = 1; //always should be in right order?
		}
	}
}

void make_cylinder(OBJECT *po)
{
	int i;
	int j;
	double h = 5;
	double theta;
	for (i=0;i<=crt_rs;i++)
	{
		for (j=0;j<=crt_vs;j++)
		{
			h = (1.0 / crt_rs) * i;
			theta = ((M_PI * 2) / crt_vs) * j;
			po->vertices_cyl[i][j][0] = cos(theta);
			po->vertices_cyl[i][j][1] = (h * height) - .5;
			po->vertices_cyl[i][j][2] = -sin(theta);
			po->vertices_cyl[i][j][3] = 1;
		}
	}
}

void make_cone(OBJECT *po)
{
	int i;
	int j;
	double h = 5;
	double theta;
	for (i=0;i<=crt_vs;i++)
	{
		for (j=0;j<=crt_rs;j++)
		{
			h = (1.0 / crt_vs) * i;
			theta = ((M_PI * 2) / crt_rs) * j;
			po->vertices_con[j][i][0] = cos(theta) * (1-h);
			po->vertices_con[j][i][1] = (height * h) - .5;
			po->vertices_con[j][i][2] = -sin(theta) * (1-h);
			po->vertices_con[j][i][3] = 1;
		}
	}
}

void make_torus(OBJECT *po)
{
	int i;
	int j;
	double theta;
	double phi;
	for (i=0;i<=crt_vs;i++)
	{
		for (j=0;j<=crt_rs;j++)
		{
			phi = ((M_PI * 2) / crt_rs) * j;
			theta = ((M_PI * 2) / crt_vs) * i;
			po->vertices_tor[j][i][0] = ((cos(theta)) + 2) * cos(phi);
			po->vertices_tor[j][i][1] = sin(theta);
			po->vertices_tor[j][i][2] = ((cos(theta)) + 2) * -sin(phi);
			po->vertices_tor[j][i][3] = 1;
		}
	}
}

/***********************************
 FUNCTION: draw_quad 
 ARGS: - a vertex array
 - 4 indices into the vertex array defining a quad face
 - an index into the color array.
 RETURN: none
 DOES:  helper drawing function; draws one quad. 
 For the normal to work out, follow left-hand-rule (i.e., counter clock
 wise) 
*************************************/
void draw_quad(GLfloat vertices[][4], GLfloat *normals, int iv1, int iv2, int iv3, int iv4, int ic) {
  glBegin(crt_render_mode); 
  {
    glColor3fv(colors[ic]);
    glNormal3fv(normals);
    /*note the explicit use of homogeneous coords below: glVertex4f*/
    glVertex4fv(vertices[iv1]);
    glVertex4fv(vertices[iv2]);
    glVertex4fv(vertices[iv3]);
    glVertex4fv(vertices[iv4]);
  }
  glEnd();
}

void draw_param_quad(GLfloat vertices[][50][4], int line, int col, int ic) {
  glBegin(crt_render_mode); 
  {
    glColor3fv(colors[ic]);
    /*note the explicit use of homogeneous coords below: glVertex4f*/
    glVertex4fv(vertices[line][col]);
    glVertex4fv(vertices[line+1][col]);
    glVertex4fv(vertices[line+1][col+1]);
    glVertex4fv(vertices[line][col+1]);
  }
  glEnd();
}

/***********************************
 FUNCTION: draw_cube_smart() 
 ARGS: none
 RETURN: none
 DOES: draws a cube from quads
************************************/
void draw_cube_smart(OBJECT *po)
{
  /* sides */
  draw_quad(po->vertices_cube_smart, po->normals_cube_smart[0],0,1,2,3, BLUE);
  draw_quad(po->vertices_cube_smart, po->normals_cube_smart[1],1,5,6,2, BLUE);
  draw_quad(po->vertices_cube_smart, po->normals_cube_smart[2],5,4,7,6, BLUE);
  draw_quad(po->vertices_cube_smart, po->normals_cube_smart[3],4,0,3,7, BLUE);
    
  /* top and bottom */
  //draw_quad(po->vertices_cube_smart,3,2,6,7, BLUE);
  //draw_quad(po->vertices_cube_smart,0,1,5,4, BLUE);
}

void draw_sphere(OBJECT *po)
{
  int i;
  int j;
  for (i=0;i<crt_rs;i++)
  {
	  for (j=0;j<crt_vs;j++)
	  {
		draw_param_quad(po->vertices_sph, i, j, RED);
	  }
  }
}

void draw_cylinder(OBJECT *po)
{
  int i;
  int j;
  for (i=0;i<crt_rs;i++)
  {
	  for (j=0;j<crt_vs;j++)
	  {
		draw_param_quad(po->vertices_cyl, i, j, GREEN);
	  }
  }
}

void draw_cone(OBJECT *po)
{
  int i;
  int j;
  for (i=0;i<crt_rs;i++)
  {
	  for (j=0;j<crt_vs;j++)
	  {
		draw_param_quad(po->vertices_con, i, j, CYAN);
	  }
  }
}

void draw_torus(OBJECT *po)
{
  int i;
  int j;
  for (i=0;i<crt_rs;i++)
  {
	  for (j=0;j<crt_vs;j++)
	  {
		draw_param_quad(po->vertices_tor, i, j, YELLOW);
	  }
  }
}

/***********************************
 FUNCTION: draw_axes
 ARGS: none
 RETURN: none
 DOES: draws main X, Y, Z axes
************************************/
void draw_axes( void ) {
  glLineWidth( 5.0 );

  glDisable(GL_LIGHTING);

  glBegin(GL_LINES); 
  {
    glColor3fv(colors[1]);
    glVertex4fv(vertices_axes[0]);
    glVertex4fv(vertices_axes[1]);
		
    glColor3fv(colors[4]);
    glVertex4fv(vertices_axes[0]);
    glVertex4fv(vertices_axes[2]);
		
    glColor3fv(colors[6]);
    glVertex4fv(vertices_axes[0]);
    glVertex4fv(vertices_axes[3]);

  }
  glEnd();
  glLineWidth( 1.0 );

  glEnable(GL_LIGHTING);
	
}

void draw_objects() {
  int i;
  for(i=0; i<num_objects; i++){
    OBJECT *cur;
    cur = &my_objects[i];

    glMaterialfv(GL_FRONT, GL_AMBIENT, cur->amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, cur->diff);
    glMaterialfv(GL_FRONT, GL_SPECULAR, cur->spec);
    glMaterialfv(GL_FRONT, GL_SHININESS, &cur->shine);
    //glMaterialfv(GL_FRONT, GL_EMISSION, cur->emi);

  switch(cur->sid){
  	case 1: //cube
    	draw_cube_smart(cur);
    	break;
	case 3:
		draw_sphere(cur);
		break;
	case 4:
		draw_cylinder(cur);
		break;
	case 5:
		draw_cone(cur);
		break;
	case 6:
		draw_torus(cur);
		break;
  	default: break;
  }

  }
}

void my_display() {

  // clear all pixels, reset depth 
  glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT );
  
  glLoadIdentity();
  //setup the camera (1st person? 3rd person?)

  gluLookAt(my_cam.pos[0],my_cam.pos[1], my_cam.pos[2],
	    my_cam.at[0],my_cam.at[1],my_cam.at[2],
	    my_cam.up[0], my_cam.up[1], my_cam.up[2]);

  //update the flashlight to follow the person
  
  //draw the objects
  draw_axes();

  draw_objects();

  // this buffer is ready
  glutSwapBuffers();
}

void my_idle(void) {
  //EC: double jumping
	if (my_cam.pos[1] > 0)
	{
		yaccel -= 0.8;
		my_cam.pos[1] += 0.001 * yaccel;
		my_cam.at[1] = my_cam.pos[1];
	}
	else
	{
		yaccel = 0;
		my_cam.pos[1] = 0;
		my_cam.at[1] = 0;
		jumpamount = 0;
	}
	glutPostRedisplay();
  return ;
}
