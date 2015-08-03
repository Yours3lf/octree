#ifndef octree_h
#define octree_h

#include "intersection.h"
#include <vector>

template< class t >
class octree
{
  static const mm::vec3 min_bv_size; //1x1x1 box
  static const int max_life_boundary; //64
  static bool is_setup;
  static octree** root_ptr; //in order to expand the octree we need to be able to modify the root node that the user has

  aabb bv; //bounding volume of this node

  //0: left-bottom-front
  //1: right-bottom-front
  //2: left-top-front
  //3: right-top-front
  //4: left-bottom-back
  //5: right-bottom-back
  //6: left-top-back
  //7: right-top-back
  //Should be able to store only one pointer, and add an offset to it, to address
  //all of the children
  vector<octree<t>*> children; //child nodes
  int life;

  std::vector<t> objects; //objects stored in this node
  octree* parent;  
  int max_lifespan;
  char active_children; //bitmask

  void expand_octree( shape* obv )
  {
    assert( is_setup );

    unsigned octant = 8; //will contain which octant was the original node

    mm::vec3 extsize = bv.get_extents() * 2; //half-size of the new root node

    //find out in which octant is the current root node
    //based on the object's bv (that is outside the current root node)
    plane octree_planes[3];
    octree_planes[0] = plane( mm::vec3( 1, 0, 0 ), bv.get_pos() + mm::vec3( bv.get_extents().x, 0, 0 ) );
    octree_planes[1] = plane( mm::vec3( 0, 1, 0 ), bv.get_pos() + mm::vec3( 0, bv.get_extents().y, 0 ) );
    octree_planes[2] = plane( mm::vec3( 0, 0, 1 ), bv.get_pos() + mm::vec3( 0, 0, bv.get_extents().z ) );

    bool is_right = obv->is_on_right_side( &octree_planes[0] ),
      is_top = obv->is_on_right_side( &octree_planes[1] ),
      is_back = obv->is_on_right_side( &octree_planes[2] );

    if( is_right )
    {
      if( is_top )
      {
        if( is_back )
        {
          octant = 7;
        }
        else
        {
          octant = 3;
        }
      }
      else
      {
        if( is_back )
        {
          octant = 5;
        }
        else
        {
          octant = 1;
        }
      }
    }
    else
    {
      if( is_top )
      {
        if( is_back )
        {
          octant = 6;
        }
        else
        {
          octant = 2;
        }
      }
      else
      {
        if( is_back )
        {
          octant = 4;
        }
        else
        {
          octant = 0;
        }
      }
    }

    auto newroot = new octree( aabb( bv.get_pos() + mm::vec3(
      ( is_right ? bv.get_extents().x : -bv.get_extents().x ),
      ( is_top ? bv.get_extents().y : -bv.get_extents().y ),
      ( is_back ? bv.get_extents().z : -bv.get_extents().z )
      ), extsize ) );
    octree<t>* oldroot = *root_ptr;
    newroot->children[octant] = this;
    newroot->active_children |= ( 1 << octant );
    *root_ptr = newroot;
    oldroot->parent = *root_ptr;
  }

  octree* get_fitting_parent( const t& o, shape* obv )
  {
    assert( is_setup );

    if( obv->is_inside( &bv ) )
    {
      return this;
    }
    else if( parent )
      return parent->get_fitting_parent( o, obv );
    else //this is the root node
      return 0;
  }

  void update_recursively()
  {
    assert( is_setup );

    if( !objects.size() )
    {
      if( !has_children() )
      {
        if( life == -1 )
          life = max_lifespan;
        else if( life > 0 )
          --life;
      }
    }
    else
    {
      if( life != -1 )
      {
        if( max_lifespan <= 64 )
          max_lifespan *= 2;

        life = -1;
      }
    }

    for( int c = 0; c < 8; ++c )
      if( is_child_active( c ) )
      children[c]->update_recursively();

    for( int c = 0; c < 8; ++c )
      if( is_child_active( c ) && !children[c]->life )
      {
        //TODO we should delete the dead branch here
        active_children ^= ( 1 << c ); //remove dead branch from octree
        children[c] = 0;
      }
  }

  bool is_child_active( unsigned c )
  {
    assert( is_setup );

    return active_children & ( 1 << c );
  }

  bool is_leaf()
  {
    assert( is_setup );

    return objects.size() == 1;
  }

  bool is_root()
  {
    assert( is_setup );

    return !parent;
  }

  bool has_children()
  {
    assert( is_setup );

    return active_children;
  }

  bool is_empty()
  {
    assert( is_setup );

    if( !objects.empty() ) //we have objects in this node
    {
      return false;
    }
    else
    {
      //check child nodes recursively
      for( int c = 0; c < 8; ++c )
      {
        if( is_child_active( c ) && !children[c]->is_empty() )
          return false;
      }

      return true;
    }
  }

public:

  void reposition_object( const t& o, shape* obv )
  {
    assert( is_setup );

    for( auto i = objects.begin(); i != objects.end(); ++i )
      if( *i == o ) //is this the one we're looking for?
      {
        if( !obv->is_inside( &bv ) ) //doesnt fit anymore, need to reposition
        {
          objects.erase( i );

          if( parent )
          {
            auto c = parent->get_fitting_parent( o, obv );
            if( c )
              c->insert( o, obv ); //try to insert it as far down as possible
            else
            {
              ( *root_ptr )->expand_octree( obv );
              ( *root_ptr )->insert( o, obv );
            }
          }
          else
          {
            ( *root_ptr )->expand_octree( obv );
            ( *root_ptr )->insert( o, obv );
          }
        }
        else
          ; //object still fits, nothing to do

        return;
      }

    //recursively check children
    for( int c = 0; c < 8; ++c )
      if( is_child_active( c ) )
      children[c]->reposition_object( o, obv );
  }

  void update( const std::vector<std::pair<t, shape*> >& objs )
  {
    assert( is_setup );

    for( auto& c : objs )
    {
      reposition_object( c.first, c.second );
    }

    ( *root_ptr )->update_recursively();

    unsigned counter = 0;
    for( int c = 0; c < 8; ++c )
      if( is_child_active( c ) ) ++counter;

    //if the root doesn't contain any objects, and the
    if( !( *root_ptr )->objects.size() && counter == 1 )
    {
      octree<t>* active_node = 0;
      for( int c = 0; c < 8; ++c )
        if( is_child_active( c ) )
        {
          active_node = children[c];
          break;
        }

      if( active_node )
      {
        //TODO delete root node after removing from tree
        *root_ptr = active_node;
        ( *root_ptr )->parent = 0;
      }
    }
  }

  bool remove( const t& o )
  {
    assert( is_setup );

    for( auto i = objects.begin(); i != objects.end(); ++i )
      if( *i == o )
      {
        objects.erase( i );
        return true;
      }

    //not found try children
    for( int c = 0; c < 8; ++c )
    {
      if( active_children & ( 1 << c ) )
        if( children[c]->remove( o ) ) //when the object is removed we don't need to check further
        return true;
    }

    return false;
  }

  bool is_in_frustum( const t& o, shape* f )
  {
    assert( is_setup );

    for( auto& c : objects )
      if( c == o ) //is this the droid you're looking for?
      {
        return true; //found the object
        /*
        if((&bv)->intersects(f)) //is it in yet?
        return 1; //in
        else
        return 0; //culled
        */
      }

    for( int c = 0; c < 8; ++c )
      if( is_child_active( c ) && ( &children[c]->bv )->is_intersecting( f ) )
      if( children[c]->is_in_frustum( o, f ) )
      return true;

    return false;
  }

  void get_culled_objects( std::vector<t>& objs, shape* f )
  {
    assert( is_setup );

    if( bv.is_intersecting( f ) )
    {
      for( auto& c : objects )
      {
        objs.push_back( c );
      }

      for( int c = 0; c < 8; ++c )
        if( is_child_active( c ) )
        children[c]->get_culled_objects( objs, f );
    }
  }

  void get_boxes( std::vector<aabb>& boxes )
  {
    assert( is_setup );

    boxes.push_back( bv );

    for( int c = 0; c < 8; ++c )
    {
      if( is_child_active( c ) )
      {
        children[c]->get_boxes( boxes );
      }
    }
  }

  void insert( const t& o, shape* obv )
  {
    assert( is_setup );

    //check if shape fits, if not the octree should be extended
    if( obv->is_inside( &bv ) )
    {
      //min node size is 1, so if the object fits, and the node has the minimum size, then insert here, and return
      //no further subdividing is allowed
      //also if the node contains less than 3 objects than insert here
      //no further subdividing is required
      if( bv.get_extents().x * 2 <= 1 || objects.size() < 3 )
      {
        objects.push_back( o );
        vector<t>(objects).swap(objects); //trim the fat
        return;
      }

      //build bvs of the octants
      mm::vec3 subsize = bv.get_extents() / 2;
      mm::vec3 basepos = bv.min + subsize;

      aabb children_bv[8];
      children_bv[0] = is_child_active( 0 ) ? children[0]->bv : aabb( basepos + mm::vec3( 0 ), subsize ); //left-bottom-front
      children_bv[1] = is_child_active( 1 ) ? children[1]->bv : aabb( basepos + mm::vec3( bv.get_extents().x, 0, 0 ), subsize ); //right-bottom-front
      children_bv[2] = is_child_active( 2 ) ? children[2]->bv : aabb( basepos + mm::vec3( 0, bv.get_extents().y, 0 ), subsize ); //left-top-front
      children_bv[3] = is_child_active( 3 ) ? children[3]->bv : aabb( basepos + mm::vec3( bv.get_extents().xy, 0 ), subsize ); //right-top-front

      children_bv[4] = is_child_active( 4 ) ? children[4]->bv : aabb( basepos + mm::vec3( 0, 0, bv.get_extents().z ), subsize ); //left-bottom-back
      children_bv[5] = is_child_active( 5 ) ? children[5]->bv : aabb( basepos + mm::vec3( bv.get_extents().x, 0, bv.get_extents().z ), subsize ); //right-bottom-back
      children_bv[6] = is_child_active( 6 ) ? children[6]->bv : aabb( basepos + mm::vec3( 0, bv.get_extents().y, bv.get_extents().z ), subsize ); //left-top-back
      children_bv[7] = is_child_active( 7 ) ? children[7]->bv : aabb( basepos + mm::vec3( bv.get_extents().xy, bv.get_extents().z ), subsize ); //right-top-back

      bool found = false;
      for( int c = 0; c < 8; ++c )
      {
        //try to fit the object into one of the octants
        if( obv->is_inside( &children_bv[c] ) )
        {
          if( !is_child_active( c ) )
          {
            children[c] = new octree<t>( children_bv[c] );

            children[c]->parent = this;
            active_children |= ( 1 << c ); //activate this node
          }

          //this will make sure we're inserting into the smallest possible octant down the tree
          children[c]->insert( o, obv ); //insert into child node (recursively)

          found = true;
        }
      }

      if( !found ) //didn't fit into any subnode
      {
        objects.push_back( o );
        vector<t>( objects ).swap( objects ); //trim the fat
      }
    }
    else
    {
      if( is_root() )
      {
        ( *root_ptr )->expand_octree( obv );
        ( *root_ptr )->insert( o, obv );
      }
      else
        ; //not a root node, stop recursion here
    }
  }

  void set_up_octree( octree** o )
  {
    root_ptr = o;
    is_setup = true;
  }

  octree( const aabb& bbvv ) : active_children( 0 ), parent( 0 ), bv( bbvv ), life( -1 ), max_lifespan( 8 )
  {
    children.resize(8);
  }

  octree() : active_children( 0 ), parent( 0 ), life( -1 ), max_lifespan( 8 )
  {
    children.resize( 8 );
  }

#if USE_MYMATH_ALLOCATOR == 1
  void* operator new( size_t s )
  {
#ifdef _WIN32
    void* m = _aligned_malloc( s, MYMATH_ALIGNMENT );
#else
    void* m = 0;

    if( posix_memalign( &m, MYMATH_ALIGNMENT, s ) )
      m = 0;
#endif

    if( !m )
      throw std::bad_alloc();

    return m;
  }

    void operator delete( void* m )
  {
#ifdef _WIN32
    _aligned_free( m );
#else
    free( p );
#endif
  }
#endif
};

template< class t >
const mm::vec3 octree<t>::min_bv_size = 1;

template< class t >
octree<t>** octree<t>::root_ptr = 0;

template< class t >
const int octree<t>::max_life_boundary = 64;

template< class t >
bool octree<t>::is_setup = false;

#endif
