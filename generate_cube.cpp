#include "moab/Interface.hpp"
#include "moab/Core.hpp"
#include "moab/Types.hpp"
#include "moab/GeomTopoTool.hpp"
#include "DagMC.hpp"
#include "GenerateHierarchy.hpp"
#include <iostream>
#include <map>

#define CHKERR if (MB_SUCCESS != rval) return rval

using namespace moab;

Tag category_tag;
Tag geom_tag;
Tag name_tag;
Tag obj_name_tag;
Tag dim_tag, id_tag, sense_tag;

//Core moab_instance;
//Interface& MBI= moab_instance;
//Interface *MBI();

//Core mbi
Core core;
Interface *mbi= &core;
DagMC *DAG;


// Create file containing geometry for 1x1x1 cube 
//ErrorCode build_cube( const char* output_file_name );

//print_tree();
void check_tree ( std::map< int, std::vector<int> > ref_map );
std::map< int, std::vector<int> > generate_map();
ErrorCode get_all_handles();
Range get_children_by_dimension(EntityHandle parent, int desired_dimension);
const char* get_object_name( EntityHandle object );

ErrorCode build_cube( double scale_vec[3], 
                      double trans_vec[3], 
                      int    object_id )
{
  ErrorCode rval;
  
  // Define a 1x1x1 cube centered at orgin

  // coordinates of each corner 
  const double coords[] = {
    0.5, -0.5, -0.5, 
    0.5,  0.5, -0.5,
   -0.5,  0.5, -0.5,
   -0.5, -0.5, -0.5,
    0.5, -0.5,  0.5, 
    0.5,  0.5,  0.5,
   -0.5,  0.5,  0.5,
   -0.5, -0.5,  0.5 };

  // connectivity of 2 triangles per 
  //  each face of the cube
  const int connectivity[] = {
    0, 3, 1,  3, 2, 1, // -Z
    0, 1, 4,  5, 4, 1, // +X
    1, 2, 6,  6, 5, 1, // +Y
    6, 2, 3,  7, 6, 3, // -X
    0, 4, 3,  7, 3, 4, // -Y
    4, 5, 7,  5, 6, 7 // +Z
  };

  
  // Create the geometry
  const unsigned num_verts = 8; //sizeof(coords) / (3*sizeof(double));
  const unsigned num_tris = 12; //sizeof(connectivity) / (3*sizeof(int));
  EntityHandle verts[num_verts], tris[num_tris], surf;

  rval = mbi->create_meshset( MESHSET_SET, surf );
  CHKERR;

  // scale coords
  int i;
  double scaled_coords[24];
  for ( i = 0; i < num_verts; i++ ) 
    { 
      scaled_coords[3*i]   = coords[3*i]*scale_vec[0];
      scaled_coords[3*i+1] = coords[3*i+1]*scale_vec[1];
      scaled_coords[3*i+2] = coords[3*i+2]*scale_vec[2];
    }

  // translate coords
  double trans_coords[24];
  for ( i = 0; i < num_verts; i++ ) 
    {
      trans_coords[3*i]   = scaled_coords[3*i] + trans_vec[0];
      trans_coords[3*i+1] = scaled_coords[3*i+1] + trans_vec[1];
      trans_coords[3*i+2] = scaled_coords[3*i+2] + trans_vec[2];
    }

  // create vertices and add to meshset
  for (unsigned i = 0; i < num_verts; ++i) 
    {
      rval = mbi->create_vertex( trans_coords + 3*i, verts[i] ); 
      CHKERR;

      rval = mbi->add_entities( surf, &verts[i], 1 );
      CHKERR;
      
    }

  // create triangles and add to meshset
  for (unsigned i = 0; i < num_tris; ++i) 
    {
      const EntityHandle conn[] = { verts[connectivity[3*i  ]], 
                                    verts[connectivity[3*i+1]], 
                                    verts[connectivity[3*i+2]] };
      rval = mbi->create_element( MBTRI, conn, 3, tris[i] );
      CHKERR;

      rval = mbi->add_entities( surf, &tris[i], 1 );
      CHKERR;
    }


  // set name tag
  rval = mbi->tag_set_data( name_tag, &surf, 1, "Surface\0" );
  CHKERR;  
 
  // set object name tag
  std::string object_name;
  rval = mbi->tag_set_data( obj_name_tag, &surf, 1, object_name.c_str() ); 
  CHKERR;

  // set id tag
  rval = mbi->tag_set_data( id_tag, &surf, 1, &object_id );
  CHKERR;

  // set geom tag
  int two = 2;
  rval = mbi->tag_set_data( geom_tag, &surf, 1, &(two) );
  CHKERR;
  
  // set category tag
  rval = mbi->tag_set_data( category_tag, &surf, 1, "Surface\0" );
  CHKERR;
  
  // create volume meshset associated with surface meshset
  EntityHandle volume;
  rval = mbi->create_meshset( MESHSET_SET, volume );
  CHKERR;
 
  // set surface as child of volume 
  rval = mbi->add_parent_child( volume, surf );
  CHKERR;
  
  // set sense tag    
  EntityHandle surf_volumes[2];
  surf_volumes[0] = volume;
  surf_volumes[1] = 0;
  rval = mbi->tag_set_data(sense_tag, &surf, 1, surf_volumes);
  CHKERR;
 
  // set name, id, geom, and category tags for volume
  rval = mbi->tag_set_data( name_tag, &volume, 1, "Volume\0" );
  CHKERR;  
  rval = mbi->tag_set_data( obj_name_tag, &surf, 1, object_name.c_str() ); 
  CHKERR;
  rval = mbi->tag_set_data( id_tag, &volume, 1, &(object_id) );
  CHKERR;
  int three = 3;
  rval = mbi->tag_set_data( geom_tag, &volume, 1, &(three) );
  CHKERR;
  rval = mbi->tag_set_data( category_tag, &volume, 1, "Volume\0" );
  CHKERR;
  
  
  
  return MB_SUCCESS;
}
/*
EntityHandle scale_cube( EntityHandle verts, scale_vec )
{
  int i;
  EntityHandle scaled_vert;
  double scaled_coords[3];
  double coords[3];
  Range scaled_verts;  

  for ( i = 0; i < 8; i++ ) 
    { 
      rval = mbi->get_coords(verts[i], coords);
      scaled_coords[0] = coords[0]*scale_vec[0];
      scaled_coords[1] = coords[1]*scale_vec[1];
      scaled_coords[2] = coords[2]*scale_vec[2];
      
      rval = mbi->create_vertex( scaled_coords, scaled_vert );
      scaled_verts.insert(scaled_vert);
    }




}
*/
/*
static bool run_test( std::string name, int argc, char* argv[] )
{
  if (argc == 1)
    return true;
  for (int i = 1; i < argc; ++i)
    if (name == argv[i])
      return true;
  return false;
}

#define RUN_TEST(A) do { \
  if (run_test( #A, argc, argv )) { \
    std::cout << #A << "... " << std::endl; \
    if (MB_SUCCESS != A ( dagmc ) ) { \
      ++errors; \
    } \
  } \
} while(false)
*/

int main(int  argc, char **argv)
{
  ErrorCode rval;
  const char* output_file_name = "test_geom.vtk";
 
  // get all handles (dimension, id, sense)
  rval = get_all_handles();


  //  GenerateHierarchy gh;
  //GenerateHierarchy gh (mbi,rval); 
  int object_id = 1;

  double tmp_scale3[3] = {8, 8, 8};
  double tmp_trans3[3] = {0, 0, 0};

  rval = build_cube( tmp_scale3, tmp_trans3, object_id++ );
  CHKERR; 
  double tmp_scale[3] = {1, 1, 1};
  double tmp_trans[3] = {0, 0, 0};
  rval = build_cube( tmp_scale, tmp_trans, object_id++ );
  CHKERR; 

  double tmp_scale2[3] = {4, 4, 4};
  double tmp_trans2[3] = {0, 0, 0};

  //create ref map
  std::map< int, std::vector<int> > ref_map { {1, {2, 3}}, 
                                                {2, {NULL}}, 
                                                {3,    {2}} }  ;

  rval = build_cube( tmp_scale2, tmp_trans2, object_id++ );
  CHKERR; 
 

  GenerateHierarchy *gh = new GenerateHierarchy(mbi, rval);
  gh->build_hierarchy();
  gh->construct_topology();

  DAG = DagMC::instance(); //static member fxn
 
  rval = mbi->write_mesh( output_file_name );
  CHKERR;
  //print_tree();
  check_tree( ref_map );
  
  return 0;

}

ErrorCode get_all_handles()
{
  ErrorCode rval;

  rval = mbi->tag_get_handle( NAME_TAG_NAME, NAME_TAG_SIZE, MB_TYPE_OPAQUE,
                                name_tag, MB_TAG_SPARSE|MB_TAG_CREAT);
  CHKERR;

  rval = mbi->tag_get_handle( "OBJECT_NAME", 32, MB_TYPE_OPAQUE,
                               obj_name_tag, MB_TAG_SPARSE|MB_TAG_CREAT);
  CHKERR;

  int negone = -1;
  rval = mbi->tag_get_handle( GEOM_DIMENSION_TAG_NAME, 1, MB_TYPE_INTEGER,
                                geom_tag, MB_TAG_SPARSE|MB_TAG_CREAT,&negone);
  CHKERR;

  rval = mbi->tag_get_handle("GEOM_SENSE_2", 2, MB_TYPE_HANDLE,
                                sense_tag, MB_TAG_SPARSE|MB_TAG_CREAT );
  CHKERR;

  rval = mbi->tag_get_handle( GLOBAL_ID_TAG_NAME, 
                              1, MB_TYPE_INTEGER, 
                              id_tag,
                              MB_TAG_DENSE|MB_TAG_CREAT );
  CHKERR;

  rval = mbi->tag_get_handle( CATEGORY_TAG_NAME, 
                              CATEGORY_TAG_SIZE,
                              MB_TYPE_OPAQUE,
                              category_tag,
                              MB_TAG_SPARSE|MB_TAG_CREAT );

  CHKERR;
  return MB_SUCCESS;

}

// print each volume's children
//void print_tree()
std::map< int, std::vector<int> > generate_map()
{

  std::map< int, std::vector<int> > gh_map;
  
  for ( int i =1; i <= DAG->num_entities(3) ; i++)
    {
      EntityHandle volume = DAG->entity_by_index(3, i);
      Range children = get_children_by_dimension( volume, 3);
      std::cout << "Vol " << i << " eh: "<< volume <<std::endl;    
     
      // create parent level of map
      gh_map[i]; 
 
      // const char* volume_name = get_object_name( volume );
      if( children.size() != 0)
        {
          std::cout << "Volume " << i << " has children: " << std::endl;
      
          for (Range::iterator j = children.begin() ; j != children.end() ; ++j )
            {
              //const char* child_name = get_object_name( *j );
              std::cout << *j << std::endl;

              // add children to parent's map
              gh_map[i].push_back(*j);
            }
         }
    }

  return gh_map;
}

void check_tree ( std::map< int, std::vector<int> > ref_map )
{
  std::map< int, std::vector<int> > test_map;
 
  //generate parent map 
  for ( int i = 1; i <= DAG->num_entities(3) ; i++)
    {
      EntityHandle volume = DAG->entity_by_index(3, i);
      Range children = get_children_by_dimension( volume, 3);
     
      // create parent level of map
      test_map[i]; 
 
      // const char* volume_name = get_object_name( volume );
      for (Range::iterator j = children.begin() ; j != children.end() ; ++j )
        {
          // add children to parent's map
          test_map[i].push_back(*j);
        }
    }

  //check test map against ref map
//  for ( std::map< int, std::vector<int> >::const_iterator it = ref_map.begin(); 
  //      it !=ref_map.end(); ++it)
  for ( auto it = ref_map.cbegin(); it != ref_map.cend(); ++it)
    {
      std::cout<< it-> first << " " << it->second.first << " " <<
                  it->second.second << std::endl;
    }
/*
  for ( int i = 1; i <= DAG->num_entities(3) ; i++)
    {
      if( ref_map.find(i) != ref_map.end() )
       {
         std::map<int, int>::iterator it = ref_map.find(i);
         //if (ref_map.find(i) != test_map.find(i))
         std::cout << it->second << std::endl;
//           std::cout << "parent vols: " << i << std::endl;


       }       
    } 
*/
}

Range get_children_by_dimension(EntityHandle parent, int desired_dimension)
{
  Range all_children, desired_children;
  Range::iterator it;
  ErrorCode rval;
  int actual_dimension;

  all_children.clear();
  rval = mbi->get_child_meshsets(parent, all_children);

  for ( it = all_children.begin() ; it != all_children.end() ; ++it)
    {
      rval = mbi->tag_get_data(geom_tag, &(*it), 1, &actual_dimension);
      if ( actual_dimension == desired_dimension )
        {
          desired_children.insert(*it);
        }
    }

  return desired_children;
  
}

const char* get_object_name( EntityHandle object )
{
  const void* p;
  int len=32;
  const char* str;
  ErrorCode rval;
  
  rval = mbi->tag_get_by_ptr( obj_name_tag, &object, 1, &p, &len);

  str = static_cast<const char*>(p);
 
  return str;
}
