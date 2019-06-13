#include "Machine/py_abstract_machine.hpp"

#include <cstdio>

#include <pybind11/complex.h>
#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>

#include "Utils/messages.hpp"

namespace netket {

bool ShouldIDoIO() noexcept {
  auto rank = 0;
  auto const status = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (status == MPI_SUCCESS) {
    return rank == 0;
  }
  std::fprintf(stderr,
               "[NetKet] MPI_Comm_rank failed: doing I/O on all processes.\n");
  return true;
}

template <class Function, class... Args>
auto ShouldNotThrow(Function &&function, Args &&... args) noexcept
    -> decltype(std::declval<Function &&>()(std::declval<Args &&>()...)) {
  try {
    return std::forward<Function>(function)(std::forward<Args>(args)...);
  } catch (std::exception &e) {
    if (ShouldIDoIO()) {
      std::fprintf(
          stderr,
          "[NetKet] Fatal error: exception was thrown in a `noexcept` context\n"
          "[NetKet]        Info: %s\n"
          "[NetKet]\n"
          "[NetKet] This is a bug. Please, be so kind to open an issue at\n"
          "[NetKet]             https://github.com/netket/netket/issues\n"
          "[NetKet]\n"
          "[NetKet] Aborting...\n",
          e.what());
    }
    MPI_Abort(MPI_COMM_WORLD, -1);
    std::abort();  // This call is unreachable and is here just to tell the
                   // compiler that this branch never returns (MPI_Abort is not
                   // marked [[noreturn]]).
  } catch (...) {
    if (ShouldIDoIO()) {
      std::fprintf(
          stderr,
          "[NetKet] Fatal error: exception was thrown in a `noexcept` context\n"
          "[NetKet] Aborting...\n");
    }
    MPI_Abort(MPI_COMM_WORLD, -1);
    std::abort();  // This call is unreachable and is here just to tell the
                   // compiler that this branch never returns (MPI_Abort is not
                   // marked [[noreturn]]).
  }
}

int PyAbstractMachine::Npar() const {
  PYBIND11_OVERLOAD_PURE_NAME(
      int,                  /* Return type */
      AbstractMachine,      /* Parent class */
      "_number_parameters", /* Name of the function in Python */
      Npar,                 /* Name of function in C++ */
  );
}

int PyAbstractMachine::Nvisible() const {
  PYBIND11_OVERLOAD_PURE_NAME(
      int,               /* Return type */
      AbstractMachine,   /* Parent class */
      "_number_visible", /* Name of the function in Python */
      Nvisible,          /* Name of function in C++ */
  );
}

bool PyAbstractMachine::IsHolomorphic() const noexcept {
  return ShouldNotThrow([this]() {
    PYBIND11_OVERLOAD_PURE_NAME(
        bool,              /* Return type */
        AbstractMachine,   /* Parent class */
        "_is_holomorphic", /* Name of the function in Python */
        IsHolomorphic,     /* Name of function in C++ */
    );
  });
}

const AbstractHilbert &PyAbstractMachine::GetHilbert() const noexcept {
  // return ShouldNotThrow([this]() {
  PYBIND11_OVERLOAD_PURE_NAME(
      const AbstractHilbert &, /* Return type */
      AbstractMachine,         /* Parent class */
      "_get_hilbert",          /* Name of the function in Python */
      GetHilbert,              /* Name of function in C++ */
  );
  // });
}

PyAbstractMachine::VectorType PyAbstractMachine::GetParameters() {
  PYBIND11_OVERLOAD_PURE_NAME(
      VectorType,        /* Return type */
      AbstractMachine,   /* Parent class */
      "_get_parameters", /* Name of the function in Python */
      GetParameters,     /* Name of function in C++ */
  );
}

void PyAbstractMachine::SetParameters(VectorConstRefType pars) {
  PYBIND11_OVERLOAD_PURE_NAME(
      void,              /* Return type */
      AbstractMachine,   /* Parent class */
      "_set_parameters", /* Name of the function in Python */
      SetParameters,     /* Name of function in C++ */
      pars);
}

void PyAbstractMachine::InitRandomPars(int const seed, double const sigma) {
  VectorType par(Npar());
  netket::RandomGaussian(par, seed, sigma);
  SetParameters(par);
}

Complex PyAbstractMachine::LogVal(VisibleConstType v) {
  PYBIND11_OVERLOAD_PURE_NAME(Complex,         /* Return type */
                              AbstractMachine, /* Parent class */
                              "log_val", /* Name of the function in Python */
                              LogVal,    /* Name of function in C++ */
                              v);
}

Complex PyAbstractMachine::LogVal(VisibleConstType v,
                                  const LookupType & /*unused*/) {
  return LogVal(v);
}

void PyAbstractMachine::InitLookup(VisibleConstType /*unused*/,
                                   LookupType & /*unused*/) {}

void PyAbstractMachine::UpdateLookup(VisibleConstType /*unused*/,
                                     const std::vector<int> & /*unused*/,
                                     const std::vector<double> & /*unused*/,
                                     LookupType & /*unused*/) {}

Complex PyAbstractMachine::LogValDiff(VisibleConstType old_v,
                                      const std::vector<int> &to_change,
                                      const std::vector<double> &new_conf) {
  auto const old_value = LogVal(old_v);
  VisibleType new_v{old_v};
  GetHilbert().UpdateConf(new_v, to_change, new_conf);
  auto const new_value = LogVal(new_v);
  return new_value - old_value;
}

PyAbstractMachine::VectorType PyAbstractMachine::LogValDiff(
    VisibleConstType old_v, const std::vector<std::vector<int>> &to_change,
    const std::vector<std::vector<double>> &new_conf) {
  assert(to_change.size() == new_conf.size());
  VectorType result(to_change.size());
  for (auto i = size_t{0}; i < to_change.size(); ++i) {
    result(i) = LogValDiff(old_v, to_change[i], new_conf[i]);
  }
  return result;
}

Complex PyAbstractMachine::LogValDiff(VisibleConstType v,
                                      const std::vector<int> &to_change,
                                      const std::vector<double> &new_conf,
                                      const LookupType & /*unused*/) {
  return LogValDiff(v, to_change, new_conf);
}

PyAbstractMachine::VectorType PyAbstractMachine::DerLog(VisibleConstType v) {
  PYBIND11_OVERLOAD_PURE_NAME(VectorType,      /* Return type */
                              AbstractMachine, /* Parent class */
                              "der_log", /* Name of the function in Python */
                              DerLog,    /* Name of function in C++ */
                              v);
}

PyAbstractMachine::VectorType PyAbstractMachine::DerLog(
    VisibleConstType v, const LookupType & /*lt*/) {
  return DerLog(v);
}

PyAbstractMachine::VectorType PyAbstractMachine::DerLogChanged(
    VisibleConstType old_v, const std::vector<int> &to_change,
    const std::vector<double> &new_conf) {
  VisibleType new_v{old_v};
  GetHilbert().UpdateConf(new_v, to_change, new_conf);
  return DerLog(new_v);
}

void PyAbstractMachine::Save(const std::string &filename) const {
  PYBIND11_OVERLOAD_PURE_NAME(void,            /* Return type */
                              AbstractMachine, /* Parent class */
                              "save",  /* Name of the function in Python */
                              Save,    /* Name of function in C++ */
                              filename /*Arguments*/
  );
}

void PyAbstractMachine::Load(const std::string &filename) {
  PYBIND11_OVERLOAD_PURE_NAME(void,            /* Return type */
                              AbstractMachine, /* Parent class */
                              "load",  /* Name of the function in Python */
                              Load,    /* Name of function in C++ */
                              filename /*Arguments*/
  );
}

}  // namespace netket
