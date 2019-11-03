#include "functionmesh.h"

#include <iostream>
#include <fstream>

#include <Windows.h>

struct asynch_gen_params
{
	FunctionMesh* fm;
	int depth;
	int max_depth;
	Variable::var_type var;
	Vector3 min;
	Vector3 max;
	FunctionMesh::FunctionMeshTreeNode** gen_tree;
};

// Asynchronous generation of mesh trees.
// gen_tree will be filled with a "new"ed tree, so will require a "delete."
DWORD WINAPI AsynchronousGenerateMeshTree(LPVOID vparams)
{
	struct asynch_gen_params* params = (struct asynch_gen_params*)vparams;

	*(params->gen_tree) = new FunctionMesh::FunctionMeshTreeNode(params->fm, params->depth, params->max_depth, params->var, params->min, params->max);

	return 0;
}

FunctionMesh::FunctionMesh(Term *f_of_xyz)
{
	this->f_of_xyz = f_of_xyz->simplify();

    Term* dfdx_temp = f_of_xyz->derivative('x');
    Term* dfdy_temp = f_of_xyz->derivative('y');
    Term* dfdz_temp = f_of_xyz->derivative('z');
    Term* dfdw_temp = f_of_xyz->derivative('w');

    dfdx = dfdx_temp->simplify();
    dfdy = dfdy_temp->simplify();
    dfdz = dfdz_temp->simplify();
    dfdw = dfdw_temp->simplify();

    dfdx->print(); std::cout << std::endl;
    dfdy->print(); std::cout << std::endl;
    dfdz->print(); std::cout << std::endl;
    dfdw->print(); std::cout << std::endl;

    delete dfdx_temp;
    delete dfdy_temp;
    delete dfdz_temp;
    delete dfdw_temp;

    Vector3 min = Vector3(-1,-1,-1);
    Vector3 max = Vector3(1,1,1);

	FunctionMeshTreeNode* mesh_tree_x = 0;
	FunctionMeshTreeNode* mesh_tree_y = 0;
	FunctionMeshTreeNode* mesh_tree_z = 0;
	FunctionMeshTreeNode* mesh_tree_w = 0;

	struct asynch_gen_params params_x =
	{
		this,
		1,
		default_depth,
		Variable::VAR_X,
		min,
		max,
		&mesh_tree_x
	};

	struct asynch_gen_params params_y =
	{
		this,
		1,
		default_depth,
		Variable::VAR_Y,
		min,
		max,
		&mesh_tree_y
	};

	struct asynch_gen_params params_z =
	{
		this,
		1,
		default_depth,
		Variable::VAR_Z,
		min,
		max,
		&mesh_tree_z
	};

	struct asynch_gen_params params_w =
	{
		this,
		1,
		default_depth,
		Variable::VAR_W,
		min,
		max,
		&mesh_tree_w
	};

	DWORD threadID;
	HANDLE asynch_threads[4];

	asynch_threads[0] = CreateThread(NULL, 0, AsynchronousGenerateMeshTree, &params_x, 0, &threadID);
	asynch_threads[1] = CreateThread(NULL, 0, AsynchronousGenerateMeshTree, &params_y, 0, &threadID);
	asynch_threads[2] = CreateThread(NULL, 0, AsynchronousGenerateMeshTree, &params_z, 0, &threadID);
	asynch_threads[3] = CreateThread(NULL, 0, AsynchronousGenerateMeshTree, &params_w, 0, &threadID);

	WaitForMultipleObjects(4, asynch_threads, TRUE, INFINITE);

	mesh_tree_x->GetMeshData(&vertices, &gradients, &debug_vertices, &debug_colors);
	mesh_tree_y->GetMeshData(&vertices, &gradients, &debug_vertices, &debug_colors);
	mesh_tree_z->GetMeshData(&vertices, &gradients, &debug_vertices, &debug_colors);
	mesh_tree_w->GetMeshData(&vertices, &gradients, &debug_vertices, &debug_colors);

	delete mesh_tree_x;
	delete mesh_tree_y;
	delete mesh_tree_z;
    delete mesh_tree_w;

    std::cout << "Function mesh constructed." << std::endl;
}

FunctionMesh::~FunctionMesh()
{
    delete f_of_xyz;
    delete dfdx;
    delete dfdy;
    delete dfdz;

    std::cout << "Function mesh deconstructed." << std::endl;
}

void FunctionMesh::FunctionMeshTreeLeaf::GetMeshData(std::vector<Vector4> *vertices_out, std::vector<Vector4>* gradient_out, std::vector<Vector4>* debug_vertices_out, std::vector<Vector3>* debug_colors_out)
{
    vertices_out->insert(vertices_out->end(), vertex_data.cbegin(), vertex_data.cend());

    gradient_out->insert(gradient_out->end(), gradient_data.cbegin(), gradient_data.cend());

	debug_vertices_out->insert(debug_vertices_out->end(), debug_vertices.cbegin(), debug_vertices.cend());
	debug_colors_out->insert(debug_colors_out->end(), debug_colors.cbegin(), debug_colors.cend());
}

void FunctionMesh::FunctionMeshTreeNode::GetMeshData(std::vector<Vector4> *vertices_out, std::vector<Vector4> *gradient_out, std::vector<Vector4>* debug_vertices_out, std::vector<Vector3>* debug_colors_out)
{
    for (int i = 0; i < descendents.size(); i++)
    {
        if (descendents[i] != 0)
        {
            descendents[i]->GetMeshData(vertices_out, gradient_out, debug_vertices_out, debug_colors_out);
        }
    }
}



FunctionMesh::FunctionMeshTreeNode::FunctionMeshTreeNode(FunctionMesh* mesh, int depth, int depth_to_compute, Variable::var_type largest_var, Vector3 min, Vector3 max)
  : FunctionMeshTree(mesh, largest_var)
{
    is_leaf = false;

    int res = ((depth == 1) ? initial_branch_factor : branch_factor);

    float step_length = (max.x - min.x)/res;

    // Decide whether to recurse in each cell.
    for (int i = 0; i < res; i++)
    {   for (int j = 0; j < res; j++)
        {   for (int k = 0; k < res; k++)
            {
                FunctionMeshTree* new_tree;
                if (depth + 1 == depth_to_compute)
                    new_tree = new FunctionMeshTreeLeaf(mesh, depth+1, largest_var,
                                                                   min + Vector3(step_length*i, step_length*j, step_length*k),
                                                                   min + Vector3(step_length*(i+1), step_length*(j+1), step_length*(k+1)));
                else
                    new_tree = new FunctionMeshTreeNode(mesh, depth+1, depth_to_compute, largest_var,
                                                                   min + Vector3(step_length*i, step_length*j, step_length*k),
                                                                   min + Vector3(step_length*(i+1), step_length*(j+1), step_length*(k+1)));

                if (new_tree->IsEmpty())
                    delete new_tree;
                else
                    descendents.push_back(new_tree);
            }
        }
    }

    is_empty = descendents.empty();

}

FunctionMesh::FunctionMeshTreeNode::~FunctionMeshTreeNode()
{
    for (int i = 0; i < descendents.size(); i++)
    {
        if (descendents[i] != 0)
            delete descendents[i];
    }
}

FunctionMesh::FunctionMeshTreeLeaf::FunctionMeshTreeLeaf(FunctionMesh* mesh, int depth, Variable::var_type largest_var, Vector3 min, Vector3 max)
    : FunctionMeshTree(mesh, largest_var)
{
    is_leaf = true;

    // min, max are in generating coordinates
    // e1, e2, e3, x1, x2, x3 are in function coordinates.
    //
    // This mostly has to do with making the transition between the two
    // so that the code for generating the mesh can be the same for each
    // of the four patches.

    Vector4 e1;
    Vector4 e2;
    Vector4 e3;
    Vector4 e4;

    switch (largest_var)
    {
    case Variable::VAR_X:
        e1 = Vector4(0,1,0,0);
        e2 = Vector4(0,0,1,0);
        e3 = Vector4(0,0,0,1);
        e4 = Vector4(1,0,0,0);
        break;
    case Variable::VAR_Y:
        e1 = Vector4(1,0,0,0);
        e2 = Vector4(0,0,1,0);
        e3 = Vector4(0,0,0,1);
        e4 = Vector4(0,1,0,0);
        break;
    case Variable::VAR_Z:
        e1 = Vector4(1,0,0,0);
        e2 = Vector4(0,1,0,0);
        e3 = Vector4(0,0,0,1);
        e4 = Vector4(0,0,1,0);
        break;
    case Variable::VAR_W:
        e1 = Vector4(1,0,0,0);
        e2 = Vector4(0,1,0,0);
        e3 = Vector4(0,0,1,0);
        e4 = Vector4(0,0,0,1);
        break;
    }

    int res = (depth == 0 ? initial_branch_factor : branch_factor);

    double step_length = (max.x - min.x)/res;
    Vector4 x1_step = step_length*e1;
    Vector4 x2_step = step_length*e2;
    Vector4 x3_step = step_length*e3;
    Vector4 function_coords_min = min.x*e1 + min.y*e2 + min.z*e3 + e4;

    // Pre-compute values on the grid we're responsible for...
    double* value_array = new double[(res+1)*(res+1)*(res+1)];

    for (int i = 0; i < res + 1; i++)
    {   for (int j = 0; j < res + 1; j++)
        {   for (int k = 0; k < res + 1; k++)
            {
                value_array[i*(res+1)*(res+1) + j*(res+1) + k] = mesh->f_of_xyz->eval(function_coords_min + x1_step*i + x2_step*j + x3_step*k);
            }
        }
    }

    // Decide whether to recurse in each cell.
    for (int i = 0; i < res; i++)
    {   for (int j = 0; j < res; j++)
        {   for (int k = 0; k < res; k++)
            {
                // v for value
                double v000 = value_array[(res+1)*(res+1)*i    + (res+1)*j    + k];
                double v100 = value_array[(res+1)*(res+1)*(i+1)+ (res+1)*j    + k];
                double v010 = value_array[(res+1)*(res+1)*i    + (res+1)*(j+1)+ k];
                double v110 = value_array[(res+1)*(res+1)*(i+1)+ (res+1)*(j+1)+ k];
                double v001 = value_array[(res+1)*(res+1)*i    + (res+1)*j    + k + 1];
                double v101 = value_array[(res+1)*(res+1)*(i+1)+ (res+1)*j    + k + 1];
                double v011 = value_array[(res+1)*(res+1)*i    + (res+1)*(j+1)+ k + 1];
                double v111 = value_array[(res+1)*(res+1)*(i+1)+ (res+1)*(j+1)+ k + 1];

                Vector4 pos = function_coords_min + i*x1_step + j*x2_step + k*x3_step;

                // p for positive
                char p000 = v000 >= 0;
                char p001 = v001 >= 0;
                char p010 = v010 >= 0;
                char p011 = v011 >= 0;
                char p100 = v100 >= 0;
                char p101 = v101 >= 0;
                char p110 = v110 >= 0;
                char p111 = v111 >= 0;

                // e for edge.
                char e000_001 = p000 ^ p001; char e000_010 = p000 ^ p010; char e000_100 = p000 ^ p100;
                char e011_010 = p011 ^ p010; char e011_001 = p011 ^ p001; char e011_111 = p011 ^ p111;
                char e101_100 = p101 ^ p100; char e101_111 = p101 ^ p111; char e101_001 = p101 ^ p001;
                char e110_111 = p110 ^ p111; char e110_100 = p110 ^ p100; char e110_010 = p110 ^ p010;

                // EI for edge index.
                const int EI000_001 = 0;
                const int EI000_010 = 1;
                const int EI000_100 = 2;
                const int EI011_010 = 3;
                const int EI011_001 = 4;
                const int EI011_111 = 5;
                const int EI101_100 = 6;
                const int EI101_111 = 7;
                const int EI101_001 = 8;
                const int EI110_111 = 9;
                const int EI110_100 = 10;
                const int EI110_010 = 11;

                const int EI001_000 = EI000_001;
                const int EI010_000 = EI000_010;
                const int EI100_000 = EI000_100;
                const int EI010_011 = EI011_010;
                const int EI001_011 = EI011_001;
                const int EI111_011 = EI011_111;
                const int EI100_101 = EI101_100;
                const int EI111_101 = EI101_111;
                const int EI001_101 = EI101_001;
                const int EI111_110 = EI110_111;
                const int EI100_110 = EI110_100;
                const int EI010_110 = EI110_010;

                // M for bitMask.
                const char M000 = 1;
                const char M001 = 1 << 1;
                const char M010 = 1 << 2;
                const char M011 = 1 << 3;
                const char M100 = 1 << 4;
                const char M101 = 1 << 5;
                const char M110 = 1 << 6;
                const char M111 = 1 << 7;

                // ev for edge vertices. Only active edges are filled; the rest is uninitialized.
                Vector4 ev[12];

                if (e000_001) ev[EI000_001] = pos + (-v000)/(v001 - v000)*x3_step;
                if (e000_010) ev[EI000_010] = pos + (-v000)/(v010 - v000)*x2_step;
                if (e000_100) ev[EI000_100] = pos + (-v000)/(v100 - v000)*x1_step;
                if (e011_010) ev[EI011_010] = pos + x2_step + (-v010)/(v011 - v010)*x3_step;
                if (e011_001) ev[EI011_001] = pos + (-v001)/(v011 - v001)*x2_step + x3_step;
                if (e011_111) ev[EI011_111] = pos + (-v011)/(v111 - v011)*x1_step + x2_step + x3_step;
                if (e101_100) ev[EI101_100] = pos + x1_step + (-v100)/(v101 - v100)*x3_step;
                if (e101_111) ev[EI101_111] = pos + x1_step + (-v101)/(v111 - v101)*x2_step + x3_step;
                if (e101_001) ev[EI101_001] = pos + (-v001)/(v101 - v001)*x1_step + x3_step;
                if (e110_111) ev[EI110_111] = pos + x1_step + x2_step + (-v110)/(v111 - v110)*x3_step;
                if (e110_100) ev[EI110_100] = pos + x1_step + (-v100)/(v110 - v100)*x2_step;
                if (e110_010) ev[EI110_010] = pos + (-v010)/(v110 - v010)*x1_step + x2_step;

                char flags = p000*M000 | p001*M001 | p010*M010 | p011*M011
                           | p100*M100 | p101*M101 | p110*M110 | p111*M111;
				if (p111)
					flags = ~flags; // Removes some redundant cases;

				// Debug cube stuff

				// l for location
				Vector4 l000 = function_coords_min + x1_step*i + x2_step*j + x3_step*k;
				Vector4 l001 = function_coords_min + x1_step*i + x2_step*j + x3_step*(k+1);
				Vector4 l010 = function_coords_min + x1_step*i + x2_step*(j+1) + x3_step*k;
				Vector4 l011 = function_coords_min + x1_step*i + x2_step*(j+1) + x3_step*(k+1);
				Vector4 l100 = function_coords_min + x1_step*(i+1) + x2_step*j + x3_step*k;
				Vector4 l101 = function_coords_min + x1_step*(i+1) + x2_step*j + x3_step*(k+1);
				Vector4 l110 = function_coords_min + x1_step*(i+1) + x2_step*(j+1) + x3_step*k;
				Vector4 l111 = function_coords_min + x1_step*(i+1) + x2_step*(j+1) + x3_step*(k+1);

				debug_vertices.push_back(l000);
				debug_colors.push_back(p000 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l100);
				debug_colors.push_back(p100 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l001);
				debug_colors.push_back(p001 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l101);
				debug_colors.push_back(p101 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l010);
				debug_colors.push_back(p010 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l110);
				debug_colors.push_back(p110 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l011);
				debug_colors.push_back(p011 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l111);
				debug_colors.push_back(p111 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l000);
				debug_colors.push_back(p000 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l010);
				debug_colors.push_back(p010 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l001);
				debug_colors.push_back(p001 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l011);
				debug_colors.push_back(p011 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l100);
				debug_colors.push_back(p100 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l110);
				debug_colors.push_back(p110 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l101);
				debug_colors.push_back(p101 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l111);
				debug_colors.push_back(p111 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l000);
				debug_colors.push_back(p000 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l001);
				debug_colors.push_back(p001 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l010);
				debug_colors.push_back(p010 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l011);
				debug_colors.push_back(p011 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l100);
				debug_colors.push_back(p100 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l101);
				debug_colors.push_back(p101 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));

				debug_vertices.push_back(l110);
				debug_colors.push_back(p110 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));
				debug_vertices.push_back(l111);
				debug_colors.push_back(p111 ? Vector3(0, 1, 0) : Vector3(1, 0, 0));


#define ADD_VERTEX(v) \
  { vertex_data.push_back(v); gradient_data.push_back(Vector4(mesh->dfdx->eval(v),mesh->dfdy->eval(v), mesh->dfdz->eval(v), mesh->dfdw->eval(v))); }

// This got tedious to type out after a while below.
#define ADD_TRIANGLE(a,b,c) \
  { ADD_VERTEX(ev[EI##a]); ADD_VERTEX(ev[EI##b]); ADD_VERTEX(ev[EI##c]) }


                switch (flags)
                {
                // Case 0

                case 0:
                    // Empty cube!
                    break;


                // Case 1: Corner.
                case M000:
                    ADD_VERTEX(ev[EI000_001]);
                    ADD_VERTEX(ev[EI000_010]);
                    ADD_VERTEX(ev[EI000_100]);
                    break;
                case M001:
                    ADD_VERTEX(ev[EI000_001]);
                    ADD_VERTEX(ev[EI011_001]);
                    ADD_VERTEX(ev[EI101_001]);
                    break;
                case M010:
                    ADD_VERTEX(ev[EI011_010]);
                    ADD_VERTEX(ev[EI000_010]);
                    ADD_VERTEX(ev[EI110_010]);
                    break;
                case M011:
                    ADD_VERTEX(ev[EI011_010]);
                    ADD_VERTEX(ev[EI011_001]);
                    ADD_VERTEX(ev[EI011_111]);
                    break;
                case M100:
                    ADD_VERTEX(ev[EI000_100]);
                    ADD_VERTEX(ev[EI110_100]);
                    ADD_VERTEX(ev[EI101_100]);
                    break;
                case M101:
                    ADD_VERTEX(ev[EI101_100]);
                    ADD_VERTEX(ev[EI101_111]);
                    ADD_VERTEX(ev[EI101_001]);
                    break;
                case M110:
                    ADD_VERTEX(ev[EI110_111]);
                    ADD_VERTEX(ev[EI110_100]);
                    ADD_VERTEX(ev[EI110_010]);
                    break;
                case ~M111:
                    ADD_VERTEX(ev[EI110_111]);
                    ADD_VERTEX(ev[EI101_111]);
                    ADD_VERTEX(ev[EI011_111]);
                    break;

                // Case 2: An edge.
                case M000 | M001:
                    ADD_VERTEX(ev[EI000_010]);
                    ADD_VERTEX(ev[EI000_100]);
                    ADD_VERTEX(ev[EI011_001]);

                    ADD_VERTEX(ev[EI011_001]);
                    ADD_VERTEX(ev[EI101_001]);
                    ADD_VERTEX(ev[EI000_100]);
                    break;

                case M000 | M010:
                    ADD_VERTEX(ev[EI000_001]);
                    ADD_VERTEX(ev[EI000_100]);
                    ADD_VERTEX(ev[EI011_010]);

                    ADD_VERTEX(ev[EI011_010]);
                    ADD_VERTEX(ev[EI110_010]);
                    ADD_VERTEX(ev[EI000_100]);
                    break;

                case M000 | M100:
                    ADD_VERTEX(ev[EI000_001]);
                    ADD_VERTEX(ev[EI000_010]);
                    ADD_VERTEX(ev[EI101_100]);

                    ADD_VERTEX(ev[EI101_100]);
                    ADD_VERTEX(ev[EI110_100]);
                    ADD_VERTEX(ev[EI000_010]);
                    break;

                case M011 | M010:
                    ADD_VERTEX(ev[EI011_001]);
                    ADD_VERTEX(ev[EI011_111]);
                    ADD_VERTEX(ev[EI000_010]);

                    ADD_VERTEX(ev[EI000_010]);
                    ADD_VERTEX(ev[EI110_010]);
                    ADD_VERTEX(ev[EI011_111]);
                    break;

                case M011 | M001:
                    ADD_VERTEX(ev[EI011_111]);
                    ADD_VERTEX(ev[EI011_010]);
                    ADD_VERTEX(ev[EI101_001]);

                    ADD_VERTEX(ev[EI101_001]);
                    ADD_VERTEX(ev[EI000_001]);
                    ADD_VERTEX(ev[EI011_010]);
                    break;

                case ~(M011 | M111):
                    ADD_VERTEX(ev[EI011_001]);
                    ADD_VERTEX(ev[EI011_010]);
                    ADD_VERTEX(ev[EI101_111]);

                    ADD_VERTEX(ev[EI101_111]);
                    ADD_VERTEX(ev[EI110_111]);
                    ADD_VERTEX(ev[EI011_010]);
                    break;

                case M101 | M100:
                    ADD_VERTEX(ev[EI101_111]);
                    ADD_VERTEX(ev[EI101_001]);
                    ADD_VERTEX(ev[EI110_100]);

                    ADD_VERTEX(ev[EI110_100]);
                    ADD_VERTEX(ev[EI000_100]);
                    ADD_VERTEX(ev[EI101_001]);
                    break;

                case ~(M101 | M111):
                    ADD_VERTEX(ev[EI101_001]);
                    ADD_VERTEX(ev[EI101_100]);
                    ADD_VERTEX(ev[EI011_111]);

                    ADD_VERTEX(ev[EI011_111]);
                    ADD_VERTEX(ev[EI110_111]);
                    ADD_VERTEX(ev[EI101_100]);
                    break;

                case M101 | M001:
                    ADD_VERTEX(ev[EI101_111]);
                    ADD_VERTEX(ev[EI101_100]);
                    ADD_VERTEX(ev[EI011_001]);

                    ADD_VERTEX(ev[EI011_001]);
                    ADD_VERTEX(ev[EI000_001]);
                    ADD_VERTEX(ev[EI101_100]);
                    break;

                case ~(M110 | M111):
                    ADD_VERTEX(ev[EI110_010]);
                    ADD_VERTEX(ev[EI110_100]);
                    ADD_VERTEX(ev[EI011_111]);

                    ADD_VERTEX(ev[EI011_111]);
                    ADD_VERTEX(ev[EI101_111]);
                    ADD_VERTEX(ev[EI110_100]);
                    break;

                case M110 | M100:
                    ADD_VERTEX(ev[EI110_010]);
                    ADD_VERTEX(ev[EI110_111]);
                    ADD_VERTEX(ev[EI000_100]);

                    ADD_VERTEX(ev[EI000_100]);
                    ADD_VERTEX(ev[EI101_100]);
                    ADD_VERTEX(ev[EI110_111]);
                    break;

                case M110 | M010:
                    ADD_VERTEX(ev[EI110_100]);
                    ADD_VERTEX(ev[EI110_111]);
                    ADD_VERTEX(ev[EI000_010]);

                    ADD_VERTEX(ev[EI000_010]);
                    ADD_VERTEX(ev[EI011_010]);
                    ADD_VERTEX(ev[EI110_111]);
                    break;

                // Case 5: 3 vertices on a common face

                // Left face
                case M000 | M001 | M010:
                    ADD_TRIANGLE(000_100, 110_010, 101_001);
                    ADD_TRIANGLE(110_010, 101_001, 011_010);
                    ADD_TRIANGLE(101_001, 011_010, 011_001);
                    break;

                case M001 | M000 | M011:
                    ADD_TRIANGLE(101_001, 000_100, 011_111);
                    ADD_TRIANGLE(100_000, 011_111, 000_010);
                    ADD_TRIANGLE(000_010, 011_010, 011_111);
                    break;

                case M011 | M010 | M001:
                    ADD_TRIANGLE(011_111, 010_110, 001_101);
                    ADD_TRIANGLE(010_110, 001_101, 010_000);
                    ADD_TRIANGLE(010_000, 001_000, 001_101);
                    break;

                case M010 | M011 | M000:
                    ADD_TRIANGLE(010_110, 011_111, 000_100);
                    ADD_TRIANGLE(011_111, 000_100, 011_001);
                    ADD_TRIANGLE(011_001, 000_001, 000_100);
                    break;

                // Right face
                case M100 | M110 | M101:
                    ADD_TRIANGLE(100_000, 110_010, 101_001);
                    ADD_TRIANGLE(110_010, 101_001, 101_111);
                    ADD_TRIANGLE(101_111, 110_111, 110_010);
                    break;

                case ~(M100 | M101 | M111):
                    ADD_TRIANGLE(101_001, 111_011, 100_000);
                    ADD_TRIANGLE(111_011, 100_000, 100_110);
                    ADD_TRIANGLE(100_110, 111_110, 111_011);
                    break;

                case ~(M111 | M110 | M101):
                    ADD_TRIANGLE(111_011, 110_010, 101_001);
                    ADD_TRIANGLE(110_010, 101_001, 101_100);
                    ADD_TRIANGLE(101_100, 110_100, 110_010);
                    break;

                case ~(M110 | M111 | M100):
                    ADD_TRIANGLE(110_010, 111_011, 100_000);
                    ADD_TRIANGLE(111_011, 100_000, 111_101);
                    ADD_TRIANGLE(111_101, 100_101, 100_000);
                    break;

                // Bottom face
                case M000 | M001 | M100:
                    ADD_TRIANGLE(000_010, 100_110, 001_011);
                    ADD_TRIANGLE(100_110, 001_011, 100_101);
                    ADD_TRIANGLE(100_101, 001_101, 001_011);
                    break;

                case M000 | M100 | M101:
                    ADD_TRIANGLE(000_010, 100_110, 101_111);
                    ADD_TRIANGLE(000_010, 101_111, 000_001);
                    ADD_TRIANGLE(000_001, 101_001, 101_111);
                    break;

                case M100 | M101 | M001:
                    ADD_TRIANGLE(100_110, 101_111, 001_011);
                    ADD_TRIANGLE(100_110, 001_011, 001_000);
                    ADD_TRIANGLE(001_000, 100_000, 100_110);
                    break;

                case M000 | M001 | M101:
                    ADD_TRIANGLE(000_010, 001_011, 101_111);
                    ADD_TRIANGLE(000_010, 101_111, 000_100);
                    ADD_TRIANGLE(000_100, 100_101, 101_111);
                    break;

                // Top face
                case M010 | M011 | M110:
                    ADD_TRIANGLE(010_000, 001_011, 110_100);
                    ADD_TRIANGLE(011_001, 110_100, 110_111);
                    ADD_TRIANGLE(110_111, 011_111, 011_001);
                    break;

                case ~(M010 | M110 | M111):
                    ADD_TRIANGLE(010_000, 110_100, 111_101);
                    ADD_TRIANGLE(010_000, 111_101, 010_011);
                    ADD_TRIANGLE(010_011, 011_111, 111_101);
                    break;

                case ~(M110 | M111 | M011):
                    ADD_TRIANGLE(110_100, 111_101, 011_001);
                    ADD_TRIANGLE(110_100, 011_001, 011_010);
                    ADD_TRIANGLE(011_010, 110_010, 110_100);
                    break;

                case ~(M010 | M011 | M111):
                    ADD_TRIANGLE(010_000, 011_001, 111_101);
                    ADD_TRIANGLE(010_000, 111_101, 111_110);
                    ADD_TRIANGLE(111_110, 010_110, 010_000);
                    break;

                // Front face
                case M000 | M100 | M010:
                    ADD_TRIANGLE(000_001, 100_101, 010_011);
                    ADD_TRIANGLE(100_101, 010_011, 010_110);
                    ADD_TRIANGLE(010_110, 100_110, 100_101);
                    break;

                case M000 | M100 | M110:
                    ADD_TRIANGLE(000_001, 100_101, 110_111);
                    ADD_TRIANGLE(000_001, 110_111, 110_010);
                    ADD_TRIANGLE(110_010, 000_010, 000_001);
                    break;

                case M100 | M110 | M010:
                    ADD_TRIANGLE(100_101, 110_111, 010_011);
                    ADD_TRIANGLE(100_101, 010_011, 010_000);
                    ADD_TRIANGLE(010_000, 100_000, 100_101);
                    break;

                case M110 | M010 | M000:
                    ADD_TRIANGLE(110_111, 010_011, 000_001);
                    ADD_TRIANGLE(110_111, 000_001, 000_100);
                    ADD_TRIANGLE(000_100, 110_100, 110_111);
                    break;

                // Back face
                case M001 | M011 | M101:
                    ADD_TRIANGLE(001_000, 011_010, 101_100);
                    ADD_TRIANGLE(011_010, 101_100, 101_111);
                    ADD_TRIANGLE(101_111, 011_111, 011_010);
                    break;

                case ~(M001 | M101 | M111):
                    ADD_TRIANGLE(001_000, 101_100, 111_110);
                    ADD_TRIANGLE(001_000, 111_110, 111_011);
                    ADD_TRIANGLE(111_011, 001_011, 001_000);
                    break;

                case ~(M101 | M111 | M011):
                    ADD_TRIANGLE(111_110, 011_010, 101_100);
                    ADD_TRIANGLE(011_010, 101_100, 101_001);
                    ADD_TRIANGLE(101_001, 011_001, 011_010);
                    break;

                case ~(M111 | M011 | M001):
                    ADD_TRIANGLE(111_110, 011_010, 001_000);
                    ADD_TRIANGLE(111_110, 001_000, 001_101);
                    ADD_TRIANGLE(001_101, 111_101, 111_110);
                    break;

                // Case 8: a whole face is positive.
                case M000 | M001 | M011 | M010:
                    ADD_TRIANGLE(000_100, 010_110, 011_111);
                    ADD_TRIANGLE(000_100, 001_101, 011_111);
                    break;

                case M000 | M100 | M110 | M010:
                    ADD_TRIANGLE(000_001, 100_101, 110_111);
                    ADD_TRIANGLE(000_001, 110_111, 010_011);
                    break;

                case M000 | M100 | M101 | M001:
                    ADD_TRIANGLE(000_010, 100_110, 101_111);
                    ADD_TRIANGLE(000_010, 101_111, 001_011);
                    break;

                // Case 9: A corner and 3 adjacent edges.
                case M001 | M011 | M000 | M101:
                    ADD_TRIANGLE(011_111, 011_010, 010_000);
                    ADD_TRIANGLE(000_010, 011_111, 000_100);
                    ADD_TRIANGLE(011_111, 000_100, 101_111);
                    ADD_TRIANGLE(000_100, 111_101, 100_101);
                    break;

                case M000 | M100 | M010 | M001:
					ADD_TRIANGLE(011_010, 010_110, 011_001);
					ADD_TRIANGLE(011_001, 010_110, 001_101);
					ADD_TRIANGLE(001_101, 010_110, 110_100);
					ADD_TRIANGLE(110_100, 100_101, 101_001);
                    break;

                case M010 | M000 | M011 | M110:
					ADD_TRIANGLE(100_110, 110_111, 100_000);
					ADD_TRIANGLE(100_000, 110_111, 000_001);
					ADD_TRIANGLE(000_001, 110_111, 111_011);
					ADD_TRIANGLE(111_011, 011_001, 001_000);
                    break;

                case M100 | M000 | M110 | M101:
                    ADD_TRIANGLE(110_111, 111_101, 110_010);
					ADD_TRIANGLE(010_110, 111_101, 001_101);
					ADD_TRIANGLE(010_110, 001_101, 000_010);
					ADD_TRIANGLE(000_010, 101_001, 001_000);
                    break;
                }
            }
        }
    }

    is_empty = vertex_data.empty();

    delete value_array;
}

FunctionMesh::FunctionMeshTree::FunctionMeshTree(FunctionMesh *mesh, Variable::var_type largest_var)
{
    this->mesh = mesh;
    this->largest_var = largest_var;
    is_empty = false;
}
