#include "moab/Interface.hpp"
#include "moab/Core.hpp"
#include "moab/Types.hpp"
#include "moab/GeomTopoTool.hpp"
#include "DagMC.hpp"
#include "GenerateHierarchy.hpp"
#include <array>
#include <iostream>
#include <map>
#include <set>


//#define CHKERR1 if (MB_SUCCESS != rval) {std::cout << rval << std::endl; return rval;};
#define CHKERR1 if (MB_SUCCESS != rval) return rval;

using namespace moab;

//class GeomtopoTool;
GeomTopoTool *myGeomTool;

Tag category_tag;
Tag geom_tag;
Tag name_tag;
Tag obj_name_tag;
Tag dim_tag, id_tag; //sense_tag;

//Core mbi
Core core;
Interface *mbi= &core;
DagMC *DAG;


void print_tree();
bool check_tree ( std::map< int, std::set<int> > ref_map );
ErrorCode get_all_handles();
Range get_children_by_dimension(EntityHandle parent, int desired_dimension);
const char* get_object_name( EntityHandle object );
ErrorCode heappermute(int v[], int n);
void swap(int *x, int *y);
void get_cube_info( int cube_id, std::array<double, 3> &scale, std::array<double, 3> &trans );

int len = 6;

ErrorCode build_cube( std::array<double, 3> scale_vec, 
                      std::array<double, 3> trans_vec, 
                      int    object_id )
{
  myGeomTool = new GeomTopoTool(mbi);

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
  CHKERR1;

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
      CHKERR1;

      rval = mbi->add_entities( surf, &verts[i], 1 );
      CHKERR1;
      
    }

  // create triangles and add to meshset
  for (unsigned i = 0; i < num_tris; ++i) 
    {
      const EntityHandle conn[] = { verts[connectivity[3*i  ]], 
                                    verts[connectivity[3*i+1]], 
                                    verts[connectivity[3*i+2]] };
      rval = mbi->create_element( MBTRI, conn, 3, tris[i] );
      CHKERR1;

      rval = mbi->add_entities( surf, &tris[i], 1 );
      CHKERR1;
    }


  // set name tag
  rval = mbi->tag_set_data( name_tag, &surf, 1, "Surface\0" );
  CHKERR1;  
 
  // set object name tag
  std::string object_name;
  rval = mbi->tag_set_data( obj_name_tag, &surf, 1, object_name.c_str() ); 
  CHKERR1;

  // set id tag
  rval = mbi->tag_set_data( id_tag, &surf, 1, &object_id );
  CHKERR1;

  // set geom tag
  int two = 2;
  rval = mbi->tag_set_data( geom_tag, &surf, 1, &(two) );
  CHKERR1;
  
  // set category tag
  rval = mbi->tag_set_data( category_tag, &surf, 1, "Surface\0" );
  CHKERR1;
  
  // create volume meshset associated with surface meshset
  EntityHandle volume;
  rval = mbi->create_meshset( MESHSET_SET, volume );
  CHKERR1;
 
  // set name, id, geom, and category tags for volume
  rval = mbi->tag_set_data( name_tag, &volume, 1, "Volume\0" );
  CHKERR1;  
  rval = mbi->tag_set_data( obj_name_tag, &surf, 1, object_name.c_str() ); 
  CHKERR1;
  rval = mbi->tag_set_data( id_tag, &volume, 1, &(object_id) );
  CHKERR1;
  int three = 3;
  rval = mbi->tag_set_data( geom_tag, &volume, 1, &(three) );
  CHKERR1;
  rval = mbi->tag_set_data( category_tag, &volume, 1, "Volume\0" );
  CHKERR1;


  // set surface as child of volume 
  rval = mbi->add_parent_child( volume, surf );
  CHKERR1;
  
  // set sense tag    
  rval = myGeomTool->set_sense(surf, volume, SENSE_FORWARD);
  CHKERR1;
 
  
  
  
  return MB_SUCCESS;
}
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

//  int num[3] = {1, 2, 3};
  int num[6] = {1, 2, 3, 4, 5, 6};

  heappermute(num, len);



/*
  std::array<double, 3> tmp_scale, tmp_trans;
  tmp_scale = {1, 1, 1};
  tmp_trans = {0, 0, 0};
  rval = build_cube( tmp_scale, tmp_trans, 1 );
  CHKERR1; 

  std::array<double, 3> tmp_scale2, tmp_trans2;
  tmp_scale2 = {4, 4, 4};
  tmp_trans2 = {0, 0, 0};
  rval = build_cube( tmp_scale2, tmp_trans2, 2 );
  CHKERR1; 

  std::array<double, 3> tmp_scale3, tmp_trans3;
  tmp_scale3 = {8, 8, 8};
  tmp_trans3 = {0, 0, 0};
  rval = build_cube( tmp_scale3, tmp_trans3, 3 );
  CHKERR1; 

 
  //create ref map
  std::map< int, std::set<int> > ref_map { {1, {1}   }, 
                                           {2, {2, 1}  }, 
                                           {3, {3, 2}  }  };

  GenerateHierarchy *gh = new GenerateHierarchy(mbi, rval);
  gh->build_hierarchy();
  rval = gh->construct_topology();
  CHKERR1;

  DAG = DagMC::instance(); //static member fxn
 
  rval = mbi->write_mesh( output_file_name );
  CHKERR1;

  print_tree();

  bool result;
  result = check_tree( ref_map );
  
  if (result == true)
    std::cout << "PASS" << std::endl;
  else  
    std::cout << "FAIL" << std::endl;

*/
  return 0;

}

ErrorCode get_all_handles()
{
  ErrorCode rval;

  rval = mbi->tag_get_handle( NAME_TAG_NAME, NAME_TAG_SIZE, MB_TYPE_OPAQUE,
                                name_tag, MB_TAG_SPARSE|MB_TAG_CREAT);
  CHKERR1;

  rval = mbi->tag_get_handle( "OBJECT_NAME", 32, MB_TYPE_OPAQUE,
                               obj_name_tag, MB_TAG_SPARSE|MB_TAG_CREAT);
  CHKERR1;

  int negone = -1;
  rval = mbi->tag_get_handle( GEOM_DIMENSION_TAG_NAME, 1, MB_TYPE_INTEGER,
                                geom_tag, MB_TAG_SPARSE|MB_TAG_CREAT,&negone);
  CHKERR1;

  rval = mbi->tag_get_handle( GLOBAL_ID_TAG_NAME, 
                              1, MB_TYPE_INTEGER, 
                              id_tag,
                              MB_TAG_DENSE|MB_TAG_CREAT );
  CHKERR1;

  rval = mbi->tag_get_handle( CATEGORY_TAG_NAME, 
                              CATEGORY_TAG_SIZE,
                              MB_TYPE_OPAQUE,
                              category_tag,
                              MB_TAG_SPARSE|MB_TAG_CREAT );

  CHKERR1;
  return MB_SUCCESS;

}

// print each volume's children
void print_tree()
{
  for ( int i =1; i <= DAG->num_entities(3) ; i++)
    {
      EntityHandle volume = DAG->entity_by_index(3, i);
      Range children = get_children_by_dimension( volume, 3);
     
      if( children.size() != 0)
        {
          std::cout << "Volume " << i << " has children: " << std::endl;
      
          for (Range::iterator j = children.begin() ; j != children.end() ; ++j )
            {
              std::cout << *j << std::endl;

            }
         }
    }

  return;
}

bool check_tree ( std::map< int, std::set<int> > ref_map )
{
  ErrorCode rval;
  std::map< int, std::set<int> > test_map;
  int vol_id;
  std::set<int> ref_set;
  std::set<int> test_set;
  std::set<int>::iterator it;

  if (ref_map.size() != DAG->num_entities(3) )
   {
    return false;
   }

  //go through volumes, create sets of children
  for ( int i = 1; i <= DAG->num_entities(3) ; i++)
    {
      //get vol id
      EntityHandle volume = DAG->entity_by_index(3, i);
      rval = mbi->tag_get_data(id_tag, &volume, 1, &vol_id );
      CHKERR1;

      //check if test vol in ref map
      if (ref_map.find(vol_id) == ref_map.end())
        {
          return false;
        }

      //put range of child surfaces into set
      Range child_surfs;
      test_set.clear(); 
      child_surfs = get_children_by_dimension( volume, 2);

      for (Range::iterator j = child_surfs.begin() ; j != child_surfs.end() ; ++j )
        {
            int child_id;
            
            rval = mbi->tag_get_data(id_tag, &(*j), 1, &child_id );
            test_set.insert(child_id);
        }

      // compare sets
      if ( test_set != ref_map[vol_id] )
        {
         return false;
        }
    }

  return true;
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

void swap(int *x, int *y)
{
  int temp;

  temp = *x; 
  *x = *y;
  *y = temp;
}

ErrorCode heappermute(int v[], int n)
{
  ErrorCode rval; 
  const char* output_file_name = "test_geom.vtk";
  std::array<double, 3> scale, trans;

  std::map< int, std::set<int> > ref_map { {1, {1}         }, 
                                           {2, {2, 1}      }, 
                                           {3, {3, 2}      },
                                           {4, {4}         },
                                           {5, {5}         },
                                           {6, {6, 5, 4, 3}}  };
  if (n == 1)
    {
      //build cubes
      for (int i = 0; i < len; i++)
        {
          get_cube_info(v[i], scale, trans);
          build_cube(scale, trans, v[i]);
        }

      //test tree
      GenerateHierarchy *gh = new GenerateHierarchy(mbi, rval);
      rval = gh->build_hierarchy();
      CHKERR1;
      rval = gh->construct_topology();
      CHKERR1;
  
      DAG = DagMC::instance(); 
   
      rval = mbi->write_mesh( output_file_name );
      CHKERR1;
  
      print_tree();
  
      // check the tree
      bool result;
      result = check_tree( ref_map );
      
      if (result == true)
        std::cout << "PASS" << std::endl;
      else  
        std::cout << "FAIL" << std::endl;

      // delete the geometry so new one can be built
      mbi->delete_mesh();
            
    }
  
  else
    {
      for (int i = 0; i < n; i++)
        {
          heappermute(v, n-1);
          if (n % 2 == 1)
            { 
              swap(&v[0], &v[n-1]);
            }

          else
            {
              swap(&v[i], &v[n-1]);
            }
        } 
    }

  return MB_SUCCESS;
}

void get_cube_info( int cube_id, std::array<double, 3> &scale, std::array<double, 3> &trans )
{
 
  if ( cube_id == 1 )
    {
      scale = {1, 1, 1};
      trans = {0, 0, 0};
    }
  if ( cube_id == 2 )
    {
      scale = {4, 4, 4};
      trans = {0, 0, 0};
    }
  if ( cube_id == 3 )
    {
      scale = {8, 8, 8};
      trans = {0, 0, 0};
    }
  if ( cube_id == 4 )
    {
      scale = {4, 4, 4};
      trans = {0, 0, -10};
    }
  if ( cube_id == 5 )
    {
      scale = {4, 4, 4};
      trans = {10, 0, -10};
    }
  if ( cube_id == 6 )
    {
      scale = {40, 40, 40};
      trans = {0, 0, 0};
    }

}
