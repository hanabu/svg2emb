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


static const float LINE_PITCH = 2.0;          /* mm */
static const float TRIPLE_STITCH_WIDTH = 0.1; /* mm */


/*
 * make star stitch for conductive thread connection
 */
static std::vector<math::vector2d>
make_star(const math::vector2d center, const float r)
{
  static const float vertex[][2] = {
    {  1.000000,  0.000000 },
    { -0.900969,  0.433884 },
    {  0.623490, -0.781831 },
    { -0.222521,  0.974928 },
    { -0.222521, -0.974928 },
    {  0.623490,  0.781831 },
    { -0.900969, -0.433884 },
  };

  std::vector<math::vector2d> points;
  points.reserve(8);
  for(int i=0; i<7; ++i){
    math::vector2d pos(vertex[i]);
    points.push_back(center + pos*r);
  }
  points.push_back(points.front());
  return points;
}


/*
 * Make points on SVG path
 */
static std::vector<math::vector2d>
make_points_on_path(const NSVGshape* shape,
                    const float pitch)
{
  for(NSVGpath* path=shape->paths; NULL!=path; path=path->next){
    std::vector<math::vector2d> points;
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

      /* add point at every pitch */
      float t=0.0;
      carryov = curve.move_on_curve(t, carryov);
      while( t<1.0 ){
        points.push_back( curve.curve(t) );
        /* go next stitch */
        carryov = curve.move_on_curve(t, pitch);
      }
    }
    if( 0.25*pitch > carryov  ){
      /* move last point */
      points.back() = lastpt;
    }else{
      /* add one more point at last */
      points.push_back(lastpt);
    }
    
    return points;
  }
}



/*
 * path as single stitch
 */
static std::vector<math::vector2d>
make_single_stitch(const NSVGshape* shape, const float pitch,
                   bool startstar, bool endstar)
{
  std::vector<math::vector2d> points
    = make_points_on_path(shape, pitch);

  if( startstar || endstar ){
    if( 1>=points.size() ){
      /* too few points (too short segment) */
      return points;
    }

    std::vector<math::vector2d> stitchsegment;
    stitchsegment.reserve(stitchsegment.size()+16);

    if( startstar ){
      /* make star at start */
      std::vector<math::vector2d> star
        = make_star(points.front(), 0.5*pitch);
      stitchsegment.insert(stitchsegment.end(), star.begin(), star.end());
    }

    /* join points */
    stitchsegment.insert(stitchsegment.end(), points.begin(), points.end());

    if( endstar ){
      /* make star at end */
      std::vector<math::vector2d> star
        = make_star(points.back(), 0.5*pitch);
      stitchsegment.insert(stitchsegment.end(), star.begin(), star.end());
    }

    return stitchsegment;
  }else{
    /* no star, stitches are points only*/
    return points;
  }
}


/*
 * path as tripple stitch
 */
static std::vector<math::vector2d>
make_tripple_stitch(const NSVGshape* shape, const float pitch,
                    bool startstar, bool endstar)
{
  std::vector<math::vector2d> points
    = make_points_on_path(shape, pitch);

  if( 1>=points.size() ){
    /* Too short path */
    return points;
  }

  std::vector<math::vector2d> stitchsegment;
  stitchsegment.reserve(stitchsegment.size()*3 + 16);
  
  /* first pass */
  stitchsegment.insert(stitchsegment.end(), points.begin(), points.end());

  if( endstar ){
    /* make star at end */
    std::vector<math::vector2d> star
      = make_star(points.back(), 0.5*pitch);
    stitchsegment.insert(stitchsegment.end(), star.begin(), star.end());
  }

  /* second pass, reverse order */
  stitchsegment.insert(stitchsegment.end(), points.rbegin(), points.rend());

  if( startstar ){
    /* make star at start */
    std::vector<math::vector2d> star
      = make_star(points.front(), 0.5*pitch);
    stitchsegment.insert(stitchsegment.end(), star.begin(), star.end());
  }

  /* third pass */
  stitchsegment.insert(stitchsegment.end(), points.begin(), points.end());

  return stitchsegment;
}



/* ========================================================================= */


class SVGParser {
public:
  virtual void
  parse_shape(const NSVGshape* shape,
              std::vector<std::vector<math::vector2d> >& stitches) const = 0;
};

class SVGParserNormal : public SVGParser {
public:
  void parse_shape(const NSVGshape* shape,
                   std::vector<std::vector<math::vector2d> >& stitches) const
  {
    if( NSVG_PAINT_NONE!=shape->fill.type ){
      /* TODO : implement filled shape */
    }
    if( NSVG_PAINT_COLOR==shape->stroke.type ){
      /* Trace path */
      std::vector<math::vector2d> stitchsegment;
      if( TRIPLE_STITCH_WIDTH <= shape->strokeWidth ){
        /* wide stroke => tipple stitch */
        stitchsegment = make_tripple_stitch(shape, LINE_PITCH, false, false);
      }else{
        /* narrow stroke => single stitch */
        stitchsegment = make_single_stitch(shape, LINE_PITCH, false, false);
      }

      if( 2<=stitchsegment.size() ){
        stitches.push_back(stitchsegment);
      }
    }
  }
};


/*
 * Special parser for Fritzing 0.9
 */
class SVGParserFritzing09 : public SVGParser {
public:
  void parse_shape(const NSVGshape* shape,
                   std::vector<std::vector<math::vector2d> >& stitches) const
  {
    if( NSVG_PAINT_COLOR==shape->stroke.type ){
      if( 0==strcmp(shape->id, "boardoutline") ){
        /* ignore outline */
      }else if( 0==strncmp(shape->id, "connector", 9) ){
        /* pad/hole */
        std::vector<math::vector2d> stitchsegment;
        stitchsegment = make_tripple_stitch(shape, 0.5*LINE_PITCH, false,false);
      
        if( 2<=stitchsegment.size() ){
          stitches.push_back(stitchsegment);
        }
      }else{
        /* this path should be wire, */
        /* trace path with tripple stitch */
        std::vector<math::vector2d> stitchsegment;
        stitchsegment = make_tripple_stitch(shape, LINE_PITCH, true, true);
      
        if( 2<=stitchsegment.size() ){
          stitches.push_back(stitchsegment);
        }
      }
    }
  }
};


/* ========================================================================= */


/*
 * Parse SVG, make stitches
 */
std::vector<std::vector<math::vector2d> >
parse_SVG(const char* filename, const SVGParser& parser)
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
    if( NSVG_FLAGS_VISIBLE!=shape->flags ){
      /* skip hidden shapes */
      continue;
    }
    parser.parse_shape(shape, stitches);
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
    std::vector<math::vector2d> stitchsegment = stitches[i];

    for(size_t j=0; j<stitchsegment.size(); ++j){
      math::vector2d pos = stitchsegment[j];

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
 * Print help message
 */
void print_help()
{
  fputs("svg2emb [-m normal|fritzing09] INPUT.svg OUTPUT.pes\n", stderr);
}


/*
 * main 
 */
int main(int argc, char* argv[])
{
  static SVGParserNormal     parser_normal;
  static SVGParserFritzing09 parser_fritzing09;
  SVGParser* svg_parser = &parser_normal;
  const char* svgfile = NULL;
  const char* outfile = NULL;

  /* parse commandline */
  for(int i=1; i<argc; ++i){
    if( '-' == argv[i][0] ){
      switch( argv[i][1] ){
      case 'm': /* -m : SVG parser mode */
        if( ++i < argc ){
          if( 0==strcasecmp(argv[i], "fritzing09") ){
            svg_parser = &parser_fritzing09;
          }else{
            svg_parser = &parser_normal;
          }
        }
        break;
      defaults:
        /* Unknown option */
        fprintf(stderr, "Unknown option \"%s\"\n\n", argv[i]);
        print_help();
        return -1;
      }
    }else{
      /* filename options */
      if( NULL==svgfile ){
        svgfile = argv[i];
      }else if( NULL==outfile ){
        outfile = argv[i];
      }else{
        fputs("Too many arguments\n\n", stderr);
        print_help();
        return -1;
      }
    }
  }

  if( NULL==svgfile || NULL==outfile ){
    fputs("Too few arguments\n\n", stderr);
    print_help();
    return -1;
  }

  try{

    std::vector<std::vector<math::vector2d> > stitches
      = parse_SVG(svgfile, *svg_parser);
    if( stitches.empty() ){
      fputs("Empty SVG.\n", stdout);
      return 1;
    }

    /* ToDo : reorder stitches */

    save_embroidery(outfile, stitches);

    return 0;
  }catch(std::exception e){
    fputs(e.what(), stdout);
    fputs("\n", stdout);
    return 1;
  }
}
