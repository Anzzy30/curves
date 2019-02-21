/**
* \file bezier_curve.h
* \brief class allowing to create a Bezier curve of dimension 1 <= n <= 3.
* \author Steve T.
* \version 0.1
* \date 06/17/2013
*/


#ifndef _CLASS_LINEAR_PROBLEM_DETAILS
#define _CLASS_LINEAR_PROBLEM_DETAILS

#include <hpp/spline/bezier_curve.h>
#include <hpp/spline/linear_variable.h>
#include <hpp/spline/curve_constraint.h>
#include <hpp/spline/optimization/definitions.h>
#include <hpp/spline/bernstein.h>

#include <Eigen/StdVector>

namespace spline
{
namespace  optimization
{
template<typename Point, int Dim, typename Numeric>
struct problem_data
{
     problem_data() : bezier(0){}
    ~problem_data() {if (bezier) delete bezier;}

    typedef linear_variable<Dim, Numeric>     var_t;
    typedef std::vector<var_t>     T_var_t;
    typedef bezier_curve<Numeric, Numeric, Dim, true, linear_variable<Dim, Numeric> > bezier_t;

    std::vector<var_t> variables_; // includes constant variables
    std::size_t numVariables; // total number of variable (* DIM for total size)
    std::size_t numControlPoints; // total number of variable (* DIM for total size)
    std::size_t startVariableIndex; //before that index, variables are constant
    std::size_t numStateConstraints;
    bezier_t* bezier;


    problem_data(const problem_data& other)
        : variables_(other.variables_)
        , numVariables(other.numVariables)
        , numControlPoints(other.numControlPoints)
        , startVariableIndex(other.startVariableIndex)
        , numStateConstraints(other.numStateConstraints)
    {
        const bezier_t& b = *other.bezier;
        bezier = new bezier_t(b.waypoints().begin(), b.waypoints().end(),b.T_, b.mult_T_);
    }
};

inline std::size_t num_active_constraints(const constraint_flag& flag)
{
    long lValue = (long)(flag);
    std::size_t iCount = 0;
    while (lValue != 0)
    {
        lValue = lValue & (lValue - 1);
        iCount++;
    }
    return (flag & NONE) ? iCount-1 : iCount;
}

template <int Dim, typename Numeric, typename LinearVar>
LinearVar fill_with_zeros(const LinearVar& var,const std::size_t i,
                          const std::size_t startVariableIndex,
                          const std::size_t numVariables)
{
    typedef Eigen::Matrix<Numeric, Eigen::Dynamic, Eigen::Dynamic> matrix_t;
    typename LinearVar::matrix_dim_x_t B;
    B = matrix_t::Zero(Dim,numVariables*Dim);
    if( startVariableIndex  <= i  && i<= startVariableIndex +numVariables-1 )
        B.block(0,Dim*(i-startVariableIndex),Dim,Dim) = var.B();
    return LinearVar (B,var.c());
}


template <typename Point, int Dim, typename Numeric, typename Bezier, typename LinearVar>
Bezier* compute_linear_control_points(const problem_data<Point, Dim, Numeric>& pData,
        const std::vector<LinearVar>& linearVars, const Numeric totalTime)
{
    std::vector<LinearVar> res;
    // now need to fill all this with zeros...
    std::size_t totalvar = linearVars.size();
    for (std::size_t i = 0; i < totalvar; ++i)
        res.push_back( fill_with_zeros<Dim, Numeric, LinearVar>(linearVars[i],i,
                                                                pData.startVariableIndex,
                                                                pData.numVariables));
    return new Bezier(res.begin(),res.end(), totalTime);
}


template<typename Point, int Dim, typename Numeric>
problem_data<Point, Dim, Numeric> setup_control_points(const problem_definition<Point, Dim, Numeric>& pDef)
{
    typedef Numeric num_t;
    typedef Point   point_t;
    typedef linear_variable<Dim, Numeric>     var_t;
    typedef problem_data<Point, Dim, Numeric> problem_data_t;

    const curve_constraints<Point>& constraints = pDef.curveConstraints;
    const std::size_t& degree = pDef.degree;
    const constraint_flag& flag = pDef.flag;

    const std::size_t numControlPoints = pDef.degree +1;
    const std::size_t numActiveConstraints = num_active_constraints(flag);
    if (numActiveConstraints >= numControlPoints)
        throw std::runtime_error("In setup_control_points; too many constraints for the considered degree");


    problem_data_t problemData;
    typename problem_data_t::T_var_t& variables_ = problemData.variables_;

    std::size_t numConstants = 0;
    std::size_t i =0;
    if(flag & INIT_POS)
    {
        variables_.push_back(var_t(pDef.start));
        ++numConstants;
        ++i;
        if(flag & INIT_VEL)
        {
            point_t vel = pDef.start + (constraints.init_vel / (num_t)degree) / pDef.totalTime;
            variables_.push_back(var_t(vel));
            ++numConstants;
            ++i;
            if(flag & INIT_ACC)
            {
                point_t acc = (constraints.init_acc / (num_t)(degree * (degree-1)))
                        / (pDef.totalTime *  pDef.totalTime)
                        + 2* vel- pDef.start;;
                variables_.push_back(var_t(acc));
                ++numConstants;
                ++i;
                if(flag & INIT_JERK){
                  point_t jerk = constraints.init_jerk*pDef.totalTime*pDef.totalTime*pDef.totalTime/(degree*(degree-1)*(degree-2))
                  + 3*acc -3*vel +pDef.start;
                  variables_.push_back(var_t(jerk));
                  ++numConstants;
                  ++i;
                }
            }
        }
    }
    const std::size_t first_variable_idx = i;
    // variables
    for(; i + 4< numControlPoints; ++i)
        variables_.push_back(var_t());
    //end constraints
    if(flag & END_POS)
    {
        if(flag & END_VEL)
        {
            point_t vel = pDef.end - (constraints.end_vel  / (num_t)degree) / pDef.totalTime;
            if(flag & END_ACC)
            {
                point_t acc = (constraints.end_acc  / (num_t)(degree * (degree-1)))
                        / (pDef.totalTime) * (pDef.totalTime)
                        + 2* vel - pDef.end;
                if(flag & END_JERK){
                  point_t jerk = -constraints.end_jerk*pDef.totalTime*pDef.totalTime*pDef.totalTime/(degree*(degree-1)*(degree-2))
                  + 3*acc -3*vel + pDef.end;
                  variables_.push_back(var_t(jerk));
                  ++numConstants;
                  ++i;
                }else while(i<numControlPoints -3){
                  variables_.push_back(var_t());
                  ++i;
                }
                variables_.push_back(var_t(acc));
                ++numConstants; ++i;
            }
            else while(i<numControlPoints-2)
            {
                variables_.push_back(var_t());
                ++i;
            }
            variables_.push_back(var_t(vel));
            ++numConstants; ++i;
        }
        else
        {
            while(i<numControlPoints-1)
            {
                variables_.push_back(var_t());
                ++i;
            }
        }
        variables_.push_back(var_t(pDef.end));
        ++numConstants; ++i;
    }
    // add remaining variables (only if no end_pos constraints)
    for(; i<numControlPoints; ++i)
        variables_.push_back(var_t());

    assert(numControlPoints > numConstants);
    assert(numControlPoints == variables_.size());


    problemData.numControlPoints = numControlPoints;
    problemData.numVariables = numControlPoints-numConstants;
    problemData.startVariableIndex =first_variable_idx;
    problemData.numStateConstraints = numActiveConstraints - problemData.numVariables;
    problemData.bezier = compute_linear_control_points<Point, Dim, Numeric,
                                            bezier_curve<Numeric, Numeric, Dim, true,var_t>,
                                            var_t>(problemData, variables_,  pDef.totalTime);
    return problemData;
}


// TODO assumes constant are inside constraints...
template<typename Point, int Dim, typename Numeric>
long compute_num_ineq_control_points
(const problem_definition<Point, Dim, Numeric>& pDef, const problem_data<Point, Dim, Numeric> & pData)
{
    typedef problem_definition<Point, Dim, Numeric> problem_definition_t;
    long rows(0);
    // rows depends on each constraint size, and the number of waypoints
    for (typename problem_definition_t::CIT_vectorx_t cit = pDef.inequalityVectors_.begin();
         cit != pDef.inequalityVectors_.end(); ++cit)
        rows += cit->rows() * pData.numControlPoints;
    return rows;
}

template<typename Point, int Dim, typename Numeric>
long compute_num_ineq_state_constraints
(const problem_definition<Point, Dim, Numeric>& pDef, const problem_data<Point, Dim, Numeric> & pData)
{
    //TODO
    return 0;
}

template<typename Point, int Dim, typename Numeric>
std::vector<bezier_curve<Numeric, Numeric, Dim, true,
            linear_variable<Dim, Numeric> > >
split(const problem_definition<Point, Dim, Numeric>& pDef, problem_data<Point, Dim, Numeric> & pData)
{
    typedef linear_variable<Dim, Numeric> linear_variable_t;
    typedef bezier_curve< Numeric, Numeric, Dim, true,linear_variable_t> bezier_t;
    typedef std::vector<bezier_t> T_bezier_t;

    const Eigen::VectorXd& times = pDef.splitTimes_;
    T_bezier_t res;
    bezier_t& current = *pData.bezier;
    Numeric current_time = 0.;
    Numeric tmp;
    for(int i = 0; i < times.rows(); ++i)
    {
        tmp =times[i];
        std::pair<bezier_t, bezier_t> pairsplit = current.split(tmp-current_time);
        res.push_back(pairsplit.first);
        current = pairsplit.second;
        current_time += tmp-current_time;
    }
    res.push_back(current);
    return res;
}

template<typename Point, int Dim, typename Numeric>
void initInequalityMatrix
(const problem_definition<Point, Dim, Numeric>& pDef, problem_data<Point, Dim, Numeric> & pData,
    problem<Point, Dim, Numeric>& prob)
{
    typedef problem_definition<Point, Dim, Numeric> problem_definition_t;
    typedef typename problem_definition_t::matrix_x_t matrix_x_t;
    typedef typename problem_definition_t::vectorx_t vectorx_t;
    typedef Eigen::Matrix<Numeric, Dim, Eigen::Dynamic> matrix_dimx_t;
    typedef Eigen::Matrix<Numeric, Dim, 1> vector_dim_t;
    typedef std::vector<matrix_dimx_t, Eigen::aligned_allocator<matrix_dimx_t> > T_matrix_dimx_t;
    typedef std::vector<matrix_dimx_t, Eigen::aligned_allocator<vector_dim_t> > T_vector_dim_t;
    typedef typename T_matrix_dimx_t::const_iterator CIT_matrix_dimx_t;
    typedef typename T_vector_dim_t::const_iterator CIT_vector_dim_t;
    typedef bezier_curve<Numeric, Numeric, Dim, true, linear_variable<Dim, Numeric> >  bezier_t;
    typedef std::vector<bezier_t> T_bezier_t;
    typedef typename T_bezier_t::const_iterator CIT_bezier_t;
    typedef typename bezier_t::t_point_t t_point;
    typedef typename bezier_t::t_point_t::const_iterator cit_point;

    long cols =  pData.numVariables * Dim;
    long rows = compute_num_ineq_control_points<Point, Dim, Numeric>(pDef, pData);
    //rows+= compute_num_ineq_state_constraints<Point, Dim, Numeric>(pDef, pData); // TODO
    prob.ineqMatrix = matrix_x_t::Zero(rows,cols);
    prob.ineqVector = vectorx_t::Zero(rows);

    // compute sub-bezier curves
    T_bezier_t beziers = split<Point, Dim, Numeric>(pDef,pData);

    assert(pDef.inequalityMatrices_.size() == pDef.inequalityVectors_.size());
    assert(pDef.inequalityMatrices_.size() == beziers.size());

    long currentRowIdx = 0;
    typename problem_definition_t::CIT_matrix_dim_t cmit = pDef.inequalityMatrices_.begin();
    typename problem_definition_t::CIT_vectorx_t cvit = pDef.inequalityVectors_.begin();
    // for each bezier split ..
    for (CIT_bezier_t bit = beziers.begin();
         bit != beziers.end(); ++bit, ++cvit, ++cmit)
    {
        //compute vector of linear expressions of each control point

        const t_point& wps = bit->waypoints();
        // each control has a linear expression depending on all variables
        for(cit_point cit = wps.begin(); cit != wps.end(); ++cit)
        {
            prob.ineqMatrix.block(currentRowIdx, 0,cmit->rows(),cols)
                    = (*cmit)*(cit->B()) ; // constraint inequality for current bezier * expression of control point
            prob.ineqVector.segment(currentRowIdx,cmit->rows()) = *cvit - (*cmit)*(cit->c()) ;
            currentRowIdx += cmit->rows();
        }
    }
    assert (rows == currentRowIdx); // we filled all the constraints
}

template<typename Point, int Dim, typename Numeric, typename In >
quadratic_variable<Numeric> bezier_product(
        const problem_data<Point, Dim, Numeric>& pData, In PointsBegin1, In PointsEnd1, In PointsBegin2, In PointsEnd2)
{
    typedef Eigen::Matrix<Numeric, Eigen::Dynamic, 1> vector_x_t;
    unsigned int nPoints1 = (unsigned int)(std::distance(PointsBegin1,PointsEnd1)),
                 nPoints2 = (unsigned int)(std::distance(PointsBegin2,PointsEnd2));
    assert(nPoints1 > 0); assert(nPoints2 > 0);
    unsigned int deg1 = nPoints1-1, deg2 = nPoints2 -1;
    unsigned int newDeg = (deg1 + deg2);
    // the integral of the primitive will simply be the last control points of the primitive,
    // divided by the degree of the primitive, newDeg. We will store this in matrices for bilinear terms,
    // and a vector for the linear terms, as well as another one for the constants.
    quadratic_variable<Numeric> res(vector_x_t::Zero(PointsBegin1->B().cols()));
    // depending on the index, the fraction coefficient of the bernstein polynom
    // is either the fraction given by  (i+j)/ (deg1+deg2), or 1 - (i+j)/ (deg1+deg2).
    // The trick is that the condition is given by whether the current index in
    // the combinatorial is odd or even.
    // time parametrization is not relevant for the cost

    Numeric ratio;
    for(unsigned int i = 0; i < newDeg+1; ++i)
    {
        unsigned int j = i > deg2 ? i-deg2 : 0;
        for(; j< std::min(deg1,i)+1;++j)
        {
            ratio = (Numeric)(bin(deg1,j)*bin(deg2,i-j)) / (Numeric)(bin(newDeg,i));
            In itj = PointsBegin1 + j ;
            In iti = PointsBegin2 +(i-j) ;
            res+= ((*itj) * (*iti)) * ratio;
        }
    }
    return res/(newDeg+1);
}

inline constraint_flag operator~(constraint_flag a)
{return static_cast<constraint_flag>(~static_cast<const int>(a));}

inline constraint_flag operator|(constraint_flag a, constraint_flag b)
{return static_cast<constraint_flag>(static_cast<const int>(a) | static_cast<const int>(b));}

inline constraint_flag operator&(constraint_flag a, constraint_flag b)
{return static_cast<constraint_flag>(static_cast<const int>(a) & static_cast<const int>(b));}

inline constraint_flag operator^(constraint_flag a, constraint_flag b)
{return static_cast<constraint_flag>(static_cast<const int>(a) ^ static_cast<const int>(b));}

inline constraint_flag& operator|=(constraint_flag& a, constraint_flag b)
{return (constraint_flag&)((int&)(a) |= static_cast<const int>(b));}

inline constraint_flag& operator&=(constraint_flag& a, constraint_flag b)
{return (constraint_flag&)((int&)(a) &= static_cast<const int>(b));}

inline constraint_flag& operator^=(constraint_flag& a, constraint_flag b)
{return (constraint_flag&)((int&)(a) ^= static_cast<const int>(b));}

} // namespace optimization
} // namespace spline
#endif //_CLASS_LINEAR_PROBLEM_DETAILS

