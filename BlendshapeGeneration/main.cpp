#include "common.h"

#include "blendshapegeneration.h"
#include <QtWidgets/QApplication>

#include "testcases.h"

#include "densematrix.h"
#include "densevector.h"
#include "basicmesh.h"
#include "meshdeformer.h"
#include "meshtransferer.h"
#include "cereswrapper.h"

#include "Geometry/matrix.hpp"

vector<int> loadLandmarks(const string &filename) {
  const int npts = 73;
  vector<int> v(npts);
  ifstream fin(filename);
  for (int i = 0; i < npts; ++i) {
    fin >> v[i];
    cout << v[i] << endl;
  }
  return v;
}

void laplacianDeformation() {
#ifdef __APPLE__
  const string datapath = "/Users/phg/Data/FaceWarehouse_Data_0/";
#endif

#ifdef __linux__
  const string datapath = "/home/phg/Data/FaceWarehouse_Data_0/";
#endif

  BasicMesh m;
  m.load(datapath + "Tester_1/Blendshape/shape_0.obj");

  MeshDeformer deformer;
  deformer.setSource(m);

  vector<int> landmarks = loadLandmarks(datapath+"landmarks_74_new.txt");
  deformer.setLandmarks(landmarks);

  int objidx = 2;
  BasicMesh T;
  T.load(datapath + "Tester_1/TrainingPose/pose_" + to_string(objidx) + ".obj");

  PointCloud lm_points;
  lm_points.points = T.verts.row(landmarks);
  BasicMesh D = deformer.deformWithMesh(T, lm_points);

  D.write("deformed" + to_string(objidx) + ".obj");
}

Array1D<double> estimateWeights(const BasicMesh &S,
                                const BasicMesh &B0,
                                vector<Array2D<double>> &dB,
                                const Array1D<double> &w0,  // init value
                                const Array1D<double> &wp,  // prior
                                double w_prior,
                                int itmax) {
  Array1D<double> w;

  return w;
}

PhGUtils::Matrix3x3d triangleGradient(const BasicMesh &m, int fidx) {
  PhGUtils::Matrix3x3d G;



  return G;
}

vector<Array2D<double>> refineBlendShapes(const vector<BasicMesh> &S,
                                          const vector<Array2D<double>> &Sgrad,
                                          const vector<BasicMesh> &B,
                                          const vector<Array1D<double>> &alpha,
                                          double beta, double gamma,
                                          const Array2D<double> prior,
                                          const Array2D<double> w_prior,
                                          const vector<int> stationary_indices) {
  vector<Array2D<double>> B_new;

  return B_new;
}

void blendShapeGeneration() {
  // load the meshes
  string datapath = "/home/phg/Data/FaceWarehouse_Data_0/";
  string A_path = datapath + "Tester_1/Blendshape/";
  string B_path = datapath + "Tester_106/Blendshape/";
  string S_path = datapath + "Tester_106/TrainingPose/";

  const int nshapes = 46; // 46 in total (1..46)
  const int nposes = 19; // 20 in total (0..19)

  vector<BasicMesh> A(nshapes+1);
  vector<BasicMesh> B(nshapes+1);
  vector<BasicMesh> B_ref(nshapes+1);
  vector<BasicMesh> S0(nposes);             // ground truth training meshes

  // load the landmarks
  vector<int> landmarks = loadLandmarks(datapath+"landmarks_74_new.txt");

  // load the template blendshapes and ground truth blendshapes
  for(int i=0;i<=nshapes;++i) {
    A[i].load(A_path + "shape_" + to_string(i) + ".obj");
    B_ref[i].load(B_path + "shape_" + to_string(i) + ".obj");
  }

  B[0] = B_ref[0];

  // reference shapes for convenience
  auto& A0 = A[0];
  auto& B0 = B[0];

  // load the training poses
  for(int i=0;i<nposes;++i){
    S0[i].load(S_path + "pose_" + to_string(i) + ".obj");
  }

  const bool synthesizeTrainingPoses = false;
  if( synthesizeTrainingPoses ) {
    // estimate the blendshape weights from the input training poses, then use
    // the estimated weights to generate a new set of training poses

    vector<Array1D<double>> alpha_ref(nposes);
  }

  // create point clouds from S0
  vector<PointCloud> P(nposes);
  for(int i=0;i<nposes;++i) {
    P[i] = S0[i].samplePoints(4, -0.10);
  }

  vector<BasicMesh> S(nposes);  // meshes reconstructed from point clouds
  // use Laplacian deformation to reconstruct a set of meshes from the sampled
  // point clouds
  for(int i=0;i<nposes;++i) {
    MeshDeformer deformer;
    deformer.setSource(S0[i]);
    deformer.setLandmarks(landmarks);
    PointCloud lm_points;
    lm_points.points = S0[i].verts.row(landmarks);
    S[i] = deformer.deformWithPoints(P[i], lm_points);
  }
  auto S_init = S;

  // find the stationary set of verteces
  vector<int> stationary_indices = B0.filterVertices([=](double *v) {
    return v[2] <= -0.45;
  });

  // use deformation transfer to create an initial set of blendshapes
  MeshTransferer transferer;
  transferer.setSource(A0); // set source and compute deformation gradient
  transferer.setTarget(B0); // set target and compute deformation gradient

  for(int i=1;i<nshapes;++i) {
    B[i] = transferer.transfer(A[i]);
  }
  auto B_init = B;

  // compute deformation gradient prior from the template mesh
  int nfaces = A0.faces.nrow;

  Array2D<double> MB0 = Array2D<double>::zeros(nfaces, 9);
  Array2D<double> MA0 = Array2D<double>::zeros(nfaces, 9);

  for(int j=0;j<nfaces;++j) {
    auto MB0j = triangleGradient(B0, j);
    auto MB0j_ptr = MB0.rowptr(j);
    for(int k=0;k<9;++k) MB0j_ptr[k] = MB0j(k);
    auto MA0j = triangleGradient(A0, j);
    auto MA0j_ptr = MA0.rowptr(j);
    for(int k=0;k<9;++k) MA0j_ptr[k] = MA0j(k);
  }

  double kappa = 0.1;
  Array2D<double> prior = Array2D<double>::zeros(nshapes, 9*nfaces);
  Array2D<double> w_prior = Array2D<double>::zeros(nshapes, nfaces);
  for(int i=0;i<nshapes;++i) {
    // prior for shape i
    auto &Ai = A[i+1];
    Array2D<double> Pi = Array2D<double>::zeros(nfaces, 9);
    auto w_prior_i = w_prior.rowptr(i);
    for(int j=0;j<nfaces;++j) {
      auto MAij = triangleGradient(Ai, j);
      auto MA0j_ptr = MA0.rowptr(j);
      // create a 3x3 matrix from MA0j_ptr
      auto MA0j = PhGUtils::Matrix3x3d(MA0j_ptr);
      auto GA0Ai = MAij * (MA0j.inv());
      auto MB0j_ptr = MB0.rowptr(j);
      auto MB0j = PhGUtils::Matrix3x3d(MB0j_ptr);
      auto Pij = GA0Ai * MB0j - MB0j;
      double MAij_norm = (MAij-MA0j).norm();
      w_prior_i[j] = (1+MAij_norm)/(kappa+MAij_norm);

      auto Pij_ptr = Pi.rowptr(j);
      for(int k=0;k<9;++k) Pij_ptr[k] = Pij(k);
    }
    auto prior_i = prior.rowptr(i);
    // prior(i,:) = Pi;
    memcpy(prior_i, Pi.data.get(), sizeof(double)*nfaces*9);
  }

  // compute the delta shapes
  vector<Array2D<double>> dB(nshapes);
  for(int i=0;i<nshapes;++i) {
    dB[i] = B[i+1].verts - B0.verts;
  }

  // estimate initial set of blendshape weights
  vector<Array1D<double>> alpha(nposes);
  for(int i=0;i<nposes;++i) {
    alpha[i] = estimateWeights(S[i], B0, dB,
                               Array1D<double>::zeros(nshapes),
                               Array1D<double>::zeros(nshapes),
                               0.0, 5);
  }
  auto alpha_init = alpha;

  cout << "initialization done." << endl;

  // reset the parameters
  B = B_init;
  S = S_init;
  alpha = alpha_init;

  // main blendshape refinement loop

  // compute deformation gradients for S
  vector<Array2D<double>> Sgrad(nposes);
  for(int i=0;i<nposes;++i) {
    Sgrad[i] = Array2D<double>::zeros(nfaces, 9);
    for(int j=0;j<nfaces;++j) {
      // assign the reshaped gradient to the j-th row of Sgrad[i]
      auto Sij = triangleGradient(S[i], j);
      auto Sij_ptr = Sgrad[i].rowptr(j);
      for(int k=0;k<9;++k) Sij_ptr[k] = Sij(k);
    }
  }

  // refine blendshapes as well as blendshape weights
  bool converged = false;
  double ALPHA_THRES = 1e-6;
  double B_THRES = 1e-6;
  double beta_max = 0.5, beta_min = 0.1;
  double gamma_max = 0.01, gamma_min = 0.01;
  double eta_max = 10.0, eta_min = 1.0;
  int iters = 0;
  const int maxIters = 10;
  DenseMatrix B_error = DenseMatrix::zeros(maxIters, nshapes);
  DenseMatrix S_error = DenseMatrix::zeros(maxIters, nposes);
  while( !converged && iters < maxIters ) {
      cout << "iteration " << iters << " ..." << endl;
      converged = true;
      ++iters;

      // refine blendshapes
      double beta = sqrt(iters/maxIters) * (beta_min - beta_max) + beta_max;
      double gamma = gamma_max + iters/maxIters*(gamma_min-gamma_max);
      double eta = eta_max + iters/maxIters*(eta_min-eta_max);

      // B_new is a set of point clouds
      auto B_new = refineBlendShapes(S, Sgrad, B, alpha, beta, gamma, prior, w_prior, stationary_indices);

      //B_norm = zeros(1, nshapes);
      for(int i=0;i<nshapes;++i) {
          //B_norm(i) = norm(B[i+1].vertices-B_new[i], 2);
          B[i+1].verts = B_new[i];
          //B_error(iters, i) = sqrt(max(sum((B{i+1}.vertices-B_ref{i+1}.vertices).^2, 2)));
          //B{i+1} = alignMesh(B{i+1}, B{1}, stationary_indices);
      }
      //fprintf('max(B_error) = %.6f\n', max(B_error(iters, :)));
      //converged = converged & (max(B_norm) < B_THRES);

      // update delta shapes
      for(int i=0;i<nshapes;++i) {
        dB[i] = B[i+1].verts - B0.verts;
      }

      // update weights
      vector<Array1D<double>> alpha_new(nposes);
      for(int i=0;i<nposes;++i) {
        alpha_new[i] = estimateWeights(S[i], B0, dB, alpha[i], alpha_ref[i], eta, 2);
      }

      //alpha_norm = norm(cell2mat(alpha) - cell2mat(alpha_new));
      //disp(alpha_norm);
      alpha = alpha_new;
      //converged = converged & (alpha_norm < ALPHA_THRES);

      for(int i=0;i<nposes;++i) {
        // make a copy of B0
        auto Ti = B0;
          for(int j=0;j<nshapes;++j) {
              Ti.verts += alpha[i](j) * dB[j];
          }
          //Ti = alignMesh(Ti, S0{i});
          //S_error(iters, i) = sqrt(max(sum((Ti.vertices-S0{i}.vertices).^2, 2)));

          // update the reconstructed mesh
          S[i] = Ti;
      }
      //fprintf('Emax = %.6f\tEmean = %.6f\n', max(S_error(iters,:)), mean(S_error(iters,:)));


      // The reconstructed mesh are updated using the new set of blendshape
      // weights, need to use laplacian deformation to refine them again
      for(int i=0;i<nposes;++i) {
        MeshDeformer deformer;
        deformer.setSource(S[i]);
        deformer.setLandmarks(landmarks);
        PointCloud lm_points;
        lm_points.points = S0[i].verts.row(landmarks);
        S[i] = deformer.deformWithPoints(P[i], lm_points);
      }

      // compute deformation gradients for S
      for(int i=0;i<nposes;++i) {
        Sgrad[i] = Array2D<double>::zeros(nfaces, 9);
        for(int j=0;j<nfaces;++j) {
          // assign the reshaped gradient to the j-th row of Sgrad[i]
          auto Sij = triangleGradient(S[i], j);
          auto Sij_ptr = Sgrad[i].rowptr(j);
          for(int k=0;k<9;++k) Sij_ptr[k] = Sij(k);
        }
      }
  }
}

int main(int argc, char *argv[])
{
#if 0
  QApplication a(argc, argv);
  BlendshapeGeneration w;
  w.show();
  return a.exec();
#else
  google::InitGoogleLogging(argv[0]);

  global::initialize();

#if RUN_TESTS
  TestCases::testMatrix();
  TestCases::testSaprseMatrix();
  TestCases::testCeres();

  return 0;
#else

  //laplacianDeformation();
  blendShapeGeneration();

  global::finalize();
  return 0;
#endif

#endif
}
