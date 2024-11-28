#ifndef SAMPLES_OPENGL_TEXTURE_LOADER_H
#define SAMPLES_OPENGL_TEXTURE_LOADER_H

#include <string_view>

struct Image
{
  void* pixel = nullptr;
  int width = 0;
  int height = 0;
  int comp = 0; //stat that depends on the format -> .jpeg has 3 I think and stuff....
};


class TextureManager
{
  int texture_index_ = 0;
  int current_image_index_ = 0;
  unsigned int textures_ = 0;


 public :

  unsigned int CreateTexture(const char* path);
};



#endif //SAMPLES_OPENGL_TEXTURE_LOADER_H