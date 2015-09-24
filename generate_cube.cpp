#include "moab/Interface.hpp"
#include "moab/Core.hpp"
#include "moab/Types.hpp"
#include "moab/GeomTopoTool.hpp"
#include "DagMC.hpp"
#include <iostream>


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
Core mbi;
DagMC *DAG;


// Create file containing geometry for 1x1x1 cube 
ErrorCode write_geometry( const char* output_file_name );

ErrorCode write_geometry( double scale_vec[3], 
                          double trans_vec[3] )
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

  //  const unsigned tris_per_surf[] = {12};
  
  // Create the geometry
  const unsigned num_verts = 8; //sizeof(coords) / (3*sizeof(double));
  const unsigned num_tris = 12; //sizeof(connectivity) / (3*sizeof(int));
  EntityHandle verts[num_verts], tris[num_tris], surf;

  rval = mbi.create_meshset( MESHSET_SET, surf );
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
      rval = mbi.create_vertex( trans_coords + 3*i, verts[i] ); 
      CHKERR;

      rval = mbi.add_entities( surf, &verts[i], 1 );
      CHKERR;
      
    }

  // create triangles and add to meshset
  for (unsigned i = 0; i < num_tris; ++i) 
    {
      const EntityHandle conn[] = { verts[connectivity[3*i  ]], 
                                    verts[connectivity[3*i+1]], 
                                    verts[connectivity[3*i+2]] };
      rval = mbi.create_element( MBTRI, conn, 3, tris[i] );
      CHKERR;

      rval = mbi.add_entities( surf, &tris[i], 1 );
      CHKERR;
    }


  // set name tag
  rval = mbi.tag_set_data( name_tag, &surf, 1, "Surface\0" );
  CHKERR;  
 
  // set object name tag
  std::string object_name;
  rval = mbi.tag_set_data( obj_name_tag, &surf, 1, object_name.c_str() ); 
  CHKERR;

  // set id tag
  int ids = 1;
  rval = mbi.tag_set_data( id_tag, &surf, 1, &ids );
  CHKERR;

  // set geom tag
  int two = 2;
  rval = mbi.tag_set_data( geom_tag, &surf, 1, &(two) );
  CHKERR;
  
  // set category tag
  rval = mbi.tag_set_data( category_tag, &surf, 1, "Surface\0" );
  CHKERR;
  
  // create volume meshset associated with surface meshset
  EntityHandle volume;
  rval = mbi.create_meshset( MESHSET_SET, volume );
  CHKERR;
 
  // set surface as child of volume 
  rval = mbi.add_parent_child( volume, surf );
  CHKERR;
  
  // set sense tag    
  EntityHandle surf_volumes[2];
  surf_volumes[0] = volume;
  surf_volumes[1] = 0;
  rval = mbi.tag_set_data(sense_tag, &surf, 1, surf_volumes);
  CHKERR;
 
  // set name, id, geom, and category tags for volume
  rval = mbi.tag_set_data( name_tag, &volume, 1, "Volume\0" );
  CHKERR;  
  rval = mbi.tag_set_data( obj_name_tag, &surf, 1, object_name.c_str() ); 
  CHKERR;

  rval = mbi.tag_set_data( id_tag, &volume, 1, &(ids) );
  CHKERR;
  int three = 3;
  rval = mbi.tag_set_data( geom_tag, &volume, 1, &(three) );
  CHKERR;
  rval = mbi.tag_set_data( category_tag, &volume, 1, "Volume\0" );
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
      rval = mbi.get_coords(verts[i], coords);
      scaled_coords[0] = coords[0]*scale_vec[0];
      scaled_coords[1] = coords[1]*scale_vec[1];
      scaled_coords[2] = coords[2]*scale_vec[2];
      
      rval = mbi.create_vertex( scaled_coords, scaled_vert );
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

  //DAG = DagMC::instance( MBI );

  ErrorCode rval;
  const char* output_file_name = "test_geom.h5m";
 /* 
  // get all handles (dimension, id, sense)
  rval = mbi.tag_get_handle( NAME_TAG_NAME, NAME_TAG_SIZE, MB_TYPE_OPAQUE,
                                name_tag, MB_TAG_SPARSE|MB_TAG_CREAT );
  rval = mbi.tag_get_handle( "OBJECT_NAME", 32, MB_TYPE_OPAQUE,
                                obj_name_tag, MB_TAG_SPARSE|MB_TAG_CREAT );
  rval = mbi.tag_get_handle( GEOM_DIMENSION_TAG_NAME, 
                              1, MB_TYPE_INTEGER, 
                              dim_tag,
                              MB_TAG_SPARSE|MB_TAG_CREAT );
  CHKERR;

  rval = mbi.tag_get_handle( GLOBAL_ID_TAG_NAME, 
                              1, MB_TYPE_INTEGER, 
                              id_tag,
                              MB_TAG_DENSE|MB_TAG_CREAT );

  CHKERR;

  rval = mbi.tag_get_handle( CATEGORY_TAG_NAME, 
                              CATEGORY_TAG_SIZE,
                              MB_TYPE_OPAQUE,
                              category_tag,
                              MB_TAG_SPARSE|MB_TAG_CREAT );

  CHKERR;

  int negone = -1;
  rval = mbi.tag_get_handle(GEOM_DIMENSION_TAG_NAME,
                             1, MB_TYPE_INTEGER,
                             geom_tag,
                             MB_TAG_SPARSE|MB_TAG_CREAT, &negone);
  CHKERR;

  rval = mbi.tag_get_handle( "GEOM_SENSE_2", 
                              2, MB_TYPE_HANDLE, 
                              sense_tag,
                              MB_TAG_SPARSE|MB_TAG_CREAT );
  CHKERR;

*/

  double tmp_scale[3] = {1, 1, 1};
  double tmp_trans[3] = {0, 0, 0};
  rval = write_geometry( tmp_scale, tmp_trans );

  CHKERR; 
  
  rval = mbi.write_mesh( output_file_name );
  CHKERR;

  return 0;

}
