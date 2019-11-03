#ifndef FUNCTIONMESH_H
#define FUNCTIONMESH_H

#include <vector>

#include "term.h"
#include "variable.h"
#include "shared/Vectors.h"

class FunctionMesh
{
public:
    FunctionMesh(Term* f_of_xyz);

    virtual ~FunctionMesh();

    class FunctionMeshTree;

private:
    Term* f_of_xyz;
    Term* dfdx;
    Term* dfdy;
    Term* dfdz;
    Term* dfdw;

    const int default_depth = 6;
public:

	std::vector<Vector4> vertices;
	std::vector<Vector4> gradients;

	std::vector<Vector4> debug_vertices;
	std::vector<Vector3> debug_colors;

    class FunctionMeshTree
    {
    public:
        FunctionMeshTree(FunctionMesh* mesh, Variable::var_type largest_var);
        virtual ~FunctionMeshTree() {}

        // Traverse the tree, building up a list of vertices in function coordinates.
        // Those vertices are appended to *out.
        virtual void GetMeshData(std::vector<Vector4>* vertices_out, std::vector<Vector4>* gradient_out, std::vector<Vector4>* debug_vertices_out, std::vector<Vector3>* debug_colors_out) = 0;

        bool IsEmpty() { return is_empty; }
    protected:

        int depth;
        bool is_leaf;
        bool is_empty;

        FunctionMesh* mesh;
        Variable::var_type largest_var;

        // Vertex data is held in a tree; The top layer is an initial_branch_factor^3 grid,
        // then subsequent layers of the tree branch by branch_factor in each dimension.
        const int initial_branch_factor = 2;
        const int branch_factor = 2;
    private:


    };


    class FunctionMeshTreeLeaf : public FunctionMeshTree
    {
    public:
        FunctionMeshTreeLeaf(FunctionMesh* mesh, int depth, Variable::var_type largest_var, Vector3 min, Vector3 max);
        virtual ~FunctionMeshTreeLeaf() {}

        virtual void GetMeshData(std::vector<Vector4>* vertices_out, std::vector<Vector4> *gradient_out, std::vector<Vector4>* debug_vertices_out, std::vector<Vector3>* debug_colors_out);
    private:
        // Flattened two-dimensional array of positions of vertices
        std::vector<Vector4> vertex_data;
        std::vector<Vector4> gradient_data;

		std::vector<Vector4> debug_vertices;
		std::vector<Vector3> debug_colors;
    };

    class FunctionMeshTreeNode : public FunctionMeshTree
    {
    public:
        FunctionMeshTreeNode(FunctionMesh* mesh, int depth, int depth_to_compute, Variable::var_type largest_var, Vector3 min, Vector3 max);

        virtual ~FunctionMeshTreeNode();

        virtual void GetMeshData(std::vector<Vector4>* vertices_out, std::vector<Vector4>* gradient_out, std::vector<Vector4>* debug_vertices_out, std::vector<Vector3>* debug_colors_out);

    private:
        // Flattened two-dimensional array of descendents. Null ptrs indicate no data at a position.
        std::vector<FunctionMeshTree*> descendents;
    };

};

#endif // FUNCTIONMESH_H
