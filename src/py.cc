#include <pybind11/pybind11.h>

namespace py = pybind11;

int add(int i, int j) {
  return i + j;
}

PYBIND11_MODULE(foopy, m) {
  m.doc() = "pybind11 foo example plugin";
  m.def("add", &add, "A function which adds two numbers", py::arg("i"),
        py::arg("j"));
}