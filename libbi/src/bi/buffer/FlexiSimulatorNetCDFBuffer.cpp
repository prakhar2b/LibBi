/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev: 2589 $
 * $Date: 2012-05-23 13:15:11 +0800 (Wed, 23 May 2012) $
 */
#include "FlexiSimulatorNetCDFBuffer.hpp"

#include "../math/view.hpp"

using namespace bi;

FlexiSimulatorNetCDFBuffer::FlexiSimulatorNetCDFBuffer(const Model& m,
    const std::string& file, const FileMode mode) :
    NetCDFBuffer(file, mode), m(m), vars(NUM_VAR_TYPES) {
  /* pre-condition */
  assert (mode == READ_ONLY || mode == WRITE);

  map();
}

FlexiSimulatorNetCDFBuffer::FlexiSimulatorNetCDFBuffer(const Model& m,
    const int T, const std::string& file, const FileMode mode) :
    NetCDFBuffer(file, mode), m(m), vars(NUM_VAR_TYPES) {
  if (mode == NEW || mode == REPLACE) {
    create(T); // set up structure of new file
  } else {
    map(T);
  }
}

FlexiSimulatorNetCDFBuffer::~FlexiSimulatorNetCDFBuffer() {
  unsigned i, j;
  for (i = 0; i < vars.size(); ++i) {
    for (j = 0; j < vars[i].size(); ++j) {
      delete vars[i][j];
    }
  }
}

void FlexiSimulatorNetCDFBuffer::create(const long T) {
  int id, i;
  VarType type;
  Var* var;
  Dim* dim;

  /* dimensions */
  nrDim = createDim("nr", T);
  for (i = 0; i < m.getNumDims(); ++i) {
    dim = m.getDim(i);
    nDims.push_back(createDim(dim->getName().c_str(), dim->getSize()));
  }
  nrpDim = createDim("nrp");

  /* time variable */
  tVar = ncFile->add_var("time", netcdf_real, nrDim);
  BI_ERROR(tVar != NULL && tVar->is_valid(), "Could not create time variable");

  /* nrp dimension indexing variables */
  startVar = ncFile->add_var("start", ncInt, nrDim);
  BI_ERROR(startVar != NULL && startVar->is_valid(), "Could not create start variable");

  lenVar = ncFile->add_var("len", ncInt, nrDim);
  BI_ERROR(lenVar != NULL && lenVar->is_valid(), "Could not create len variable");

  /* other variables */
  for (i = 0; i < NUM_VAR_TYPES; ++i) {
    type = static_cast<VarType>(i);
    vars[type].resize(m.getNumVars(type), NULL);
    if (type == D_VAR || type == R_VAR) {
      for (id = 0; id < (int)vars[type].size(); ++id) {
        var = m.getVar(type, id);
        if (var->getIO()) {
          vars[type][id] = new NcVarBuffer<real>(createFlexiVar(var));
        }
      }
    }
  }
}

void FlexiSimulatorNetCDFBuffer::map(const long T) {
  std::string name;
  int id, i;
  VarType type;
  Var* node;
  Dim* dim;

  /* dimensions */
  BI_ERROR(hasDim("nr"), "File must have nr dimension");
  nrDim = mapDim("nr", T);
  for (i = 0; i < m.getNumDims(); ++i) {
    dim = m.getDim(i);
    BI_ERROR(hasDim(dim->getName().c_str()), "File must have " <<
        dim->getName() << " dimension");
    nDims.push_back(mapDim(dim->getName().c_str(), dim->getSize()));
  }
  BI_ERROR(hasDim("nrp"), "File must have nrp dimension");
  nrpDim = mapDim("nrp");

  /* time variable */
  tVar = ncFile->get_var("time");
  BI_ERROR(tVar != NULL && tVar->is_valid(), "File does not contain" <<
      " variable time");
  BI_ERROR(tVar->num_dims() == 1, "Variable time has " << tVar->num_dims() <<
      " dimensions, should have 1");
  BI_ERROR(tVar->get_dim(0) == nrDim, "Dimension 0 of variable time" <<
      " should be nr");

  /* nrp dimension indexing variables */
  startVar = ncFile->get_var("start");
  BI_ERROR(startVar != NULL && startVar->is_valid(), "File does not" <<
      " contain variable start");
  BI_ERROR(startVar->num_dims() == 1, "Variable start has " <<
      startVar->num_dims() << " dimensions, should have 1");
  BI_ERROR(startVar->get_dim(0) == nrpDim, "Dimension 0 of variable" <<
      " start should be nrp");

  lenVar = ncFile->get_var("len");
  BI_ERROR(lenVar != NULL && lenVar->is_valid(), "File does not" <<
      " contain variable len");
  BI_ERROR(lenVar->num_dims() == 1, "Variable len has " <<
      lenVar->num_dims() << " dimensions, should have 1");
  BI_ERROR(lenVar->get_dim(0) == nrpDim, "Dimension 0 of variable" <<
      " len should be nrp");

  /* other variables */
  for (i = 0; i < NUM_VAR_TYPES; ++i) {
    type = static_cast<VarType>(i);
    if (type == D_VAR || type == R_VAR) {
      vars[type].resize(m.getNumVars(type), NULL);
      for (id = 0; id < m.getNumVars(type); ++id) {
        node = m.getVar(type, id);
        if (hasVar(node->getName().c_str())) {
          vars[type][id] = new NcVarBuffer<real>(mapVar(m.getVar(type, id)));
        }
      }
    }
  }
}

real FlexiSimulatorNetCDFBuffer::readTime(const int t) {
  /* pre-condition */
  assert (t >= 0 && t < nrDim->size());

  real time;
  BI_UNUSED NcBool ret;
  ret = tVar->set_cur(t);
  BI_ASSERT(ret, "Indexing out of bounds reading " << tVar->name());
  ret = tVar->get(&time, 1);
  BI_ASSERT(ret, "Inconvertible type reading " << tVar->name());

  return time;
}

void FlexiSimulatorNetCDFBuffer::writeTime(const int t, const real& x) {
  /* pre-condition */
  assert (t >= 0 && t < nrDim->size());

  BI_UNUSED NcBool ret;
  ret = tVar->set_cur(t);
  BI_ASSERT(ret, "Indexing out of bounds writing " << tVar->name());
  ret = tVar->put(&x, 1);
  BI_ASSERT(ret, "Inconvertible type writing " << tVar->name());
}

int FlexiSimulatorNetCDFBuffer::readStart(const int t) {
  /* pre-condition */
  assert (t >= 0 && t < nrDim->size());

  int start;
  BI_UNUSED NcBool ret;
  ret = startVar->set_cur(t);
  BI_ASSERT(ret, "Indexing out of bounds reading " << startVar->name());
  ret = startVar->get(&start, 1);
  BI_ASSERT(ret, "Inconvertible type reading " << startVar->name());

  return start;
}

void FlexiSimulatorNetCDFBuffer::writeStart(const int t, const int& x) {
  /* pre-condition */
  assert (t >= 0 && t < nrDim->size());

  BI_UNUSED NcBool ret;
  ret = startVar->set_cur(t);
  BI_ASSERT(ret, "Indexing out of bounds writing " << startVar->name());
  ret = startVar->put(&x, 1);
  BI_ASSERT(ret, "Inconvertible type writing " << startVar->name());
}

int FlexiSimulatorNetCDFBuffer::readLen(const int t) {
  /* pre-condition */
  assert (t >= 0 && t < nrDim->size());

  int len;
  BI_UNUSED NcBool ret;
  ret = lenVar->set_cur(t);
  BI_ASSERT(ret, "Indexing out of bounds reading " << lenVar->name());
  ret = lenVar->get(&len, 1);
  BI_ASSERT(ret, "Inconvertible type reading " << lenVar->name());

  return len;
}

void FlexiSimulatorNetCDFBuffer::writeLen(const int t, const int& x) {
  /* pre-condition */
  assert (t >= 0 && t < nrDim->size());

  BI_UNUSED NcBool ret;
  ret = lenVar->set_cur(t);
  BI_ASSERT(ret, "Indexing out of bounds writing " << lenVar->name());
  ret = lenVar->put(&x, 1);
  BI_ASSERT(ret, "Inconvertible type writing " << lenVar->name());
}