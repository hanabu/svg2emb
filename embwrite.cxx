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

#include<embwrite.hxx>
#include<cassert>
#include<cmath>
#include<list>
#include<vector>
#include<stdexcept>

#include<mathtransm.hxx>
#include<cubicbezier.hxx>



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


struct connect{
  int stitch1;
  int frontback1;
  int stitch2;
  int frontback2;
};



/*
 * make star stitch for conductive thread connection
 */
static std::vector<math::vector2d>
make_star(const math::vector2d center, const float r)
{
  std::vector<math::vector2d> points;
  points.reserve(8);
  for(int i=0; i<7; ++i){
    float rad = (M_PI*2.0/7.0)*(2*i);
    math::vector2d pos(cos(rad), sin(rad));
    points.push_back(center + pos*r);
  }
  points.push_back(points.front());
  return points;
}


/* ========================================================================= */

class MergedStitch
{
private:
  /* vector< pair< stitch id , reverse> > */
  std::list< std::pair<int,bool> > stitchids;
public:
  MergedStitch(int stitchid) :
    stitchids(1, std::pair<int,bool>(stitchid, false))
  {
  }


  void merge(const MergedStitch& other, bool thisback, bool otherback)
  {
    if( thisback ){
      if( otherback ){
        /* push_back( reverse(other) ) */
        std::list< std::pair<int,bool> >::const_reverse_iterator it;
        for(it=other.stitchids.rbegin(); it!=other.stitchids.rend(); ++it){
          stitchids.push_back( std::pair<int,bool>(it->first, !it->second) );
        }
      }else{
        /* push_back( other ) */
        std::list< std::pair<int,bool> >::const_iterator it;
        for(it=other.stitchids.begin(); it!=other.stitchids.end(); ++it){
          stitchids.push_back( std::pair<int,bool>(it->first, it->second) );
        }
      }
    }else{
      if( otherback ){
        /* other.push_front(other) */
        std::list< std::pair<int,bool> >::const_reverse_iterator it;
        for(it=other.stitchids.rbegin(); it!=other.stitchids.rend(); ++it){
          stitchids.push_back( std::pair<int,bool>(it->first, it->second) );
        }
      }else{
        /* push_front( reverse(other) ) */
        std::list< std::pair<int,bool> >::const_iterator it;
        for(it=other.stitchids.begin(); it!=other.stitchids.end(); ++it){
          stitchids.push_back( std::pair<int,bool>(it->first, !it->second) );
        }
      }
    }
  }


  std::pair<math::vector2d,math::vector2d>
  edge_points(const std::vector<std::vector<math::vector2d> >& stitches) const
  {
    std::pair<math::vector2d,math::vector2d> edgepoints;
    if( stitchids.front().second ){
      edgepoints.first = stitches[stitchids.front().first].back();
    }else{
      edgepoints.first = stitches[stitchids.front().first].front();
    }
    if( stitchids.back().second ){
      edgepoints.second = stitches[stitchids.back().first].front();
    }else{
      edgepoints.second = stitches[stitchids.back().first].back();
    }

    return edgepoints;
  }


  std::pair< float,std::pair<bool,bool> >
  calc_distance(const MergedStitch& other,
                const std::vector<std::vector<math::vector2d> >& stitches) const
  {
    std::pair<math::vector2d,math::vector2d> thisedge
      = edge_points(stitches);
    std::pair<math::vector2d,math::vector2d> otheredge
      = other.edge_points(stitches);
    std::pair<bool,bool> backflag;
    float dist;
    float min_dist;

    dist = (thisedge.first-otheredge.first).square_norm();
    min_dist = dist;
    backflag.first = false; backflag.second = false;

    dist = (thisedge.first-otheredge.second).square_norm();
    if( dist < min_dist ){
      min_dist = dist;
      backflag.first = false; backflag.second = true;
    }
    dist = (thisedge.second-otheredge.first).square_norm();
    if( dist < min_dist ){
      min_dist = dist;
      backflag.first = true; backflag.second = false;
    }
    dist = (thisedge.second-otheredge.second).square_norm();
    if( dist < min_dist ){
      min_dist = dist;
      backflag.first = true; backflag.second = true;
    }

    return std::pair<float,std::pair<bool,bool> >(min_dist, backflag);
  }


  std::list< std::pair<int,bool> >::const_iterator begin() const
  {
    return stitchids.begin();
  }

  std::list< std::pair<int,bool> >::const_iterator end() const
  {
    return stitchids.end();
  }  
};

/* ========================================================================= */
/*  Implementation  of  EmbroideryWriter                                     */
/* ========================================================================= */

/*
 * Default constructor
 */
EmbroideryWriter::EmbroideryWriter()
{
}


/*
 * Make points on Bezier curve
 */
std::vector<math::vector2d>
EmbroideryWriter::make_points_on_bezier(const float* bezierpts,
                                        int npts, float pitch)
{
  std::vector<math::vector2d> points;
  math::vector2d lastpt;
  float carryov = 0.0;

  for(int i=0; i<npts-1; i+=3) {
    math::vector2d p0(&bezierpts[i*2  ]);
    math::vector2d p1(&bezierpts[i*2+2]);
    math::vector2d p2(&bezierpts[i*2+4]);
    math::vector2d p3(&bezierpts[i*2+6]);
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


/*
 * Check if embroidery is empty
 */
bool EmbroideryWriter::is_empty() const
{
  return stitches.empty();
}


/*
 * Optimize stitch order to minimize jumps
 */
void EmbroideryWriter::optimize_order()
{
  if( 1>=stitches.size() ){
    /* only one stitch, no optimization needed */
    return;
  }


  /* Create initial merged stitches */
  std::list<MergedStitch> merged;
  for(int i=0; i<stitches.size(); ++i){
    merged.push_back(MergedStitch(i));
  }

  while( 1<merged.size() ){
    float max_dist = 0.0;
    std::list<MergedStitch>::iterator max_i=merged.begin();
    std::list<MergedStitch>::iterator max_j=merged.begin();
    std::pair<bool,bool> max_backflag(false, false);

    /* find farest stitch segment */
    for(std::list<MergedStitch>::iterator i=merged.begin();
        i!=merged.end(); ++i){

      std::pair<math::vector2d,math::vector2d> edgei
        = i->edge_points(stitches);
      
      /* find nearest other stitch segment from stitch[i] */
      float min_dist = 1.0e+10;
      std::list<MergedStitch>::iterator min_j=merged.begin();
      std::pair<bool,bool> min_backflag(false, false);
      for(std::list<MergedStitch>::iterator j=merged.begin();
          j!=merged.end(); ++j){
        if( i==j ){ continue; }
        
        std::pair<float, std::pair<bool,bool> > dist
          = i->calc_distance(*j, stitches);
        if( dist.first < min_dist ){
          min_dist     = dist.first;         
          min_backflag = dist.second;
          min_j = j;
        }
      }

      if( min_dist<1.0e+10 && max_dist<min_dist ){
        max_dist = min_dist;
        max_i = i;
        max_j = min_j;
        max_backflag = min_backflag;
      }
    }

    /* stitch segment max_i is farest segment from other */
    /* merge max_i and max_j  */
    max_i->merge(*max_j, max_backflag.first, max_backflag.second);
    merged.erase(max_j);    
  }


  /* find largest distance in merged */
  /* (start&finish at largest distance) */
  const MergedStitch allmerged = merged.front();
  std::list<std::pair<int,bool> >::const_iterator it=allmerged.begin();
  std::list<std::pair<int,bool> >::const_iterator startit = it;
  float max_dist = 0.0;
  for(;;){
    std::list<std::pair<int,bool> >::const_iterator curr = it;
    std::list<std::pair<int,bool> >::const_iterator next = ++it;
    if( next==allmerged.end() ){
      break;
    }

    math::vector2d currbackp;
    if( curr->second ){
      currbackp = stitches[curr->first].front();
    }else{
      currbackp = stitches[curr->first].back();
    }
    math::vector2d nextfrontp;
    if( next->second ){
      nextfrontp = stitches[next->first].back();
    }else{
      nextfrontp = stitches[next->first].front();
    }

    float dist = (nextfrontp - currbackp).square_norm();
    if( max_dist < dist ){
      max_dist = dist;
      startit = next;
    }
  }

  
  /* reorder stitches */
  std::vector<std::vector<math::vector2d> > newstitches;
  for(it=startit; it!=allmerged.end(); ++it){
    if( it->second ){
      /* reverse order */
      newstitches.push_back(
        std::vector<math::vector2d>(stitches[it->first].rbegin(),
                                    stitches[it->first].rend())
                            );
    }else{
      newstitches.push_back(stitches[it->first]);
    }
  }
  for(it=allmerged.begin(); it!=startit; ++it){
    if( it->second ){
      /* reverse order */
      newstitches.push_back(
        std::vector<math::vector2d>(stitches[it->first].rbegin(),
                                    stitches[it->first].rend())
                            );
    }else{
      newstitches.push_back(stitches[it->first]);
    }
  }
}


/*
 * Write embroidery format
 */
void EmbroideryWriter::write(const char* filename)
  const throw(std::runtime_error)
{
  EmbColor black = { 0, 0, 0 };
  EmbThread thread = { black, "Black", "900" };
  EmbPattern* pat;

  pat = embPattern_create();

  /* set thread & color */
  embPattern_addThread(pat, thread);
  embPattern_changeColor(pat, 0);

  for(size_t i=0; i<stitches.size(); ++i){
    const std::vector<math::vector2d>& stitchsegment = stitches[i];

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
 * path as single stitch
 */
void
EmbroideryWriter::add_single_stitch(const std::vector<math::vector2d>& points,
                                    float starsize,
                                    bool startstar, bool endstar)
{
  if( startstar || endstar ){
    if( 1>=points.size() ){
      /* too few points (too short segment) */
      return;
    }

    std::vector<math::vector2d> stitchsegment;
    stitchsegment.reserve(stitchsegment.size()+16);

    if( startstar ){
      /* make star at start */
      std::vector<math::vector2d> star
        = make_star(points.front(), 0.5*starsize);
      stitchsegment.insert(stitchsegment.end(), star.begin(), star.end());
    }

    /* join points */
    stitchsegment.insert(stitchsegment.end(), points.begin(), points.end());

    if( endstar ){
      /* make star at end */
      std::vector<math::vector2d> star
        = make_star(points.back(), 0.5*starsize);
      stitchsegment.insert(stitchsegment.end(), star.begin(), star.end());
    }

    stitches.push_back(stitchsegment);
  }else{
    /* no star, stitches are points only*/
    stitches.push_back(points);
  }
}


/*
 * path as tripple stitch
 */
void
EmbroideryWriter::add_tripple_stitch(const std::vector<math::vector2d>& points,
                                     float starsize,
                                     bool startstar, bool endstar)
{
  if( 1>=points.size() ){
    /* Too short path */
    return;
  }

  std::vector<math::vector2d> stitchsegment;
  stitchsegment.reserve(stitchsegment.size()*3 + 16);
  
  /* first pass */
  stitchsegment.insert(stitchsegment.end(), points.begin(), points.end());

  if( endstar ){
    /* make star at end */
    std::vector<math::vector2d> star
      = make_star(points.back(), 0.5*starsize);
    stitchsegment.insert(stitchsegment.end(), star.begin(), star.end());
  }

  /* second pass, reverse order */
  stitchsegment.insert(stitchsegment.end(), points.rbegin(), points.rend());

  if( startstar ){
    /* make star at start */
    std::vector<math::vector2d> star
      = make_star(points.front(), 0.5*starsize);
    stitchsegment.insert(stitchsegment.end(), star.begin(), star.end());
  }

  /* third pass */
  stitchsegment.insert(stitchsegment.end(), points.begin(), points.end());

  stitches.push_back(stitchsegment);
}


/*
 * Add star only
 */
void EmbroideryWriter::add_star(const math::vector2d& p, float starsize)
{
    std::vector<math::vector2d> star
      = make_star(p, 0.5*starsize);
  stitches.push_back(star);
}

