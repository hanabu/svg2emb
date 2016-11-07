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

#ifndef _MATHVECTOR_HXX
#define _MATHVECTOR_HXX 1

#include<cmath>

namespace math
{
  template<class T,int DIM> class vector {
  private:
    T v[DIM];

  public:
    /**
     * Default constuctor, make zero vector
     */
    inline vector(){
      for(int i=0; i<DIM; ++i){
        v[i] = 0.0;
      }
    }


    /**
     * Constructor from array
     */
    inline vector(const T array[]){
      for(int i=0; i<DIM; ++i){
        v[i] = array[i];
      }
    }


    /**
     * Convinient constructor for 2D
     */
    inline vector(const T x, const T y){
      v[0] = x;
      v[1] = y;
      for(int i=2; i<DIM; ++i){
        v[i] = 0.0;
      }
    }


    /**
     * Convinient constructor for 3D
     */
    inline vector(const T x, const T y, const T z){
      v[0] = x;
      v[1] = y;
      v[2] = z;
      for(int i=3; i<DIM; ++i){
        v[i] = 0.0;
      }
    }


    /**
     * Convinient constructor for 4D
     */
    inline vector(const T x, const T y, const T z, const T t){
      v[0] = x;
      v[1] = y;
      v[2] = z;
      v[3] = t;
      for(int i=4; i<DIM; ++i){
        v[i] = 0.0;
      }
    }


    /**
     * Copy constructor
     */
    inline vector(const vector<T,DIM>& other){
      for(int i=0; i<DIM; ++i){
        v[i] = other.v[i];
      }
    }


    /**
     * this = other
     */
    inline vector<T,DIM> operator=(const vector<T,DIM>& other){
      for(int i=0; i<DIM; ++i){
        v[i] = other.v[i];
      }
      return (*this);
    }


    /**
     * element access
     */
    inline T& operator[](const int i){
      return v[i];
    }


    /**
     * const element access
     */
    inline const T& operator[](const int i) const{
      return v[i];
    }


    /**
     * this += other
     */
    inline void operator+=(const vector<T,DIM>& other){
      for(int i=0; i<DIM; ++i){
        v[i] += other.v[i];
      }
    }


    /**
     * this + other
     */
    inline vector<T,DIM> operator+(const vector<T,DIM>& other) const{
      vector<T,DIM> added(*this);
      for(int i=0; i<DIM; ++i){
        added.v[i] += other.v[i];
      }
      return added;
    }


    /**
     * this -= other
     */
    inline void operator-=(const vector<T,DIM>& other){
      for(int i=0; i<DIM; ++i){
        v[i] -= other.v[i];
      }
    }


    /**
     * this - other
     */
    inline vector<T,DIM> operator-(const vector<T,DIM>& other) const{
      vector<T,DIM> substracted(*this);
      for(int i=0; i<DIM; ++i){
        substracted.v[i] -= other.v[i];
      }
      return substracted;
    }


    /**
     * scalar * vector
     */
    friend inline vector<T,DIM> operator*(const T scalar,
                                          const vector<T,DIM>& vec){
      vector<T,DIM> multiplied(vec);
      for(int i=0; i<DIM; ++i){
        multiplied.v[i] *= scalar ;
      }
      return multiplied;
    }


    /**
     * this *= scalar
     */
    inline void operator*=(const T scalar){
      for(int i=0; i<DIM; ++i){
        v[i] *= scalar;
      }
    }


    /**
     * this * scalar
     */
    inline vector<T,DIM> operator*(const T scalar) const{
      return scalar*(*this);
    }
    

    /**
     * this /= scalar
     */
    inline void operator/=(const T scalar){
      (*this) *= (1.0/scalar);
    }


    /**
     * this / scalar
     */
    inline vector<T,DIM> operator/(const T scalar) const{
      return (1.0/scalar) * (*this);
    }


    /**
     * this * other (dot product)
     */
    inline T operator*(const vector<T,DIM>& other) const{
      T product = 0.0;
      for(int i=0; i<DIM; ++i){
        product += v[i] * other.v[i];
      }
      return product;
    }


    /**
     * |this|^2
     */
    inline T square_norm() const{
      T sqnorm = 0.0;
      for(int i=0; i<DIM; ++i){
        sqnorm += v[i] * v[i];
      }
      return sqnorm;
    }


    /**
     * |this|
     */
    inline T norm() const{
      return sqrt(square_norm());
    }


    /*
     * make normalized vector ( |v|==1 )
     */
    inline vector<T,DIM> normalize() const{
      return (*this) * (1.0/sqrt(square_norm()));
    }

  }; /* end of class */


  typedef class vector<float,2> vector2d;
  typedef class vector<float,3> vector3d;

} /* end of namespace math */

#endif
