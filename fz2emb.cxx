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


/*
 * run fz2emb as:
 *
 *  funzip input.fzz | fz2emb output.pes
 */

#include<cstdio>

#include<cmath>
#include<cstring>
#include<memory>
#include<set>
#include<list>
#include<vector>
#include<stdexcept>

/* libxml2 */
#include<libxml/parser.h>
#include<libxml/xpath.h>


#include<mathvector.hxx>
#include<embwrite.hxx>

/* Fritzing unit (1/100in) to mm */
#define fz2mm(fz) (0.254*(fz))

#define iabs(i) ((0<=(i))?(i):(-i))

static const float LINE_PITCH = 2.0;  /* mm */
static const char* WIRE_MODULEID = "WireModuleID";


class FzWires
{
private:
  class FzPoint {
  public:
    math::vector2d p;
    bool conn;
    std::vector<int> wires;

    FzPoint(math::vector2d point, bool connectboard) :
      p(point), conn(connectboard) {}
  };

  std::vector<FzPoint> points;
  std::vector<std::pair<int,int> > wires;

protected:
  int find_or_add_point(const math::vector2d& point, bool connectboard)
  {
    for(int i=0; i<points.size(); ++i){
      if( 0.01 > (point-points[i].p).square_norm() ){
        /* near point found */
        points[i].conn =  points[i].conn || connectboard;
        return i;
      }
    }

    /* No point found */
    points.push_back( FzPoint(point, connectboard) );
    return points.size()-1;
  }

  int find_next_merge_point(int ptidx, std::vector<bool>& wireprocessed) const
  {
    if( 2!=points[ptidx].wires.size() ){
      return -1;
    }
     
    /* connect other wire at ptidx */
    int otherwire = points[ptidx].wires[0];
    if( wireprocessed[ otherwire ] ){
      otherwire = points[ptidx].wires[1];
    }

    if( wireprocessed[ otherwire ] ){
      /* can not merge (in case, last point of loop wire) */
      return -1;
    }

    wireprocessed[otherwire] = true;

    /* next merge point */
    if( wires[otherwire].first == ptidx ){
      return wires[otherwire].second;
    }else{
      return wires[otherwire].first;
    }
  }

public:
  FzWires()
  {
  }


  void add_wire(const math::vector2d& p1, const math::vector2d& p2,
                bool connectboard1, bool connectboard2)
  {
    std::pair<int,int> wire(find_or_add_point(p1, connectboard1),
                            find_or_add_point(p2, connectboard2) );
    
    wires.push_back(wire);

    int wireid = wires.size()-1;
    points[wire.first] .wires.push_back(wireid);
    points[wire.second].wires.push_back(wireid);
  }


  EmbroideryWriter make_stitches()
  {
    EmbroideryWriter emb;


    /* merge lines */
    std::vector<bool> wireprocessed(wires.size(), false);
    for(int i=0; i<wires.size(); ++i){
      if( wireprocessed[i] ){
        /* this wire is already processed, skip */
        continue;
      }
      std::list<int> wirepoints;

      /* start merging wire */
      int ptidx1 = wires[i].first;
      int ptidx2 = wires[i].second;
      wireprocessed[i] = true;
      wirepoints.push_back(ptidx1);
      wirepoints.push_back(ptidx2);

      while( 0<=ptidx1 ){
        /* find next point */
        ptidx1 = find_next_merge_point(ptidx1, wireprocessed);
        if( 0<=ptidx1  ){
          wirepoints.push_front(ptidx1);
        }
      }
      while( 0<=ptidx2 ){
        /* find next point */
        ptidx2 = find_next_merge_point(ptidx2, wireprocessed);
        if( 0<=ptidx2  ){
          wirepoints.push_back(ptidx2);
        }
      }

      
      std::vector<math::vector2d> stitchsegment;
      std::list<int>::const_iterator it=wirepoints.begin();
      int befidx = *it;
      for(++it; it!=wirepoints.end(); ++it){
        math::vector2d& p1 = points[befidx].p;
        math::vector2d& p2 = points[*it   ].p;
        befidx = *it;

        float len = (p2-p1).norm();
        int pts = (int)(len/LINE_PITCH)+1;
        for(int j=0; j<pts; ++j){
          stitchsegment.push_back(p1 + ((float)j/(float)pts)*(p2-p1));
        }
      }
      stitchsegment.push_back(points[befidx].p);

      emb.add_tripple_stitch(stitchsegment, LINE_PITCH, false, false);
    }


    for(int i=0; i<points.size(); ++i){
      if( 3<=points[i].wires.size() ){
        /* wire junction, make star */
        emb.add_star(points[i].p, LINE_PITCH);
      }
      if( points[i].conn ){
        /* PCB connection */
        emb.add_star(points[i].p, LINE_PITCH);
      }
    }

    return emb;
  }

}; /* end of class FzWires */


/* libxml2 helper, get attribute as std::string */
static std::string xmlGetStringProp(xmlNodePtr xmlnode, const char* attrname)
{
  xmlChar* attr = xmlGetProp(xmlnode, (const xmlChar*)attrname);
  if( NULL==attr ){
    return std::string(); /* empty string */
  }else{
    std::string attrval((const char*) attr);
    xmlFree(attr);
    return attrval;
  }
}


/* libxml2 helper, get attribute as double */
static double xmlGetDoubleProp(xmlNodePtr xmlnode, const char* attrname)
{
  xmlChar* attr = xmlGetProp(xmlnode, (const xmlChar*)attrname);
  if( NULL==attr ){
    return nan(attrname);
  }else{
    double attrval = strtod((const char*) attr, NULL);
    xmlFree(attr);
    return attrval;
  }
}


/* libxml2 helper, enumerate xpath nodes as std::vector */
static std::vector<xmlNodePtr>
xmlXPathVector(xmlNodePtr node, const char* xpathstr, xmlXPathContextPtr ctx)
{
  xmlXPathObjectPtr xpath;

  if( NULL==node ){
    xpath = xmlXPathEval((const xmlChar*)xpathstr, ctx);    
  }else{
    xpath = xmlXPathNodeEval(node, (const xmlChar*)xpathstr, ctx);
  }

  std::vector<xmlNodePtr> result;
  if( NULL!=xpath && NULL!=xpath->nodesetval ) {
    for(int i=0; i<xpath->nodesetval->nodeNr; ++i){
      xmlNodePtr childnode = xpath->nodesetval->nodeTab[i];
      result.push_back(childnode);
    }
  }
  xmlXPathFreeObject(xpath);

  return result;
}


/*
 * Parse Fritzing XML
 */
static FzWires parse_fritzing_wires() throw(std::runtime_error)
{
  FzWires wires;

  xmlDocPtr	xmldoc = xmlReadFd(fileno(stdin), NULL, NULL, XML_PARSE_RECOVER);
  if( NULL==xmldoc ){
    throw std::runtime_error("XML parse error.");
  }


  /* find module/instances/instance elements */
  xmlXPathContextPtr xpathctx = xmlXPathNewContext(xmldoc);

  
  std::vector<xmlNodePtr> xmlinsts
    = xmlXPathVector(NULL, "/module/instances/instance", xpathctx);

  /* make modelIndex for wires */
  std::set<std::string> wireindexes;
  for(int i=0; i<xmlinsts.size(); ++i){
    std::string modref(xmlGetStringProp(xmlinsts[i], "moduleIdRef"));
    if( 0==modref.compare(WIRE_MODULEID) ){
      /* this instance is wire, add modelIndex attribute */
      wireindexes.insert(xmlGetStringProp(xmlinsts[i], "modelIndex"));
    }
  }

  for(int i=0; i<xmlinsts.size(); ++i){

    /* get moduleIdRef attribute */
    std::string modref(xmlGetStringProp(xmlinsts[i], "moduleIdRef"));
    if( 0!=modref.compare(WIRE_MODULEID) ){
      /* not wire, ignore */
      continue;
    }

    /* parse wire */
    /* find views/pcbView */
    std::vector<xmlNodePtr> xmlpcbs
      = xmlXPathVector(xmlinsts[i], "views/pcbView", xpathctx);
    if( ! xmlpcbs.empty() ){

      /* check connection to board */
      bool connectboard[2];
      std::vector<xmlNodePtr> xmlconnectors
        = xmlXPathVector(xmlpcbs[0], "connectors/connector", xpathctx);
      for(int i=0; i<2 && i<xmlconnectors.size(); ++i){
        connectboard[i] = false;
        std::vector<xmlNodePtr> xmlconnects
          = xmlXPathVector(xmlconnectors[i], "connects/connect", xpathctx);
        for(int j=0; j<xmlconnects.size(); ++j){
          std::string modelidx(xmlGetStringProp(xmlconnects[j], "modelIndex"));
          if( wireindexes.find(modelidx) == wireindexes.end() ){
            /* connect to non wire instance */
            connectboard[i] = true;
            break;
          }
        }
      }

      /* find geometry node */
      std::vector<xmlNodePtr> xmlgeoms
        = xmlXPathVector(xmlpcbs[0], "geometry", xpathctx);
      if( !xmlgeoms.empty() ){
        double x  = xmlGetDoubleProp(xmlgeoms[0], "x");
        double y  = xmlGetDoubleProp(xmlgeoms[0], "y");
        double x1 = xmlGetDoubleProp(xmlgeoms[0], "x1");
        double y1 = xmlGetDoubleProp(xmlgeoms[0], "y1");
        double x2 = xmlGetDoubleProp(xmlgeoms[0], "x2");
        double y2 = xmlGetDoubleProp(xmlgeoms[0], "y2");

        if( ! ( std::isnan(x ) || std::isnan(y ) ||
                std::isnan(x1) || std::isnan(y1) ||
                std::isnan(x2) || std::isnan(y2) ) ){
          /* valid wire node, make line */
          math::vector2d p1(fz2mm(x+x1), fz2mm(y+y1));
          math::vector2d p2(fz2mm(x+x2), fz2mm(y+y2));

          wires.add_wire(p1, p2, connectboard[0], connectboard[1]);
        }
      }

    }
  }

  xmlXPathFreeContext(xpathctx);
  xmlFreeDoc(xmldoc);

  return wires;
}


/*
 * Print help message
 */
void print_help()
{
  fputs("funzip INPUT.fzz | fz2emb OUTPUT.pes\n", stderr);
}


/*
 * main 
 */
int main(int argc, char* argv[])
{

  const char* outfile = NULL;

  /* parse commandline */
  for(int i=1; i<argc; ++i){
    if( '-' == argv[i][0] ){
      switch( argv[i][1] ){
      case 'h': /* -h : print help */
        print_help();
        return 0;
        break;
      defaults:
        /* Unknown option */
        fprintf(stderr, "Unknown option \"%s\"\n\n", argv[i]);
        print_help();
        return -1;
      }
    }else{
      /* filename options */
      if( NULL==outfile ){
        outfile = argv[i];
      }else{
        fputs("Too many arguments\n\n", stderr);
        print_help();
        return -1;
      }
    }
  }

  if( NULL==outfile ){
    fputs("Too few arguments\n\n", stderr);
    print_help();
    return -1;
  }

  try{
    FzWires wires = parse_fritzing_wires();
    EmbroideryWriter emb = wires.make_stitches();

    if( emb.is_empty() ){
      fputs("Empty Fritzing PCB.\n", stdout);
      return 1;
    }

    emb.optimize_order();
    emb.write(outfile);

    return 0;
  }catch(std::exception e){
    fputs(e.what(), stdout);
    fputs("\n", stdout);
    return 1;
  }
}
