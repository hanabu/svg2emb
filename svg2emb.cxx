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

#include<mathtransm.hxx>
#include<cubicbezier.hxx>
#include<embwrite.hxx>


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





/* ========================================================================= */


static const float LINE_PITCH = 2.0;          /* mm */
static const float TRIPLE_STITCH_WIDTH = 0.1; /* mm */



class SVGParser {
public:
  virtual void
  parse_shape(const NSVGshape* shape,
              EmbroideryWriter& emb) const = 0;
};

class SVGParserNormal : public SVGParser {
public:
  void parse_shape(const NSVGshape* shape, EmbroideryWriter& emb) const
  {
    if( NSVG_PAINT_NONE!=shape->fill.type ){
      /* TODO : implement filled shape */
    }
    if( NSVG_PAINT_COLOR==shape->stroke.type ){
      /* Trace path */
      for(NSVGpath* path=shape->paths; NULL!=path; path=path->next){
        std::vector<math::vector2d> points
          = EmbroideryWriter::make_points_on_bezier(path->pts, path->npts,
                                                  LINE_PITCH);
        if( TRIPLE_STITCH_WIDTH <= shape->strokeWidth ){
          /* wide stroke => tipple stitch */
          emb.add_tripple_stitch(points, LINE_PITCH, false, false);
        }else{
          /* narrow stroke => single stitch */
          emb.add_single_stitch(points, LINE_PITCH, false, false);
        }
      }
    }
  }
};


/*
 * Special parser for Fritzing 0.9
 */
class SVGParserFritzing09 : public SVGParser {
public:
  void parse_shape(const NSVGshape* shape, EmbroideryWriter& emb) const
  {
    if( NSVG_PAINT_COLOR==shape->stroke.type ){
      if( 0==strcmp(shape->id, "boardoutline") ){
        /* ignore outline */
      }else if( 0==strncmp(shape->id, "connector", 9) ){
        /* pad/hole */
        for(NSVGpath* path=shape->paths; NULL!=path; path=path->next){
          std::vector<math::vector2d> points
            = EmbroideryWriter::make_points_on_bezier(path->pts, path->npts,
                                                  LINE_PITCH);
          emb.add_tripple_stitch(points, 0.5*LINE_PITCH, false,false);
        } 
      }else{
        /* this path should be wire, */
        /* trace path with tripple stitch */
        for(NSVGpath* path=shape->paths; NULL!=path; path=path->next){
          std::vector<math::vector2d> points
            = EmbroideryWriter::make_points_on_bezier(path->pts, path->npts,
                                                  LINE_PITCH);
          emb.add_tripple_stitch(points, LINE_PITCH, true, true);
        } 
      }
    }
  }
};


/* ========================================================================= */


/*
 * Parse SVG, make stitches
 */
void
parse_SVG(const char* filename, const SVGParser& parser, EmbroideryWriter& emb)
  throw(std::runtime_error)
{
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
    parser.parse_shape(shape, emb);
  }

  /* relase SVG */
  nsvgDelete(svg);
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
    EmbroideryWriter emb;
    parse_SVG(svgfile, *svg_parser, emb);
    if( emb.is_empty() ){
      fputs("Empty SVG.\n", stdout);
      return 1;
    }

    /* ToDo : reorder stitches */

    emb.write(outfile);

    return 0;
  }catch(std::exception e){
    fputs(e.what(), stdout);
    fputs("\n", stdout);
    return 1;
  }
}
