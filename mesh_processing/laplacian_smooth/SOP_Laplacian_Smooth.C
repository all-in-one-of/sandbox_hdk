
#include "SOP_Laplacian_Smooth.h"

#include <SOP/SOP_Guide.h>
#include <GU/GU_Detail.h>
#include <GA/GA_Iterator.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Matrix3.h>
#include <UT/UT_Matrix4.h>
#include <UT/UT_Vector3.h>
#include <SYS/SYS_Math.h>
#include <stddef.h>

#include <GU/GU_PrimPoly.h>
#include <GU/GU_PrimTube.h>
#include <GU/GU_PrimMetaBall.h>
#include <GU/GU_PrimNURBCurve.h>
#include <GU/GU_PrimPacked.h>
#include <GU/GU_PackedGeometry.h>

#include <GU/GU_Smooth.h>

#include "converters.hpp"

#include <igl/cotmatrix.h>
#include <igl/barycenter.h>
#include <igl/invert_diag.h>
#include <igl/massmatrix.h>
#include <igl/principal_curvature.h>
#include <igl/grad.h>
#include <igl/parula.h>

#include <Eigen/Dense>
#include <Eigen/Sparse>

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "laplacian_smooth",
        "Laplacian Smooth",
        SOP_Laplacian_Smooth::myConstructor,
		SOP_Laplacian_Smooth::myTemplateList,
        1,
        1,
        NULL));
}

static PRM_Name names[] = {
    PRM_Name("curvature",          "Add Principal Curvature"),
    PRM_Name("false_curve_colors", "Add False Curve Colors"),
    PRM_Name("grad_attrib",        "Add Gradient of Attribute (scalar)"),
    PRM_Name("grad_attrib_name",   "Scalar Attribute Name"),
    PRM_Name("laplacian",          "Laplacian (Smoothing)"),
    PRM_Name("eigenvectors",       "Eigen Decomposition (Disabled)"),
};

static PRM_Range  laplaceRange(PRM_RANGE_PRM, 0, PRM_RANGE_PRM, 10);

PRM_Template
SOP_Laplacian_Smooth::myTemplateList[] = {
    PRM_Template(PRM_TOGGLE, 1, &names[0], PRMzeroDefaults),
    PRM_Template(PRM_TOGGLE, 1, &names[1], PRMzeroDefaults),
    PRM_Template(PRM_TOGGLE, 1, &names[2], PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &names[3], 0),
    PRM_Template(PRM_FLT_J,  1, &names[4], PRMzeroDefaults, 0),
    PRM_Template(PRM_TOGGLE, 1, &names[5], PRMzeroDefaults),
    PRM_Template(),
};

namespace SOP_IGL
{
int compute_laplacian(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F,
                      const Eigen::SparseMatrix<double> &L, Eigen::MatrixXd &U)
{
    Eigen::SparseMatrix<double> M;
    igl::massmatrix(U,F,igl::MASSMATRIX_TYPE_BARYCENTRIC, M);

    // Solve (M-delta*L) U = M*U
    const auto & S = (M - 0.001*L);
    Eigen::SimplicialLLT<Eigen::SparseMatrix<double > > solver(S);

    if(solver.info() != Eigen::Success)
      return solver.info();
    U = solver.solve(M*U).eval();
    // Compute centroid and subtract (also important for numerics)
    Eigen::VectorXd dblA;
    igl::doublearea(U,F,dblA);
    double area = 0.5*dblA.sum();
    Eigen::MatrixXd BC;
    igl::barycenter(U,F,BC);
    Eigen::RowVector3d centroid(0,0,0);
    for(int i = 0;i<BC.rows();i++)
    {
      centroid += 0.5*dblA(i)/area*BC.row(i);
    }
    U.rowwise() -= centroid;
//    // Normalize to unit surface area (important for numerics)
//    U.array() /= sqrt(area);
    return Eigen::Success;
}
} // end of SOP_IGL namespce

OP_Node *
SOP_Laplacian_Smooth::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_Laplacian_Smooth(net, name, op);
}

SOP_Laplacian_Smooth::SOP_Laplacian_Smooth(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{
}

SOP_Laplacian_Smooth::~SOP_Laplacian_Smooth()
{
}

OP_ERROR
SOP_Laplacian_Smooth::cookMySop(OP_Context &context)
{

    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    fpreal t = context.getTime();
    duplicateSource(0, context);

    // Copy to eigen.
    gdp->convex(); // only triangles for now.
    uint numPoints = gdp->getNumPoints();
    uint numPrims  = gdp->getNumPrimitives();
    Eigen::MatrixXd V(numPoints, 3); // points
    Eigen::MatrixXi F(numPrims, 3); // faces

    SOP_IGL::detail_to_eigen(*gdp, V, F);

    /*    Laplacian smoothing   */
    const float laplacian = LAPLACIAN(t);
    std::cout << "+++++++++++ laplacian : " << laplacian << std::endl;

    if (laplacian != 0)
    {
        int laplacian_iterations = (int)ceil(laplacian)+1;
        std::cout << "+++++++++++ laplacian_iterations : " << laplacian_iterations << std::endl;
        float laplacian_ratio    = laplacian - floorf(laplacian);
        laplacian_ratio = laplacian_ratio != 0.f ? laplacian_ratio : 1.f;
        std::cout << "+++++++++++ laplacian_ratio : " << laplacian_ratio << std::endl;

        // Start the interrupt server
        UT_AutoInterrupt boss("Laplacian smoothing...");
        Eigen::SparseMatrix<double> L;
        // Compute Laplace-Beltrami operator: #V by #V
        igl::cotmatrix(V,F,L);
        // Smoothing:
        Eigen::MatrixXd U; U = V;
        Eigen::MatrixXd T;
        T = Eigen::MatrixXd::Zero(V.rows(), V.cols());

        while(laplacian_iterations)
        {
            // User interaption/
            if (boss.wasInterrupted())
                return error();

            if (SOP_IGL::compute_laplacian(V, F, L, U) != Eigen::Success) {
                addWarning(SOP_MESSAGE, "Can't compute laplacian with current geometry.");
                return error();
            }

            laplacian_iterations--;

             if (laplacian_iterations > 0)
             {
                 T += (U - T);
                 std::cout << "++++++++++++ IF" << std::endl;
             }
             else
             {
                 T += (U - T) * laplacian_ratio;
                 std::cout << "++++++++++++ ELSE" << std::endl;
             }

//            T = U;
        }

        // Copy back to Houdini:
        GA_Offset ptoff;
        GA_FOR_ALL_PTOFF(gdp, ptoff)
        {
            const GA_Index ptidx = gdp->pointIndex(ptoff);
            if ((uint)ptidx < T.rows())
            {
                const UT_Vector3 pos(T((uint)ptidx, 0),
                                     T((uint)ptidx, 1),
                                     T((uint)ptidx, 2));
                gdp->setPos3(ptoff, pos);
            }
        }
    }

    gdp->getP()->bumpDataId();
    return error();
}

const char *
SOP_Laplacian_Smooth::inputLabel(unsigned) const
{
    return "Geometry to Analyze";
}
