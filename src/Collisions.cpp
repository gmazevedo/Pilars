
Collisions::Collisions()
{
    // ctor
}

glm::vec4 aplica_transformacoes_min(SceneObject a, glm::mat4 modelA)
{

    glm::vec4 a_new_min = glm::vec4(a.bbox_min.x, a.bbox_min.y, a.bbox_min.z, 0.0f) * modelA;

    a_new_min.x += modelA[3][0];
    a_new_min.y += modelA[3][1];
    a_new_min.z += modelA[3][2];

    return a_new_min;
}

glm::vec4 aplica_transformacoes_max(SceneObject a, glm::mat4 modelA)
{

    glm::vec4 a_new_max = glm::vec4(a.bbox_max.x, a.bbox_max.y, a.bbox_max.z, 0.0f) * modelA;

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

    return (a_new_max.x > b_new_min.x &&
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
    bool colidiu = false;
    int i = 0, j = 0;
    int quantidade_obj = 4;
    std::string lista_objetos[quantidade_obj] = {"bunny", "book", "sphere", "cube"};

    for (i = 0; i < quantidade_obj; i++)
    {
        colidiu = colidiu || _colisao(lista_objetos[i], "camera", j);

        if (colidiu)
            return true;
    }

    return false;
}

std::string clique(glm::vec4 origem, glm::vec4 direcao)
{
    int max_obj;
    bool colidiu = false;
    int i = 0, j = 0;
    int quantidade_obj = 3;
    std::string lista_objetos[quantidade_obj] = {"bunny", "book", "sphere"};

    for (i = 0; i < quantidade_obj; i++)
    {
        max_obj = g_VirtualScene[lista_objetos[i]].ultimo_obj;
        for (j = 0; j <= max_obj; j++)
        {
            colidiu = intersect(origem, direcao, lista_objetos[i], j);

            if (colidiu)
            {
                return lista_objetos[i];
            }
        }
    }

    return " ";
}
