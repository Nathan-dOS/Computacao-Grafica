#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "GL/glut.h"

#define MAXVERTEXS 30 
#define NPOLYGON 6 
#define PHI 3.141572 

//  tipo de curvas ------
#define HERMITE 1
#define BEZIER 2
#define BSPLINE 3
#define CATMULLR 4

//  tipo de transformação ------
#define TRANSLACAO 1
#define ROTACAO 2
#define SCALA 3
#define CISALHA 4

GLenum doubleBuffer;    

typedef struct spts 
{ 
    float v[3];
} tipoPto; 

tipoPto ptsCurva[MAXVERTEXS];
tipoPto ptsContrle[MAXVERTEXS];

int windW, windH;
int nPtsCtrole, nPtsCurva; // nppuntos;

int jaCurva = 0;
int ptoSelect = -1;
int tipoCurva = 0;
int tipoTransforma = 0;

	// matriz de trabalho
float M[4][4] = 
	{{0.0, 0.0, 0.0, 0.0},
	 {0.0, 0.0, 0.0, 0.0},
	 {0.0, 0.0, 0.0, 0.0},
	 {0.0, 0.0, 0.0, 0.0}};

	// matriz de Hermite
float Mh[4][4] = 
	{{1.0, 0.0, 0.0, 0.0},
	 {0.0, 1.0, 0.0, 0.0},
	 {0.0, 0.0, 1.0, 0.0},
	 {0.0, 0.0, 0.0, 1.0}};

	// matriz de Bezier
float Mb[4][4] = 
	{{1.0, 0.0, 0.0, 0.0},
	 {0.0, 1.0, 0.0, 0.0},
	 {0.0, 0.0, 1.0, 0.0},
	 {0.0, 0.0, 0.0, 1.0}};

	// matriz de B-spline  * 6
float Ms[4][4] = 
	{{1.0, 0.0, 0.0, 0.0},
	 {0.0, 1.0, 0.0, 0.0},
	 {0.0, 0.0, 1.0, 0.0},
	 {0.0, 0.0, 0.0, 1.0}};

	// matriz de Catmull Rom  * 2
float Mc[4][4] = 
	{{-1.0, 3.0, -3.0, 1.0},
	 {2.0, -5.0, 4.0, -1.0},
	 {-1.0, 0.0, 1.0, 0.0},
	 {0.0, 2.0, 0.0, 0.0}};

float MCor[9][3] =
	{
	 {1.0, 0.5, 0.0},
	 {0.8, 0.7, 0.5},
	 {0.5, 0.5, 0.5},
	 {0.5, 1.0, 0.5},
	 {0.5, 0.5, 1.0},
	 {1.0, 0.5, 1.0},
	 {0.0, 0.0, 1.0},
	 {0.0, 1.0, 0.0},
	 {1.0, 0.0, 0.0}
	};
int nCors = 9;


void ptoCurva(float t, int j, float pp[3])
{
    int i, ji;
	float cc;
    tipoPto ptsCont[MAXVERTEXS];

	pp[0]=pp[1]=pp[2]=0.0;

		// preparando os pontos de controle por causa de hermite
	for(i=0; i<4; i++)
	{
		ji = (j+i)%nPtsCtrole;
		ptsCont[i].v[0] = ptsContrle[ji].v[0];
		ptsCont[i].v[1] = ptsContrle[ji].v[1];
		ptsCont[i].v[2] = ptsContrle[ji].v[2];
	}
	
	if(tipoCurva == HERMITE) {
		ptsCont[j+1].v[0] = ptsContrle[j+3].v[0];
		ptsCont[j+1].v[1] = ptsContrle[j+3].v[1];
		ptsCont[j+1].v[2] = ptsContrle[j+3].v[2];
		
		ptsCont[j+2].v[0] = ptsContrle[j+0].v[0] - ptsContrle[j+1].v[0];
		ptsCont[j+2].v[1] = ptsContrle[j+0].v[1] - ptsContrle[j+1].v[1];
		ptsCont[j+2].v[2] = ptsContrle[j+0].v[2] - ptsContrle[j+1].v[2];

		ptsCont[j+3].v[0] = ptsContrle[j+3].v[0] - ptsContrle[j+2].v[0];
		ptsCont[j+3].v[1] = ptsContrle[j+3].v[1] - ptsContrle[j+2].v[1];
		ptsCont[j+3].v[2] = ptsContrle[j+3].v[2] - ptsContrle[j+2].v[2];
	}

	for(i=0; i<4; i++)
	{
		cc = t*t*t*M[0][i] + t*t*M[1][i] + t*M[2][i] + M[3][i];
		pp[0] += cc * ptsCont[i].v[0];
		pp[1] += cc * ptsCont[i].v[1];
		pp[2] += cc * ptsCont[i].v[2];
	}
}

void init(void)
{
	int i;

	jaCurva = 0;

	nPtsCtrole = 0;
	nPtsCurva = 0;

	for(i=0; i<MAXVERTEXS; i++)
	{
		ptsContrle[i].v[0] = 0.0;
		ptsContrle[i].v[1] = 0.0;
		ptsContrle[i].v[2] = 0.0;

		ptsCurva[i].v[0] = 0.0;
		ptsCurva[i].v[1] = 0.0;
		ptsCurva[i].v[2] = 0.0;
	}
}


static void Key(unsigned char key, int x, int y)
{
    switch (key) 
	{
      case 27:
			exit(0);
    }
}

void coord_line(void)
{
    glLineWidth(1);

	glColor3f(1.0, 0.0, 0.0);

	// vertical line

	glBegin(GL_LINE_STRIP);
		glVertex2f(-windW, 0);
		glVertex2f(windW, 0);
    glEnd();

	// horizontal line 

    glBegin(GL_LINE_STRIP);
		glVertex2f(0, -windH);
		glVertex2f(0, windH);
    glEnd();
}

void verticesDraw(int n, tipoPto vertices[MAXVERTEXS], float cr, float cg, float cb)
{
	int i;
	glColor3f(cb, cg, cr);
	glPointSize(3);
	glBegin(GL_POINTS); 
	for(i=0; i<n; i++)
		glVertex2f(vertices[i].v[0], vertices[i].v[1]);
	glEnd();
}

void Poligono(int n, tipoPto vertices[MAXVERTEXS], float cb, float cg, float cr )
{
	int i;

	glColor3f(cb, cg, cr); 

	glBegin(GL_LINE_STRIP);
	for(i=0; i<n; i++) {
		glVertex2fv(vertices[i].v);
	}
	glEnd();
}

void geraCurva(int j)
{
	float t=0.0;
	float dt;

	if (nPtsCtrole>3)
	{
		dt = 1.0f / ((float)(MAXVERTEXS - 1));
		for(nPtsCurva = 0; nPtsCurva<MAXVERTEXS; nPtsCurva++)
		{
			t = (float)(nPtsCurva)*dt;
			ptoCurva(t, j, ptsCurva[nPtsCurva].v);
		}
		jaCurva = 1;
	}
}

static void Draw(void)
{
	int c, j=0;
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

	coord_line();	                                       // define eixos do sistema cartesiano
	
	verticesDraw(nPtsCtrole, ptsContrle, 0.0, 1.0, 0.0);   // Mostra os pontos de controle
	Poligono(nPtsCtrole, ptsContrle, 0.0, 0.0, 0.0);	   // mostra o polígono de controle

	if(jaCurva)		                                       // opcao ativa para as curvas
		while(j<nPtsCtrole){
			geraCurva(j);                                  // gera o pedaço da curva
			c = j%(nCors-3);		                       // define indico da cor do pedaço
				// mostra o pedaço da curva
			Poligono(nPtsCurva, ptsCurva, MCor[c][0], MCor[c][1], MCor[c][2]);	 
			j++;
		}

    if (doubleBuffer) 
	{
	   glutSwapBuffers(); 
    } else {
	   glFlush();     
    }
}

static void Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_FALSE;

    for (i = 1; i < argc; i++) 
	{
	   if (strcmp(argv[i], "-sb") == 0)   
	   {
	      doubleBuffer = GL_FALSE;
	   } else if (strcmp(argv[i], "-db") == 0) 
	   {
	      doubleBuffer = GL_TRUE;
	   }
    }
}

void processMenuCurvas(int option) 
{
	int i, j;

	tipoCurva = option;
	if (nPtsCtrole>3) {
		switch (option) 
		{
			case HERMITE: 
				jaCurva = 1;
				for(i=0; i<4; i++)           // M := Mh  <--  como matiz de HERMITE
					for(j=0; j<4; j++)
						M[i][j] = Mh[i][j];
				break;

			case BEZIER:
				jaCurva=1;
				for(i=0; i<4; i++)           // M := Mb  <-- copia matriz de BEZIER
					for(j=0; j<4; j++)
						M[i][j] = Mb[i][j];
				break;

			case BSPLINE:
				jaCurva=1;
				for(i=0; i<4; i++)           // M := Ms  <-- copia matriz de BSPLINE
					for(j=0; j<4; j++)
						M[i][j] = Ms[i][j] / 6;
				break;

			case CATMULLR: 
				jaCurva = 1;
				for(i=0; i<4; i++)            // M := Mc  <-- copia matriz de CatMULLR
					for(j=0; j<4; j++)
						M[i][j] = Mc[i][j] / 2;
				break;
		break;
		}	
	}
	glutPostRedisplay(); 
}

void processMenuTransforma(int option) 
{
	printf("Transforma opcao %d \n", option);

	tipoTransforma = option;

	// calcula 

	switch (option) 
	{
		case TRANSLACAO:  
			printf("\n Translacao....\n");
			break;

		case ROTACAO:
			printf("\n Rotacao....\n");
			break;

		case SCALA:
			printf("\n Scala....\n");
			break;

		case CISALHA: 
			printf("\n Cisalha....\n");
			break;

		break;
	}

	glutPostRedisplay();
}

void processMenuEvents(int option) 
{
	printf("Menu opcao %d \n", option); 

	switch (option) 
	{
		case 1: Poligono(nPtsCtrole, ptsContrle, 0.0, 1.0, 0.0); break;
		case 2: init();
		break;
	}
	glutPostRedisplay();
}


void createGLUTMenus() 
{
	int menu, submenu1, submenu2;

	submenu1 = glutCreateMenu(processMenuCurvas);
	glutAddMenuEntry("Hermite", HERMITE);
	glutAddMenuEntry("Bezier", BEZIER);
	glutAddMenuEntry("B-Spline", BSPLINE);
	glutAddMenuEntry("CatmullR", CATMULLR);

    submenu2 = glutCreateMenu(processMenuTransforma);
	glutAddMenuEntry("Translacao", TRANSLACAO);
	glutAddMenuEntry("Rotacao", ROTACAO);
	glutAddMenuEntry("Scala", SCALA);
	glutAddMenuEntry("Cisalha", CISALHA);
	glutAddMenuEntry("Manipular Pto", 0);

	menu = glutCreateMenu(processMenuEvents);
	glutAddMenuEntry("Poligono de Control",1);
	glutAddSubMenu("Tipo de Curva",submenu1);
	glutAddSubMenu("Transformacao",submenu2);
	glutAddMenuEntry("Limpiar Todo...",2);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
} 

void motion(int x, int y)
{
	x = x - windW; y = windH - y;
	if(jaCurva)
		if(!tipoTransforma)
		{
			ptsContrle[ptoSelect].v[0] = (float)x;
			ptsContrle[ptoSelect].v[1] = (float)y;
		}
		else
		{
			printf(" transformando, ");
			switch (tipoTransforma) 
			{
				case TRANSLACAO:  printf(" Translacao, ");
					//rocTranslacao();
					break;

				case ROTACAO:   printf(" Rotacao, ");
					//procRotacao();
					break;

				case SCALA:    printf(" Scala, ");
				//	procScala();
					break;

				case CISALHA:   printf(" Cisalha, ");
				//	procCisalha();
					break;

				break;
			}
		}

//	CurveDraw();

	glutPostRedisplay();
}

int buscaPuntoClick(int x, int y)
{
	int i;
	float dx, dy, dd;

	ptoSelect = -1;
	for(i=0; i<nPtsCtrole; i++)
	{
		dx = ptsContrle[i].v[0] - (float)x;
		dy = ptsContrle[i].v[1] - (float)y;
		dd = sqrt(dx*dx + dy*dy);
		if(dd < 3.00)
		{
			ptoSelect = i;
			break;
		}
	}
	return ptoSelect;
}

void mouse(int button, int state, int x, int y)
{ 
	   // esta aqui porque ocorreu um evento: Buttom de mause foi presionado ou solto 
	   // estando o cursor na posição (x, y) de canvas
	if(button == GLUT_LEFT)
		if(state == GLUT_DOWN)
		{
			x = x - windW; 
			y = windH - y;
			if(!jaCurva)        // ainda esta na geracao dos pontos de controle
			{
				// gera UM ponto de controle por cada CLICK
				glColor3f(0.0, 1.0, 0.0);
				glPointSize(3);
				glBegin(GL_POINTS); 
				glVertex2i(x, y);
				glEnd();

				ptsContrle[nPtsCtrole].v[0] = (float)x;
				ptsContrle[nPtsCtrole].v[1] = (float)y;
				nPtsCtrole++;
			}
			else              // todos os pontos de controle ja estao definidos
			{
				// busca o ponto selecionado para mover com mouse
				ptoSelect = buscaPuntoClick(x, y);
				if((ptoSelect>=0)&&(ptoSelect<nPtsCtrole))
				{
					glColor3f(1.0, 0.0, 0.0);
					glPointSize(4);
					glBegin(GL_POINTS); 
					glVertex2i(x, y);
					glEnd();
				}
			}
		}
}

static void Reshape(int width, int height)
{
    windW = width/2;
    windH = height/2;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluOrtho2D(-windW, windW, -windH, windH);

    glMatrixMode(GL_MODELVIEW);

	Draw();
}

int main(int argc, char **argv)
{
    GLenum type;

    glutInit(&argc, argv);
    Args(argc, argv);

    type = GLUT_RGB;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;

    glutInitDisplayMode(type);
    glutInitWindowSize(600, 500);
    glutCreateWindow("Basic Program Using Glut and Gl");

	init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);

	glutMotionFunc(motion);
	glutMouseFunc(mouse);

	createGLUTMenus();  

    glutMainLoop();

	return (0);
}
