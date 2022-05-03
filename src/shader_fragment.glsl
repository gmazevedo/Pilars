
#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

in vec3 colorg; // Gourad shading recebido do shader_vertex

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE   0
#define BUNNY    1
#define CAMERA   2
#define CHAO     3
#define PAREDE   4
#define PAREDEM  5
#define PAREDEP  6
#define BOOK     7
#define CYLINDER 8
#define FLY      9
#define PHONG    10


uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;//chão
uniform sampler2D TextureImage1;//parede
uniform sampler2D TextureImage2;//livro
uniform sampler2D TextureImage3;//galaxia
uniform sampler2D TextureImage4;//mosca

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec3 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,1.0,0.0));
    vec3 I = vec3(1.0,1.0,1.0);

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);
    vec4 pvetor;
    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0, raio = 2.0f, theta, phi;
    vec4 pesfera;

    //variáveis para Phong Shading
    int q = 10; //intensidade do reflexo
    vec4 r = -l + (2*n*dot(n,l));
    float angulo_incidencia = dot(r,v);

    if ( object_id == SPHERE )
    {
        // PREENCHA AQUI as coordenadas de textura da esfera, computadas com
        // projeção esférica EM COORDENADAS DO MODELO. Utilize como referência
        // o slides 134-150 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // A esfera que define a projeção deve estar centrada na posição
        // "bbox_center" definida abaixo.

        // Você deve utilizar:
        //   função 'length( )' : comprimento Euclidiano de um vetor
        //   função 'atan( , )' : arcotangente. Veja https://en.wikipedia.org/wiki/Atan2.
        //   função 'asin( )'   : seno inverso.
        //   constante M_PI
        //   variável position_model

        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        pesfera = bbox_center + (raio * (position_model - bbox_center) / length(position_model - bbox_center));
        pvetor  = pesfera - bbox_center;

        theta = atan(pvetor.x, pvetor.z);
        phi   = asin(pvetor.y / raio);

        U = (theta + M_PI) / (2 * M_PI);
        V = (phi + (M_PI/2)) / M_PI;

    }
    else if ( object_id == BUNNY )
    {
        // PREENCHA AQUI as coordenadas de textura do coelho, computadas com
        // projeção planar XY em COORDENADAS DO MODELO. Utilize como referência
        // o slides 99-104 do documento Aula_20_Mapeamento_de_Texturas.pdf,
        // e também use as variáveis min*/max* definidas abaixo para normalizar
        // as coordenadas de textura U e V dentro do intervalo [0,1]. Para
        // tanto, veja por exemplo o mapeamento da variável 'p_v' utilizando
        // 'h' no slides 158-160 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // Veja também a Questão 4 do Questionário 4 no Moodle.

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - minx)/(maxx - minx);
        V = (position_model.y - miny)/(maxy - miny);
    }
    else if ( object_id == CHAO)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*7;
        V = texcoords.y*7;
    }
    else if ( object_id == PAREDE)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*10;
        V = texcoords.y*10;
    }
    else if ( object_id == PAREDEM)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*5;
        V = texcoords.y*10;
    }
    else if ( object_id == PAREDEP)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*2.34;
        V = texcoords.y*4;
    }
    else if ( object_id == CYLINDER)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;
    }
    else if ( object_id == BOOK)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;
    }
    else if ( object_id == FLY)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - minx)/(maxx - minx);
        V = (position_model.y - miny)/(maxy - miny);
    }
    else if ( object_id == PHONG)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*2;
        V = texcoords.y*2;
    }

    //ILUMINAÇÃO E TEXTURAS

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    vec3 Kd0 = texture(TextureImage3, vec2(U,V)).rgb;

    // Equação de Iluminação
    float lambert = max(0,dot(n,l));

    color = Kd0 * I * (lambert + 0.05);

    //termo de phong para adicionar reflexo nos objetos desejados
    vec3 luz = vec3(1.0,1.0,1.0);
    vec3 phong = (lambert + 0.1) + I * luz * pow(max(0, angulo_incidencia),q);

    if(object_id == CHAO)
    {
        color = texture(TextureImage0, vec2(U,V)).rgb * phong;
    }
    else if(object_id == PAREDE)
    {
        color = texture(TextureImage1, vec2(U,V)).rgb * (lambert + 0.1);
    }
    else if(object_id == PAREDEM)
    {
        color = texture(TextureImage1, vec2(U,V)).rgb * (lambert + 0.1);
    }
    else if(object_id == PAREDEP)
    {
        color = texture(TextureImage1, vec2(U,V)).rgb * (lambert + 0.1);
    }
    else if(object_id == CYLINDER)
    {
        color = texture(TextureImage3, vec2(U,V)).rgb * (lambert + 0.1);
    }
    else if(object_id == BOOK)
    {
        color = texture(TextureImage2, vec2(U,V)).rgb * (lambert + 0.1);
    }
    else if(object_id == FLY)
    {
        color = texture(TextureImage4, vec2(U,V)).rgb * colorg;
    }
    else if(object_id == PHONG)
    {
        vec3 phong = (lambert + 0.1) + I * vec3(10.0,10.0,10.0) * pow(max(0, angulo_incidencia),10); //reflexo exagerado para a esfera de demonstração
        color = texture(TextureImage0, vec2(U,V)).rgb * phong;
    }

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec3(1.0,1.0,1.0)/2.2);

    if(object_id == CAMERA)
    {
        color = vec3(0.0, 0.0, 0.0);
    }
}

