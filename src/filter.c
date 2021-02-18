/*

   filter.c CopyRight 2021 Remco Schellekens, see LICENSE for more details.

   This file contains all functions for applying filters to layers
   All filters are permanent and not reversable, in some cases a temporary
   framebuffer has to be created, so might costs temporary memory

*/

#include <stdlib.h>
#include <stdio.h>
#include "sil.h"
#include "log.h"


/*****************************************************************************
  Applyfilter to given layer
  filter can be :
   SILFLTR_LIGHTEN:        Add 20% more white to all pixels
   SILFLTR_DARKEN:         Substract 20% more white from all pixels
   SILFLTR_BORDER:         Draw a dotted border at the edges of the layer
   SILFLTR_AUTOCROPALPHA:  Autocrop layer based on pixels with zero alpha
   SILFLTR_AUTOCROPFIRST:  Autocrop layer based on pixels that are the same 
                           as first pixel topleft corner of image
   SILFLTR_BLUR:           Blur the layer (3x3 box blur)
   SILFLTR_GRAYSCALE:      Remove color from layer
   SILFLTR_REVERSECOLOR:   Reverse colorspectrum 
   SILFLTR_FLIPX:          Flip layer over the X-axis
   SILFLTR_FLIPY:          Flip layer over the Y-axis
   SILFLTR_ROTATECOLOR:    rotate colorchannels Red=Green,Green=Blue,Blue=Red
   SILFLTR_BLENDFIRST:     Every pixel, that is the same as the first pixel 
                           in the topleft corner, will get alpha set to 0

  
 Returns SILERR_ALLOK if all wend well, otherwise the errorcode

 *****************************************************************************/

UINT sil_applyFilterLayer(SILLYR *layer, BYTE filter) {
  UINT err=SILERR_ALLOK;
  SILFB *dest;
  UINT cnt;
  int minx,miny,maxx,maxy;
  BYTE red,green,blue,alpha;
  UINT dred,dgreen,dblue,dalpha;
  BYTE red2,green2,blue2,alpha2;

#ifndef SIL_LIVEDANGEROUS
  if ((NULL==layer)||(0==layer->init)) {
    log_warn("Trying to applyFilter against non-existing or non-initialized layer");
    sil_setErr(SILERR_NOTINIT);
    return SILERR_NOTINIT;
  }
  if (NULL==layer->fb) {
    log_warn("Trying to applyFilter against layer with no framebuffer");
    sil_setErr(SILERR_NOTINIT);
    return SILERR_NOTINIT;
  }
  if (0==layer->fb->size) {
    log_warn("Trying to applyFilter against layer with zero size framebuffer");
    sil_setErr(SILERR_WRONGFORMAT);
    return SILERR_WRONGFORMAT;
  }
#endif


  switch(filter) {
    case SILFLTR_LIGHTEN:
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          if (red<205)  {
            red+=50;
          } else
            red=255;
          if (blue<205) 
            blue+=50;
          else
            blue=255;
          if (green<205) 
            green+=50;
          else
            green=255;
          sil_putPixelLayer(layer,x,y,red,green,blue,alpha); 
        }
      }
      break;

    case SILFLTR_DARKEN:
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          if (red>50)  {
            red-=50;
          } else
            red=0;
          if (blue>50) 
            blue-=50;
          else
            blue=0;
          if (green>50) 
            green-=50;
          else
            green=0;
          sil_putPixelLayer(layer,x,y,red,green,blue,alpha); 
        }
      }
      break;


    case SILFLTR_BORDER:
      for (int x=0;x<layer->fb->width;x++) {
        sil_putPixelLayer(layer,x,0,255*(x%2),255*(x%2),255*(x%2),255);
        sil_putPixelLayer(layer,x,layer->fb->height-1,255*(x%2),255*(x%2),255*(x%2),255);
      }
      for (int y=0;y<layer->fb->height;y++) {
        sil_putPixelLayer(layer,0,y,255*(y%2),255*(y%2),255*(y%2),255);
        sil_putPixelLayer(layer,layer->fb->width-1,y,255*(y%2),255*(y%2),255*(y%2),255);
      }
      break;

    case SILFLTR_AUTOCROPFIRST:
      minx=layer->fb->width-1;
      miny=layer->fb->height-1;
      maxx=0;
      maxy=0;
      sil_getPixelLayer(layer,0,0,&red2,&green2,&blue2,&alpha2); 
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          if ((red2!=red)||(green2!=green)||(blue2!=blue)) {
            if (x<minx) minx=x;
            if (y<miny) miny=y;
            if (maxx<x) maxx=x;
            if (maxy<y) maxy=y;
          } 
        }
      }
      err=sil_resizeLayer(layer,minx,miny,maxx+1,maxy+1);
      break;

    case SILFLTR_AUTOCROPALPHA:
      minx=layer->fb->width-1;
      miny=layer->fb->height-1;
      maxx=0;
      maxy=0;
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          if (alpha) {
            if (x<minx) minx=x;
            if (y<miny) miny=y;
            if (maxx<x) maxx=x;
            if (maxy<y) maxy=y;
          } 
        }
      }
      err=sil_resizeLayer(layer,minx,miny,maxx+1,maxy+1);
      break;

    case SILFLTR_BLENDFIRST:
      sil_getPixelLayer(layer,0,0,&red,&green,&blue,&alpha); 
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red2,&green2,&blue2,&alpha2); 
          if ((green==green2)&&(red==red2)&&(blue==blue2)) {
            sil_putPixelLayer(layer,x,y,red,green,blue,0); 
          }
        }
      }
      break;

    case SILFLTR_FLIPX:
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<(layer->fb->height)/2;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          sil_getPixelLayer(layer,x,(layer->fb->height)-y-1,&red2,&green2,&blue2,&alpha2); 
          sil_putPixelLayer(layer,x,y,red2,green2,blue2,alpha2); 
          sil_putPixelLayer(layer,x,(layer->fb->height)-y-1,red,green,blue,alpha); 
        }
      }
      break;

    case SILFLTR_FLIPY:
      for (int x=0;x<(layer->fb->width)/2;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          sil_getPixelLayer(layer,(layer->fb->width)-x-1,y,&red2,&green2,&blue2,&alpha2); 
          sil_putPixelLayer(layer,x,y,red2,green2,blue2,alpha2); 
          sil_putPixelLayer(layer,(layer->fb->width)-x-1,y,red,green,blue,alpha); 
        }
      }
      break;

    case SILFLTR_GRAYSCALE:
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          red=0.21*(float)red+0.71*(float)green+0.07*(float)blue;
          green=red;
          blue=red;
          sil_putPixelLayer(layer,x,y,red,green,blue,alpha); 
        }
      }
      break;

    case SILFLTR_REVERSECOLOR:
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          sil_putPixelLayer(layer,x,y,255-red,255-green,255-blue,alpha); 
        }
      }
      break;

    case SILFLTR_ROTATECOLOR:
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha); 
          sil_putPixelLayer(layer,x,y,green,blue,red,alpha); 
        }
      }
      break;

    case SILFLTR_BLUR:
      /* for this, we need to create a seperate FB temporary */

      dest=sil_initFB(layer->fb->width,layer->fb->height,layer->fb->type);
      if (NULL==dest) {
        log_info("ERR: Cant create framebuffer for blur filter");
        return sil_getErr();
      }
      for (int x=0;x<layer->fb->width;x++) {
        for (int y=0;y<layer->fb->height;y++) {
          cnt=0;
          /* mid */
          sil_getPixelLayer(layer,x,y,&red,&green,&blue,&alpha);
          dred=red;
          dgreen=green;
          dblue=blue;
          dalpha=alpha;
          cnt=1;
          /* top */
          if (y-1>0) {
            sil_getPixelLayer(layer,x,y-1,&red,&green,&blue,&alpha);
            dred+=red;
            dgreen+=green;
            dblue+=blue;
            dalpha+=alpha;
            cnt++;
            /* topright */
            if (x+1<layer->fb->width) {
              sil_getPixelLayer(layer,x+1,y-1,&red,&green,&blue,&alpha);
              dred+=red;
              dgreen+=green;
              dblue+=blue;
              dalpha+=alpha;
              cnt++;
            }
          }
          /* right */
          if (x+1<layer->fb->width) {
            sil_getPixelLayer(layer,x+1,y,&red,&green,&blue,&alpha);
            dred+=red;
            dgreen+=green;
            dblue+=blue;
            dalpha+=alpha;
            cnt++;
            /* bottomright */
            if (y+1<layer->fb->height) {
              sil_getPixelLayer(layer,x+1,y+1,&red,&green,&blue,&alpha);
              dred+=red;
              dgreen+=green;
              dblue+=blue;
              dalpha+=alpha;
              cnt++;
            }
          }
          /* bottom */
          if (y+1<layer->fb->height) {
            sil_getPixelLayer(layer,x,y+1,&red,&green,&blue,&alpha);
            dred+=red;
            dgreen+=green;
            dblue+=blue;
            dalpha+=alpha;
            cnt++;
            /* bottomleft */
            if (x-1>0) {
              sil_getPixelLayer(layer,x-1,y+1,&red,&green,&blue,&alpha);
              dred+=red;
              dgreen+=green;
              dblue+=blue;
              dalpha+=alpha;
              cnt++;
            }
          }
          /* left */
          if (x-1>0) {
            sil_getPixelLayer(layer,x-1,y,&red,&green,&blue,&alpha);
            dred+=red;
            dgreen+=green;
            dblue+=blue;
            dalpha+=alpha;
            cnt++;
            /* topleft */
            if (y-1>0) {
              sil_getPixelLayer(layer,x-1,y-1,&red,&green,&blue,&alpha);
              dred+=red;
              dgreen+=green;
              dblue+=blue;
              dalpha+=alpha;
              cnt++;
            }
          }
          red=dred/cnt;
          green=dgreen/cnt;
          blue=dblue/cnt;
          alpha=dalpha/cnt;
          sil_putPixelFB(dest,x,y,red,green,blue,alpha); 
        }
      }
      /* swap framebuffers and remove the old one */
      if (layer->fb->buf) free(layer->fb->buf);
      layer->fb->buf=dest->buf;
      break;
  }
  sil_setErr(err);
  return err;
}
