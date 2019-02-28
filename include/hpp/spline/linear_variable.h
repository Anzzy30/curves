/**
* \file linear_variable.h
* \brief storage for variable points of the form p_i = a_i x + b_i
* \author Steve T.
* \version 0.1
* \date 07/02/2019
*/


#ifndef _CLASS_LINEAR_VARIABLE
#define _CLASS_LINEAR_VARIABLE

#include "curve_abc.h"

#include "MathDefs.h"

#include <math.h>
#include <vector>
#include <Eigen/Core>
#include <stdexcept>

namespace spline
{
template <int Dim, typename Numeric=double>
struct linear_variable
{
    typedef Eigen::Matrix<Numeric, Dim, Dim> matrix_dim_t;
    typedef Eigen::Matrix<Numeric, Dim, Eigen::Dynamic> matrix_dim_x_t;
    typedef Eigen::Matrix<Numeric, Dim, 1> point_dim_t;
    typedef Eigen::Matrix<Numeric, Eigen::Dynamic, 1> vector_x_t;
    typedef Eigen::Matrix<Numeric, Eigen::Dynamic, Eigen::Dynamic> matrix_x_t;
    typedef linear_variable<Dim, Numeric> linear_variable_t;

    linear_variable(): B_(matrix_dim_t::Identity()), c_(point_dim_t::Zero()), zero(false){} //variable
    linear_variable(const point_dim_t& c):B_(matrix_dim_t::Zero()),c_(c), zero(false) {} // constant
    linear_variable(const matrix_dim_x_t& B, const point_dim_t& c):B_(B),c_(c), zero(false) {} //mixed

    // linear evaluation
    point_dim_t operator()(const Eigen::Ref<const vector_x_t>& val) const
    {
        if(isZero())
            return c();
        return B() * val + c();
    }

    linear_variable_t& operator+=(const linear_variable_t& w1)
    {
        if (w1.isZero())
            return *this;
        if(isZero())
        {
            this->B_ = w1.B_;
            zero = w1.isZero();
        }
        else
        {
            this->B_ += w1.B_;
        }
        this->c_ += w1.c_;
        return *this;
    }
    linear_variable_t& operator-=(const linear_variable_t& w1)
    {
        if (w1.isZero())
            return *this;
        if(isZero())
        {
            this->B_ = -w1.B_;
            zero = w1.isZero();
        }
        else
        {
            this->B_ -= w1.B_;
        }
        this->c_ -= w1.c_;
        return *this;
    }
    linear_variable_t& operator/=(const double d)
    {
        B_ /= d;
        c_ /= d;
        return *this;
    }
    linear_variable_t& operator*=(const double d)
    {
        B_ *= d;
        c_ *= d;
        return *this;
    }

    static linear_variable_t Zero(size_t /*dim=0*/){
        return linear_variable_t(matrix_x_t::Identity(Dim,Dim), vector_x_t::Zero(Dim));
    }

    const matrix_dim_x_t& B() const {return B_;}
    const point_dim_t& c () const {return c_;}
    bool isZero () const {return zero;}

private:
    matrix_dim_x_t B_;
    point_dim_t c_;
    bool zero;
};

template <int D, typename N>
inline linear_variable<D,N> operator+(const linear_variable<D,N>& w1, const linear_variable<D,N>& w2)
{
    linear_variable<D,N> res(w1.B(), w1.c());
    return res+=w2;
}

template <int D, typename N>
linear_variable<D,N> operator-(const linear_variable<D,N>& w1, const linear_variable<D,N>& w2)
{
    linear_variable<D,N> res(w1.B(), w1.c());
    return res-=w2;
}

template <int D, typename N>
linear_variable<D,N> operator*(const double k, const linear_variable<D,N>& w){
    linear_variable<D,N> res(w.B(), w.c());
    return res*=k;
}

template <int D, typename N>
linear_variable<D,N> operator*(const linear_variable<D,N>& w,const double k)
{
    linear_variable<D,N> res(w.B(), w.c());
    return res*=k;
}

template <int D, typename N>
linear_variable<D,N> operator/(const linear_variable<D,N>& w,const double k)
{
    linear_variable<D,N> res(w.B(), w.c());
    return res/=k;
}
} // namespace spline
#endif //_CLASS_LINEAR_VARIABLE

