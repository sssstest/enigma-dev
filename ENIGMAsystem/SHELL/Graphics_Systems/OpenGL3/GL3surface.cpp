/** Copyright (C) 2008-2013 Josh Ventura, Robert B. Colton, Dave "biggoron", Harijs Grinbergs
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "../General/OpenGLHeaders.h"
#include "../General/GLTextureStruct.h"
#include "Graphics_Systems/graphics_mandatory.h"

using namespace std;
#include <cstddef>
#include <iostream>
#include <cmath>

#include "GL3shapes.h"

#include <stdio.h> //for file writing (surface_save)
#include "Universal_System/nlpo2.h"
#include "Universal_System/spritestruct.h"
#include "Universal_System/backgroundstruct.h"
#include "Collision_Systems/collision_types.h"

#define __GETR(x) (gs_scalar)(((x & 0x0000FF))/255.0)
#define __GETG(x) (gs_scalar)(((x & 0x00FF00) >> 8)/255.0)
#define __GETB(x) (gs_scalar)(((x & 0xFF0000) >> 16)/255.0)

namespace enigma_user {
  extern int room_width, room_height/*, sprite_idmax*/;
}
#include "../General/GSsurface.h"
#include "../General/GLSurfaceStruct.h"
#include "../General/GStextures.h"

#ifdef DEBUG_MODE
  #include <string>
  #include "libEGMstd.h"
  #include "Widget_Systems/widgets_mandatory.h"
  #define get_surface(surf,id)\
    if (size_t(id) >= enigma::surface_max or !enigma::surface_array[id]) {\
      show_error("Attempting to use non-existing surface " + toString(id), false);\
      return;\
    }\
    enigma::surface* surf = enigma::surface_array[id];
  #define get_surfacev(surf,id,r)\
    if (size_t(id) >= enigma::surface_max or !enigma::surface_array[id]) {\
      show_error("Attempting to use non-existing surface " + toString(id), false);\
      return r;\
    }\
    enigma::surface* surf = enigma::surface_array[id];
#else
  #define get_surface(surf,id)\
    enigma::surface* surf = enigma::surface_array[id];
  #define get_surfacev(surf,id,r)\
    enigma::surface* surf = enigma::surface_array[id];
#endif

namespace enigma
{
  surface **surface_array;
  size_t surface_max=0;
  extern unsigned int bound_framebuffer;
}

namespace enigma_user
{

bool surface_is_supported()
{
    return GLEW_ARB_framebuffer_object;
}

int surface_create(int width, int height)
{
    if (GLEW_ARB_framebuffer_object)
    {
      GLuint fbo;

      unsigned int id,
        w=(int)width,
        h=(int)height; //get the integer width and height, and prepare to search for an id

      if (enigma::surface_max==0) {
        enigma::surface_array=new enigma::surface*[1];
        enigma::surface_max=1;
      }

      for (id=0; enigma::surface_array[id]!=NULL; id++)
      {
        if (id+1>=enigma::surface_max)
        {
          enigma::surface **oldarray=enigma::surface_array;
          enigma::surface_array=new enigma::surface*[enigma::surface_max+1];

          for (unsigned int i=0; i<enigma::surface_max; i++)
            enigma::surface_array[i]=oldarray[i];

          enigma::surface_array[enigma::surface_max]=NULL;
          enigma::surface_max++;
          delete[] oldarray;
        }
      }

      enigma::surface_array[id] = new enigma::surface;
      enigma::surface_array[id]->width = w;
      enigma::surface_array[id]->height = h;

      glGenFramebuffers(1, &fbo);
      int texture = enigma::graphics_create_texture(w,h,0,false);

      glPushAttrib(GL_TEXTURE_BIT);

      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureStructs[texture]->gltex, 0);
      glDrawBuffer(GL_COLOR_ATTACHMENT0);
      glReadBuffer(GL_COLOR_ATTACHMENT0);
      glClearColor(1,1,1,0);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, enigma::bound_framebuffer);

      enigma::surface_array[id]->tex = texture;
      enigma::surface_array[id]->fbo = fbo;

      texture_reset();

      glPopAttrib();

      return id;
    }
    return -1;
}

int surface_create_msaa(int width, int height, int samples)
{
  if (!GLEW_ARB_framebuffer_object)
  {
    return -1;
  }

  GLuint fbo;

  size_t id,
  w = (int)width,
  h = (int)height; //get the integer width and height, and prepare to search for an id

  if (enigma::surface_max==0) {
    enigma::surface_array=new enigma::surface*[1];
    enigma::surface_max=1;
  }

  for (id=0; enigma::surface_array[id]!=NULL; id++)
  {
    if (id+1 >= enigma::surface_max)
    {
      enigma::surface **oldarray=enigma::surface_array;
      enigma::surface_array=new enigma::surface*[enigma::surface_max+1];

      for (size_t i=0; i<enigma::surface_max; i++)
        enigma::surface_array[i]=oldarray[i];

      enigma::surface_array[enigma::surface_max]=NULL;
      enigma::surface_max++;
      delete[] oldarray;
    }
  }

  enigma::surface_array[id] = new enigma::surface;
  enigma::surface_array[id]->width = w;
  enigma::surface_array[id]->height = h;

  int texture = enigma::graphics_create_texture(w,h,0,false);
  glGenFramebuffers(1, &fbo);
  glPushAttrib(GL_TEXTURE_BIT);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureStructs[texture]->gltex);

  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA, w, h, false);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureStructs[texture]->gltex, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
    glClearColor(1,1,1,0);
  glClear(GL_COLOR_BUFFER_BIT);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, enigma::bound_framebuffer);
  glPopAttrib();

  enigma::surface_array[id]->tex = texture;
  enigma::surface_array[id]->fbo = fbo;

  return id;
}

void surface_set_target(int id)
{
  get_surface(surf,id);
  texture_reset();
  //This fixes several consecutive surface_set_target() calls without surface_reset_target.
  if (enigma::bound_framebuffer != 0) glPopAttrib(); glPopMatrix();
  //
  enigma::bound_framebuffer = surf->fbo;
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surf->fbo); //bind it
  glPushMatrix(); //So you can pop it in the reset
  glPushAttrib(GL_VIEWPORT_BIT); //same
  glViewport(0,0,surf->width,surf->height);
  glLoadIdentity();
  glOrtho(0, surf->width, 0, surf->height, -1, 1);
}

void surface_reset_target(void)
{
  texture_reset();
  enigma::bound_framebuffer = 0;
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glPopAttrib();
  glPopMatrix();
}

void surface_free(int id)
{
  get_surface(surf,id);
  if (enigma::bound_framebuffer == surf->fbo) glBindFramebuffer(GL_DRAW_FRAMEBUFFER, enigma::bound_framebuffer=0);
  enigma::graphics_delete_texture(surf->tex);
  glDeleteFramebuffers(1, &surf->fbo);
  surf->width = surf->height = surf->tex = surf->fbo = 0;
  delete surf;
  enigma::surface_array[id] = NULL;
}

bool surface_exists(int id)
{
    return size_t(id) < enigma::surface_max && enigma::surface_array[id] != NULL;
}

void draw_surface(int id, gs_scalar x, gs_scalar y)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);
  int w=surf->width;
  int h=surf->height;

  const gs_scalar data[4*8] = {
    x, y, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
    x+w, y, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
    x+w, y+h, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    x, y+h, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0
  };
  plane2D_rotated(data);
}

void draw_surface_stretched(int id, gs_scalar x, gs_scalar y, gs_scalar w, gs_scalar h)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  const gs_scalar data[4*8] = {
    x, y, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
    x+w, y, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
    x+w, y+h, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    x, y+h, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0
  };
  plane2D_rotated(data);
}

void draw_surface_part(int id, gs_scalar left, gs_scalar top, gs_scalar width, gs_scalar height, gs_scalar x, gs_scalar y)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  const float tbw=surf->width,tbh=surf->height;

  const gs_scalar data[4*8] = {
    x, y, left/tbw,top/tbh, 1.0, 1.0, 1.0, 1.0,
    x+width, y, (left+width)/tbw,top/tbh, 1.0, 1.0, 1.0, 1.0,
    x+width, y+height, (left+width)/tbw,(top+height)/tbh, 1.0, 1.0, 1.0, 1.0,
    x, y+height, left/tbw,(top+height)/tbh, 1.0, 1.0, 1.0, 1.0
  };
  plane2D_rotated(data);
}

void draw_surface_tiled(int id, gs_scalar x, gs_scalar y)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  x=surf->width-fmod(x,surf->width);
  y=surf->height-fmod(y,surf->height);
  const int hortil= int (ceil(room_width/(surf->width))),
            vertil= int (ceil(room_height/(surf->height)));

  for (int i=0; i<hortil; i++)
  {
    for (int c=0; c<vertil; c++)
    {
      const gs_scalar data[4*8] = {
        i*surf->width-x,c*surf->height-y, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        (i+1)*surf->width-x,c*surf->height-y, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        (i+1)*surf->width-x,(c+1)*surf->height-y, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
        i*surf->width-x,(c+1)*surf->height-y, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0
      };
      plane2D_rotated(data);
    }
  }
}

void draw_surface_tiled_area(int id, gs_scalar x, gs_scalar y, gs_scalar x1, gs_scalar y1, gs_scalar x2, gs_scalar y2)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  gs_scalar sw,sh,i,j,jj,left,top,width,height,X,Y;
  sw = surf->width;
  sh = surf->height;

  i = x1-(fmod(x1,sw) - fmod(x,sw)) - sw*(fmod(x1,sw)<fmod(x,sw));
  j = y1-(fmod(y1,sh) - fmod(y,sh)) - sh*(fmod(y1,sh)<fmod(y,sh));
  jj = j;

  for(; i<=x2; i+=sw)
  {
    for(; j<=y2; j+=sh)
    {
      if(i <= x1) left = x1-i;
      else left = 0;
      X = i+left;

      if(j <= y1) top = y1-j;
      else top = 0;
      Y = j+top;

      if(x2 <= i+sw) width = ((sw)-(i+sw-x2)+1)-left;
      else width = sw-left;

      if(y2 <= j+sh) height = ((sh)-(j+sh-y2)+1)-top;
      else height = sh-top;

      const gs_scalar data[4*8] = {
        X,Y, left/sw,top/sh, 1.0, 1.0, 1.0, 1.0,
        X+width,Y, (left+width)/sw,top/sh, 1.0, 1.0, 1.0, 1.0,
        X+width,Y+height, (left+width)/sw,(top+height)/sh, 1.0, 1.0, 1.0, 1.0,
        X,Y+height, left/sw,(top+height)/sh, 1.0, 1.0, 1.0, 1.0
      };
      plane2D_rotated(data);
    }
    j = jj;
  }
}

void draw_surface_ext(int id, gs_scalar x, gs_scalar y, gs_scalar xscale, gs_scalar yscale, double rot, int color, gs_scalar alpha)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  const gs_scalar w=surf->width*xscale, h=surf->height*yscale;
  rot *= M_PI/180;

  gs_scalar ulcx = x + xscale * cos(M_PI+rot) + yscale * cos(M_PI/2+rot),
            ulcy = y - yscale * sin(M_PI+rot) - yscale * sin(M_PI/2+rot),
            ulcx2 = ulcx+ h * cos(3*M_PI/2 + rot),
            ulcy2 = ulcy- h * sin(3*M_PI/2 + rot);


  gs_scalar cr = w*cos(rot), sr = w*sin(rot);

  gs_scalar r = __GETR(color), g = __GETG(color), b = __GETB(color);
  const gs_scalar data[4*8] = {
    ulcx,ulcy, 0.0, 0.0, r, g, b, alpha,
    ulcx + cr, ulcy - sr, 1.0, 0.0, r, g, b, alpha,
    ulcx2 + cr, ulcy2 - sr, 1.0, 1.0, r, g, b, alpha,
    ulcx2,ulcy2, 0.0, 1.0, r, g, b, alpha
  };
  plane2D_rotated(data);
}

void draw_surface_stretched_ext(int id, gs_scalar x, gs_scalar y, gs_scalar w, gs_scalar h, int color, gs_scalar alpha)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  gs_scalar r = __GETR(color), g = __GETG(color), b = __GETB(color);
  const gs_scalar data[4*8] = {
    x,y, 0.0, 0.0, r, g, b, alpha,
    x + w, y, 1.0, 0.0, r, g, b, alpha,
    x + w, y + h, 1.0, 1.0, r, g, b, alpha,
    x, y+h, 0.0, 1.0, r, g, b, alpha
  };
  plane2D_rotated(data);
}

void draw_surface_part_ext(int id, gs_scalar left, gs_scalar top, gs_scalar width, gs_scalar height, gs_scalar x, gs_scalar y, gs_scalar xscale, gs_scalar yscale, int color, gs_scalar alpha)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  const float tbw = surf->width, tbh = surf->height;

  gs_scalar r = __GETR(color), g = __GETG(color), b = __GETB(color);
  const gs_scalar data[4*8] = {
    x,y, left/tbw,top/tbh, r, g, b, alpha,
    x+width*xscale,y, (left+width)/tbw,top/tbh, r, g, b, alpha,
    x+width*xscale,y+height*yscale, (left+width)/tbw,(top+height)/tbh, r, g, b, alpha,
    x,y+height*yscale, left/tbw,(top+height)/tbh, r, g, b, alpha
  };
  plane2D_rotated(data);
}

void draw_surface_tiled_ext(int id, gs_scalar x, gs_scalar y, gs_scalar xscale, gs_scalar yscale, int color, gs_scalar alpha)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  const gs_scalar w=surf->width*xscale, h=surf->height*yscale;
  const int hortil= int (ceil(room_width/(surf->width))),
            vertil= int (ceil(room_height/(surf->height)));
  x=w-fmod(x,w);
  y=h-fmod(y,h);
  gs_scalar r = __GETR(color), g = __GETG(color), b = __GETB(color);
  for (int i=0; i<hortil; i++)
  {
    for (int c=0; c<vertil; c++)
    {
      const gs_scalar data[4*8] = {
          i*w-x,c*h-y, 0.0, 0.0, r, g, b, alpha,
          (i+1)*w-x,c*h-y, 1.0, 0.0, r, g, b, alpha,
          (i+1)*w-x,(c+1)*h-y, 1.0, 1.0, r, g, b, alpha,
          i*w-x,(c+1)*h-y, 0.0, 1.0, r, g, b, alpha
      };
      plane2D_rotated(data);
    }
  }
}

void draw_surface_tiled_area_ext(int id, gs_scalar x, gs_scalar y, gs_scalar x1, gs_scalar y1, gs_scalar x2, gs_scalar y2, gs_scalar xscale, gs_scalar yscale, int color, gs_scalar alpha)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  gs_scalar sw,sh,i,j,jj,left,top,width,height,X,Y;
  sw = surf->width*xscale;
  sh = surf->height*yscale;

  i = x1-(fmod(x1,sw) - fmod(x,sw)) - sw*(fmod(x1,sw)<fmod(x,sw));
  j = y1-(fmod(y1,sh) - fmod(y,sh)) - sh*(fmod(y1,sh)<fmod(y,sh));
  jj = j;

  gs_scalar r = __GETR(color), g = __GETG(color), b = __GETB(color);
  for(; i<=x2; i+=sw)
  {
    for(; j<=y2; j+=sh)
    {
      if(i <= x1) left = x1-i;
      else left = 0;
      X = i+left;

      if(j <= y1) top = y1-j;
      else top = 0;
      Y = j+top;

      if(x2 <= i+sw) width = ((sw)-(i+sw-x2)+1)-left;
      else width = sw-left;

      if(y2 <= j+sh) height = ((sh)-(j+sh-y2)+1)-top;
      else height = sh-top;

      const gs_scalar data[4*8] = {
          X, Y, left/sw,top/sh, r, g, b, alpha,
          X+width,Y, (left+width)/sw,top/sh, r, g, b, alpha,
          X+width,Y+height, (left+width)/sw,(top+height)/sh, r, g, b, alpha,
          X,Y+height, left/sw,(top+height)/sh, r, g, b, alpha
      };
      plane2D_rotated(data);
    }
    j = jj;
  }
}

void draw_surface_general(int id, gs_scalar left, gs_scalar top, gs_scalar width, gs_scalar height, gs_scalar x, gs_scalar y, gs_scalar xscale, gs_scalar yscale, double rot, int c1, int c2, int c3, int c4, gs_scalar a1, gs_scalar a2, gs_scalar a3, gs_scalar a4)
{
  get_surface(surf,id);
  texture_set(textureStructs[surf->tex]->gltex);

  const gs_scalar tbw = surf->width, tbh = surf->height,
        w = width*xscale, h = height*yscale;

  rot *= M_PI/180;

  gs_scalar ulcx = x + xscale * cos(M_PI+rot) + yscale * cos(M_PI/2+rot),
            ulcy = y - yscale * sin(M_PI+rot) - yscale * sin(M_PI/2+rot),
            ulcx2 = ulcx + h * cos(3*M_PI/2 + rot),
            ulcy2 = ulcy - h * sin(3*M_PI/2 + rot),
            cr = w*cos(rot), sr = w*sin(rot);

  const gs_scalar data[4*8] = {
    ulcx,ulcy, left/tbw,top/tbh, __GETR(c1),__GETG(c1),__GETB(c1), a1,
    (ulcx + cr), (ulcy - sr), (left+width)/tbw,top/tbh,  __GETR(c2),__GETG(c2),__GETB(c2), a2,
    (ulcx2 + cr), (ulcy2 - sr), (left+width)/tbw,(top+height)/tbh,  __GETR(c3),__GETG(c3),__GETB(c3), a3,
    ulcx2,ulcy2,(top+height)/tbh,  __GETR(c4),__GETG(c4),__GETB(c4), a4
  };
  plane2D_rotated(data);
}

int surface_get_texture(int id)
{
    get_surfacev(surf,id,-1);
    return (surf->tex);
}

int surface_get_width(int id)
{
    get_surfacev(surf,id,-1);
    return (surf->width);
}

int surface_get_height(int id)
{
    get_surfacev(surf,id,-1);
    return (surf->height);
}

int surface_getpixel(int id, int x, int y)
{
    get_surfacev(surf,id,-1);
    unsigned char *pixelbuf=new unsigned char[3];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, surf->fbo);
	glReadPixels(x,y,1,1,GL_RGB,GL_UNSIGNED_BYTE,pixelbuf);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::bound_framebuffer);
    return pixelbuf[0] + (pixelbuf[1] << 8) + (pixelbuf[2] << 16);
}

int surface_getpixel_alpha(int id, int x, int y)
{
    get_surfacev(surf,id,-1);
    unsigned char *pixelbuf=new unsigned char[1];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, surf->fbo);
	glReadPixels(x,y,1,1,GL_ALPHA,GL_UNSIGNED_BYTE,pixelbuf);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::bound_framebuffer);
    return pixelbuf[0];
}

int surface_get_bound()
{
    return enigma::bound_framebuffer;
}

}

//////////////////////////////////////SAVE TO FILE AND CREATE SPRITE FUNCTIONS/////////
//Fuck whoever did this to the spec
#ifndef GL_BGR
  #define GL_BGR 0x80E0
#endif

#include "Universal_System/estring.h"
#include "Universal_System/image_formats.h"

namespace enigma_user
{

int surface_save(int id, string filename)
{
    get_surfacev(surf,id,-1);
	unsigned int w=surf->width,h=surf->height,sz=w*h;

    string ext = enigma::image_get_format(filename);

    unsigned char *rgbdata = new unsigned char[sz*4];

	glBindFramebuffer(GL_READ_FRAMEBUFFER, surf->fbo);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgbdata);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, enigma::bound_framebuffer);

	int ret;
	if (ext == ".bmp") {
		unsigned char* data = enigma::image_reverse_scanlines(rgbdata, w, h, 4);
		ret = enigma::image_save(filename, data, w, h, w, h);
		delete[] data;
	} else {
		ret = enigma::image_save(filename, rgbdata, w, h, w, h);
	}
	
	delete[] rgbdata;
	return ret;
}

int surface_save_part(int id, string filename, unsigned x, unsigned y, unsigned w, unsigned h)
{
    get_surfacev(surf,id,-1);
	unsigned int sz=w*h;

    string ext = enigma::image_get_format(filename);

    unsigned char *rgbdata = new unsigned char[sz*4];
	
	glBindFramebuffer(GL_READ_FRAMEBUFFER, surf->fbo);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x,y,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgbdata);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, enigma::bound_framebuffer);

	int ret;
	if (ext == ".bmp") {
		unsigned char* data = enigma::image_reverse_scanlines(rgbdata, w, h, 4);
		ret = enigma::image_save(filename, data, w, h, w, h);
		delete[] data;
	} else {
		ret = enigma::image_save(filename, rgbdata, w, h, w, h);
	}
	
	delete[] rgbdata;
	return ret;
}

int background_create_from_surface(int id, int x, int y, int w, int h, bool removeback, bool smooth, bool preload)
{
    get_surfacev(surf,id,-1);
    int full_width=nlpo2dc(w)+1, full_height=nlpo2dc(h)+1;
    
    unsigned sz=full_width*full_height;
    unsigned char *surfbuf=new unsigned char[sz*4];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, surf->fbo);
    glReadPixels(x,y,w,h,GL_RGBA,GL_UNSIGNED_BYTE,surfbuf);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::bound_framebuffer);
	enigma::backgroundstructarray_reallocate();
    int bckid=enigma::background_idmax;
	enigma::background_new(bckid, w, h, surfbuf, removeback, smooth, preload, false, 0, 0, 0, 0, 0, 0);
    delete[] surfbuf;
	enigma::background_idmax++;
    return bckid;
}

int sprite_create_from_surface(int id, int x, int y, int w, int h, bool removeback, bool smooth, bool preload, int xorig, int yorig)
{
    get_surfacev(surf,id,-1);
    int full_width=nlpo2dc(w)+1, full_height=nlpo2dc(h)+1;
    enigma::spritestructarray_reallocate();
    int sprid=enigma::sprite_idmax;
    enigma::sprite_new_empty(sprid, 1, w, h, xorig, yorig, 0, h, 0, w, preload, smooth);

    unsigned sz=full_width*full_height;
    unsigned char *surfbuf=new unsigned char[sz*4];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, surf->fbo);
    glReadPixels(x,y,w,h,GL_RGBA,GL_UNSIGNED_BYTE,surfbuf);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::bound_framebuffer);
    enigma::sprite_set_subimage(sprid, 0, w, h, surfbuf, surfbuf, enigma::ct_precise); //TODO: Support toggling of precise.
    delete[] surfbuf;
    return sprid;
}

int sprite_create_from_surface(int id, int x, int y, int w, int h, bool removeback, bool smooth, int xorig, int yorig)
{
	return sprite_create_from_surface(id, x, y, w, h, removeback, smooth, true, xorig, yorig);
}

void sprite_add_from_surface(int ind, int id, int x, int y, int w, int h, bool removeback, bool smooth)
{
    get_surface(surf,id);
    int full_width=nlpo2dc(w)+1, full_height=nlpo2dc(h)+1;

    unsigned sz=full_width*full_height;
    unsigned char *surfbuf=new unsigned char[sz*4];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, surf->fbo);
    glReadPixels(x,y,w,h,GL_RGBA,GL_UNSIGNED_BYTE,surfbuf);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::bound_framebuffer);
    enigma::sprite_add_subimage(ind, w, h, surfbuf, surfbuf, enigma::ct_precise); //TODO: Support toggling of precise.
    delete[] surfbuf;
}

void surface_copy_part(int destination, float x, float y, int source, int xs, int ys, int ws, int hs)
{
    get_surface(ssurf,source);
    get_surface(dsurf,destination);
    unsigned char *surfbuf=new unsigned char[ws*hs*4];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ssurf->fbo);
	glReadPixels(xs,ys,ws,hs,GL_RGBA,GL_UNSIGNED_BYTE,surfbuf);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dsurf->fbo);
    glPushMatrix();
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0,0,dsurf->width,dsurf->height);
    glLoadIdentity();
    glOrtho(-1, dsurf->width, -1, dsurf->height, -1, 1);
	glRasterPos2d(x, y);
	glDrawPixels(ws,hs,GL_RGBA,GL_UNSIGNED_BYTE,surfbuf);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::bound_framebuffer);
    glPopAttrib();
    glPopMatrix();
	glRasterPos2d(0, 0);
	delete[] surfbuf;
}

void surface_copy(int destination, float x, float y, int source)
{
    get_surface(ssurf,source);
    get_surface(dsurf,destination);
    unsigned char *surfbuf=new unsigned char[dsurf->width*dsurf->height*4];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ssurf->fbo);
	glReadPixels(0,0,dsurf->width,dsurf->height,GL_RGBA,GL_UNSIGNED_BYTE,surfbuf);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dsurf->fbo);
    glPushMatrix();
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0,0,dsurf->width,dsurf->height);
    glLoadIdentity();
    glOrtho(-1, dsurf->width, -1, dsurf->height, -1, 1);
	glRasterPos2d(x, y);
	glDrawPixels(dsurf->width,dsurf->height,GL_RGBA,GL_UNSIGNED_BYTE,surfbuf);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::bound_framebuffer);
    glPopAttrib();
    glPopMatrix();
	glRasterPos2d(0, 0);
	delete[] surfbuf;
}

}
