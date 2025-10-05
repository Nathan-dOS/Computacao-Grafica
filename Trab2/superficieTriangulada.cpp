// superficieTriangulada.cpp
// Versão triangulada com 2 fontes de luz e sombreamento por triângulo
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define Linha -1
#define Solido -2
#define Pontos -3

#define Redimensionar 4
#define GirarX 13
#define GirarY 14
#define GirarZ 15

#define BEZIER 20
#define BSPLINE 21
#define CATMULLROM 22

#define PtsControle 19

#define OBJ_CILINDRO 30
#define OBJ_CUBO     31
#define OBJ_ESFERA   32

#define sair 0

#define X 0
#define Y 1
#define Z 2

typedef float f4d[4];

typedef struct st_matriz
{
    int n, m;
    f4d **ponto;
} matriz;

int comando = GirarX;

int tipoView = GL_LINE_STRIP;

float local_scale = 0.22f;

float VARIA = 0.04f;


f4d AuxVertex[3];

f4d MatBase[4];   // matriz de base

f4d pview = {10.0, 10.0, -20.0, 0.0};

f4d vcolor[4] = {{1.0, 0.2, 0.2, 0.0},
                 {0.2, 1.0, 0.2, 0.0},
                 {0.2, 0.2, 1.0, 0.0},
                 {1.0, 1.0, 0.2, 0.0}};


matriz *pc = NULL;  //matriz de pontos de controle
matriz *pcPatch = NULL;  // matriz de pontos para um patch
matriz *pMatriz = NULL;

// duas fontes de luz (world coordinates)
f4d lightPos1 = {30.0f, 30.0f, 30.0f, 1.0f}; // luz principal
f4d lightPos2 = {-20.0f, 10.0f, -10.0f, 1.0f}; // luz secundária

void DisenaSuperficie(void);

void MontaMatrizBase(int tipoSup)
{
    if(tipoSup==BEZIER)
    {
        MatBase[0][0] = -1.0; MatBase[0][1] = 3.0;  MatBase[0][2] = -3.0; MatBase[0][3] = 1.0;
        MatBase[1][0] =  3.0; MatBase[1][1] = -6.0; MatBase[1][2] =  3.0; MatBase[1][3] = 0.0;
        MatBase[2][0] = -3.0; MatBase[2][1] = 3.0;  MatBase[2][2] =  0.0; MatBase[2][3] = 0.0;
        MatBase[3][0] =  1.0; MatBase[3][1] = 0.0;  MatBase[3][2] =  0.0; MatBase[3][3] = 0.0;
    }

    if(tipoSup==BSPLINE)
    {
        MatBase[0][0] = -1.0/6.0; MatBase[0][1] = 3.0/6.0;  MatBase[0][2] = -3.0/6.0; MatBase[0][3] = 1.0/6.0;
        MatBase[1][0] =  3.0/6.0; MatBase[1][1] = -6.0/6.0; MatBase[1][2] =  3.0/6.0; MatBase[1][3] = 0.0/6.0;
        MatBase[2][0] = -3.0/6.0; MatBase[2][1] = 0.0/6.0;  MatBase[2][2] =  3.0/6.0; MatBase[2][3] = 0.0/6.0;
        MatBase[3][0] =  1.0/6.0; MatBase[3][1] = 4.0/6.0;  MatBase[3][2] =  1.0/6.0; MatBase[3][3] = 0.0/6.0;
    }

    if(tipoSup==CATMULLROM)
    {
        MatBase[0][0] = -1.0/2.0; MatBase[0][1] = 3.0/2.0;  MatBase[0][2] = -3.0/2.0; MatBase[0][3] = 1.0/2.0;
        MatBase[1][0] =  2.0/2.0; MatBase[1][1] = -5.0/2.0; MatBase[1][2] =  4.0/2.0; MatBase[1][3] = -1.0/2.0;
        MatBase[2][0] = -1.0/2.0; MatBase[2][1] = 0.0/2.0;  MatBase[2][2] =  1.0/2.0; MatBase[2][3] = 0.0/2.0;
        MatBase[3][0] =  0.0/2.0; MatBase[3][1] = 2.0/2.0;  MatBase[3][2] =  0.0/2.0; MatBase[3][3] = 0.0/2.0;
    }

}

matriz* liberaMatriz(matriz* sup)
{
    int i;

    if(sup)
    {
        for(i=0; i< sup->n; i++)
        {
            free(sup->ponto[i]);
        }
        free(sup->ponto);
        free(sup);
    }
    return NULL;
}


matriz* AlocaMatriz(int n, int m)
{
    matriz *matTemp;
    int j;

    if((matTemp = (matriz*) malloc(sizeof(matriz)))==NULL)
    {
        printf("\n Error en alocacion de memoria para uma matriz");
        return 0;
    }

    matTemp->n = n;
    matTemp->m = m;
    matTemp->ponto = (f4d**) calloc(n, sizeof(f4d*));

    for(j=0; j<matTemp->n; j++)
         matTemp->ponto[j] = (f4d*) calloc(m, sizeof(f4d));

    return matTemp;
}


void MatrizIdentidade()
{
    int a,b;
    for(a=0; a<3; a++)
    {   for(b=0; b<3; b++)
        {   if(a==b)
            {   AuxVertex[a][b] = 1;
            }
            else
            {   AuxVertex[a][b] = 0;
            }
        }
    }
}

void MultMatriz()
{
    int j,k;
    f4d auxiliar;
    for(j=0; j< pc->n; j++)
    {
        for(k = 0; k< pc->m; k++)
        {
         auxiliar[X] = pc->ponto[j][k][X];
         auxiliar[Y] = pc->ponto[j][k][Y];
         auxiliar[Z] = pc->ponto[j][k][Z];

         pc->ponto[j][k][X] = auxiliar[X] * AuxVertex[0][X] +
                              auxiliar[Y] * AuxVertex[1][X] +
                              auxiliar[Z] * AuxVertex[2][X];

         pc->ponto[j][k][Y] = auxiliar[X] * AuxVertex[0][Y] +
                              auxiliar[Y] * AuxVertex[1][Y] +
                              auxiliar[Z] * AuxVertex[2][Y];

         pc->ponto[j][k][Z] = auxiliar[X] * AuxVertex[0][Z] +
                              auxiliar[Y] * AuxVertex[1][Z] +
                              auxiliar[Z] * AuxVertex[2][Z];
        }
    }

    for(j=0; j< pMatriz->n; j++)
    {
      for(k = 0; k< pMatriz->m; k++)
      {
         auxiliar[X] = pMatriz->ponto[j][k][X];
         auxiliar[Y] = pMatriz->ponto[j][k][Y];
         auxiliar[Z] = pMatriz->ponto[j][k][Z];

         pMatriz->ponto[j][k][X] = auxiliar[X] * AuxVertex[0][X] +
                                   auxiliar[Y] * AuxVertex[1][X] +
                                   auxiliar[Z] * AuxVertex[2][X];

         pMatriz->ponto[j][k][Y] = auxiliar[X] * AuxVertex[0][Y] +
                                   auxiliar[Y] * AuxVertex[1][Y] +
                                   auxiliar[Z] * AuxVertex[2][Y];

         pMatriz->ponto[j][k][Z] = auxiliar[X] * AuxVertex[0][Z] +
                                   auxiliar[Y] * AuxVertex[1][Z] +
                                   auxiliar[Z] * AuxVertex[2][Z];
      }
    }
}

void prod_VetParam_MatBase(float x, float *xx, float *vr)
{
    int i, j;

    xx[0] = pow(x,3);
    xx[1] = pow(x,2);
    xx[2] = x;
    xx[3] = 1.0;

    for(i=0; i<4; i++)
    {
        vr[i] = 0.0f;
        for(j=0; j<4; j++)
            vr[i] += MatBase[j][i] * xx[j];
    }
}

void prod_VetMatriz(float *v, f4d **pc, f4d *vr)
{
    int i, j;
    for(i=0; i<4; i++)
    {
        vr[i][0] = vr[i][1] = vr[i][2] = 0.0;
        for(j=0; j<4; j++)
        {
            vr[i][0] += v[j] * pc[j][i][0];
            vr[i][1] += v[j] * pc[j][i][1];
            vr[i][2] += v[j] * pc[j][i][2];
        }

    }
}

void ptsSuperficie(void)
{
    int i, j, h, n, m;
    float t,s;
    float tmp[4], vsm[4], vtm[4];
    f4d va[4];

    if(!pc) return;

    n = 0;

    for(s = 0; s<=1.01; s+=VARIA) n += 1;

    m = n;

    if (pMatriz) pMatriz = liberaMatriz(pMatriz);

    pMatriz=AlocaMatriz(n,m);

    s=0.0f;
    for(i = 0; i < pMatriz->n; i++)
    {
        t = 0.0f;
        for(j = 0; j < pMatriz->m; j++)
        {
                // calcula cada ponto: p(s, t) = S G P G^t T

            prod_VetParam_MatBase(s, tmp, vsm);    // vsm = S G
            prod_VetParam_MatBase(t, tmp, vtm);    // vtm = G^t T

            prod_VetMatriz(vsm, pcPatch->ponto, va);    // va = S G P = vsm P

            pMatriz->ponto[i][j][0] = 0.0f;
            pMatriz->ponto[i][j][1] = 0.0f;
            pMatriz->ponto[i][j][2] = 0.0f;

            for(h=0; h<4; h++)                        // p = S G P G^t T = va vtm
            {
                pMatriz->ponto[i][j][0] += va[h][0] * vtm[h];
                pMatriz->ponto[i][j][1] += va[h][1] * vtm[h];
                pMatriz->ponto[i][j][2] += va[h][2] * vtm[h];
            }
            t+=VARIA;
        }
        s+=VARIA;
    }

}

// calcula normal de triângulo (v0,v1,v2) e normaliza
void calcNormalTri(float v0[3], float v1[3], float v2[3], float n[3])
{
    float a[3], b[3];
    a[X] = v1[X] - v0[X];
    a[Y] = v1[Y] - v0[Y];
    a[Z] = v1[Z] - v0[Z];

    b[X] = v2[X] - v0[X];
    b[Y] = v2[Y] - v0[Y];
    b[Z] = v2[Z] - v0[Z];

    n[X] = a[Y]*b[Z] - a[Z]*b[Y];
    n[Y] = a[Z]*b[X] - a[X]*b[Z];
    n[Z] = a[X]*b[Y] - a[Y]*b[X];

    float s = sqrt(n[X]*n[X] + n[Y]*n[Y] + n[Z]*n[Z]);
    if(s == 0.0f) s = 1.0f;
    n[X] /= s; n[Y] /= s; n[Z] /= s;
}

// calcula contribuição de uma luz (pos) para triângulo com normal n e centro c
float luzContrib(const f4d posLight, float n[3], float c[3])
{
    float L[3];
    L[X] = posLight[X] - c[X];
    L[Y] = posLight[Y] - c[Y];
    L[Z] = posLight[Z] - c[Z];

    float dist = sqrt(L[X]*L[X] + L[Y]*L[Y] + L[Z]*L[Z]);
    if(dist == 0.0f) dist = 1.0f;
    L[X] /= dist; L[Y] /= dist; L[Z] /= dist;

    float dot = n[X]*L[X] + n[Y]*L[Y] + n[Z]*L[Z];
    if(dot < 0.0f) dot = 0.0f;

    // atenuação simples: 1 / (1 + k * d^2)
    float k = 0.005f;
    float att = 1.0f / (1.0f + k * dist * dist);

    return dot * att;
}

void MostrarUmPatch(int cc)
{
    int i, j;
    float t,v,s;
    f4d a,b,n,l;

    if(!pMatriz)  return;

    switch(tipoView)
    {
        case GL_POINTS:
          glColor3f(0.0f, 0.0f, 0.7f);
          glPointSize(1.0);
          for(i = 0; i < pMatriz->n; i++)
          {
              glBegin(tipoView);
              for(j = 0; j < pMatriz->m; j++)
                 glVertex3fv(pMatriz->ponto[i][j]);
              glEnd();
          }
          break;

        case GL_LINE_STRIP:
          glColor3f(0.0f, 0.0f, 0.7f);
          for(i = 0; i < pMatriz->n; i++)
          {
              glBegin(tipoView);
              for(j = 0; j < pMatriz->m; j++)
                 glVertex3fv(pMatriz->ponto[i][j]);
              glEnd();
          }

          for(j = 0; j < pMatriz->n; j++)
          {
              glBegin(tipoView);
              for(i = 0; i < pMatriz->m; i++)
                 glVertex3fv(pMatriz->ponto[i][j]);
              glEnd();
          }
          break;

        case GL_QUADS: // preserved if user sets it, but we will draw as filled quads triangulated
        case GL_TRIANGLES:
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

          for(i = 0; i < pMatriz->n-1; i++)
          {
              for(j = 0; j < pMatriz->m-1; j++)
              {
                // vertices do quad
                float v00[3] = { pMatriz->ponto[i][j][X],   pMatriz->ponto[i][j][Y],   pMatriz->ponto[i][j][Z]   };
                float v01[3] = { pMatriz->ponto[i][j+1][X], pMatriz->ponto[i][j+1][Y], pMatriz->ponto[i][j+1][Z] };
                float v11[3] = { pMatriz->ponto[i+1][j+1][X], pMatriz->ponto[i+1][j+1][Y], pMatriz->ponto[i+1][j+1][Z] };
                float v10[3] = { pMatriz->ponto[i+1][j][X], pMatriz->ponto[i+1][j][Y], pMatriz->ponto[i+1][j][Z] };

                // Triângulo A: v00, v01, v11
                float nA[3];
                calcNormalTri(v00, v01, v11, nA);
                float cA[3] = { (v00[X]+v01[X]+v11[X])/3.0f, (v00[Y]+v01[Y]+v11[Y])/3.0f, (v00[Z]+v01[Z]+v11[Z])/3.0f };
                float ambient = 0.25f;
                float contribA = ambient + luzContrib(lightPos1, nA, cA) + 0.6f * luzContrib(lightPos2, nA, cA);
                if(contribA > 1.0f) contribA = 1.0f;

                glBegin(GL_TRIANGLES);
                    glColor3f(contribA * vcolor[cc][X], contribA * vcolor[cc][Y], contribA * vcolor[cc][Z]);
                    glNormal3fv(nA);
                    glVertex3fv(pMatriz->ponto[i][j]);
                    glVertex3fv(pMatriz->ponto[i][j+1]);
                    glVertex3fv(pMatriz->ponto[i+1][j+1]);
                glEnd();

                // Triângulo B: v00, v11, v10
                float nB[3];
                calcNormalTri(v00, v11, v10, nB);
                float cB[3] = { (v00[X]+v11[X]+v10[X])/3.0f, (v00[Y]+v11[Y]+v10[Y])/3.0f, (v00[Z]+v11[Z]+v10[Z])/3.0f };
                float contribB = luzContrib(lightPos1, nB, cB) + 0.6f * luzContrib(lightPos2, nB, cB);
                if(contribB > 1.0f) contribB = 1.0f;

                glBegin(GL_TRIANGLES);
                    glColor3f(contribB * vcolor[cc][X], contribB * vcolor[cc][Y], contribB * vcolor[cc][Z]);
                    glNormal3fv(nB);
                    glVertex3fv(pMatriz->ponto[i][j]);
                    glVertex3fv(pMatriz->ponto[i+1][j+1]);
                    glVertex3fv(pMatriz->ponto[i+1][j]);
                glEnd();

              }
          }
          break;
    }

}

void MostrarPtosPoligControle(matriz *sup)
{
    int i, j;

    glColor3f(0.0f, 0.8f, 0.0f);
    glPolygonMode(GL_FRONT_AND_BACK, tipoView);
    glPointSize(7.0);
    for(i=0; i<sup->n; i++)
    {
        glBegin(GL_POINTS);
        for(j=0; j<sup->m; j++)
            glVertex3fv(sup->ponto[i][j]);
        glEnd();

        glBegin(GL_LINE_STRIP);
        for(j=0; j<sup->m; j++)
            glVertex3fv(sup->ponto[i][j]);
        glEnd();
    }

    for(i=0; i<sup->m; i++)
    {
        glBegin(GL_LINE_STRIP);
        for(j=0; j<sup->n; j++)
            glVertex3fv(sup->ponto[j][i]);
        glEnd();
    }
}

void copiarPtosControlePatch(int i0, int j0)
{
    int i, j, jj, ii;

    for(i=0; i<pcPatch->n; i++)
    {
        ii = (i0 + i)%(pc->n);
        for(j=0; j<pcPatch->m; j++)
        {
            jj = (j0 + j)%(pc->m);
            pcPatch->ponto[i][j][0] = pc->ponto[ii][jj][0];
            pcPatch->ponto[i][j][1] = pc->ponto[ii][jj][1];
            pcPatch->ponto[i][j][2] = pc->ponto[ii][jj][2];
        }
    }
}

void DisenaSuperficie(void)
{
    int i, j, nn;

    nn = pc->n - 3;   // numero de descolamentos (patchs)

    for (i=0; i<nn; i++)
    {
        for(j=0; j<pc->m; j++)
        {
            copiarPtosControlePatch(i, j);
            ptsSuperficie();
            MostrarUmPatch((i+j)%4);
        }
    }
}


static void init(void)
{

   glClearColor(1.0, 1.0, 1.0, 0.0);
   glEnable(GL_DEPTH_TEST);
   // Mantemos shading manual (calculado por triângulo), então não habilitamos GL_LIGHTING automático.
   glEnable(GL_MAP2_VERTEX_3);
   glEnable(GL_AUTO_NORMAL);
   glMapGrid2f(20, 0.0, 1.0, 20, 0.0, 1.0);
}

void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();

   if(pc)
   {
       MostrarPtosPoligControle(pc);
       DisenaSuperficie();    // disenhando un objeto
   }

   // Desenhar indicadores das luzes (pequenas esferas coloridas)
   glPushMatrix();
     glTranslatef(lightPos1[X], lightPos1[Y], lightPos1[Z]);
     glColor3f(1.0f, 0.9f, 0.7f);
     glutSolidSphere(0.3f, 10, 10);
   glPopMatrix();

   glPushMatrix();
     glTranslatef(lightPos2[X], lightPos2[Y], lightPos2[Z]);
     glColor3f(0.7f, 0.8f, 1.0f);
     glutSolidSphere(0.25f, 8, 8);
   glPopMatrix();

   glPopMatrix();

   glutSwapBuffers();
}


void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   if (w <= h)
      glOrtho(-10.0, 10.0, -10.0*(GLfloat)h/(GLfloat)w,
              10.0*(GLfloat)h/(GLfloat)w, -10.0, 30.0);
   else
      glOrtho(-10.0*(GLfloat)w/(GLfloat)h,
              10.0*(GLfloat)w/(GLfloat)h, -10.0, 10.0, -10.0, 30.0);
   glMatrixMode(GL_MODELVIEW);
}

void keyboard(int key, int x, int y)
{
    int i,j;
    MatrizIdentidade();
    switch(comando)
    {   case Redimensionar:
            switch (key)
            {   case GLUT_KEY_LEFT:
                    AuxVertex[0][0] = 0.95;
                    AuxVertex[1][1] = 0.95;
                    AuxVertex[2][2] = 0.95;
                    break;
                case GLUT_KEY_RIGHT:
                    AuxVertex[0][0] = 1.05;
                    AuxVertex[1][1] = 1.05;
                    AuxVertex[2][2] = 1.05;
                    break;
                case GLUT_KEY_UP:
                    AuxVertex[0][0] = 1.05;
                    AuxVertex[1][1] = 1.05;
                    AuxVertex[2][2] = 1.05;
                    break;
                case GLUT_KEY_DOWN:
                    AuxVertex[0][0] = 0.95;
                    AuxVertex[1][1] = 0.95;
                    AuxVertex[2][2] = 0.95;
                    break;
            }
            break;

        case GirarX:
            switch (key)
            {   case GLUT_KEY_LEFT:
                    AuxVertex[1][1] = cos(-0.01);
                    AuxVertex[1][2] = sin(-0.01);
                    AuxVertex[2][1] = -sin(-0.01);
                    AuxVertex[2][2] = cos(-0.01);
                    break;
                case GLUT_KEY_RIGHT:
                    AuxVertex[1][1] = cos(0.01);
                    AuxVertex[1][2] = sin(0.01);
                    AuxVertex[2][1] = -sin(0.01);
                    AuxVertex[2][2] = cos(0.01);
                    break;
                case GLUT_KEY_UP:
                    AuxVertex[1][1] = cos(0.01);
                    AuxVertex[1][2] = sin(0.01);
                    AuxVertex[2][1] = -sin(0.01);
                    AuxVertex[2][2] = cos(0.01);
                    break;
                case GLUT_KEY_DOWN:
                    AuxVertex[1][1] = cos(-0.01);
                    AuxVertex[1][2] = sin(-0.01);
                    AuxVertex[2][1] = -sin(-0.01);
                    AuxVertex[2][2] = cos(-0.01);
                    break;
            }
            break;
        case GirarY:
            switch (key)
            {   case GLUT_KEY_LEFT:
                    AuxVertex[0][0] = cos(-0.01);
                    AuxVertex[0][2] = -sin(-0.01);
                    AuxVertex[2][0] = sin(-0.01);
                    AuxVertex[2][2] = cos(-0.01);
                    break;
                case GLUT_KEY_RIGHT:
                    AuxVertex[0][0] = cos(0.01);
                    AuxVertex[0][2] = -sin(0.01);
                    AuxVertex[2][0] = sin(0.01);
                    AuxVertex[2][2] = cos(0.01);
                    break;
                case GLUT_KEY_UP:
                    AuxVertex[0][0] = cos(0.01);
                    AuxVertex[0][2] = -sin(0.01);
                    AuxVertex[2][0] = sin(0.01);
                    AuxVertex[2][2] = cos(0.01);
                    break;
                case GLUT_KEY_DOWN:
                    AuxVertex[0][0] = cos(-0.01);
                    AuxVertex[0][2] = -sin(-0.01);
                    AuxVertex[2][0] = sin(-0.01);
                    AuxVertex[2][2] = cos(-0.01);
                    break;
            }
            break;
        case GirarZ:
            switch (key)
            {   case GLUT_KEY_LEFT:
                    AuxVertex[0][0] = cos(-0.01);
                    AuxVertex[0][1] = sin(-0.01);
                    AuxVertex[1][0] = -sin(-0.01);
                    AuxVertex[1][1] = cos(-0.01);
                    MultMatriz();
                    break;
                case GLUT_KEY_RIGHT:
                    AuxVertex[0][0] = cos(0.01);
                    AuxVertex[0][1] = sin(0.01);
                    AuxVertex[1][0] = -sin(0.01);
                    AuxVertex[1][1] = cos(0.01);
                    MultMatriz();
                    break;
                case GLUT_KEY_UP:
                    AuxVertex[0][0] = cos(0.01);
                    AuxVertex[0][1] = sin(0.01);
                    AuxVertex[1][0] = -sin(0.01);
                    AuxVertex[1][1] = cos(0.01);
                    MultMatriz();
                    break;
                case GLUT_KEY_DOWN:
                    AuxVertex[0][0] = cos(-0.01);
                    AuxVertex[0][1] = sin(-0.01);
                    AuxVertex[1][0] = -sin(-0.01);
                    AuxVertex[1][1] = cos(-0.01);
                    MultMatriz();
                    break;
            }
            break;

   }
    MultMatriz();
    glutPostRedisplay();
}

int CarregaPontos( char *arch)
{
  FILE *fobj;
  char token[40];
  float px, py, pz;
  int i, j, n, m;

  printf(" \n ler  %s  \n", arch);

  if((fobj=fopen(arch,"rt"))==NULL)
  {
     printf("Error en la apertura del archivo %s \n", arch);
     return 0;
  }

  fgets(token, 40, fobj);
  fscanf(fobj, "%s %d %d", token, &n, &m);

  if (pc) pc = liberaMatriz(pc);

  pc=AlocaMatriz(n,m);

  for(j=0; j<pc->n; j++)
  {
    for(i=0; i<pc->m; i++)
     {
         fscanf(fobj, "%s %f %f %f", token, &px, &py, &pz);

         pc->ponto[j][i][0] = px * local_scale;
         pc->ponto[j][i][1] = py * local_scale;
         pc->ponto[j][i][2] = pz * local_scale;
         pc->ponto[j][i][3] = 0.0f;
     }
  }

  // espaco de matriz para um patch
  if(pcPatch) pcPatch = liberaMatriz(pcPatch);
  pcPatch = AlocaMatriz(4,4);

  return 1;
}

void processMenuEvents(int option)
{
    MatrizIdentidade();
    if (option == PtsControle)
        CarregaPontos("ptosControleCilindro4x4.txt");
    else if (option == OBJ_CILINDRO)
        CarregaPontos("ptosControleCilindro4x4.txt");
    else if (option == OBJ_CUBO)
        CarregaPontos("ptosControleCubo4x4.txt");
    else if (option == OBJ_ESFERA)
        CarregaPontos("ptosControleEsfera4x4.txt");
    else if (option == Pontos)
        tipoView = GL_POINTS;
    else if(option == Linha)
        tipoView = GL_LINE_STRIP;
    else if (option == Solido)
        tipoView = GL_TRIANGLES; // exibimos triângulos
    else if (option == sair)
        exit (0);
    else
        comando = option;

    if(option==BEZIER || option==BSPLINE || option==CATMULLROM)
    {
         MontaMatrizBase(option);
    }
    glutPostRedisplay();
}

void createGLUTMenus()
{
    int menu, submenu, SUBmenuGirar,SUBmenuSuperficie,SUBmenuPintar, SUBmenuObjetos;

    SUBmenuSuperficie = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("Bezier", BEZIER);
    glutAddMenuEntry("B-Spline",BSPLINE);
    glutAddMenuEntry("Catmull-Rom",CATMULLROM);

    SUBmenuGirar = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("EixoX", GirarX);
    glutAddMenuEntry("EixoY", GirarY);
    glutAddMenuEntry("EixoZ", GirarZ);

    SUBmenuPintar = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("Pontos",Pontos);
    glutAddMenuEntry("Malha",Linha);
    glutAddMenuEntry("Preenchido (Triângulos)",Solido);

    SUBmenuObjetos = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("Cilindro", OBJ_CILINDRO);
    glutAddMenuEntry("Cubo", OBJ_CUBO);
    glutAddMenuEntry("Esfera", OBJ_ESFERA);

    menu = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("Carregar pontos de controle (padrao)", PtsControle);
    glutAddSubMenu("Tipo de Superficie",SUBmenuSuperficie);
    glutAddSubMenu("Objetos (arquivos .txt)", SUBmenuObjetos);
    glutAddSubMenu("Objet View",SUBmenuPintar);
    glutAddMenuEntry("Redimensionar",Redimensionar);
    glutAddSubMenu("Rotacionar",SUBmenuGirar);
    glutAddMenuEntry("Sair",sair);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);

   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize(700, 700);
   glutCreateWindow("Superficies - Trianguladas");

   init();

   glutReshapeFunc(reshape);
   glutSpecialFunc(keyboard);
   glutDisplayFunc(display);
   createGLUTMenus();

   // carregar cilindro por padrao
   CarregaPontos("ptosControleCilindro4x4.txt");
   MontaMatrizBase(BEZIER);

   glutMainLoop();
   return 0;
}
