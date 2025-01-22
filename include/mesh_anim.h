// #ifndef MESH_ANIM_H
// #define MESH_ANIM_H
// #include <string>
// #include <vector>
// #include <GL/glew.h>
// #include <glm/vec2.hpp>
// #include <glm/vec3.hpp>
//
// #include "animation_info.h"
//
// struct Vertex{
//   glm::vec3 Position;
//   glm::vec3 Normal;
//   glm::vec2 TexCoords;
//
//   glm::vec3 Tangent;
//   glm::vec3 Bitangent;
//
//   int m_BoneIDs[MAX_BONE_INF];
//   float m_Weights[MAX_BONE_INF];
//   };
//
//   struct Texture{
//     unsigned int id;
//     std::string type;
//     std::string path;
//   };
//
//   class Mesh
//   {
//   public:
//     //Mesh data
//     std::vector<Vertex> vertices_;
//     std::vector<unsigned int> indices_;
//     std::vector<Texture> textures_;
//
//     [[nodiscard]] unsigned int VAO() const {return VAO_;}
//
//     Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
//     {
//       this->vertices_ = vertices;
//       this->indices_ = indices;
//       this->textures_ = textures;
//
//       SetupMesh();
//     }
//     void Draw(GLuint& shader)
//     {
//       unsigned int diffuseNr = 1;
//       unsigned int specularNr = 1;
//       for(unsigned int i = 0; i < textures_.size(); i++)
//       {
//         glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
//         // retrieve texture number (the N in diffuse_textureN)
//         std::string number;
//         std::string name = textures_[i].type;
//         if(name == "texture_diffuse")
//           number = std::to_string(diffuseNr++);
//         else if(name == "texture_specular")
//           number = std::to_string(specularNr++);
//
//         glUniform1i(glGetUniformLocation(shader, ("material." + name).append(number).c_str()), i);
//         glBindTexture(GL_TEXTURE_2D, textures_[i].id);
//       }
//       glActiveTexture(GL_TEXTURE0);
//
//       // draw mesh
//       glBindVertexArray(VAO_);
//       glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, 0);
//       glBindVertexArray(0);
//     }
//   private:
//     //Render data
//     unsigned int VAO_, VBO_, EBO_;
//     void SetupMesh()
//     {
//       glGenVertexArrays(1, &VAO_);
//       glGenBuffers(1, &VBO_);
//       glGenBuffers(1, &EBO_);
//
//       glBindVertexArray(VAO_);
//       glBindBuffer(GL_ARRAY_BUFFER, VBO_);
//
//       glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), &vertices_[0], GL_STATIC_DRAW);
//
//       glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
//       glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int),
//                    &indices_[0], GL_STATIC_DRAW);
//
//       // vertex positions
//       glEnableVertexAttribArray(0);
//       glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
//       // vertex normals
//       glEnableVertexAttribArray(1);
//       glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
//       // vertex texture coords
//       glEnableVertexAttribArray(2);
//       glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
//
//       // bone ids
//       glEnableVertexAttribArray(3);
//       glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
//
//       // bone weights
//       glEnableVertexAttribArray(4);
//       glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
//
//       glBindVertexArray(0);
//     }
//   };
//
// #endif //MESH_ANIM_H
