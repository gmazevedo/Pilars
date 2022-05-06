//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 5
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

#include <windows.h>

// Constantes
#define HEIGHT 1280
#define WIDTH 720

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
#define MAX_OBJ_MESMA_CLASSE 17
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
    int          ultimo_obj;
    glm::mat4    model[MAX_OBJ_MESMA_CLASSE];
    bool         clicado = false;
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
SceneObject BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

//Função para atualizar os pontos da curva de bezier
void UpdateBezierMovement();

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

//---------------------------------------------------------------------------
float playermovex = 0.0f;
float playermovey = 0.0f;


//Pontos da primeira curva de bezier
glm::vec3 bezier_p0 = glm::vec3 (-25.0f,2.0f,40.0f);
glm::vec3 bezier_p1 = glm::vec3 (-25.0f,20.0f,45.0f);
glm::vec3 bezier_p2 = glm::vec3 (-25.0f,20.0f,55.0f);
glm::vec3 bezier_p3 = glm::vec3 (-25.0f,2.0f,60.0f);
float t_bezier = 0.0;
float t_increment = 0.3;
glm::vec3 pontos_bezier = glm::vec3(0.0f, 0.0f, 0.0f);

void UpdateBezierMovement(float delta_time){
    t_bezier += t_increment*delta_time;
    if(t_bezier < 0 || t_bezier > 1)
    {
        t_increment *= -1;
        t_bezier += t_increment*delta_time;
    }
    pontos_bezier = bezier_p0*(1-t_bezier)*(1-t_bezier)*(1-t_bezier) + 3*t_bezier*(1-t_bezier)*(1-t_bezier)*bezier_p1 +
    (3*t_bezier)*(t_bezier)*(1-t_bezier)*bezier_p2 + (t_bezier)*(t_bezier)*(t_bezier)*bezier_p3;
}


void obb_to_aabb(std::string obj, int num_obj)
{
    glm::vec4 ponto[8];

    //Oito pontos da minha Bounding Box
    ponto[0] = glm::vec4(g_VirtualScene[obj].bbox_min.x, g_VirtualScene[obj].bbox_min.y, g_VirtualScene[obj].bbox_min.z, 1.0f);
    ponto[1] = glm::vec4(g_VirtualScene[obj].bbox_min.x, g_VirtualScene[obj].bbox_min.y, g_VirtualScene[obj].bbox_max.z, 1.0f);
    ponto[2] = glm::vec4(g_VirtualScene[obj].bbox_min.x, g_VirtualScene[obj].bbox_max.y, g_VirtualScene[obj].bbox_min.z, 1.0f);
    ponto[3] = glm::vec4(g_VirtualScene[obj].bbox_max.x, g_VirtualScene[obj].bbox_min.y, g_VirtualScene[obj].bbox_min.z, 1.0f);
    ponto[4] = glm::vec4(g_VirtualScene[obj].bbox_min.x, g_VirtualScene[obj].bbox_max.y, g_VirtualScene[obj].bbox_max.z, 1.0f);
    ponto[5] = glm::vec4(g_VirtualScene[obj].bbox_max.x, g_VirtualScene[obj].bbox_min.y, g_VirtualScene[obj].bbox_max.z, 1.0f);
    ponto[6] = glm::vec4(g_VirtualScene[obj].bbox_max.x, g_VirtualScene[obj].bbox_max.y, g_VirtualScene[obj].bbox_min.z, 1.0f);
    ponto[7] = glm::vec4(g_VirtualScene[obj].bbox_max.x, g_VirtualScene[obj].bbox_max.y, g_VirtualScene[obj].bbox_max.z, 1.0f);

    glm::vec4 novo_min = glm::vec4(-10000000.0f,-10000000.0f, -10000000.0f, 1.0f);
    glm::vec4 novo_max = glm::vec4(10000000.0f,10000000.0f,10000000.0f, 1.0f);

    int i;

    for(i=0;i<=7;i++)
    {
        glm::vec4 novo_ponto = g_VirtualScene[obj].model[num_obj] * ponto[i];
        novo_min.x = std::min(novo_min.x, novo_ponto.x);
        novo_min.y = std::min(novo_min.y, novo_ponto.y);
        novo_min.z = std::min(novo_min.z, novo_ponto.z);
        novo_max.x = std::max(novo_max.x, novo_ponto.x);
        novo_max.y = std::max(novo_max.y, novo_ponto.y);
        novo_max.z = std::max(novo_max.z, novo_ponto.z);
    }

    g_VirtualScene[obj].bbox_min = glm::vec3(novo_min.x, novo_min.y, novo_min.z);
    g_VirtualScene[obj].bbox_max = glm::vec3(novo_max.x, novo_max.y, novo_max.z);;

}

glm::vec4 aplica_transformacoes_min(SceneObject a, glm::mat4 modelA)
{

    glm::vec4 a_new_min = glm::vec4(a.bbox_min.x, a.bbox_min.y, a.bbox_min.z, 0.0f) * modelA ;

    a_new_min.x += modelA[3][0];
    a_new_min.y += modelA[3][1];
    a_new_min.z += modelA[3][2];

    return a_new_min;
}

glm::vec4 aplica_transformacoes_max(SceneObject a, glm::mat4 modelA)
{

    glm::vec4 a_new_max = glm::vec4(a.bbox_max.x, a.bbox_max.y, a.bbox_max.z, 0.0f) * modelA ;

    a_new_max.x += modelA[3][0];
    a_new_max.y += modelA[3][1];
    a_new_max.z += modelA[3][2];

    return a_new_max;
}


bool _colisao(std::string obj1, std::string obj2, int indice)
{
    SceneObject a = g_VirtualScene[obj1];
    SceneObject b = g_VirtualScene[obj2];

    glm::mat4 modelA = a.model[indice];
    glm::mat4 modelB = b.model[indice];

    glm::vec4 a_new_min = aplica_transformacoes_min(a, modelA);
    glm::vec4 a_new_max = aplica_transformacoes_max(a, modelA);
    glm::vec4 b_new_min = aplica_transformacoes_min(b, modelB);
    glm::vec4 b_new_max = aplica_transformacoes_max(b, modelB);



    return(a_new_max.x > b_new_min.x &&
            a_new_min.x < b_new_max.x &&
            a_new_max.y > b_new_min.y &&
            a_new_min.y < b_new_max.y &&
            a_new_max.z > b_new_min.z &&
            a_new_min.z < b_new_max.z);

}

bool intersect(glm::vec4 origem, glm::vec4 direcao, std::string obj, int indice)
{

    glm::mat4 modelA = g_VirtualScene[obj].model[indice];
    glm::vec4 obb_min = aplica_transformacoes_min(g_VirtualScene[obj], modelA);
    glm::vec4 obb_max = aplica_transformacoes_max(g_VirtualScene[obj], modelA);

    float tmin = (obb_min.x - origem.x) / direcao.x;
    float tmax = (obb_max.x - origem.x) / direcao.x;
    float aux;

    if (tmin > tmax)
    {
        aux = tmin;
        tmin = tmax;
        tmax = aux;
    }


    float tymin = (obb_min.y - origem.y) / direcao.y;
    float tymax = (obb_max.y - origem.y) / direcao.y;

    if (tymin > tymax)
    {
        aux = tymin;
        tymin = tymax;
        tymax = aux;
    }

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (obb_min.z - origem.z) / direcao.z;
    float tzmax = (obb_max.z - origem.z) / direcao.z;

    if (tzmin > tzmax)
    {
        aux = tzmin;
        tzmin = tzmax;
        tzmax = aux;
    }

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;

    if (tzmax < tmax)
        tmax = tzmax;

    return true;
}


bool colisao(glm::vec4 origem, glm::vec4 direcao)
{
    int max_obj;
    bool colidiu = false;
    int i = 0, j=0;
    int quantidade_obj = 4;
    //std::string lista_objetos[quantidade_obj] = {"bunny", "fly",  "parede", "cube"};
    std::string lista_objetos[quantidade_obj] = {"bunny",  "cube"};

    for(i=0; i<quantidade_obj;i++)
    {
        max_obj = g_VirtualScene[lista_objetos[i]].ultimo_obj;
        for(j=0;j<=max_obj;j++)
        {

            /*if(lista_objetos[i].compare("cylinder") == 0)
                colidiu = colidiu || intersect(origem, direcao, lista_objetos[i], i);
            else*/
                colidiu = colidiu || _colisao(lista_objetos[i], "camera", j);

            if(colidiu)
                return true;
        }
    }

    return false;
}

std::string clique(glm::vec4 origem, glm::vec4 direcao)
{
    int max_obj;
    bool colidiu = false;
    int i = 0, j=0;
    int quantidade_obj = 2;
    //std::string lista_objetos[quantidade_obj] = {"bunny", "fly",  "parede", "cube"};
    std::string lista_objetos[quantidade_obj] = {"bunny", "pilar",};

    for(i=0; i<quantidade_obj;i++)
    {
        max_obj = g_VirtualScene[lista_objetos[i]].ultimo_obj;
        for(j=0;j<=max_obj;j++)
        {

            colidiu = intersect(origem, direcao, lista_objetos[i], j);

            if(colidiu)
                return lista_objetos[i];
        }
    }

    return " ";
}


//TEXTO ANIMADO DE OBJETIVO NO INICIO DO JOGO
float tempo_antigo = 0.0f;
float timer = 0.0f;
void TextRendering_Missao(GLFWwindow* window)
{
    float tempo_agora = (float)glfwGetTime();
    float varia_tempo = tempo_agora - tempo_antigo;
    tempo_antigo = (float)glfwGetTime();

    if ( timer <= 4.5f  )
    {
        timer = timer+0.8*varia_tempo;
        TextRendering_PrintString(window, "Faca a combinacao correta para abrir a porta",  -0.6f, 0.1f, 2.5f);
  }
}

void TextRendering_Fim(GLFWwindow* window)
{
    TextRendering_PrintString(window, "Voce conseguiu escapar! Pressione ESC para fechar o jogo", -0.9f, 0.7f, 2.5f);
}

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(HEIGHT, WIDTH, "Pillar - Gessica Azevedo e Pedro Jacobus", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);
    //Esconde o mouse-----------------------------------------
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, HEIGHT, WIDTH); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 176-196 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/chao.png");         // TextureImage0
    LoadTextureImage("../../data/parede.jpg");       // TextureImage1
    LoadTextureImage("../../data/livro.jpg");        // TextureImage2
    //LoadTextureImage("../../data/galaxia.jpg");      // TextureImage3
    //LoadTextureImage("../../data/fly.png");          // TextureImage4

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel cameramodel("../../data/camera.obj");
    ComputeNormals(&cameramodel);
    SceneObject cameraobject =  BuildTrianglesAndAddToVirtualScene(&cameramodel);

    ObjModel bunnymodel("../../data/bunny.obj");
    ComputeNormals(&bunnymodel);
    SceneObject bunnyobject =  BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel cubemodel("../../data/cube.obj");
    ComputeNormals(&cubemodel);
    SceneObject cubeobject =  BuildTrianglesAndAddToVirtualScene(&cubemodel);

    ObjModel paredemodel("../../data/parede.obj");
    ComputeNormals(&paredemodel);
    SceneObject paredeobject =  BuildTrianglesAndAddToVirtualScene(&paredemodel);

    ObjModel bookmodel("../../data/book.obj");
    ComputeNormals(&bookmodel);
    SceneObject bookobject =  BuildTrianglesAndAddToVirtualScene(&bookmodel);

    //ObjModel cylindermodel("../../data/cylinder.obj");
    //ComputeNormals(&cylindermodel);
    //SceneObject cylinderobject =  BuildTrianglesAndAddToVirtualScene(&cylindermodel);

    ObjModel pilarmodel("../../data/bunny.obj");
    ComputeNormals(&pilarmodel);
    SceneObject pilarobject =  BuildTrianglesAndAddToVirtualScene(&pilarmodel);

    /*ObjModel flymodel("../../data/fly.obj");
    ComputeNormals(&flymodel);
    SceneObject flyobject =  BuildTrianglesAndAddToVirtualScene(&flymodel);*/

    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    SceneObject sphereobject =  BuildTrianglesAndAddToVirtualScene(&spheremodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        SceneObject modelobject =  BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    //Inicializa posição e camera do jogador
    float x_camera = 0.0f, y_camera = 0.0f, z_camera = 0.0f, y_obj_camera = 1.0f;
    glm::vec4 camera_position_c  = glm::vec4(x_camera,y_camera,z_camera ,1.0f); // Ponto "c", centro da câmera
    glm::vec4 camera_view_vector = glm::vec4(1.0f,1.0f,0.0f,0.0f);
    glm::vec4 passos = glm::vec4(0.0f,0.0f,0.0f,0.0f);

    float playerspeed = 4.0f;
    float timeprev = 0.0f;

    //animações
   // float altura_porta1 = 5.0f;
    //float altura_porta2 = 5.0f;
    float giro_coelho = 1.57f;

    bool fim = false;

    bool inicializa_look_camera = true;

    //g_VirtualScene["bunny"].clicado = false;
    //g_VirtualScene["fly"].clicado = false;

    int textura_bunny, x_bunny = -30.0f, y_bunny = 0.0f, z_bunny = -2.0f;
   // int x_fly = -25.0f, y_fly = 3.0f, z_fly = 40.0f;

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        float timenow = glfwGetTime();
        float deltatime = timenow - timeprev;
        timeprev = glfwGetTime();

        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(program_id);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        #define SPHERE   0
        #define BUNNY    1
        #define CAMERA   2
        #define CUBE     3
        #define PAREDE   4
        #define PAREDEM  5
        #define PAREDEP  6
        #define BOOK     7
        #define CYLINDER 8
        #define FLY      9
        #define PHONG    10
        #define PILAR 11


        glm::mat4  model_plane = Matrix_Identity(); // Transformação identidade de modelagem
        //model = Matrix_Translate(1.0f,0.0f,-8.0f);

        // Desenhamos o modelo do coelho
        g_VirtualScene["bunny"].ultimo_obj = 0;
        g_VirtualScene["cube"].ultimo_obj = 0;
        g_VirtualScene["parede"].ultimo_obj = 0;
        g_VirtualScene["cylinder"].ultimo_obj = 0;
        g_VirtualScene["pilar"].ultimo_obj=0;
         g_VirtualScene["book"].ultimo_obj=0;

        //g_VirtualScene["bunny"].clicado = false;


        if(!g_VirtualScene["bunny"].clicado)
        {
            textura_bunny = BUNNY;
        }
        else
        {
            textura_bunny = PHONG;
        }
        if(textura_bunny == PHONG && g_LeftMouseButtonPressed && !clique(camera_position_c, camera_view_vector).compare("bunny") &&  g_VirtualScene["bunny"].clicado)
        {
            if(z>0)
            {
                z_bunny = z_bunny + 1.0f;
            }
            else
            {
                z_bunny =  z_bunny - 1.0f;
            }

        }
        /*else if(textura_fly == PHONG && g_LeftMouseButtonPressed && !clique(camera_position_c, camera_view_vector).compare("fly") &&  g_VirtualScene["fly"].clicado)
            if(z>0)
            {
                z_fly = z_fly + 1.0f;
            }
            else
            {
                z_fly =  z_fly - 1.0f;
            }*/

        g_VirtualScene["bunny"].model[g_VirtualScene["bunny"].ultimo_obj] = Matrix_Translate(x_bunny,y_bunny,z_bunny);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["bunny"].model[g_VirtualScene["bunny"].ultimo_obj++]));
            glUniform1i(object_id_uniform, textura_bunny);
            DrawVirtualObject("bunny");

            g_VirtualScene["pilar"].model[g_VirtualScene["pilar"].ultimo_obj] = Matrix_Translate(camera_position_c.x,camera_position_c.y,(camera_position_c.z+5));
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["pilar"].model[g_VirtualScene["pilar"].ultimo_obj++]));
            glUniform1i(object_id_uniform, textura_bunny);
            DrawVirtualObject("pilar");

       /* g_VirtualScene["fly"].model[g_VirtualScene["fly"].ultimo_obj] = Matrix_Translate(x_fly,y_fly,z_fly)
                            * Matrix_Scale(0.3f,0.3f,0.3f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["fly"].model[g_VirtualScene["fly"].ultimo_obj]));
        glUniform1i(object_id_uniform, textura_fly);
        DrawVirtualObject("fly");*/


        // Desenhamos o modelo da esfera
        g_VirtualScene["camera"].model[0] = Matrix_Translate(0.0f,0.0f,z_camera)
                     * Matrix_Translate(passos.x, passos.y, passos.z);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["camera"].model[0]));
        glUniform1i(object_id_uniform, CAMERA);


        //----------------------------DESENHOS DO MAPA----------------------------
        if (true){ //só pra poder esconder isso, pra não ficar essa tripa enorme
        //CHÃO
        glm::mat4 model_cube = Matrix_Translate(0.0f,-2.1f,0.0f)
                             * Matrix_Scale(50.0f,0.5f,50.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        model_cube = Matrix_Translate(0.0f,-2.1f,50.0f)
                   * Matrix_Scale(50.0f,0.5f,50.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        model_cube = Matrix_Translate(-50.0f,-2.1f,50.0f)
                   * Matrix_Scale(50.0f,0.5f,50.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        model_cube = Matrix_Translate(-50.0f,-2.1f,100.0f)
                   * Matrix_Scale(50.0f,0.5f,50.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        //TETO
        model_cube = Matrix_Translate(0.0f,22.0f,0.0f)
                   * Matrix_Scale(50.0f,0.5f,50.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        model_cube = Matrix_Translate(0.0f,22.1f,50.0f)
                   * Matrix_Scale(50.0f,0.5f,50.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        model_cube = Matrix_Translate(-50.0f,22.1f,50.0f)
                   * Matrix_Scale(50.0f,0.5f,50.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        model_cube = Matrix_Translate(-50.0f,22.1f,100.0f)
                   * Matrix_Scale(50.0f,0.5f,50.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        //PAREDES
        //laterais
        glm::mat4 model_parede = Matrix_Translate(25.0f,10.0f,0.0f)
                               * Matrix_Rotate_X(1.57f)
                               * Matrix_Rotate_Y(0.0f)
                               * Matrix_Rotate_Z(1.57f)
                               * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(-25.0f,10.0f,0.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Rotate_Y(0.0f)
                     * Matrix_Rotate_Z(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");

        /*model_parede = Matrix_Translate(25.0f,10.0f,50.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Rotate_Y(0.0f)
                     * Matrix_Rotate_Z(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(-75.0f,10.0f,50.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Rotate_Y(0.0f)
                     * Matrix_Rotate_Z(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(-75.0f,10.0f,100.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Rotate_Y(0.0f)
                     * Matrix_Rotate_Z(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");

       model_parede = Matrix_Translate(-24.85f,10.0f,99.85f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Rotate_Y(0.0f)
                     * Matrix_Rotate_Z(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");*/

        //fundo
        model_parede = Matrix_Translate(0.0f,10.0f,-25.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");

        /*model_parede = Matrix_Translate(0.0f,10.0f,75.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(-50.0f,10.0f,25.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(-50.0f,10.0f,125.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(50.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDE);
        DrawVirtualObject("parede");*/

        //primeiras paredes da porta
        model_parede = Matrix_Translate(-15.0f,10.0f,25.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(20.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEM);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(15.0f,10.0f,25.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(20.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEM);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(0.0f,17.5f,25.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(10.0f,0.5f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEP);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(0.0f,5.0f,25.0f)
                    * Matrix_Rotate_X(1.57f)
                    * Matrix_Scale(20.0f,0.5f,15.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEP);
        DrawVirtualObject("parede");


        //segundas paredes das portas
       /* model_parede = Matrix_Translate(-65.0f,10.0f,75.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(20.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEM);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(-35.0f,10.0f,75.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(20.0f,0.5f,25.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEM);
        DrawVirtualObject("parede");

        model_parede = Matrix_Translate(-50.0f,17.5f,75.0f)
                     * Matrix_Rotate_X(1.57f)
                     * Matrix_Scale(10.0f,0.5f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEP);
        DrawVirtualObject("parede");*/

        //altar
     /*   model_cube = Matrix_Translate(-50.0f,-1.6f,100.0f)
                   * Matrix_Scale(10.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");

        model_cube = Matrix_Translate(-50.0f,-0.8f,100.0f)
                   * Matrix_Scale(8.0f,1.0f,8.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_cube));
        glUniform1i(object_id_uniform, CUBE);
        DrawVirtualObject("cube");*/

        //livro
      /*  glm::mat4 model_book = Matrix_Translate(-50.0f,0.5f,100.0f)
                             * Matrix_Rotate_Y(3.1f)
                             * Matrix_Scale(0.1f,0.1f,0.1f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_book));
        glUniform1i(object_id_uniform, BOOK);
        DrawVirtualObject("book");*/

        //pilares
        //primeira sala


        //sala do meio
       /* g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj] = Matrix_Translate(0.0f,25.0f,50.0f)
                                * Matrix_Rotate_Y(giro_coelho)
                                * Matrix_Rotate_X(1.57f)
                                * Matrix_Scale(1.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj++]));
        glUniform1i(object_id_uniform, CYLINDER);
        DrawVirtualObject("cylinder");

        g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj] = Matrix_Translate(-50.0f,25.0f,50.0f)
                                * Matrix_Rotate_Y(giro_coelho)
                                * Matrix_Rotate_X(1.57f)
                                * Matrix_Scale(1.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj++]));
        glUniform1i(object_id_uniform, CYLINDER);
        DrawVirtualObject("cylinder");*/

        //sala do livro
        /*g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj] = Matrix_Translate(-60.0f,25.0f,75.0f)
                                * Matrix_Rotate_Y(giro_coelho)
                                * Matrix_Rotate_X(1.57f)
                                * Matrix_Scale(1.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj++]));
        glUniform1i(object_id_uniform, CYLINDER);
        DrawVirtualObject("cylinder");

        g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj] = Matrix_Translate(-40.0f,25.0f,75.0f)
                                * Matrix_Rotate_Y(giro_coelho)
                                * Matrix_Rotate_X(1.57f)
                                * Matrix_Scale(1.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj++]));
        glUniform1i(object_id_uniform, CYLINDER);
        DrawVirtualObject("cylinder");

        g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj] = Matrix_Translate(-60.0f,25.0f,125.0f)
                                * Matrix_Rotate_Y(giro_coelho)
                                * Matrix_Rotate_X(1.57f)
                                * Matrix_Scale(1.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj++]));
        glUniform1i(object_id_uniform, CYLINDER);
        DrawVirtualObject("cylinder");

        g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj] = Matrix_Translate(-40.0f,25.0f,125.0f)
                                * Matrix_Rotate_Y(giro_coelho)
                                * Matrix_Rotate_X(1.57f)
                                * Matrix_Scale(1.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj++]));
        glUniform1i(object_id_uniform, CYLINDER);
        DrawVirtualObject("cylinder");

        g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj] = Matrix_Translate(-65.0f,25.0f,100.0f)
                                * Matrix_Rotate_Y(giro_coelho)
                                * Matrix_Rotate_X(1.57f)
                                * Matrix_Scale(1.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj++]));
        glUniform1i(object_id_uniform, CYLINDER);
        DrawVirtualObject("cylinder");

        g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj] = Matrix_Translate(-35.0f,25.0f,100.0f)
                                * Matrix_Rotate_Y(giro_coelho)
                                * Matrix_Rotate_X(1.57f)
                                * Matrix_Scale(1.0f,1.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cylinder"].model[g_VirtualScene["cylinder"].ultimo_obj++]));
        glUniform1i(object_id_uniform, CYLINDER);
        DrawVirtualObject("cylinder");
        */


        //Coelhos para demonstração do PHONG shading
        // COELHO 1
        giro_coelho += 0.2f * deltatime;
        if(giro_coelho >= 7.85f){ giro_coelho = 1.57f; }
        g_VirtualScene["bunny"].model[g_VirtualScene["bunny"].ultimo_obj] = Matrix_Translate(-20.0f,0.5f,5.0f)
                            * Matrix_Rotate_Y(giro_coelho)
                            * Matrix_Scale(1.0f,1.0f,1.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["bunny"].model[g_VirtualScene["bunny"].ultimo_obj++]));
        glUniform1i(object_id_uniform, PHONG);
        DrawVirtualObject("bunny");

        g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj] = Matrix_Translate(-19.90f,-1.5f,5.0f)
                                                                        * Matrix_Scale(2.0f,2.0f,2.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj]));
        glUniform1i(object_id_uniform, PHONG);
        DrawVirtualObject("cube");

        g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj] = Matrix_Translate(-19.90f,0.0f,5.0f)
                                                                        * Matrix_Scale(2.0f,2.0f,2.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj]));
        glUniform1i(object_id_uniform, PHONG);

        ////////////////////////////
        // COELHO 2
              /*  glm::mat4 model_book = Matrix_Translate(-50.0f,0.5f,100.0f)
                             * Matrix_Rotate_Y(3.1f)
                             * Matrix_Scale(0.1f,0.1f,0.1f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_book));
        glUniform1i(object_id_uniform, BOOK);
        DrawVirtualObject("book");*/

        g_VirtualScene["book"].model[g_VirtualScene["book"].ultimo_obj] = Matrix_Translate(-20.0f,0.5f,10.0f)
                       * Matrix_Rotate_Y(giro_coelho )
                       * Matrix_Scale(0.1f,0.1f,0.1f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["book"].model[g_VirtualScene["book"].ultimo_obj++]));
        glUniform1i(object_id_uniform, BOOK);
        DrawVirtualObject("book");

        g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj] = Matrix_Translate(-19.90f,-1.5f,10.0f)
                                                                        * Matrix_Scale(2.0f,2.0f,2.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj]));
        glUniform1i(object_id_uniform, PHONG);
        DrawVirtualObject("cube");

        g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj] = Matrix_Translate(-19.90f,0.0f,10.0f)
                                                                        * Matrix_Scale(2.0f,2.0f,2.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj]));
        glUniform1i(object_id_uniform, PHONG);

        ////////////////////////////
        // COELHO 3
        g_VirtualScene["bunny"].model[g_VirtualScene["bunny"].ultimo_obj] = Matrix_Translate(-20.0f,0.5f,15.0f)
                * Matrix_Rotate_Y(giro_coelho)
                * Matrix_Scale(1.0f,1.0f,1.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["bunny"].model[g_VirtualScene["bunny"].ultimo_obj++]));
        glUniform1i(object_id_uniform, PHONG);
        DrawVirtualObject("bunny");

        g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj] = Matrix_Translate(-19.90f,-1.5f,15.0f)
                                                                        * Matrix_Scale(2.0f,2.0f,2.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj]));
        glUniform1i(object_id_uniform, PHONG);
        DrawVirtualObject("cube");

        g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj] = Matrix_Translate(-19.90f,0.0f,15.0f)
                                                                        * Matrix_Scale(2.0f,2.0f,2.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["cube"].model[g_VirtualScene["cube"].ultimo_obj]));
        glUniform1i(object_id_uniform, PHONG);

        //porta da primeira sala

      /*  if(altura_porta1 >= -10.0f){
            altura_porta1 = altura_porta1 - 2.0f * deltatime;
        }
        model_parede = Matrix_Translate(0.0f,altura_porta1,25.0f)
                         * Matrix_Rotate_X(1.57f)
                         * Matrix_Scale(10.0f,0.5f,15.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEP);
        DrawVirtualObject("parede");*/

        //PORTA DA SALA DO LIVRO
        /*if(altura_porta2 >= -10.0f && textura_fly == PHONG){
            altura_porta2 = altura_porta2 - 2.0f * deltatime;
        }
        model_parede = Matrix_Translate(-50.0f,altura_porta2,75.0f)
                         * Matrix_Rotate_X(1.57f)
                         * Matrix_Scale(10.0f,0.5f,15.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model_parede));
        glUniform1i(object_id_uniform, PAREDEP);
        DrawVirtualObject("parede");*/


    }

        //movimentação da camera livre e colisão
        if(!fim){
            camera_view_vector = glm::vec4(x,-y,z,0.0f); // Vetor "view", sentido para onde a câmera está virada
            glm::vec4 u = crossproduct(glm::vec4(0.0f,1.0f,0.0f,0.0f), camera_view_vector);
            glm::vec4 camera_up_vector = crossproduct(camera_view_vector, u); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

            if (playermovex != 0){
                passos += playermovex*camera_view_vector*glm::vec4(1.0f,0.0f,1.0f,1.0f)*playerspeed*deltatime;

                camera_position_c += playermovex*camera_view_vector*glm::vec4(1.0f,0.0f,1.0f,1.0f)*playerspeed*deltatime; //vec4 para não se mover para cima, zerando a componente Y
                //model = Matrix_Translate(passos.x, passos.y, passos.z);

                // Desenhamos o modelo da esfera
                 g_VirtualScene["camera"].model[0] = Matrix_Translate(0.0f,y_obj_camera,z_camera)
                     * Matrix_Translate(passos.x, passos.y, passos.z);

                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["camera"].model[0]));
                glUniform1i(object_id_uniform, CAMERA);

                bool paredes = camera_position_c.z < -24.0f || camera_position_c.x < -74.0f || camera_position_c.x > 24.0f;
                paredes = paredes || (camera_position_c.z > 24   && (camera_position_c.x > 3    ||  camera_position_c.x < -3)  && camera_position_c.z < 26);
                paredes = paredes || (camera_position_c.x < -24) && camera_position_c.z  < 25;
                paredes = paredes || (camera_position_c.z > 74   && (camera_position_c.x < -54  ||  camera_position_c.x > -46) && camera_position_c.z < 76);
                paredes = paredes || (camera_position_c.x > -26  && camera_position_c.z > 76 );


                if(colisao(camera_position_c, camera_view_vector) || paredes)
                {
                    passos -= playermovex*camera_view_vector*glm::vec4(1.0f,0.0f,1.0f,1.0f)*playerspeed*deltatime;
                    camera_position_c -= playermovex*camera_view_vector*glm::vec4(1.0f,0.0f,1.0f,1.0f)*playerspeed*deltatime;
                }


            }
            if (playermovey != 0){
                passos += playermovey*u*glm::vec4(1.0f,0.0f,1.0f,1.0f)*playerspeed*deltatime;

                camera_position_c += playermovey*u*glm::vec4(1.0f,0.0f,1.0f,1.0f)*playerspeed*deltatime; //vec4 para não se mover para cima, zerando a componente Y

                // Desenhamos o modelo da esfera
                g_VirtualScene["camera"].model[0] = Matrix_Translate(0.0f,y_obj_camera,z_camera)
                     * Matrix_Translate(passos.x, passos.y, passos.z);

                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(g_VirtualScene["camera"].model[0]));
                glUniform1i(object_id_uniform, CAMERA);

                bool paredes = camera_position_c.z < -24.0f || camera_position_c.x < -74.0f || camera_position_c.x > 24.0f;
                paredes = paredes || (camera_position_c.z > 24   && (camera_position_c.x > 3    ||  camera_position_c.x < -3)  && camera_position_c.z < 26);
                paredes = paredes || (camera_position_c.x < -24) && camera_position_c.z  < 25;
                paredes = paredes || (camera_position_c.z > 74   && (camera_position_c.x < -54  ||  camera_position_c.x > -46) && camera_position_c.z < 76);
                paredes = paredes || (camera_position_c.x > -26  && camera_position_c.z > 76 );

                if(colisao(camera_position_c, camera_view_vector) || paredes)
                {
                    passos -= playermovey*u*glm::vec4(1.0f,0.0f,1.0f,1.0f)*playerspeed*deltatime;
                    camera_position_c -= playermovey*u*glm::vec4(1.0f,0.0f,1.0f,1.0f)*playerspeed*deltatime;
                }


            }

            //Gravidade aplicada no jogador (por enquanto não passa do chão sem testar colisão
            if (camera_position_c.y >= 1.0f)
            {
                camera_position_c += glm::vec4(0.0f,-15.0f,0.0f,0.0f)*deltatime;
            }

            if(g_LeftMouseButtonPressed)
            {
                DrawVirtualObject("pilar");
                std::string selecionado = clique(camera_position_c, camera_view_vector);
               /* if(selecionado.compare("bunny") == 0)
                {
                    g_VirtualScene["bunny"].clicado = true;
                }
                else if(selecionado.compare("fly") == 0)
                {
                    g_VirtualScene["fly"].clicado = true;
                }*/
            }

            glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

            // Agora computamos a matriz de Projeção.
            glm::mat4 projection;

            float nearplane = -0.1f;  // Posição do "near plane"
            float farplane  = -1000.0f; // Posição do "far plane" <------------------ VIEW DISTANCE

            if (g_UsePerspectiveProjection)
            {
                // Projeção Perspectiva.
                // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
                float field_of_view = 3.141592 / 3.0f;
                projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
            }
            else
            {
                // Projeção Ortográfica.
                // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
                // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
                // Para simular um "zoom" ortográfico, computamos o valor de "t"
                // utilizando a variável g_CameraDistance.
                float t = 1.5f*g_CameraDistance/2.5f;
                float b = -t;
                float r = t*g_ScreenRatio;
                float l = -r;
                projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
            }
            glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
            glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

            //checa se o jogador está proximo o suficiente do livro para ativar a camera look-at
            if(camera_position_c.z >= 90.0f){
                fim = true;
            }
        }
        //ativa camera look_at ao redor do livro
        else{
           if(inicializa_look_camera){
            g_CameraDistance = 10.0f;
            inicializa_look_camera = false;
           }

            camera_position_c  = glm::vec4(-50.0f+x,1.0f+y,100.0f+z,1.0f);
            glm::vec4 camera_lookat_l    = glm::vec4(-50.0f,0.5f,100.0f,1.0f);
            glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c;
            glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);

            glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

            // Agora computamos a matriz de Projeção.
            glm::mat4 projection;

            float nearplane = -0.1f;  // Posição do "near plane"
            float farplane  = -1000.0f; // Posição do "far plane" <------------------ VIEW DISTANCE

            if (g_UsePerspectiveProjection)
            {
                // Projeção Perspectiva.
                // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
                float field_of_view = 3.141592 / 3.0f;
                projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
            }
            else
            {
                // Projeção Ortográfica.
                // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
                // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
                // Para simular um "zoom" ortográfico, computamos o valor de "t"
                // utilizando a variável g_CameraDistance.
                float t = 1.5f*g_CameraDistance/2.5f;
                float b = -t;
                float r = t*g_ScreenRatio;
                float l = -r;
                projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
            }
            glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
            glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));
        }
        if(fim) TextRendering_Fim(window);

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.


        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.


        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.



        // Pegamos um vértice com coordenadas de modelo (0.5, 0.5, 0.5, 1) e o
        // passamos por todos os sistemas de coordenadas armazenados nas
        // matrizes the_model, the_view, e the_projection; e escrevemos na tela
        // as matrizes e pontos resultantes dessas transformações.
        //glm::vec4 p_model(0.5f, 0.5f, 0.5f, 1.0f);
        //TextRendering_ShowModelViewProjection(window, projection, view, model, p_model);

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_Missao(window);
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 176-196 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
SceneObject BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    SceneObject theobject;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);

    return theobject;

}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= 0.003f*dx; //0.005 é a sensibilidade da camera
    g_CameraPhi   += 0.003f*dy; //0.005 é a sensibilidade da camera

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ==============
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ==============

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    //mover camera para frente--------------------
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
            playermovex = 1.0f;
    }
    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
    {
            playermovex = 0.0f;
    }
    //Anda para trás-----------------------------
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
            playermovex = -1.0f;
    }
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
    {
            playermovex = 0.0f;
    }
    //Strafe para esquerda-----------------------
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
            playermovey = 1.0f;
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
            playermovey = 0.0f;
    }
    //Strafe para direita-----------------------
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
            playermovey = -1.0f;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
            playermovey = 0.0f;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    //if ( !g_ShowInfoText )
    //    return;

    //float pad = TextRendering_LineHeight(window);

    //char buffer[80];
    //snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    //TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    //if ( !g_ShowInfoText )
    //    return;

    //float lineheight = TextRendering_LineHeight(window);
    //float charwidth = TextRendering_CharWidth(window);

    //if ( g_UsePerspectiveProjection )
    //    TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    //else
    //    TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

