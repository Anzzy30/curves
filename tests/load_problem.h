
#ifndef _CLASS_LOAD_TEST_PROBLEMS
#define _CLASS_LOAD_TEST_PROBLEMS


#include "hpp/spline/exact_cubic.h"
#include "hpp/spline/bezier_curve.h"
#include "hpp/spline/polynom.h"
#include "hpp/spline/spline_deriv_constraint.h"
#include "hpp/spline/helpers/effector_spline.h"
#include "hpp/spline/helpers/effector_spline_rotation.h"
#include "hpp/spline/bezier_polynom_conversion.h"
#include "hpp/spline/optimization/linear_problem.h"
#include "hpp/spline/optimization/quadratic_cost.h"
#include "hpp/spline/optimization/details.h"

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>

namespace spline {

typedef Eigen::Vector3d point_t;
typedef std::vector<point_t,Eigen::aligned_allocator<point_t> >  t_point_t;
typedef polynom  <double, double, 3, true, point_t, t_point_t> polynom_t;
typedef exact_cubic <double, double, 3, true, point_t> exact_cubic_t;
typedef spline_deriv_constraint <double, double, 3, true, point_t> spline_deriv_constraint_t;
typedef bezier_curve  <double, double, 3, true, point_t> bezier_curve_t;
typedef spline_deriv_constraint_t::spline_constraints spline_constraints_t;
typedef std::pair<double, point_t> Waypoint;
typedef std::vector<Waypoint> T_Waypoint;


typedef Eigen::Matrix<double,1,1> point_one;
typedef polynom<double, double, 1, true, point_one> polynom_one;
typedef exact_cubic   <double, double, 1, true, point_one> exact_cubic_one;
typedef std::pair<double, point_one> WaypointOne;
typedef std::vector<WaypointOne> T_WaypointOne;

namespace optimization
{
typedef curve_constraints<point_t> constraint_linear;
typedef linear_variable<3, double> linear_variable_t;
typedef std::vector<linear_variable_t> T_linear_variable_t;
typedef T_linear_variable_t::const_iterator CIT_linear_variable_t;
typedef std::pair<std::size_t, std::size_t >   pair_size_t;
typedef std::pair<T_linear_variable_t, pair_size_t > var_pair_t;
typedef problem_data<point_t, 3, double> problem_data_t;
typedef problem_definition<point_t, 3, double> problem_definition_t;
typedef problem<point_t, 3, double> problem_t;


#define MAXBUFSIZE  ((int) 1e6)

Eigen::MatrixXd readMatrix(std::ifstream& infile)
{
    int cols = 0, rows = 0;
    double buff[MAXBUFSIZE];

    // Read numbers from file into buffer.
    //ifstream infile;
    //infile.open(filename);
    std::string line = "noise";
    while (!infile.eof() && !line.empty())
    {
        std::getline(infile, line);

        int temp_cols = 0;
        std::stringstream stream(line);
        while(! stream.eof())
            stream >> buff[cols*rows+temp_cols++];

        if (temp_cols == 0)
            continue;

        if (cols == 0)
            cols = temp_cols;

        rows++;
    }
    //infile.close();
    rows--;

    // Populate matrix with numbers.
    Eigen::MatrixXd result(rows,cols);
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result(i,j) = buff[ cols*i+j ];

    return result;
}

problem_definition_t loadproblem(const std::string& filename)
{
    problem_definition_t pDef;
    std::ifstream in (filename.c_str());
    if (!in.is_open())
        throw std::runtime_error("cant open filename");
    //first line is degree totaltime flag
    Eigen::Vector3d degTimeFlag = readMatrix(in);
    pDef.degree = (std::size_t)(degTimeFlag[0]);
    pDef.totalTime =degTimeFlag[1];
    pDef.flag = (constraint_flag)(degTimeFlag[2]);
    //Then startpos then empty line
    pDef.start = readMatrix(in);
    //Then endpos then empty line
    pDef.end = readMatrix(in);
    //Then splittimes then empty line
    pDef.splitTimes_ = readMatrix(in);
    // The inequality matrices, empty line, inequality vector as many times
    for (int i = 0; i< pDef.splitTimes_.rows()+1; ++i)
    {
        pDef.inequalityMatrices_.push_back(readMatrix(in));
        pDef.inequalityVectors_.push_back(readMatrix(in));
    }
    in.close();
    return pDef;
    // TODO curve constraints
}

}
}


#endif //_CLASS_LOAD_TEST_PROBLEMS
