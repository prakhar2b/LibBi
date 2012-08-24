/**
 * @file
 *
 * @author Pierre Jacob <jacob@ceremade.dauphine.fr>
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_BUFFER_SMC2NETCDFBUFFER_HPP
#define BI_BUFFER_SMC2NETCDFBUFFER_HPP

#include "NetCDFBuffer.hpp"
#include "NcVarBuffer.hpp"
#include "../state/State.hpp"
#include "../method/misc.hpp"

#include <vector>

namespace bi {
class SMC2NetCDFBuffer: public NetCDFBuffer {
public:
  /**
   * Constructor.
   *
   * @param m Model.
   * @param file NetCDF file name.
   * @param mode File open mode.
   */
  SMC2NetCDFBuffer(const Model& m, const std::string& file,
      const FileMode mode = READ_ONLY);
  /**
   * Constructor.
   *
   * @param m Model.
   * @param P Number of samples in file.
   * @param T Number of time points in file.
   * @param file NetCDF file name.
   * @param mode File open mode.
   */
  SMC2NetCDFBuffer(const Model& m, const int P, const int T,
      const std::string& file, const FileMode mode = READ_ONLY);

  /**
   * Destructor.
   */
  virtual ~SMC2NetCDFBuffer();

  /**
   * @copydoc concept::SimulatorBuffer::size1()
   */
  int size1() const;

  /**
   * @copydoc concept::SimulatorBuffer::size2()
   */
  int size2() const;

  /**
   * @copydoc #concept::SMC2NetCDFBuffer::readSample()
   */
  template<class V1>
  void readSample(const int k, V1 theta);

  /**
   * @copydoc #concept::SMC2NetCDFBuffer::writeSample()
   */
  template<class V1>
  void writeSample(const int k, const V1 theta);

  /**
   * @copydoc concept::SimulatorBuffer::readState()
   */
  template<class M1>
  void readState(const VarType type, const int t, M1 X);

  /**
   * @copydoc concept::SimulatorBuffer::writeState()
   */
  template<class M1>
  void writeState(const VarType type, const int t, const M1 X,
      const int p = 0);

  /**
   * @copydoc #concept::SMC2NetCDFBuffer::readLogWeights()
   */
  template<class V1>
  void readLogWeights(V1 lws);

  /**
   * @copydoc #concept::SMC2NetCDFBuffer::writeLogWeights()
   */
  template<class V1>
  void writeLogWeights(const V1 lws);

  /**
   * @copydoc #concept::SMC2NetCDFBuffer::writeLogWeights()
   */
  template<class V1>
  void writeLogWeights(const int timeindex, const V1 lws);

  /**
   * Write number of X particles at a given time step
   */
  template<class V1>
  void writeNumberX(const int timeindex, const V1 numberx);

  /**
   * Write evidence at given time.
   */
  void writeEvidence(const int k, const real evidence);

  /**
   * Write ESS at given time.
   */
  void writeEss(const int k, const real ess);

  /**
   * Write acceptance rate at given time.
   */
  void writeAcceptanceRate(const int k, const real acceptanceRate);

protected:
  /**
   * Set up structure of NetCDF file.
   */
  void create(const long P, const long T);

  /**
   * Map structure of existing NetCDF file.
   */
  void map(const long P = -1, const long T = -1);

  /**
   * Write a given vector in the given variable of the given dimension
   */
  template<class V1>
  void writeVector(NcVar* var, NcDim* dim, const V1 vector);

  /**
   * Model.
   */
  const Model& m;

  /**
   * Time dimension.
   */
  NcDim* nrDim;

  /**
   * Model dimensions.
   */
  std::vector<NcDim*> nDims;

  /**
   * P-dimension (trajectories).
   */
  NcDim* npDim;

  /**
   * Time variable.
   */
  NcVar* tVar;

  /**
   * Node variables, indexed by type.
   */
  std::vector<std::vector<NcVarBuffer<real>*> > vars;

  /**
   * Log-weights variable.
   */
  NcVar* lwVar;

  /**
   * Evidences variable.
   */
  NcVar* evidenceVar;

  /**
   * Effective sample size variable.
   */
  NcVar* essVar;

  /**
   * Acceptance rates variable.
   */
  NcVar* acceptanceRateVar;

  /**
   * Number of x-particles (for monitoring AdaptiveNParticlFilter)
   */
  NcVar* numberxVar;
};
}

#include "../math/view.hpp"
#include "../math/temp_vector.hpp"
#include "../math/temp_matrix.hpp"

inline int bi::SMC2NetCDFBuffer::size1() const {
  return npDim->size();
}

inline int bi::SMC2NetCDFBuffer::size2() const {
  return nrDim->size();
}

template<class V1>
void bi::SMC2NetCDFBuffer::readSample(const int p, V1 theta) {
  readSingle(P_VAR, p, 0, theta);
}

template<class V1>
void bi::SMC2NetCDFBuffer::writeSample(const int p, const V1 theta) {
  writeSingle(P_VAR, p, 0, theta);
}

template<class V1>
void bi::SMC2NetCDFBuffer::readLogWeights(V1 lws) {
  /* pre-conditions */
  assert (lws.size() == npDim->size());

  typedef typename V1::value_type temp_value_type;
  typedef typename temp_host_vector<temp_value_type>::type temp_vector_type;

  BI_UNUSED NcBool ret;
  ret = lwVar->set_cur(0l);
  BI_ASSERT(ret, "Indexing out of bounds reading variable logweight");

  if (V1::on_device || lws.inc() != 1) {
    temp_vector_type lws1(lws.size());
    ret = lwVar->get(lws1.buf(), npDim->size());
    BI_ASSERT(ret, "Inconvertible type reading variable logweight");
    lws = lws1;
  } else {
    ret = lwVar->get(lws.buf(), npDim->size());
    BI_ASSERT(ret, "Inconvertible type reading variable logweight");
  }
}

template<class V1>
void bi::SMC2NetCDFBuffer::writeLogWeights(const V1 lws) {
  /* pre-conditions */
  assert (lws.size() == npDim->size());

  typedef typename V1::value_type temp_value_type;
  typedef typename temp_host_vector<temp_value_type>::type temp_vector_type;

  BI_UNUSED NcBool ret;
  ret = lwVar->set_cur(0l);
  BI_ASSERT(ret, "Indexing out of bounds writing variable logweight");

  if (V1::on_device || lws.inc() != 1) {
    temp_vector_type lws1(lws.size());
    lws1 = lws;
    synchronize(V1::on_device);
    ret = lwVar->put(lws1.buf(), npDim->size());
  } else {
    ret = lwVar->put(lws.buf(), npDim->size());
  }
  BI_ASSERT(ret, "Inconvertible type writing variable logweight");
}

template<class V1>
void bi::SMC2NetCDFBuffer::writeLogWeights(const int timeindex, const V1 lws) {
  /* pre-conditions */
  assert (lws.size() == npDim->size());

  typedef typename V1::value_type temp_value_type;
  typedef typename temp_host_vector<temp_value_type>::type temp_vector_type;

  BI_UNUSED NcBool ret;

  if (V1::on_device || lws.inc() != 1) {
    temp_vector_type lws1(lws.size());
    lws1 = lws;
    synchronize(V1::on_device);
    ret = lwVar->put_rec(lws1.buf(), timeindex);
  } else {
    ret = lwVar->put_rec(lws.buf(), timeindex);
  }
  BI_ASSERT(ret, "Inconvertible type writing variable logweight");
}

template<class V1>
void bi::SMC2NetCDFBuffer::writeNumberX(const int timeindex, const V1 numberx) {
  /* pre-conditions */
  assert (numberx.size() == npDim->size());

  typedef typename V1::value_type temp_value_type;
  typedef typename temp_host_vector<temp_value_type>::type temp_vector_type;

  BI_UNUSED NcBool ret;

  if (V1::on_device || numberx.inc() != 1) {
    temp_vector_type numberx1(numberx.size());
    numberx1 = numberx;
    synchronize(V1::on_device);
    ret = numberxVar->put_rec(numberx1.buf(), timeindex);
  } else {
    ret = numberxVar->put_rec(numberx.buf(), timeindex);
  }
  BI_ASSERT(ret, "Inconvertible type writing variable numberx");
}

template<class V1>
void bi::SMC2NetCDFBuffer::writeVector(NcVar* var, NcDim* dim, const V1 vector){
  typedef typename V1::value_type temp_value_type;
  typedef typename temp_host_vector<temp_value_type>::type temp_vector_type;
  BI_UNUSED NcBool ret;
  ret = var->set_cur(0l);
  BI_ASSERT(ret, "Indexing out of bounds writing vector");
  if (V1::on_device || vector.inc() != 1) {
    temp_vector_type vector1(vector.size());
    vector1 = vector;
    synchronize(V1::on_device);
    ret = var->put(vector1.buf(), dim->size());
  } else {
    ret = var->put(vector.buf(), dim->size());
  }
  BI_ASSERT(ret, "Inconvertible type writing vector");
}

template<class M1>
void bi::SMC2NetCDFBuffer::readState(const VarType type, const int t,
    M1 X) {
  /* pre-conditions */
  assert (X.size1() == npDim->size());

  typedef typename M1::value_type temp_value_type;
  typedef typename temp_host_matrix<temp_value_type>::type temp_matrix_type;

  Var* var;
  host_vector<long> offsets, counts;
  int start, size, id, i, j;
  BI_UNUSED NcBool ret;

  for (id = 0; id < m.getNumVars(type); ++id) {
    var = m.getVar(type, id);
    start = m.getVarStart(type, id);
    size = var->getSize();

    if (var->getIO()) {
      BOOST_AUTO(ncVar, vars[type][id]);
      BI_ERROR (ncVar != NULL, "Variable " << var->getName() <<
          " does not exist in file");

      j = 0;
      offsets.resize(ncVar->num_dims(), false);
      counts.resize(ncVar->num_dims(), false);

      if (ncVar->get_dim(j) == nrDim) {
        offsets[j] = t;
        counts[j] = 1;
        ++j;
      }
      for (i = 0; i < var->getNumDims(); ++i, ++j) {
        offsets[j] = 0;
        counts[j] = ncVar->get_dim(j)->size();
      }
      offsets[j] = 0;
      counts[j] = X.size1();

      ret = ncVar->get_var()->set_cur(offsets.buf());
      BI_ASSERT(ret, "Indexing out of bounds reading " << ncVar->name());

      if (M1::on_device || X.lead() != X.size1()) {
        temp_matrix_type X1(X.size1(), size);
        ret = ncVar->get_var()->get(X1.buf(), counts.buf());
        BI_ASSERT(ret, "Inconvertible type reading " << ncVar->name());
        columns(X, start, size) = X1;
      } else {
        ret = ncVar->get_var()->get(columns(X, start, size).buf(),
            counts.buf());
        BI_ASSERT(ret, "Inconvertible type reading " << ncVar->name());
      }
    }
  }
}

template<class M1>
void bi::SMC2NetCDFBuffer::writeState(const VarType type, const int t,
    const M1 X, const int p) {
  /* pre-conditions */
  assert (X.size1() <= npDim->size());

  typedef typename M1::value_type temp_value_type;
  typedef typename temp_host_matrix<temp_value_type>::type temp_matrix_type;

  Var* var;
  host_vector<long> offsets, counts;
  int start, size, id, i, j;
  BI_UNUSED NcBool ret;

  for (id = 0; id < m.getNumVars(type); ++id) {
    var = m.getVar(type, id);
    start = m.getVarStart(type, id);
    size = var->getSize();

    if (var->getIO()) {
      BOOST_AUTO(ncVar, vars[type][id]);
      BI_ERROR (ncVar != NULL, "Variable " << var->getName() <<
          " does not exist in file");

      j = 0;
      offsets.resize(ncVar->num_dims(), false);
      counts.resize(ncVar->num_dims(), false);

      if (ncVar->get_dim(j) == nrDim) {
        offsets[j] = t;
        counts[j] = 1;
        ++j;
      }
      for (i = 0; i < var->getNumDims(); ++i, ++j) {
        offsets[j] = 0;
        counts[j] = ncVar->get_dim(j)->size();
      }
      offsets[j] = p;
      counts[j] = X.size1();

      ret = ncVar->get_var()->set_cur(offsets.buf());
      BI_ASSERT(ret, "Indexing out of bounds writing " << ncVar->name());

      if (M1::on_device || X.lead() != X.size1()) {
        temp_matrix_type X1(X.size1(), size);
        X1 = columns(X, start, size);
        synchronize(M1::on_device);
        ret = ncVar->get_var()->put(X1.buf(), counts.buf());
      } else {
        ret = ncVar->get_var()->put(columns(X, start, size).buf(),
            counts.buf());
      }
      BI_ASSERT(ret, "Inconvertible type reading " << ncVar->name());
    }
  }
}

#endif
