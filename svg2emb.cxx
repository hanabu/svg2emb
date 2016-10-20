/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Copyright (c) 2016, Hanabusa Masahiro All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * 3.  The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISE OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ( BSD license without advertising clause )
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include<cmath>
#include<cstring>
#include<cstdio>
#include<memory>
#include<vector>
#include<stdexcept>

#include<cubicbezier.hxx>



/*
 * Nano SVG : a simple stupid single-header-file SVG parse
 * https://github.com/memononen/nanosvg
 * 
 * licensed under zlib license,
 * Copyright (c) 2013-14 Mikko Mononen memon@inside.org
 */
#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include<nanosvg/nanosvg.h>


/*
 * Embroidermodder/libembroidery : Handles embroidery specific formats
 * https://github.com/Embroidermodder/Embroidermodder
 *
 * licensed under zlib license,
 * Copyright (c) 2011-2014
 *   Embroidermodder, Jonathan Greig, Josh Varga and all other contributers
 */
#include<libembroidery/emb-color.h>
#include<libembroidery/emb-thread.h>
#include<libembroidery/emb-pattern.h>


/* ========================================================================= */


static const float LINE_PITCH = 2.0;

/*
 * Parse SVG path
 */
static void
parse_SVG_path(const NSVGshape* shape,
               std::vector<std::vector<math::vector2d> >& stitches)
{
  for(NSVGpath* path=shape->paths; NULL!=path; path=path->next){
    std::vector<math::vector2d> stitch;
    math::vector2d lastpt;
    float carryov = 0.0;

    for(int i=0; i<path->npts-1; i+=3) {
      math::vector2d p0(&path->pts[i*2  ]);
      math::vector2d p1(&path->pts[i*2+2]);
      math::vector2d p2(&path->pts[i*2+4]);
      math::vector2d p3(&path->pts[i*2+6]);
      lastpt = p3;

      /* create bezier curve */
      CubicBezier<float,2> curve(p0, p1, p2, p3);

      /* add stitch at every LINE_PITCH */
      float t=0.0;
      carryov = curve.move_on_curve(t, carryov);
      while( t<1.0 ){
        stitch.push_back( curve.curve(t) );
        /* go next stitch */
        carryov = curve.move_on_curve(t, LINE_PITCH);
      }
    }
    if( carryov > 0.25*LINE_PITCH ){
      /* stitch at last point */
      stitch.push_back(lastpt);
    }
    
    stitches.push_back(stitch);
  }
}


/*
 * Parse SVG, make StitchElements
 */
std::vector<std::vector<math::vector2d> > parse_SVG(const char* filename)
  throw(std::runtime_error)
{
  std::vector<std::vector<math::vector2d> > stitches;

  /* read SVG */
  struct NSVGimage* svg;
  svg = nsvgParseFromFile(filename, "mm", 90);
  if( NULL==svg ){
    throw std::runtime_error("Can not read SVG file.");
  }

  /* parse SVG */
  for(NSVGshape* shape=svg->shapes; NULL!=shape; shape=shape->next){
    if( NSVG_PAINT_NONE ==shape->fill.type &&
        NSVG_PAINT_COLOR==shape->stroke.type ){
      /* Path type shape */
      parse_SVG_path(shape, stitches);
    }else if( NSVG_PAINT_NONE!=shape->fill.type ){
      /* TODO : implement filled shape */
    }
  }

  /* relase SVG */
  nsvgDelete(svg);

  return stitches;
}


/*
 * Save stitches as embroidery machine format
 */
static void
save_embroidery(const char* filename,
                const std::vector<std::vector<math::vector2d> > stitches)
  throw(std::runtime_error)
{
  EmbColor black = { 0, 0, 0 };
  EmbThread thread = { black, "Black", "900" };
  EmbPattern* pat;

  pat = embPattern_create();

  /* set thread & color */
  embPattern_addThread(pat, thread);
  embPattern_changeColor(pat, 0);

  for(size_t i=0; i<stitches.size(); ++i){
    std::vector<math::vector2d> stitch = stitches[i];

    for(size_t j=0; j<stitch.size(); ++j){
      math::vector2d pos = stitch[j];

      if( 0==j ){
        if( 0<i ){
          /* cut thread */
          embPattern_addStitchAbs(pat, pos[0], -pos[1], TRIM, 0);
        }
        /* jump to new segment */
        embPattern_addStitchAbs(pat, pos[0], -pos[1], JUMP, 0);
      }
      embPattern_addStitchAbs(pat, pos[0], -pos[1], NORMAL, 0);
    }
  }
  embPattern_addStitchRel(pat, 0.0, 0.0, END, 0);

  if( ! embPattern_write(pat, filename) ){
    /* write failed */
    throw std::runtime_error("Failed to write embroidery file.");
  }  
}


/*
 * debug print
 */
static void
print_stitches(const std::vector<std::vector<math::vector2d> > stitches,
               FILE* fp)
{
  for(size_t i=0; i<stitches.size(); ++i){
    std::vector<math::vector2d> stitch = stitches[i];
    
    for(size_t j=0; j<stitch.size(); ++j){
      math::vector2d pos = stitch[j];

      fprintf(fp, "%f,%f\n", pos[0], pos[1]);
    }
    fputs(",\n", fp);
  }
}


/*
 * main 
 */
int main(int argc, char* argv[])
{
  if( 3!=argc ){
    fputs("svg2emb INPUT.svg OUTPUT.pes\n", stderr);
    return 1;
  }

  try{
    std::vector<std::vector<math::vector2d> > stitches
      = parse_SVG(argv[1]);
    if( stitches.empty() ){
      fputs("Empty SVG.\n", stdout);
      return 1;
    }

    /* ToDo : reorder stitches */

    save_embroidery(argv[2], stitches);

    return 0;
  }catch(std::exception e){
    fputs(e.what(), stdout);
    fputs("\n", stdout);
    return 1;
  }
}
