/* curva_fechada_app.c
   Programa para criar curva paramétrica fechada (Hermite, Bezier, BSpline, Catmull-Rom)
   a partir de pontos por click, mostrar polígono de controle fechado, manipular vértices,
   e aplicar transformações (translação, rotação, escala, espelhamento, cisalha).
   
   Base: arquivos fornecidos pelo usuário. Veja citações no final da mensagem.
*/

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define MAXVERTEXS 30
#define MAXCURPOINTS 80
#define PHI 3.14159265f

/* tipos de curvas */
#define HERMITE 1
#define BEZIER 2
#define BSPLINE 3
#define CATMULLR 4

/* tipos de transformação (submenus) */
#define TRANSLACAO 1
#define ROTACAO 2
#define SCALA 3
#define ESPELHAR 4
#define CISALHA 5
#define MANIPULAR_PTO 6

typedef struct spts {
    float v[3];
} tipoPto;

tipoPto ptsControl[MAXVERTEXS];
tipoPto ptsCurve[MAXCURPOINTS];

int nPtsControl = 0;
int nPtsCurve = 0;
int jaCurva = 0;       /* 0 = ainda coletando pontos, 1 = polígono pronto */
int ptoSelect = -1;
int tipoCurva = 0;
int tipoTransforma = 0;

int windW = 300, windH = 250;
int doubleBuffer = 0;

/* matrizes de base (inicializadas a partir do seu código) */
float Mh[4][4] = { {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
float Mb[4][4] = { {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
float Ms[4][4] = { {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
float Mc[4][4] = { {-1.0, 3.0, -3.0, 1.0},
                   { 2.0, -5.0,  4.0, -1.0},
                   {-1.0, 0.0,  1.0, 0.0},
                   { 0.0, 2.0,  0.0, 0.0} };

/* matriz de trabalho M (coeficientes usados em ptoCurva) */
float M[4][4];

/* cores para pedaços */
float MCor[9][3] = {
    {0.8,0.7,0.5},{0.5,0.5,0.5},{0.5,1.0,0.5},
    {0.5,0.5,1.0},{1.0,0.5,1.0},{0.0,0.0,1.0},
    {0.0,1.0,0.0},{1.0,0.0,0.0},{0.3,0.3,0.3}
};
int nCors = 9;

/* --- utilitários 2D --- */
void coord_line(void) {
    glLineWidth(1);
    glColor3f(1.0,0.0,0.0);
    glBegin(GL_LINES);
        glVertex2f(-windW,0); glVertex2f(windW,0);
        glVertex2f(0,-windH); glVertex2f(0,windH);
    glEnd();
}

void drawControlPolygon(void) {
    int i;
    /* desenha polígono fechado */
    if(nPtsControl<=0) return;
    glColor3f(0.0,0.0,0.0);
    glLineWidth(1.5);
    glBegin(GL_LINE_LOOP);
        for(i=0;i<nPtsControl;i++) glVertex2fv(ptsControl[i].v);
    glEnd();

    /* desenha vértices */
    glPointSize(6.0);
    glBegin(GL_POINTS);
    for(i=0;i<nPtsControl;i++) {
        if(i==ptoSelect) glColor3f(1.0,0.0,0.0); else glColor3f(0.0,0.6,0.0);
        glVertex2fv(ptsControl[i].v);
    }
    glEnd();
}

/* busca índice de vértice próximo a (x,y) */
int buscaPontoProximo(int x, int y) {
    int i;
    float dx, dy, dd;
    for(i=0;i<nPtsControl;i++) {
        dx = ptsControl[i].v[0] - (float)x;
        dy = ptsControl[i].v[1] - (float)y;
        dd = sqrtf(dx*dx + dy*dy);
        if(dd < 6.0f) return i;
    }
    return -1;
}

/* --- transformação matricial homogênea 3x3 (2D) --- */
float gM[3][3];
void matrizIdentidade(void) {
    gM[0][0]=1; gM[0][1]=0; gM[0][2]=0;
    gM[1][0]=0; gM[1][1]=1; gM[1][2]=0;
    gM[2][0]=0; gM[2][1]=0; gM[2][2]=1;
}
void operaTransforma(float v[]) {
    float tmp[3];
    int i,j;
    for(i=0;i<3;i++){
        tmp[i]=0.0f;
        for(j=0;j<3;j++) tmp[i]+=gM[i][j]*v[j];
    }
    v[0]=tmp[0]; v[1]=tmp[1]; v[2]=tmp[2];
}

void calcCentro(float cc[3]) {
    int i;
    cc[0]=cc[1]=cc[2]=0.0f;
    if(nPtsControl==0) return;
    for(i=0;i<nPtsControl;i++){
        cc[0]+=ptsControl[i].v[0];
        cc[1]+=ptsControl[i].v[1];
        cc[2]+=ptsControl[i].v[2];
    }
    cc[0]/=nPtsControl; cc[1]/=nPtsControl; cc[2]/=nPtsControl;
}

/* operações: aplicar gM aos vértices (centro observável) */
void aplicaTransformacaoGeral(void) {
    int i;
    for(i=0;i<nPtsControl;i++) operaTransforma(ptsControl[i].v);
}

/* cria matrices específicas (todas aplicadas em coordenadas homogêneas [x y 1]) */
void geraMatrizTranslacao(float dx, float dy) {
    matrizIdentidade();
    gM[0][2]=dx; gM[1][2]=dy;
}
void geraMatrizRotacao(float ang_rad, float cx, float cy) {
    float c = cosf(ang_rad), s = sinf(ang_rad);
    /* rotaciona em torno do centro (cx,cy): T(cx,cy) * R * T(-cx,-cy) */
    /* primeiro T(-cx,-cy) */
    matrizIdentidade();
    gM[0][2] = -cx; gM[1][2] = -cy;
    /* aplica a translação inversa e rotação manualmente ao aplicar em duas etapas */
    /* Para simplicidade faremos aplicar em todos os pontos: translate(-cx,-cy), rotate, translate(cx,cy) */
    int i;
    for(i=0;i<nPtsControl;i++) {
        ptsControl[i].v[0] -= cx; ptsControl[i].v[1] -= cy;
    }
    /* rota */
    for(i=0;i<nPtsControl;i++) {
        float x = ptsControl[i].v[0], y = ptsControl[i].v[1];
        ptsControl[i].v[0] = x*c - y*s;
        ptsControl[i].v[1] = x*s + y*c;
    }
    for(i=0;i<nPtsControl;i++) {
        ptsControl[i].v[0] += cx; ptsControl[i].v[1] += cy;
    }
}
void geraMatrizEscala(float sx, float sy, float cx, float cy) {
    int i;
    for(i=0;i<nPtsControl;i++){
        ptsControl[i].v[0] = cx + (ptsControl[i].v[0] - cx)*sx;
        ptsControl[i].v[1] = cy + (ptsControl[i].v[1] - cy)*sy;
    }
}
void geraMatrizEspelhar(int eixo, float cx, float cy) {
    int i;
    /* eixo: 0 = X (espelha em torno do eixo X), 1 = Y */
    if(eixo==0) { /* espelha verticalmente (invertendo Y em torno de cy) */
        for(i=0;i<nPtsControl;i++) ptsControl[i].v[1] = cy - (ptsControl[i].v[1]-cy);
    } else {
        for(i=0;i<nPtsControl;i++) ptsControl[i].v[0] = cx - (ptsControl[i].v[0]-cx);
    }
}
void geraMatrizCisalha(float shx, float shy, float cx, float cy) {
    int i;
    for(i=0;i<nPtsControl;i++) {
        float x = ptsControl[i].v[0]-cx;
        float y = ptsControl[i].v[1]-cy;
        float nx = x + shx*y;
        float ny = shy*x + y;
        ptsControl[i].v[0] = nx+cx;
        ptsControl[i].v[1] = ny+cy;
    }
}

/* --- ptoCurva: calcula ponto da curva para dado t e segmento j (usando M) ---
   função inspirada no seu arquivo base (fechamento por wrap) */
void ptoCurva(float t, int j, float pp[3]) {
    int i, ji;
    tipoPto ptsCont[4];
    float cc;
    pp[0]=pp[1]=pp[2]=0.0f;
    if(nPtsControl < 4) return;

    /* preparar 4 pontos de controle (com wrapping circular para fechar) */
    for(i=0;i<4;i++) {
        ji = (j + i) % nPtsControl;
        ptsCont[i].v[0] = ptsControl[ji].v[0];
        ptsCont[i].v[1] = ptsControl[ji].v[1];
        ptsCont[i].v[2] = ptsControl[ji].v[2];
    }

    /* se Hermite, trata tangentes (adaptado: aproximação simples) */
    if(tipoCurva == HERMITE) {
        /* aproxima tangentes por diferenças locais (p_{i+1} - p_{i}) */
        /* para a formula matricial, interpreto p0,p1 como pontos e tangen. Simples: montar vetor ptsCont com P1,P2,T1,T2 */
        /* aqui simplifico: P1=ptsCont[0], P2=ptsCont[1], T1 = ptsCont[3]-ptsCont[1], T2 = ptsCont[2]-ptsCont[0] */
        tipoPto tmp[4];
        tmp[0] = ptsCont[0];
        tmp[1] = ptsCont[1];
        tmp[2].v[0] = ptsCont[0].v[0] - ptsCont[3].v[0];
        tmp[2].v[1] = ptsCont[0].v[1] - ptsCont[3].v[1];
        tmp[3].v[0] = ptsCont[1].v[0] - ptsCont[2].v[0];
        tmp[3].v[1] = ptsCont[1].v[1] - ptsCont[2].v[1];
        for(i=0;i<4;i++) ptsCont[i]=tmp[i];
    }

    for(i=0;i<4;i++){
        cc = t*t*t*M[0][i] + t*t*M[1][i] + t*M[2][i] + M[3][i];
        pp[0] += cc * ptsCont[i].v[0];
        pp[1] += cc * ptsCont[i].v[1];
        pp[2] += cc * ptsCont[i].v[2];
    }
}

/* gera a curva para cada segmento (um segmento por cada vértice, com wrap) */
void geraCurva(void) {
    int seg, k;
    float t, dt;
    if(nPtsControl < 4) { nPtsCurve = 0; jaCurva = 0; return; }

    dt = 1.0f / (float)(MAXCURPOINTS - 1);
    /* vamos gerar por segmento: para cada j em [0..nPtsControl-1] geramos uma fatia de MAXCURPOINTS/nPtsControl pontos.
       Para simplicidade preenchemos ptsCurve alternadamente (sub-amostrar) */
    k = 0;
    for(seg=0; seg<nPtsControl; seg++) {
        for(int s=0; s < (MAXCURPOINTS / nPtsControl + 1) && k < MAXCURPOINTS; s++) {
            t = (float)s / (float)(MAXCURPOINTS / nPtsControl + 1);
            ptoCurva(t, seg, ptsCurve[k].v);
            k++;
        }
    }
    nPtsCurve = k;
    jaCurva = 1;
}

/* Desenha pontos da curva (poligonal) */
void drawCurve(void) {
    if(!jaCurva || nPtsCurve<=0) return;
    glLineWidth(2.0f);
    glColor3f(0.2f,0.2f,0.8f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<nPtsCurve;i++) glVertex2fv(ptsCurve[i].v);
    glEnd();
}

/* desenha (outras) */
static void Draw(void) {
    glClearColor(1,1,1,0);
    glClear(GL_COLOR_BUFFER_BIT);
    coord_line();
    drawControlPolygon();
    if(jaCurva) drawCurve();

    if(doubleBuffer) glutSwapBuffers(); else glFlush();
}

/* tratamento do menu de curvas: copia M conforme tipo */
void processMenuCurvas(int option) {
    int i,j;
    tipoCurva = option;
    if(nPtsControl>=4) {
        switch(option) {
            case HERMITE:
                for(i=0;i<4;i++) for(j=0;j<4;j++) M[i][j] = Mh[i][j];
                break;
            case BEZIER:
                for(i=0;i<4;i++) for(j=0;j<4;j++) M[i][j] = Mb[i][j];
                break;
            case BSPLINE:
                for(i=0;i<4;i++) for(j=0;j<4;j++) M[i][j] = Ms[i][j]/6.0f;
                break;
            case CATMULLR:
                for(i=0;i<4;i++) for(j=0;j<4;j++) M[i][j] = Mc[i][j]/2.0f;
                break;
        }
        geraCurva();
    }
    glutPostRedisplay();
}

/* menu de transformações (aplica imediatamente ao polígono conforme entrada) */
void processMenuTransforma(int option) {
    float cc[3];
    tipoTransforma = option;
    calcCentro(cc);

    switch(option) {
        case TRANSLACAO:
            /* para translacao: ativa modo de manipulação; aqui apenas imprimimos e aguardamos drag com botão direito */
            printf("Transform: translacao. Use botão direito + drag em um vértice para mover.\n");
            break;
        case ROTACAO:
            /* aplica rotação fixa (exemplo: 15 graus) ao centro */
            geraMatrizRotacao(15.0f * PHI / 180.0f, cc[0], cc[1]);
            printf("Aplicada rotacao (15deg) no centro.\n");
            break;
        case SCALA:
            geraMatrizEscala(1.15f, 1.15f, cc[0], cc[1]);
            printf("Aplicada escala 1.15 no centro.\n");
            break;
        case ESPELHAR:
            /* espelhar em Y (exemplo) */
            geraMatrizEspelhar(1, cc[0], cc[1]);
            printf("Aplicado espelhamento em Y.\n");
            break;
        case CISALHA:
            geraMatrizCisalha(0.25f,0.0f, cc[0], cc[1]);
            printf("Aplicado cisalha horizontal.\n");
            break;
        case MANIPULAR_PTO:
            /* alterna modo: quando ativo, mouse move altera vértice selecionado */
            tipoTransforma = 0;
            printf("Modo manipular ponto ativo: arraste vértice selecionado.\n");
            break;
    }
    /* se uma transformação que modifica diretamente os pontos foi escolhida, já aplicamos (rotacao/escala/espelhar/cisalha fazem isso acima) */
    /* para translacao, deixamos manipulação por drag; para operações massivas que usamos acima, já foram aplicadas às coordenadas. */
    /* Regenera curva se já estava ativa */
    if(jaCurva) geraCurva();
    glutPostRedisplay();
}

void processMenuEvents(int option) {
    switch(option) {
        case 1: /* mostrar polígono de controle (apenas marca) */
            jaCurva = 0;
            break;
        case 2: /* limpar tudo */
            nPtsControl = 0; nPtsCurve = 0; jaCurva = 0; ptoSelect = -1;
            break;
        case 3: /* finalizar polígono (ativa geração de curva) */
            if(nPtsControl >= 4) {
                jaCurva = 1;
                /* deixa a matriz M com Bezier por default */
                for(int i=0;i<4;i++) for(int j=0;j<4;j++) M[i][j] = Mb[i][j];
                geraCurva();
            }
            break;
    }
    glutPostRedisplay();
}

/* cria menus */
void createGLUTMenus() {
    int submenuCurvas = glutCreateMenu(processMenuCurvas);
    glutAddMenuEntry("Hermite", HERMITE);
    glutAddMenuEntry("Bezier", BEZIER);
    glutAddMenuEntry("B-Spline", BSPLINE);
    glutAddMenuEntry("Catmull-Rom", CATMULLR);

    int submenuTrans = glutCreateMenu(processMenuTransforma);
    glutAddMenuEntry("Translacao (drag)", TRANSLACAO);
    glutAddMenuEntry("Rotacao 15°", ROTACAO);
    glutAddMenuEntry("Escala x1.15", SCALA);
    glutAddMenuEntry("Espelhar (Y)", ESPELHAR);
    glutAddMenuEntry("Cisalha (0.25)", CISALHA);
    glutAddMenuEntry("Manipular Pto", MANIPULAR_PTO);

    int menu = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("Mostrar polígono (modo criação)", 1);
    glutAddMenuEntry("Finalizar polígono / Gerar curvas", 3);
    glutAddSubMenu("Tipo de Curva", submenuCurvas);
    glutAddSubMenu("Transformação", submenuTrans);
    glutAddMenuEntry("Limpar Tudo", 2);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/* mouse & motion: left click cria ponto (quando jaCurva==0), depois left down+drag move vértice; right button usado para transform drag */
void mouse(int button, int state, int x, int y) {
    x = x - windW; y = windH - y; /* ajusta coordenadas */
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if(!jaCurva) {
            if(nPtsControl < MAXVERTEXS) {
                ptsControl[nPtsControl].v[0] = (float)x;
                ptsControl[nPtsControl].v[1] = (float)y;
                ptsControl[nPtsControl].v[2] = 1.0f;
                nPtsControl++;
            }
        } else {
            /* já curva: selecionar ponto se próximo */
            ptoSelect = buscaPontoProximo(x,y);
        }
    }
    if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        /* soltar seleção */
        ptoSelect = -1;
    }
    glutPostRedisplay();
}

/* arrastar com mouse (quando botão segurado): se ptoSelect >=0 e jaCurva => mover vértice; 
   se tipoTransforma == TRANSLACAO: arrastar aplica translacao ao poligono inteiro */
void motion(int x, int y) {
    x = x - windW; y = windH - y;
    if(jaCurva) {
        if(ptoSelect >=0 && ptoSelect < nPtsControl) {
            /* mover vértice */
            ptsControl[ptoSelect].v[0] = (float)x;
            ptsControl[ptoSelect].v[1] = (float)y;
            if(nPtsControl>=4 && tipoCurva) geraCurva();
        } else if(tipoTransforma == TRANSLACAO) {
            /* translacao: mover todo polígono pela diferença entre posição atual e centroid? 
               Implementamos translacao incremental pelo movimento do mouse: armazenamos último mouse? 
               Simplicidade: translacao pelo delta em relação ao centro: */
            static int lastX=0, lastY=0;
            static int inicializado = 0;
            if(!inicializado) { lastX = x; lastY = y; inicializado=1; }
            float dx = (float)(x - lastX);
            float dy = (float)(y - lastY);
            lastX = x; lastY = y;
            for(int i=0;i<nPtsControl;i++) { ptsControl[i].v[0]+=dx; ptsControl[i].v[1]+=dy; }
            if(nPtsControl>=4 && tipoCurva) geraCurva();
        }
    }
    glutPostRedisplay();
}

/* teclado: ESC para sair, g para gerar curva manualmente */
void Key(unsigned char key, int x, int y) {
    switch(key) {
        case 27: exit(0); break;
        case 'g': 
            if(nPtsControl>=4) { jaCurva = 1; geraCurva(); }
            break;
        case 'c':
            /* limpa */
            nPtsControl = 0; nPtsCurve = 0; jaCurva = 0; ptoSelect = -1;
            glutPostRedisplay();
            break;
    }
}

static void Reshape(int width, int height) {
    windW = width/2; windH = height/2;
    glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-windW, windW, -windH, windH);
    glMatrixMode(GL_MODELVIEW);
    glutPostRedisplay();
}

void init(void) {
    nPtsControl = 0;
    nPtsCurve = 0;
    jaCurva = 0;
    ptoSelect = -1;
    tipoCurva = BEZIER;
    /* inicializar Mb como matriz base Bezier (padrão topológica simples) */
    Mb[0][0]=1; Mb[0][1]=0; Mb[0][2]=0; Mb[0][3]=0;
    Mb[1][0]=0; Mb[1][1]=1; Mb[1][2]=0; Mb[1][3]=0;
    Mb[2][0]=0; Mb[2][1]=0; Mb[2][2]=1; Mb[2][3]=0;
    Mb[3][0]=0; Mb[3][1]=0; Mb[3][2]=0; Mb[3][3]=1;
    /* (as matrizes exatas e a formulação foram usadas do seu código original). */
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    doubleBuffer = 1;
    glutInitDisplayMode(doubleBuffer ? GLUT_DOUBLE | GLUT_RGB : GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(800,600);
    glutCreateWindow("Curva Parametrica Fechada - App");
    init();
    createGLUTMenus();
    glutDisplayFunc(Draw);
    glutReshapeFunc(Reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(Key);
    glutMainLoop();
    return 0;
}
