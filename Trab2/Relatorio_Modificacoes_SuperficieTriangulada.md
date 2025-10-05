
# Relatório de Modificações — `superficieTriangulada.cpp`

## 📘 Contexto
Este relatório documenta todas as modificações e adições realizadas no programa original `superficieFechado.cpp`,
que foi atualizado para `superficieTriangulada.cpp`, conforme solicitado.

O objetivo das alterações foi:
1. Substituir quadriláteros (`GL_QUADS`) por triângulos (`GL_TRIANGLES`);
2. Implementar iluminação manual realista por triângulo;
3. Adicionar uma segunda fonte de luz;
4. Incluir novos objetos 3D carregáveis via arquivos `.txt`;
5. Garantir que os objetos sejam superfícies **fechadas** e com **melhor realismo visual**;
6. Manter os controles originais (rotação e escala) e o sistema de menus.

---

## 🔧 Alterações Principais

### 1. Estrutura geral
- O arquivo foi **renomeado** para `superficieTriangulada.cpp`.
- Mantida a arquitetura modular (funções de matriz, cálculo de superfície, desenho e interação).
- Criadas novas funções auxiliares para iluminação e cálculo de normais.

### 2. Substituição de quadriláteros por triângulos
- O bloco de desenho de superfícies antes usava `GL_QUADS`.
- Agora, cada quadrilátero é **dividido em dois triângulos** (`GL_TRIANGLES`).
- Função modificada: `MostrarUmPatch()`.
- Cálculo manual da normal e iluminação de cada triângulo.

```cpp
// Triângulo A: v00, v01, v11
calcNormalTri(v00, v01, v11, nA);
// Triângulo B: v00, v11, v10
calcNormalTri(v00, v11, v10, nB);
```

### 3. Iluminação por triângulo
- Implementado cálculo de **iluminação difusa** com duas luzes.
- Cada triângulo calcula a intensidade de luz via produto escalar entre o vetor normal e o vetor da luz.
- A cor final é proporcional à soma ponderada das duas luzes.

Funções novas:
```cpp
void calcNormalTri(float v0[3], float v1[3], float v2[3], float n[3]);
float luzContrib(const f4d posLight, float n[3], float c[3]);
```

### 4. Duas fontes de luz
- Criadas duas luzes posicionais globais:
```cpp
f4d lightPos1 = {30.0f, 30.0f, 30.0f, 1.0f};   // Luz principal
f4d lightPos2 = {-20.0f, 10.0f, -10.0f, 1.0f}; // Luz secundária
```
- As luzes são visualizadas como pequenas esferas coloridas no `display()`:
```cpp
glutSolidSphere(0.3f, 10, 10); // esfera indicadora da luz
```

### 5. Iluminação manual
- O programa **não utiliza `glEnable(GL_LIGHTING)`**.
- Toda a iluminação é computada manualmente, triângulo a triângulo, permitindo controle total sobre a cor e intensidade.

### 6. Novos objetos e menu
- Adicionados novos tipos de objetos e opções de menu:

Constantes:
```cpp
#define OBJ_CILINDRO 30
#define OBJ_CUBO     31
#define OBJ_ESFERA   32
```

Menu:
```cpp
SUBmenuObjetos = glutCreateMenu(processMenuEvents);
glutAddMenuEntry("Cilindro", OBJ_CILINDRO);
glutAddMenuEntry("Cubo", OBJ_CUBO);
glutAddMenuEntry("Esfera", OBJ_ESFERA);
```

Carregamento de objetos:
```cpp
if (option == OBJ_CILINDRO)
    CarregaPontos("ptosControleCilindro4x4.txt");
else if (option == OBJ_CUBO)
    CarregaPontos("ptosControleCubo4x4.txt");
else if (option == OBJ_ESFERA)
    CarregaPontos("ptosControleEsfera4x4.txt");
```

### 7. Novos arquivos de pontos de controle
Foram criados três arquivos `.txt` compatíveis:
| Arquivo | Descrição |
|----------|------------|
| `ptosControleCilindro4x4.txt` | Já existente (base original) |
| `ptosControleCubo4x4.txt` | Pontos para um cubo fechado e suavizado |
| `ptosControleEsfera4x4.txt` | Pontos para uma esfera fechada aproximada |

### 8. Ajustes visuais e escala
- Mantido o fator `local_scale = 0.22f`.
- Variável `VARIA = 0.04f` define a resolução da malha.

### 9. Interface de interação
- Mantidos todos os comandos originais de rotação e escala (`setas` e menus).
- Mantido o menu de superfícies (`Bezier`, `B-Spline`, `Catmull-Rom`).
- Adicionada nova opção de visualização: `Preenchido (Triângulos)`.

---

## 🧠 Considerações Técnicas

### Iluminação
O modelo segue o **sombreamento de Lambert** (difuso puro):

\[ I = k_d (L \cdot N) / (1 + k d^2) \]

onde:
- \( L \) é o vetor direção da luz,
- \( N \) é a normal do triângulo,
- \( d \) é a distância da luz,
- \( k \) é o coeficiente de atenuação (0.005).

### Estrutura dos triângulos
Cada quadrilátero original foi dividido em:
- Triângulo 1: (v00, v01, v11)
- Triângulo 2: (v00, v11, v10)

---

## 💻 Execução

### Compilação (Linux / MinGW)
```bash
g++ superficieTriangulada.cpp -o superficieTriangulada -lGL -lGLU -lglut
```

### Controles
- **Clique direito**: menu principal
- **Rotacionar / Escalar**: setas do teclado
- **Objetos** → escolher `Cilindro`, `Cubo` ou `Esfera`
- **Objet View** → `Preenchido (Triângulos)` para visualização realista

---

## 🧾 Resumo Final das Mudanças

| Tipo de alteração | Arquivo / Função | Descrição resumida |
|--------------------|------------------|--------------------|
| ✅ Novo | `calcNormalTri()` | Cálculo da normal de triângulo |
| ✅ Novo | `luzContrib()` | Iluminação difusa manual |
| 🔄 Alterado | `MostrarUmPatch()` | Renderização com triângulos e luzes |
| 🔄 Alterado | `processMenuEvents()` | Novos objetos e modos |
| 🔄 Alterado | `createGLUTMenus()` | Novo submenu “Objetos” |
| ✅ Novo | `ptosControleCubo4x4.txt` | Pontos para cubo fechado |
| ✅ Novo | `ptosControleEsfera4x4.txt` | Pontos para esfera |
| ✅ Visual | Esferas das luzes | Indicadores visuais das fontes de luz |

---

**Autor das modificações:** Nathan de Oliveira  
**Data:** 05/10/2025  
**Versão:** 1.0 — Superfícies Trianguladas e Iluminadas
