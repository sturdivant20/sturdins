/**
 * *least-squares.hpp*
 *
 * =======  ========================================================================================
 * @file    sturdins/least-squares.hpp
 * @brief   Least squares algorithms for initializing the GNSS-INS filter.
 * @date    January 2025
 * @author  Daniel Sturdivant <sturdivant20@gmail.com>
 * @ref     1. "Principles of GNSS, Inertial, and Multisensor Integrated Navigation Systems", 2nd
 *              Edition, 2013 - Groves
 * =======  ========================================================================================
 */

// TODO: gradient descent, levenburg-marquardt
// TODO: maybe a particle filter?

#ifndef STURDINS_LEAST_SQUARES_HPP
#define STURDINS_LEAST_SQUARES_HPP

#include <Eigen/Dense>

namespace sturdins {

/**
 * *=== RangeAndRate ===*
 * @brief predicts a range and rate based on a satellite location and velocity
 * @param pos         User ECEF position [m]
 * @param vel         User ECEF velocity [m/s]
 * @param cb          User clock bias [m]
 * @param cd          User clock drift [m/s]
 * @param sv_pos      Satellite ECEF positions [m]
 * @param sv_vel      Satellite ECEF velocities [m/s]
 * @param pred_u      reference to unit vector to satellite
 * @param pred_udot   reference to unit vector rate of change to satellite
 * @param pred_psr    Reference to pseudorange prediction
 * @param pred_psrdot Reference to pseudorange-rate prediction
 */
void RangeAndRate(
    const Eigen::Vector3d &pos,
    const Eigen::Vector3d &vel,
    const double &cb,
    const double &cd,
    const Eigen::Vector3d &sv_pos,
    const Eigen::Vector3d &sv_vel,
    Eigen::Vector3d &u,
    Eigen::Vector3d &udot,
    double &pred_psr,
    double &pred_psrdot);

/**
 * *=== GaussNewton ===*
 * @brief Least Squares solver for GNSS position, velocity, and timing terms
 * @param x           Initial state estimate
 * @param P           Initial covariance estimate
 * @param sv_pos      Satellite ECEF positions [m]
 * @param sv_vel      Satellite ECEF velocities [m/s]
 * @param psr         Pseudorange measurements [m]
 * @param psrdot      Pseudorange-rate measurements [m/s]
 * @param psr_var     Pseudorange measurement variance [m^2]
 * @param psrdot_var  Pseudorange-rate measurement variance [(m/s)^2]
 */
bool GaussNewton(
    Eigen::VectorXd &x,
    Eigen::MatrixXd &P,
    const Eigen::MatrixXd &sv_pos,
    const Eigen::MatrixXd &sv_vel,
    const Eigen::VectorXd &psr,
    const Eigen::VectorXd &psrdot,
    const Eigen::VectorXd &psr_var,
    const Eigen::VectorXd &psrdot_var);
}  // namespace sturdins

#endif