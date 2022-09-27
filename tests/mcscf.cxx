#include "ut_common.hpp"
#include <asci/fcidump.hpp>
#include <asci/util/mcscf.hpp>
#include <iomanip>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>

const std::string fci_file = "/home/dbwy/devel/casscf/ASCI-CI/tests/ref_data/h2o.ccpvdz.fci.dat";
const std::string rdm_file = "/home/dbwy/devel/casscf/ASCI-CI/tests/ref_data/h2o.ccpvdz.cas88.rdms.bin";

TEST_CASE("MCSCF") {

  ROOT_ONLY(MPI_COMM_WORLD);

  spdlog::null_logger_mt("davidson");
  spdlog::null_logger_mt("ci_solver");
  spdlog::null_logger_mt("diis");
  spdlog::null_logger_mt("casscf");

  const size_t norb  = asci::read_fcidump_norb(fci_file);
  const size_t norb2 = norb  * norb;
  const size_t norb4 = norb2 * norb2;

  using asci::NumOrbital;
  using asci::NumInactive;
  using asci::NumActive;
  using asci::NumVirtual;
  using asci::NumElectron;

  std::vector<double> T(norb2), V(norb4);
  auto E_core = asci::read_fcidump_core(fci_file);
  asci::read_fcidump_1body(fci_file, T.data(), norb);
  asci::read_fcidump_2body(fci_file, V.data(), norb);

  size_t n_inactive = 1;
  size_t n_active   = 8;
  size_t n_virtual  = norb - n_inactive - n_active;
  NumElectron nalpha(4);

  NumInactive ninact(n_inactive);
  NumActive   nact(n_active);
  NumVirtual  nvirt(n_virtual);


  size_t na2 = n_active  * n_active;
  size_t na4 = na2       * na2;
  std::vector<double> active_ordm(na2), active_trdm(na4);
  asci::MCSCFSettings settings;

  const double ref_E = -76.1114493227;
  
  SECTION("CASSCF - No Guess - Singlet") {
     auto E = asci::casscf_bfgs(settings, nalpha, nalpha, NumOrbital(norb),
       ninact, nact, nvirt, E_core, T.data(), norb, V.data(), norb,
       active_ordm.data(), n_active, active_trdm.data(), n_active,
       MPI_COMM_WORLD);

     REQUIRE(E == Approx(ref_E).margin(1e-7));
  }

  
  SECTION("CASSCF - With Guess - Singlet") {
    asci::read_rdms_binary(rdm_file, n_active, active_ordm.data(), n_active,
      active_trdm.data(), n_active);
     auto E = asci::casscf_bfgs(settings, nalpha, nalpha, NumOrbital(norb),
       ninact, nact, nvirt, E_core, T.data(), norb, V.data(), norb,
       active_ordm.data(), n_active, active_trdm.data(), n_active,
       MPI_COMM_WORLD);

     REQUIRE(E == Approx(ref_E).margin(1e-7));
  }

  spdlog::drop_all();

}
