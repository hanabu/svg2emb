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
 * math::vector
 * Vector manipulation class
 */

#ifndef _MATHTRANSM_HXX
#define _MATHTRANSM_HXX 1

#include<cmath>
#include<mathvector.hxx>

namespace math
{

  /*
   * Transformation matrix
   *
   *   ( m_11 m_12 ... m_1n  t_1  )   ( v_1 )
   *   ( m_21 m_22 ... m_2n  t_2  )   ( v_2 )
   *   (           ...            ) * ( ... )
   *   ( m_n1 m_n2 ... m_nn  t_n  )   ( v_n )
   *   (  0     0        0    1   )   (  1  )
   */


  template<class T,int DIM> class transform_matrix {
  private:
    T m[DIM][DIM]; /* rotation, scale, skew */
    T t[DIM];      /* translation */

  public:

    /**
     * Default constuctor (identity transformation)
     */
    inline transform_matrix(){
      /* make identity matrix */
      for(int i=0; i<DIM; ++i){
        for(int j=0; j<DIM; ++j){
          m[i][j] = 0.0;
        }
        m[i][i] = 1.0;
        t[i] = 0.0;
      }
    }


    /**
     * Constructor with translation (no rotate, skew)
     */
    inline transform_matrix(const vector<T,DIM> trans)
    {
      /* make identity matrix */
      transform_matrix();

      for(int i=0; i<DIM; ++i){
        t[i] = trans[i];
      }      
    }


    /**
     * Constructor from basis vectors of new coordinate system
     */
    inline transform_matrix(const vector<T,DIM> basis[DIM],
                            const vector<T,DIM>& trans)
    {
      for(int i=0; i<DIM; ++i){
        for(int j=0; j<DIM; ++j){
          m[i][j] = basis[j][i];
        }
        t[i] = trans[i];
      }      
    }


    /**
     * Rotate on X-Y plane with translation
     */
    inline transform_matrix(const T rotate, const vector<T,DIM>& trans)
    {
      /* make identity matrix */
      transform_matrix();

      /* rotate on X-Y */
      m[0][0] =  cos(rotate);
      m[1][0] =  sin(rotate);
      m[0][1] = -m[1][0]; /* -sin(rotate) */
      m[1][1] =  m[0][0]; /*  cos(rotate) */

      for(int i=0; i<DIM; ++i){
        t[i] = trans[i];
      }
    }


    /**
     * Rotate on X-Y plane, no translation
     */
    inline transform_matrix(const T rotate)
    {
      transform_matrix(rotate, vector<T,DIM>());
    }


    /**
     * Copy constructor
     */
    inline transform_matrix(const transform_matrix<T,DIM>& other){
      memcpy(m, other.m, sizeof(m));
      memcpy(t, other.t, sizeof(t));
    }


    /**
     * this = other
     */
    inline transform_matrix<T,DIM>
    operator=(const transform_matrix<T,DIM>& other)
    {
      memcpy(m, other.m, sizeof(m));
      memcpy(t, other.t, sizeof(t));
      return (*this);
    }


    /**
     * this * other
     */
    inline transform_matrix<T,DIM>
    operator*(const transform_matrix<T,DIM>& other)
    {
      transform_matrix<T,DIM> newm;

      for(int i=0; i<DIM; ++i){
        for(int j=0; j<DIM; ++j){
          newm.m[i][j] = 0.0;
          for(int k=0; k<DIM; ++k){
            newm.m[i][j] += m[i][k] * other.m[k][j];
          }
        }

        newm.t[i] = t[i];
        for(int k=0; k<DIM; ++k){
          newm.t[i] += m[i][k] * other.t[k];
        }
      }

      return newm;
    }
    

    /**
     * this *= other
     */
    inline transform_matrix<T,DIM>
    operator*=(const transform_matrix<T,DIM>& other)
    {
      transform_matrix<T,DIM> newm = this*other;
      return *this = newm;
    }


    /**
     * Transform vector
     *  = M * v
     */
    inline vector<T,DIM> operator*(const vector<T,DIM>& v)
    {
      vector<T,DIM> newv;

      for(int i=0; i<DIM; ++i){
        newv[i] = t[i];
        for(int k=0; k<DIM; ++k){
          newv[i] += m[i][k] * v[k];
        }
      }
      return newv;
    }

  }; /* end of class */


} /* end of namespace math */

#endif
