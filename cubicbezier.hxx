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


#ifndef _CUBICBEZIER_HXX
#define _CUBICBEZIER_HXX 1

#include<mathvector.hxx>


/*
 * Cubic Bezier curve
 *
 * B(t) = (1-t)^3*p0 + 3*(1-t)^2*t*p1 + 3*(1-t)*t^2*p2 + t^3*p3
 *
 * dB(t)/dt = 3*(1-t)^2*v1 + 6*(1-t)*t*v2 + 3*t^2*v3
 *  (v1=p1-p0, v2=p2-p1, v3=p3-p2)
 */
template<class T, int DIM> class CubicBezier{
private:
  const static int CURVE_SEGMENTS = 32;
  const static T dt = 1.0/CURVE_SEGMENTS;

  math::vector<T,DIM> p0;
  math::vector<T,DIM> p1;
  math::vector<T,DIM> p2;
  math::vector<T,DIM> p3;
  math::vector<T,DIM> v1; /* p1-p0 */ 
  math::vector<T,DIM> v2; /* p1-p0 */ 
  math::vector<T,DIM> v3; /* p1-p0 */ 

  /* divide curve into segments, dl[] hold length of each segment */
  T dl[CURVE_SEGMENTS];

public:

  /*
   * Constructor
   * four control point P0-P3
   */
  inline CubicBezier(const math::vector<T,DIM> P0,
                     const math::vector<T,DIM> P1,
                     const math::vector<T,DIM> P2,
                     const math::vector<T,DIM> P3) :
    p0(P0), p1(P1), p2(P2), p3(P3),
    v1(P1-P0), v2(P2-P1), v3(P3-P2)
  {
    /* calc dl (curve segment length) with Simpson's integration rule */
    T lleft = sqrt( delivertive(0.0).square_norm() );
    for(int i=0; i<CURVE_SEGMENTS; ++i){
      T t  = ((T)i) * dt;
      T lcenter = sqrt( delivertive(t+0.5*dt).square_norm() );
      T lright  = sqrt( delivertive(t+dt)    .square_norm() );
      dl[i] = (dt/6.0)*(lleft + 4.0*lcenter + lright);

      lleft = lright;
    }
  }


  /*
   * B(t) : Get point on curve
   */
  inline math::vector<T,DIM> curve(const T t) const{
    return (     (1.0-t)*(1.0-t)*(1.0-t) ) * p0
      +    ( 3.0*(1.0-t)*(1.0-t)*     t  ) * p1
      +    ( 3.0*(1.0-t)*     t *     t  ) * p2
      +    (          t *     t *     t  ) * p3;
  }


  /*
   * dB(t)/dt : Get delivertive respect to t
   */
  inline math::vector<T,DIM> delivertive(const T t) const {
    return ( 3.0*(1.0-t)*(1.0-t) ) * v1
      +    ( 6.0*(1.0-t)*     t  ) * v2
      +    ( 3.0*     t *     t  ) * v3;
  }


  /*
   * length of curve
   */
  inline T length() const {
    T sumlength = 0.0;

    for(int i=0; i<CURVE_SEGMENTS; ++i){
      sumlength += dl[i];
    }
    return sumlength;
  }


  /*
   * move distance len along with the curve.
   * return :
   *   when destination is on this curve, 0.0
   *   otherwise remaining distance
   */
  inline T move_on_curve(T& t, const T len) const {

    int i = (int)( t*CURVE_SEGMENTS );
    if( i<0 || CURVE_SEGMENTS<=i ){
      /* t out of range */
      return len;
    }

    /* start at segment boundary */
    T segi = t*CURVE_SEGMENTS - ((T)i);
    T movelen = len + dl[i]*segi;

    for(; i<CURVE_SEGMENTS; ++i){
      if( movelen < dl[i] ){
        /* destination exist in this segment */
        t = dt*( ((T)i) + movelen/dl[i] );
        return 0.0;
      }else{
        /* go next segment */
        movelen -= dl[i];
      }
    }

    /* destination is out of this curve */
    t = 1.0;
    return movelen;
  }


  
}; /* end of class */

#endif
