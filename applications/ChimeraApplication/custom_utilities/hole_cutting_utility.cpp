#include "hole_cutting_utility.h"

namespace Kratos
{

template <int TDim>
void ChimeraHoleCuttingUtility::CreateHoleAfterDistance(
    ModelPart &rModelPart, ModelPart &rHoleModelPart,
    ModelPart &rHoleBoundaryModelPart, const double Distance)
{
    KRATOS_TRY;
    ChimeraHoleCuttingUtility::RemoveOutOfDomainElements<TDim>(rModelPart, rHoleModelPart, 1, Distance,
                                    true);
    ChimeraHoleCuttingUtility::ExtractBoundaryMesh<TDim>(rHoleModelPart, rHoleBoundaryModelPart);
    KRATOS_CATCH("");
}

template <int TDim>
void ChimeraHoleCuttingUtility::RemoveOutOfDomainElements(
    ModelPart &rModelPart, ModelPart &rModifiedModelPart,
    const int MainDomainOrNot, const double OverLapDistance,
    const bool GetInside)
{
    KRATOS_TRY;
    std::vector<IndexType> vector_of_node_ids;

    int count = 0;

    for (auto &i_element : rModelPart.Elements())
    {
        double nodal_distance = 0.0;
        IndexType numPointsOutside = 0;
        IndexType j = 0;
        Geometry<Node<3>> &geom = i_element.GetGeometry();

        for (j = 0; j < geom.size(); j++)
        {
            nodal_distance =
                i_element.GetGeometry()[j].FastGetSolutionStepValue(DISTANCE);

            nodal_distance = nodal_distance * MainDomainOrNot;
            if (nodal_distance < -1 * OverLapDistance)
            {
                numPointsOutside++;
            }
        }

        /* Any node goes out of the domain means the element need to be INACTIVE ,
       otherwise the modified patch boundary wont find any nodes on background
     */
        if (numPointsOutside > 0)
        {
            i_element.Set(ACTIVE, false);
            IndexType num_nodes_per_elem = i_element.GetGeometry().PointsNumber();
            if (GetInside)
                rModifiedModelPart.AddElement(rModelPart.pGetElement(i_element.Id()));
            for (j = 0; j < num_nodes_per_elem; j++)
            {
                i_element.GetGeometry()[j].FastGetSolutionStepValue(VELOCITY_X, 0) =
                    0.0;
                i_element.GetGeometry()[j].FastGetSolutionStepValue(VELOCITY_Y, 0) =
                    0.0;
                if (num_nodes_per_elem - 1 > 2)
                    i_element.GetGeometry()[j].FastGetSolutionStepValue(VELOCITY_Z, 0) =
                        0.0;
                i_element.GetGeometry()[j].FastGetSolutionStepValue(PRESSURE, 0) = 0.0;
                i_element.GetGeometry()[j].FastGetSolutionStepValue(VELOCITY_X, 1) =
                    0.0;
                i_element.GetGeometry()[j].FastGetSolutionStepValue(VELOCITY_Y, 1) =
                    0.0;
                if (num_nodes_per_elem - 1 > 2)
                    i_element.GetGeometry()[j].FastGetSolutionStepValue(VELOCITY_Z, 1) =
                        0.0;
                i_element.GetGeometry()[j].FastGetSolutionStepValue(PRESSURE, 1) = 0.0;
                if (GetInside)
                    vector_of_node_ids.push_back(i_element.GetGeometry()[j].Id());
            }
        }
        else
        {
            if (!GetInside)
            {
                count++;
                IndexType num_nodes_per_elem =
                    i_element.GetGeometry().PointsNumber(); // Size()
                rModifiedModelPart.AddElement(
                    rModelPart.pGetElement(i_element.Id())); // AddElement()
                for (j = 0; j < num_nodes_per_elem; j++)
                    vector_of_node_ids.push_back(i_element.GetGeometry()[j].Id());
            }
        }
    }

    // sorting and making unique list of node ids
    std::set<IndexType> s(vector_of_node_ids.begin(), vector_of_node_ids.end());
    vector_of_node_ids.assign(s.begin(), s.end());

    // Add unique nodes in the ModelPart
    for (auto i_node_id = vector_of_node_ids.begin();
         i_node_id != vector_of_node_ids.end(); i_node_id++)
    {
        Node<3>::Pointer pnode = rModelPart.Nodes()(*i_node_id);
        rModifiedModelPart.AddNode(pnode);
    }

    KRATOS_CATCH("");
}

template <int TDim>
void ChimeraHoleCuttingUtility::ExtractBoundaryMesh(
    ModelPart &rVolumeModelPart, ModelPart &rExtractedBoundaryModelPart,
    bool GetInternal)
{
    KRATOS_TRY;

   // Needed structures for the ExtractSurfaceMesh operation
    struct KeyComparator
    {
        bool operator()(const vector<IndexType> &lhs,
                        const vector<IndexType> &rhs) const
        {
            if (lhs.size() != rhs.size())
                return false;
            for (IndexType i = 0; i < lhs.size(); i++)
                if (lhs[i] != rhs[i])
                    return false;
            return true;
        }
    };

    struct KeyHasher
    {
        IndexType operator()(const vector<int> &k) const
        {
            IndexType seed = 0.0;
            std::hash<int> hasher;
            for (IndexType i = 0; i < k.size(); i++)
                seed ^= hasher(k[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    IndexType n_nodes = rVolumeModelPart.ElementsBegin()->GetGeometry().size();
    KRATOS_ERROR_IF(!(n_nodes != 3 || n_nodes != 4))
        << "Hole cutting process is only supported for tetrahedral and "
           "triangular elements"
        << std::endl;

    // Some type-definitions
    typedef std::unordered_map<vector<IndexType>, IndexType, KeyHasher,
                               KeyComparator>
        hashmap;
    typedef std::unordered_map<vector<IndexType>, vector<IndexType>, KeyHasher,
                               KeyComparator>
        hashmap_vec;

    // Create map to ask for number of faces for the given set of node ids
    // representing on face in the model part
    hashmap n_faces_map;
    const int num_elements =
        static_cast<int>(rVolumeModelPart.NumberOfElements());
    const auto elements_begin = rVolumeModelPart.ElementsBegin();
    // Fill map that counts number of faces for given set of nodes
#pragma omp parallel for
    for (int i_e = 0; i_e < num_elements; ++i_e)
    {
        auto i_element = elements_begin + i_e;
        Element::GeometryType::GeometriesArrayType faces;
        if (TDim == 2)
            faces = i_element->GetGeometry().GenerateEdges();
        else if (TDim == 3)
            faces = i_element->GetGeometry().GenerateFaces();

        for (IndexType i_face = 0; i_face < faces.size(); i_face++)
        {
            // Create vector that stores all node is of current i_face
            vector<IndexType> ids(faces[i_face].size());

            // Store node ids
            for (IndexType i = 0; i < faces[i_face].size(); i++)
                ids[i] = faces[i_face][i].Id();

            //*** THE ARRAY OF IDS MUST BE ORDERED!!! ***
            std::sort(ids.begin(), ids.end());

// Fill the map
#pragma omp critical
            n_faces_map[ids] += 1;
        }
    }
    // Create a map to get nodes of skin face in original order for given set of
    // node ids representing that face The given set of node ids may have a
    // different node order
    hashmap_vec ordered_skin_face_nodes_map;

    // Fill map that gives original node order for set of nodes
#pragma omp parallel for
    for (int i_e = 0; i_e < num_elements; ++i_e)
    {
        auto i_element = elements_begin + i_e;
        Element::GeometryType::GeometriesArrayType faces;
        if (TDim == 2)
            faces = i_element->GetGeometry().GenerateEdges();
        else if (TDim == 3)
            faces = i_element->GetGeometry().GenerateFaces();

        for (IndexType i_face = 0; i_face < faces.size(); i_face++)
        {
            // Create vector that stores all node is of current i_face
            vector<IndexType> ids(faces[i_face].size());
            vector<IndexType> unsorted_ids(faces[i_face].size());

            // Store node ids
            for (IndexType i = 0; i < faces[i_face].size(); i++)
            {
                ids[i] = faces[i_face][i].Id();
                unsorted_ids[i] = faces[i_face][i].Id();
            }

            //*** THE ARRAY OF IDS MUST BE ORDERED!!! ***
            std::sort(ids.begin(), ids.end());
#pragma omp critical
            {
                if (n_faces_map[ids] == 1)
                    ordered_skin_face_nodes_map[ids] = unsorted_ids;
            }
        }
    }
    // First assign to skin model part all nodes from original model_part,
    // unnecessary nodes will be removed later
    IndexType id_condition = 1;

    // Add skin faces as triangles to skin-model-part (loop over all node sets)
    std::vector<IndexType> vector_of_node_ids;
    for (typename hashmap::const_iterator it = n_faces_map.begin();
         it != n_faces_map.end(); it++)
    {
        // If given node set represents face that is not overlapping with a face of
        // another element, add it as skin element
        if (it->second == 1)
        {
            // If skin edge is a triangle store triangle in with its original
            // orientation in new skin model part
            if (it->first.size() == 2)
            {
                // Getting original order is important to properly reproduce skin edge
                // including its normal orientation
                vector<IndexType> original_nodes_order =
                    ordered_skin_face_nodes_map[it->first];

                Node<3>::Pointer pnode1 =
                    rVolumeModelPart.Nodes()(original_nodes_order[0]);
                Node<3>::Pointer pnode2 =
                    rVolumeModelPart.Nodes()(original_nodes_order[1]);

                // Storing the node ids list
                vector_of_node_ids.push_back(original_nodes_order[0]);
                vector_of_node_ids.push_back(original_nodes_order[1]);

                Properties::Pointer properties =
                    rExtractedBoundaryModelPart.rProperties()(0);
                Condition const &rReferenceLineCondition =
                    KratosComponents<Condition>::Get(
                        "LineCondition2D2N"); // Condition2D

                // Skin edges are added as conditions
                Line2D2<Node<3>> line1(pnode1, pnode2);
                Condition::Pointer p_condition1 =
                    rReferenceLineCondition.Create(id_condition++, line1, properties);
                rExtractedBoundaryModelPart.Conditions().push_back(p_condition1);
            }
            // If skin face is a triangle store triangle in with its original
            // orientation in new skin model part
            if (it->first.size() == 3)
            {
                // Getting original order is important to properly reproduce skin face
                // including its normal orientation
                vector<IndexType> original_nodes_order =
                    ordered_skin_face_nodes_map[it->first];
                Node<3>::Pointer pnode1 =
                    rVolumeModelPart.Nodes()(original_nodes_order[0]);
                Node<3>::Pointer pnode2 =
                    rVolumeModelPart.Nodes()(original_nodes_order[1]);
                Node<3>::Pointer pnode3 =
                    rVolumeModelPart.Nodes()(original_nodes_order[2]);

                // Storing the node ids list
                vector_of_node_ids.push_back(original_nodes_order[0]);
                vector_of_node_ids.push_back(original_nodes_order[1]);
                vector_of_node_ids.push_back(original_nodes_order[2]);
                Properties::Pointer properties =
                    rExtractedBoundaryModelPart.rProperties()(0);
                Condition const &rReferenceTriangleCondition =
                    KratosComponents<Condition>::Get(
                        "SurfaceCondition3D3N"); // Condition3D

                // Skin faces are added as conditions
                Triangle3D3<Node<3>> triangle1(pnode1, pnode2, pnode3);
                Condition::Pointer p_condition1 = rReferenceTriangleCondition.Create(
                    id_condition++, triangle1, properties);
                rExtractedBoundaryModelPart.Conditions().push_back(p_condition1);
            }
            // If skin face is a quadrilateral then divide in two triangles and store
            // them with their original orientation in new skin model part
            if (it->first.size() == 4)
            {
                // Getting original order is important to properly reproduce skin
                // including its normal orientation
                vector<IndexType> original_nodes_order =
                    ordered_skin_face_nodes_map[it->first];

                Node<3>::Pointer pnode1 =
                    rVolumeModelPart.Nodes()(original_nodes_order[0]);
                Node<3>::Pointer pnode2 =
                    rVolumeModelPart.Nodes()(original_nodes_order[1]);
                Node<3>::Pointer pnode3 =
                    rVolumeModelPart.Nodes()(original_nodes_order[2]);
                Node<3>::Pointer pnode4 =
                    rVolumeModelPart.Nodes()(original_nodes_order[3]);
                // Storing the node ids list
                vector_of_node_ids.push_back(original_nodes_order[0]);
                vector_of_node_ids.push_back(original_nodes_order[1]);
                vector_of_node_ids.push_back(original_nodes_order[2]);
                vector_of_node_ids.push_back(original_nodes_order[3]);
                Properties::Pointer properties =
                    rExtractedBoundaryModelPart.rProperties()(0);
                Condition const &rReferenceTriangleCondition =
                    KratosComponents<Condition>::Get(
                        "SurfaceCondition3D3N"); // Condition3D

                // Add triangle one as condition
                Triangle3D3<Node<3>> triangle1(pnode1, pnode2, pnode3);
                Condition::Pointer p_condition1 = rReferenceTriangleCondition.Create(
                    id_condition++, triangle1, properties);
                rExtractedBoundaryModelPart.Conditions().push_back(p_condition1);

                // Add triangle two as condition
                Triangle3D3<Node<3>> triangle2(pnode1, pnode3, pnode4);
                Condition::Pointer p_condition2 = rReferenceTriangleCondition.Create(
                    id_condition++, triangle2, properties);
                rExtractedBoundaryModelPart.Conditions().push_back(p_condition2);
            }
        }
    }

    // sorting and making unique list of node ids
    std::set<IndexType> sort_set(vector_of_node_ids.begin(),
                                 vector_of_node_ids.end());
    vector_of_node_ids.assign(sort_set.begin(), sort_set.end());

    for (const auto &i_node_id : vector_of_node_ids)
    {
        // Adding the nodes to the rExtractedBoundaryModelPart
        Node<3>::Pointer pnode = rVolumeModelPart.Nodes()(i_node_id);
        rExtractedBoundaryModelPart.AddNode(pnode);
    }

    // for multipatch
    const int num_nodes =
        static_cast<int>(rExtractedBoundaryModelPart.NumberOfNodes());
    const auto nodes_begin = rExtractedBoundaryModelPart.NodesBegin();

#pragma omp parallel for
    for (int i_n = 0; i_n < num_nodes; ++i_n)
    {
        auto i_node = nodes_begin + i_n;
        i_node->Set(TO_ERASE, false);
    }

    const int num_conditions =
        static_cast<int>(rExtractedBoundaryModelPart.NumberOfConditions());
    const auto conditions_begin = rExtractedBoundaryModelPart.ConditionsBegin();

#pragma omp parallel for
    for (int i_c = 0; i_c < num_conditions; ++i_c)
    {
        auto i_condition = conditions_begin + i_c;
        i_condition->Set(TO_ERASE, false);
    }

    for (auto &i_condition : rExtractedBoundaryModelPart.Conditions())
    {
        auto &geo = i_condition.GetGeometry();
        bool is_internal = true;
        for (const auto &node : geo)
            is_internal = is_internal && node.Is(CHIMERA_INTERNAL_BOUNDARY);
        if (is_internal)
        {
            if (!GetInternal)
            {
                i_condition.Set(TO_ERASE);
                for (auto &node : geo)
                    node.Set(TO_ERASE);
            }
        }
        else
        {
            if (GetInternal)
            {
                i_condition.Set(TO_ERASE);
                for (auto &node : geo)
                    node.Set(TO_ERASE);
            }
        }
    }

    rExtractedBoundaryModelPart.RemoveConditions(TO_ERASE);
    rExtractedBoundaryModelPart.RemoveNodes(TO_ERASE);
    KRATOS_CATCH("");
}

//
// Specializeing the functions for diff templates
//
template void ChimeraHoleCuttingUtility::RemoveOutOfDomainElements<2>(ModelPart &rModelPart,
                                ModelPart &rModifiedModelPart,
                                const int MainDomainOrNot,
                                const double OverLapDistance,
                                const bool GetInside);

template void ChimeraHoleCuttingUtility::RemoveOutOfDomainElements<3>(ModelPart &rModelPart,
                                ModelPart &rModifiedModelPart,
                                const int MainDomainOrNot,
                                const double OverLapDistance,
                                const bool GetInside);

template void ChimeraHoleCuttingUtility::ExtractBoundaryMesh<2>( ModelPart &rVolumeModelPart, ModelPart &rExtractedBoundaryModelPart, bool GetInternal);
template void ChimeraHoleCuttingUtility::ExtractBoundaryMesh<3>( ModelPart &rVolumeModelPart, ModelPart &rExtractedBoundaryModelPart, bool GetInternal);


template void ChimeraHoleCuttingUtility::CreateHoleAfterDistance<2>(ModelPart &rModelPart, ModelPart &rHoleModelPart, ModelPart &rHoleBoundaryModelPart, const double Distance);
template void ChimeraHoleCuttingUtility::CreateHoleAfterDistance<3>(ModelPart &rModelPart, ModelPart &rHoleModelPart, ModelPart &rHoleBoundaryModelPart, const double Distance);


} // namespace Kratos