
#include <fstream>
#include <iomanip>
#include <iostream>
#include <navtools/frames.hpp>
#include <satutils/ephemeris.hpp>

#include "navtools/attitude.hpp"
#include "navtools/constants.hpp"
#include "sturdins/ins.hpp"
#include "sturdins/least-squares.hpp"
#include "test_common.hpp"

int main() {
  std::cout << std::setprecision(10);

  // parse ephemeris
  std::vector<satutils::KeplerEphem<double>> eph =
      ParseEphemeris<double>("src/sturdins/tests/sv_ephem.bin");

  // --- RUN KINEMATIC NAVIGATION FILTER ---
  std::ifstream fin("src/sturdins/tests/truth_data.bin", std::ios::binary);
  if (!fin) {
    std::cerr << "Error opening file!\n";
  }
  std::ofstream fout("src/sturdins/tests/ins_results.bin", std::ios::binary);
  if (!fout) {
    std::cerr << "Error opening file!\n";
  }

  sturdins::Ins filt;
  NavData<double> truth;
  NavResult<double> result;
  Eigen::Vector3d lla, ned_v, ecef_p, ecef_v, rpy, wb, fb;
  Eigen::Vector3d drift_a{Eigen::Vector3d::Zero()};
  Eigen::Vector3d drift_g{Eigen::Vector3d::Zero()};
  Eigen::Vector2d clock_sim_state{Eigen::Vector2d::Zero()};
  double cb = 0, cd = 0;

  double time = 0.0;
  double T = 0.01;
  double ToW = 521400;
  Eigen::VectorXd psr_var = 30.0 * Eigen::VectorXd::Ones(eph.size());
  Eigen::VectorXd psrdot_var = 0.01 * Eigen::VectorXd::Ones(eph.size());
  double psr_std = 5.48;
  double psrdot_std = 0.1;
  int i = 0;
  while (fin.read(reinterpret_cast<char *>(&truth), sizeof(truth))) {
    // std::cout << "Next data point: \n";
    // std::cout << "\tLLA: [" << truth.lat << ", " << truth.lon << ", " << truth.h << "] \n";
    // std::cout << "\tNEDv: [" << truth.vn << ", " << truth.ve << ", " << truth.vd << "] \n";
    // std::cout << "\tRPY: [" << truth.roll << ", " << truth.pitch << ", " << truth.yaw << "] \n";
    // std::cout << "\tf_ib_b: [" << truth.fx << ", " << truth.fy << ", " << truth.fz << "] \n";
    // std::cout << "\tw_ib_b: [" << truth.wx << ", " << truth.wy << ", " << truth.wz << "] \n\n";

    // extract truth data
    lla << navtools::DEG2RAD<> * truth.lat, navtools::DEG2RAD<> * truth.lon, truth.h;
    ned_v << truth.vn, truth.ve, truth.vd;
    rpy << navtools::DEG2RAD<> * truth.roll, navtools::DEG2RAD<> * truth.pitch,
        navtools::DEG2RAD<> * truth.yaw;
    wb << truth.wx, truth.wy, truth.wz;
    fb << truth.fx, truth.fy, truth.fz;
    navtools::lla2ecef<double>(ecef_p, lla);
    navtools::ned2ecefv<double>(ecef_v, ned_v, lla);

    if (i == 0) {
      // initialize to truth
      filt.SetPosition(navtools::DEG2RAD<> * truth.lat, navtools::DEG2RAD<> * truth.lon, truth.h);
      filt.SetVelocity(truth.vn, truth.ve, truth.vd);
      filt.SetAttitude(
          navtools::DEG2RAD<> * truth.roll,
          navtools::DEG2RAD<> * truth.pitch,
          navtools::DEG2RAD<> * truth.yaw);
      filt.SetClock(clock_sim_state(0), clock_sim_state(1));
      filt.SetClockSpec(h0, h1, h2);
      filt.SetImuSpec(Ba, Na, Bg, Ng);
      std::cout << filt.phi_ << ", " << filt.lam_ << ", " << filt.h_ << ", " << filt.vn_ << ", "
                << filt.ve_ << ", " << filt.vd_ << ", " << filt.cb_ << ", " << filt.cd_ << "\n";

      // initialize with LS
      // Eigen::VectorXd x{Eigen::Vector<double, 8>::Zero()};
      // Eigen::MatrixXd P{Eigen::Matrix<double, 8, 8>::Zero()};
      // sturdins::GaussNewton(
      //     x, P, meas.sv_pos, meas.sv_vel, meas.psr, meas.psrdot, psr_var, psrdot_var);
      // Eigen::Vector3d xyz = x.segment(0, 3);
      // navtools::ecef2lla(lla, xyz);
      // std::cout << "LLA:    [" << truth.lat << ", " << truth.lon << ", " << truth.h << "]\n";
      // std::cout << "LLA LS: [" << navtools::RAD2DEG<> * lla(0) << ", "
      //           << navtools::RAD2DEG<> * lla(1) << ", " << lla(2) << "]\n";
    }

    // simulate imu
    ImuModel(wb, fb, drift_g, drift_a);

    // simulate clock
    ClockModel(clock_sim_state, T);
    cb = clock_sim_state(0);
    cd = clock_sim_state(1);

    // Propagate filter
    filt.Mechanize(wb, fb, T);
    filt.Propagate(wb, fb, T);

    if (~(i % 20)) {
      // simulate gnss measurements
      MeasurementData meas =
          MeasurementModel(ToW, psr_std, psrdot_std, ecef_p, ecef_v, cb, cd, eph);

      // Filter correction
      filt.GnssUpdate(meas.sv_pos, meas.sv_vel, meas.psr, meas.psrdot, psr_var, psrdot_var);

      // Extract and save states
      Eigen::Vector3d f_rpy = navtools::dcm2euler<double>(filt.C_b_l_, true);
      result.t = time;
      result.lat = navtools::RAD2DEG<> * filt.phi_;
      result.lon = navtools::RAD2DEG<> * filt.lam_;
      result.h = filt.h_;
      result.vn = filt.vn_;
      result.ve = filt.ve_;
      result.vd = filt.vd_;
      result.roll = navtools::RAD2DEG<> * f_rpy(0);
      result.pitch = navtools::RAD2DEG<> * f_rpy(1);
      result.yaw = navtools::RAD2DEG<> * f_rpy(2);
      result.cb = filt.cb_;
      result.cd = filt.cd_;
      fout.write(reinterpret_cast<char *>(&result), sizeof(result));

      std::cout << "Next data point: \n";
      std::cout << "\tLLA:      [" << truth.lat << ", " << truth.lon << ", " << truth.h << "]\n";
      std::cout << "\tEst LLA:  [" << result.lat << ", " << result.lon << ", " << result.h
                << "] \n";
      std::cout << "\tNEDv:     [" << truth.vn << ", " << truth.ve << ", " << truth.vd << "] \n";
      std::cout << "\tEst NEDv: [" << result.vn << ", " << result.ve << ", " << result.vd << "] \n";
      std::cout << "\tClock:     [" << cb << ", " << cd << "] \n";
      std::cout << "\tEst Clock: [" << result.cb << ", " << result.cd << "] \n\n";
    }

    time += T;
    ToW += T;
    i++;
  }
  fin.close();
  fout.close();

  return 0;
}