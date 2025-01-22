#ifndef ANIMATION_INFO_H
#define ANIMATION_INFO_H
#include<glm/glm.hpp>

#define MAX_BONE_INF 4

struct BoneInfo
{
    /*id is index in finalBoneMatrices*/
    int id;

    /*offset matrix transforms vertex from model space to bone space*/
    glm::mat4 offset;

};

#endif //ANIMATION_INFO_H
