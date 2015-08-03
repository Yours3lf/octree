#ifndef intersection_h
#define intersection_h

#include "mymath/mymath.h"
#include <vector>

#ifndef FLT_MAX
#define FLT_MAX 3.402823466e+38
#endif

#define INVALID (FLT_MAX)

//based on the fast, constant time multi-dispatcher design pattern
template<class lhs,
class rhs = lhs,
class ret = void,
class cbk = ret( *)( lhs, rhs )>
class dispatcher
{
  typedef std::vector<cbk> matrix;
  matrix callbacks;
  int elements;
public:
  dispatcher() : elements( 0 )
  {
  }

  void set_elements( int num )
  {
    elements = num;
    callbacks.resize( num * num );

    for( int c = 0; c < num * num; ++c )
      callbacks[c] = 0;
  }

  template<class _lhs, class _rhs>
  void add( cbk func )
  {
    int idx_lhs = _lhs::get_class_idx();
    int idx_rhs = _rhs::get_class_idx();

    callbacks[idx_lhs * elements + idx_rhs] = func;
  }

  ret go( lhs _lhs, rhs _rhs )
  {
    int idx_lhs = _lhs->get_class_index();
    int idx_rhs = _rhs->get_class_index();

    assert( idx_lhs >= 0 || idx_rhs >= 0 || idx_lhs < elements || idx_rhs < elements );
    assert( callbacks[idx_lhs * elements + idx_rhs] != 0 );

    return callbacks[idx_lhs * elements + idx_rhs]( _lhs, _rhs );
  }
};

//forward declarations
class sphere;
class plane;
class aabb;
class frustum;
class ray;
class triangle;

//generic abstract shape class
//needed so that any shape can intersect any other shape
class shape
{
  static dispatcher<shape*, shape*, bool> _is_on_right_side;
  static dispatcher<shape*, shape*, bool> _is_inside;
  static dispatcher<shape*, shape*, bool> _is_intersecting;
  static dispatcher<shape*, shape*, mm::vec2> _intersect;
  static bool is_setup;
public:
  static void set_up_intersection();

  bool is_on_right_side( shape* s )
  {
    assert( is_setup );
    return _is_on_right_side.go( this, s );
  }

  bool is_inside( shape* s )
  {
    assert( is_setup );
    return _is_inside.go( this, s );
  }

  //x: min, y: max intersection
  mm::vec2 intersect( shape* s )
  {
    assert( is_setup );
    return _intersect.go( this, s );
  }

  bool is_intersecting( shape* s )
  {
    assert( is_setup );
    return _is_intersecting.go( this, s );
  }

  virtual int get_class_index() const = 0;
};

dispatcher<shape*, shape*, bool> shape::_is_on_right_side;
dispatcher<shape*, shape*, bool> shape::_is_inside;
dispatcher<shape*, shape*, bool> shape::_is_intersecting;
dispatcher<shape*, shape*, mm::vec2> shape::_intersect;
bool shape::is_setup = false;

class MM_16_BYTE_ALIGNED ray : public shape
{
public:
  //define a ray by origin and direction
  mm::vec3 origin, direction;

  static int get_class_idx()
  {
    static int idx = 0;
    return idx;
  }

  int get_class_index() const
  {
    return get_class_idx();
  }

  ray( const mm::vec3& o = mm::vec3( 0 ), const mm::vec3& d = mm::vec3( 0, 0, -1 ) ) : origin( o ), direction( d )
  {
  }
};

class MM_16_BYTE_ALIGNED triangle : public shape
{
public:
  //define a triangle by three points
  mm::vec3 k, l, m;

  static int get_class_idx()
  {
    static int idx = 1;
    return idx;
  }

  int get_class_index() const
  {
    return get_class_idx();
  }

  triangle( const mm::vec3& kk = mm::vec3( 0 ), const mm::vec3& ll = mm::vec3( 0 ), const mm::vec3& mm = mm::vec3( 0 ) ) : k( kk ), l( ll ), m( mm )
  {
  }
};

class MM_16_BYTE_ALIGNED sphere : public shape
{
  //define a sphere by a center (xyz) and a radius (w)
  mm::vec4 data;
public:

  void set_center( const mm::vec3& c )
  {
    data = mm::vec4( c, data.w );
  }

  mm::vec3 get_center() const
  {
    return data.xyz;
  }

  void set_radius( float r )
  {
    data = mm::vec4( data.xyz, r );
  }

  float get_radius() const
  {
    return data.w;
  }

  static int get_class_idx()
  {
    static int idx = 2;
    return idx;
  }

  int get_class_index() const
  {
    return get_class_idx();
  }

  sphere( const mm::vec3& c = mm::vec3( 0 ), float r = float() ) : data( c, r )
  {
  }
};

class MM_16_BYTE_ALIGNED plane : public shape
{
  mm::vec4 data; //xyz: point on plane, w: cache -(normal dot point)
  mm::vec3 normal; //normal vector pointing out of the plane
public:

  mm::vec3 get_point() const
  {
    return data.xyz;
  }

  void set_point( const mm::vec3& p )
  {
    data = mm::vec4( p, data.w );
  }

  float get_minus_n_dot_p() const
  {
    return data.w;
  }

  void set_minus_n_dot_p( float f )
  {
    data = mm::vec4( data.xyz, f );
  }

  mm::vec3 get_normal() const
  {
    return normal;
  }

  void set_normal( const mm::vec3& n )
  {
    normal = n;
  }

  static int get_class_idx()
  {
    static int idx = 3;
    return idx;
  }

  int get_class_index() const
  {
    return get_class_idx();
  }

  //define a plane by 3 points
  void set_up( const mm::vec3& a, const mm::vec3& b, const mm::vec3& c )
  {
    mm::vec3 tmp1, tmp2;

    tmp1 = a - b;
    tmp2 = c - b;

    normal = mm::normalize( mm::cross( tmp2, tmp1 ) );
    data = mm::vec4( a, -mm::dot( normal, a ) );
  }

  //signed distance
  float distance( const mm::vec3& p )
  {
    return get_minus_n_dot_p() + mm::dot( normal, p );
  }

  //define a plane by a normal and a point
  plane( const mm::vec3& n = mm::vec3( 0 ), const mm::vec3& p = mm::vec3( 0 ) ) : normal( n )
  {
    data = mm::vec4( p, -mm::dot( n, p ) );
  }

  plane( const mm::vec3& a, const mm::vec3& b, const mm::vec3& c )
  {
    set_up( a, b, c );
  }
};

class MM_16_BYTE_ALIGNED aabb : public shape
{
public:
  mm::vec3 min, max; //minimum/maximum apex of the aabb

  mm::vec3 get_extents() const
  {
    return mm::abs( max - min ) * 0.5;
  }

  mm::vec3 get_pos() const
  {
    return min + get_extents();
  }

  static int get_class_idx()
  {
    static int idx = 4;
    return idx;
  }

  int get_class_index() const
  {
    return get_class_idx();
  }

  //returns the vertices of the triangles of the aabb in counter clockwise order
  void get_vertices( std::vector<mm::vec3>& v ) const
  {
    //left
    v.push_back( mm::vec3( min.x, max.yz ) );
    v.push_back( mm::vec3( min.x, max.y, min.z ) );
    v.push_back( mm::vec3( min.xyz ) );

    v.push_back( mm::vec3( min.xyz ) );
    v.push_back( mm::vec3( min.xy, max.z ) );
    v.push_back( mm::vec3( min.x, max.yz ) );

    //front
    v.push_back( mm::vec3( min.xy, max.z ) );
    v.push_back( mm::vec3( max.x, min.y, max.z ) );
    v.push_back( mm::vec3( max.xyz ) );

    v.push_back( mm::vec3( max.xyz ) );
    v.push_back( mm::vec3( min.x, max.yz ) );
    v.push_back( mm::vec3( min.xy, max.z ) );

    //right
    v.push_back( mm::vec3( max.xy, min.z ) );
    v.push_back( mm::vec3( max.xyz ) );
    v.push_back( mm::vec3( max.x, min.y, max.z ) );

    v.push_back( mm::vec3( max.x, min.y, max.z ) );
    v.push_back( mm::vec3( max.x, min.yz ) );
    v.push_back( mm::vec3( max.xy, min.z ) );

    //back
    v.push_back( mm::vec3( max.xy, min.z ) );
    v.push_back( mm::vec3( max.x, min.yz ) );
    v.push_back( mm::vec3( min.xyz ) );

    v.push_back( mm::vec3( min.xyz ) );
    v.push_back( mm::vec3( min.x, max.y, min.z ) );
    v.push_back( mm::vec3( max.xy, min.z ) );

    //top
    v.push_back( mm::vec3( min.x, max.y, min.z ) );
    v.push_back( mm::vec3( min.x, max.yz ) );
    v.push_back( mm::vec3( max.xyz ) );

    v.push_back( mm::vec3( max.xyz ) );
    v.push_back( mm::vec3( max.xy, min.z ) );
    v.push_back( mm::vec3( min.x, max.y, min.z ) );

    //bottom
    v.push_back( mm::vec3( max.x, min.y, max.z ) );
    v.push_back( mm::vec3( min.xy, max.z ) );
    v.push_back( mm::vec3( min.xyz ) );

    v.push_back( mm::vec3( min.xyz ) );
    v.push_back( mm::vec3( max.x, min.yz ) );
    v.push_back( mm::vec3( max.x, min.y, max.z ) );
  }

  mm::vec3 get_pos_vertex( const mm::vec3& n ) const
  {
    mm::vec3 res = min;

    if( n.x >= 0 )
      res.x = max.x;

    if( n.y >= 0 )
      res.y = max.y;

    if( n.z >= 0 )
      res.z = max.z;

    return res;
  }

  void expand( const mm::vec3& p )
  {
    min = mm::min( min, p );
    max = mm::max( max, p );
  }

  mm::vec3 get_neg_vertex( const mm::vec3& n ) const
  {
    mm::vec3 res = max;

    if( n.x >= 0 )
      res.x = min.x;

    if( n.y >= 0 )
      res.y = min.y;

    if( n.z >= 0 )
      res.z = min.z;

    return res;
  }

  void reset_minmax()
  {
    min = mm::vec3( FLT_MAX );
    max = mm::vec3( -FLT_MAX );
  }

  aabb()
  {
    reset_minmax();
  }

  aabb( const mm::vec3& p, const mm::vec3& e )
  {
    min = p - e;
    max = p + e;
  }
};

//haxx
#ifdef _WIN32
#undef FAR
#endif

class MM_16_BYTE_ALIGNED frustum : public shape
{
public:
  plane planes[6];
  mm::vec3 points[8];

  enum which_plane
  {
    TOP = 0, BOTTOM, LEFT, RIGHT, NEAR, FAR
  };

  enum which_point
  {
    NTL = 0, NTR, NBL, NBR, FTL, FTR, FBL, FBR
  };

  static int get_class_idx()
  {
    static int idx = 5;
    return idx;
  }

  int get_class_index() const
  {
    return get_class_idx();
  }

  frustum()
  {
  }

  void set_up( const mm::camera<float>& cam, const mm::frame<float>& f )
  {
    mm::vec3 nc = cam.pos - cam.view_dir * f.near_ll.z;
    mm::vec3 fc = cam.pos - cam.view_dir * f.far_ll.z;

    mm::vec3 right = -mm::normalize( mm::cross( cam.up_vector, cam.view_dir ) );

    float nw = f.near_lr.x - f.near_ll.x;
    float nh = f.near_ul.y - f.near_ll.y;

    float fw = f.far_lr.x - f.far_ll.x;
    float fh = f.far_ul.y - f.far_ll.y;

    //near top left
    mm::vec3 ntl = nc + cam.up_vector * nh - right * nw;
    mm::vec3 ntr = nc + cam.up_vector * nh + right * nw;
    mm::vec3 nbl = nc - cam.up_vector * nh - right * nw;
    mm::vec3 nbr = nc - cam.up_vector * nh + right * nw;

    mm::vec3 ftl = fc + cam.up_vector * fh - right * fw;
    mm::vec3 ftr = fc + cam.up_vector * fh + right * fw;
    mm::vec3 fbl = fc - cam.up_vector * fh - right * fw;
    mm::vec3 fbr = fc - cam.up_vector * fh + right * fw;

    points[NTL] = ntl;
    points[NTR] = ntr;
    points[NBL] = nbl;
    points[NBR] = nbr;

    points[FTL] = ftl;
    points[FTR] = ftr;
    points[FBL] = fbl;
    points[FBR] = fbr;

    planes[TOP].set_up( ntr, ntl, ftl );
    planes[BOTTOM].set_up( nbl, nbr, fbr );
    planes[LEFT].set_up( ntl, nbl, fbl );
    planes[RIGHT].set_up( nbr, ntr, fbr );
    planes[NEAR].set_up( ntl, ntr, nbr );
    planes[FAR].set_up( ftr, ftl, fbl );
  }

  void get_vertices( std::vector<mm::vec3>& v ) const
  {
    //top
    v.push_back( points[NTL] );
    v.push_back( points[NTR] );
    v.push_back( points[FTR] );

    v.push_back( points[NTL] );
    v.push_back( points[FTR] );
    v.push_back( points[FTL] );

    //bottom
    v.push_back( points[NBL] );
    v.push_back( points[FBL] );
    v.push_back( points[FBR] );

    v.push_back( points[NBL] );
    v.push_back( points[FBR] );
    v.push_back( points[NBR] );

    //left
    v.push_back( points[NTL] );
    v.push_back( points[FTL] );
    v.push_back( points[NBL] );

    v.push_back( points[NBL] );
    v.push_back( points[FTL] );
    v.push_back( points[FBL] );

    //right
    v.push_back( points[NTR] );
    v.push_back( points[NBR] );
    v.push_back( points[FTR] );

    v.push_back( points[NBR] );
    v.push_back( points[FBR] );
    v.push_back( points[FTR] );

    //near
    v.push_back( points[NBL] );
    v.push_back( points[NTR] );
    v.push_back( points[NTL] );

    v.push_back( points[NBL] );
    v.push_back( points[NBR] );
    v.push_back( points[NTR] );

    //far
    v.push_back( points[FTL] );
    v.push_back( points[FTR] );
    v.push_back( points[FBR] );

    v.push_back( points[FTR] );
    v.push_back( points[FBR] );
    v.push_back( points[FBL] );
  }
};

namespace inner
{
  //only tells if the sphere is on the right side of the plane!
  static bool is_on_right_side_sp( shape* aa, shape* bb )
  {
    auto a = static_cast<sphere*>( aa );
    auto b = static_cast<plane*>( bb );

    float dist = b->distance( a->get_center() );
    //dist + radius == how far is the sphere from the plane (usable when we want to do lod using the near plane)

    if( dist < -a->get_radius() )
      return false;

    return true;
  }

  static bool is_on_right_side_ps( shape* aa, shape* bb )
  {
    return is_on_right_side_sp( bb, aa );
  }

  //only tells if the sphere is on the right side of the plane!
  static bool is_on_right_side_ap( shape* aa, shape* bb )
  {
    auto a = static_cast<aabb*>( aa );
    auto b = static_cast<plane*>( bb );

    if( b->distance( a->get_pos_vertex( b->get_normal() ) ) < 0 )
      return false;

    return true;
  }

  static bool is_on_right_side_pa( shape* aa, shape* bb )
  {
    return is_on_right_side_ap( bb, aa );
  }

  static bool is_intersecting_ss( shape* aa, shape* bb )
  {
    auto a = static_cast<sphere*>( aa );
    auto b = static_cast<sphere*>( bb );

    mm::vec3 diff = a->get_center() - b->get_center();
    float dist = mm::dot( diff, diff );

    float rad_sum = b->get_radius() + a->get_radius();

    if( dist > rad_sum * rad_sum ) //squared distance check
      return false;

    return true;
  }

  //checks if a sphere intersects a plane
  static bool is_intersecting_sp( shape* aa, shape* bb )
  {
    auto a = static_cast<sphere*>( aa );
    auto b = static_cast<plane*>( bb );

    float dist = b->distance( a->get_center() );

    if( abs( dist ) <= a->get_radius() )
      return true;

    return false;
  }

  static bool is_intersecting_ps( shape* aa, shape* bb )
  {
    return is_intersecting_sp( bb, aa );
  }

  static bool is_intersecting_pp( shape* aa, shape* bb )
  {
    auto a = static_cast<plane*>( aa );
    auto b = static_cast<plane*>( bb );

    mm::vec3 vector = mm::cross( a->get_normal(), b->get_normal() );

    //if the cross product yields a null vector
    //then the angle is either 0 or 180
    //that is the two normals are parallel
    // sin(alpha) = 0
    // ==> |a| * |b| * sin(alpha) = 0
    if( mm::all( mm::equal( vector, mm::vec3( 0 ) ) ) )
      return false;

    return true;
  }

  static bool is_intersecting_aa( shape* aa, shape* bb )
  {
    auto a = static_cast<aabb*>( aa );
    auto b = static_cast<aabb*>( bb );

    mm::vec3 t = b->get_pos() - a->get_pos();

    mm::vec3 aextent = a->get_extents();
    mm::vec3 bextent = b->get_extents();

    if( abs( t.x ) > ( aextent.x + bextent.x ) )
      return false;

    if( abs( t.y ) > ( aextent.y + bextent.y ) )
      return false;

    if( abs( t.z ) > ( aextent.z + bextent.z ) )
      return false;

    return true;
  }

  static bool is_intersecting_as( shape* aa, shape* bb )
  {
    auto a = static_cast<aabb*>( aa );
    auto b = static_cast<sphere*>( bb );

    //square distance check between spheres and aabbs
    mm::vec3 vec = b->get_center() - mm::clamp( a->get_pos() + ( b->get_center() - a->get_pos() ), a->min, a->max );

    float sqlength = mm::dot( vec, vec );

    if( sqlength > b->get_radius() * b->get_radius() )
      return false;

    return true;
  }

  static bool is_intersecting_sa( shape* aa, shape* bb )
  {
    return is_intersecting_as( bb, aa );
  }

  static bool is_intersecting_ap( shape* aa, shape* bb )
  {
    auto a = static_cast<aabb*>( aa );
    auto b = static_cast<plane*>( bb );

    mm::vec3 p = a->get_pos_vertex( b->get_normal() );
    mm::vec3 n = a->get_neg_vertex( b->get_normal() );

    float dist_p = b->distance( p );
    float dist_n = b->distance( n );

    if( ( dist_n > 0 && dist_p > 0 ) ||
      ( dist_n < 0 && dist_p < 0 ) )
      return false;

    return true;
  }

  static bool is_intersecting_pa( shape* aa, shape* bb )
  {
    return is_intersecting_ap( bb, aa );
  }

  static bool is_intersecting_fs( shape* aa, shape* bb )
  {
    auto a = static_cast<frustum*>( aa );

    for( int c = 0; c < 6; ++c )
    {
      if( !is_on_right_side_ps( &a->planes[c], bb ) )
        return false;
    }

    return true;
  }

  static bool is_intersecting_sf( shape* aa, shape* bb )
  {
    return is_intersecting_fs( bb, aa );
  }

  static bool is_intersecting_fa( shape* aa, shape* bb )
  {
    auto a = static_cast<frustum*>( aa );
    auto b = static_cast<aabb*>( bb );

    bool res = true;
    for( int c = 0; c < 6; ++c )
    {
      if( !bb->is_on_right_side( &a->planes[c] ) )
      {
        res = false;
        break;
      }
    }
    return res;
  }

  static bool is_intersecting_af( shape* aa, shape* bb )
  {
    return is_intersecting_fa( bb, aa );
  }

  //is a inside b?
  static bool is_inside_sa( shape* aa, shape* bb )
  {
    auto a = static_cast<sphere*>( aa );
    auto b = static_cast<aabb*>( bb );

    mm::vec3 spheremax = a->get_center() + a->get_radius();
    mm::vec3 spheremin = a->get_center() - a->get_radius();

    if( mm::all( mm::lessThanEqual( spheremax, b->max ) ) && mm::all( mm::greaterThanEqual( spheremin, b->min ) ) )
      return true;

    return false;
  }

  //is a inside b?
  static bool is_inside_as( shape* aa, shape* bb )
  {
    auto a = static_cast<aabb*>( aa );
    auto b = static_cast<sphere*>( bb );

    mm::vec3 minvec = a->min - b->get_center();
    mm::vec3 maxvec = a->max - b->get_center();
    float sqrmaxlength = mm::dot( maxvec, maxvec );
    float sqrminlength = mm::dot( minvec, minvec );
    float sqrradius = b->get_radius() * b->get_radius();

    if( sqrmaxlength <= sqrradius && sqrminlength <= sqrradius )
      return true;

    return false;
  }

  //is a inside b?
  static bool is_inside_aa( shape* aa, shape* bb )
  {
    auto a = static_cast<aabb*>( aa );
    auto b = static_cast<aabb*>( bb );

    if( mm::all( mm::greaterThanEqual( a->min, b->min ) ) && mm::all( mm::lessThanEqual( a->max, b->max ) ) )
      return true;

    return false;
  }

  //is a inside b?
  static bool is_inside_ss( shape* aa, shape* bb )
  {
    auto a = static_cast<sphere*>( aa );
    auto b = static_cast<sphere*>( bb );

    mm::vec3 spheredist = b->get_center() - a->get_center();

    if( mm::dot( spheredist, spheredist ) <= b->get_radius() * b->get_radius() )
      return true;

    return false;
  }

  static bool is_intersecting_rt( shape* aa, shape* bb )
  {
    auto a = static_cast<ray*>( aa );
    auto b = static_cast<triangle*>( bb );

    mm::vec3 e = b->k - b->m;
    mm::vec3 f = b->l - b->m;

    mm::vec3 g = a->origin - b->m;

    //apply barycentric triangle math
    float t = 1.0f / mm::dot( mm::cross( a->direction, f ), e );
    mm::vec3 tkl = t * mm::vec3( mm::dot( mm::cross( g, e ), f ),
      mm::dot( mm::cross( a->direction, f ), g ),
      mm::dot( mm::cross( g, e ), a->direction ) );

    //barycentric coordinate check
    //if between 0...1 the point is inside
    return tkl.y > 0 && tkl.z > 0 && ( tkl.y + tkl.z ) < 1;
  }

  static mm::vec2 intersect_rt( shape* aa, shape* bb )
  {
    auto a = static_cast<ray*>( aa );
    auto b = static_cast<triangle*>( bb );

    //klm
    //TODO http://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution
  }

  static bool is_intersecting_tr( shape* aa, shape* bb )
  {
    return is_intersecting_rt( bb, aa );
  }

  static mm::vec2 intersect_tr( shape* aa, shape* bb )
  {
    return intersect_rt( bb, aa );
  }

  static bool is_intersecting_rs( shape* aa, shape* bb )
  {
    auto a = static_cast<ray*>( aa );
    auto b = static_cast<sphere*>( bb );

    mm::vec3 diff = b->get_center() - a->origin;
    float dist = mm::dot( diff, diff ) - ( b->get_radius() * b->get_radius() );

    if( dist <= 0 )
      return true;

    float dist2 = mm::dot( a->direction, diff );

    if( dist2 >= 0 )
      return false;

    return ( dist2 * dist2 - dist ) >= 0;
  }

  static mm::vec2 intersect_rs( shape* aa, shape* bb )
  { //we assume ray dir is a unit vector
    auto a = static_cast<ray*>( aa );
    auto b = static_cast<sphere*>( bb );

    mm::vec3 x = a->origin - b->get_center();

    float bbb = mm::dot( a->direction, x ) * 2;
    float ccc = mm::dot( x, x ) - b->get_radius() * b->get_radius();

    float sqr = bbb * bbb - 4 * ccc;

    if( sqr > 0 )
    {
      float root = std::sqrt( sqr );
      float inv_2a = 0.5f;

      float t1 = ( -bbb - root ) * inv_2a;
      float t2 = ( -bbb + root ) * inv_2a;

      if( t1 < 0 )
      {
        if( t2 < 0 )
        {
          return INVALID;
        }
        else
        {
          return mm::vec2( t2, t1 );
        }
      }
      else
      {
        return mm::vec2( t1, t2 );
      }
    }

    return INVALID;
  }

  static bool is_intersecting_sr( shape* aa, shape* bb )
  {
    return is_intersecting_rs( bb, aa );
  }

  static mm::vec2 intersect_sr( shape* aa, shape* bb )
  {
    return intersect_rs( bb, aa );
  }

  static bool is_intersecting_ra( shape* aa, shape* bb )
  {
    aabb* ab = static_cast<aabb*>( bb );
    ray* r = static_cast<ray*>( aa );
    mm::vec3 invR;

    // compute intersection of ray with all six bbox planes
#ifdef _DEBUG
    //in debug mode, pay attention to asserts
    for( int c = 0; c < 3; ++c )
    {
      if( mm::impl::is_eq( r->direction[c], 0 ) )
      {
        invR[c] = FLT_MAX;
      }
      else
      {
        invR[c] = 1.0f / r->direction[c];
      }
    }
#else
    //in release mode we dgaf about div by zero
    invR = 1.0f / r->direction;
#endif

    mm::vec3 tbot = invR * ( ab->min - r->origin );
    mm::vec3 ttop = invR * ( ab->max - r->origin );

    // re-order intersections to find smallest and largest on each axis
    mm::vec3 tmin = mm::min( ttop, tbot );
    mm::vec3 tmax = mm::max( ttop, tbot );

    // find the largest tmin and the smallest tmax
    float largest_tmin = mm::max( mm::max( tmin.x, tmin.y ), mm::max( tmin.x, tmin.z ) );
    float smallest_tmax = mm::min( mm::min( tmax.x, tmax.y ), mm::min( tmax.x, tmax.z ) );

    return smallest_tmax > largest_tmin ? true : false;
  }

  static mm::vec2 intersect_ra( shape* aa, shape* bb )
  {
    aabb* ab = static_cast<aabb*>( bb );
    ray* r = static_cast<ray*>( aa );
    mm::vec3 invR;

    // compute intersection of ray with all six bbox planes
#ifdef _DEBUG
    //in debug mode, pay attention to asserts
    for( int c = 0; c < 3; ++c )
    {
      if( mm::impl::is_eq( r->direction[c], 0 ) )
      {
        invR[c] = FLT_MAX;
      }
      else
      {
        invR[c] = 1.0f / r->direction[c];
      }
    }
#else
    //in release mode we dgaf about div by zero
    invR = 1.0f / r->direction;
#endif

    mm::vec3 tbot = invR * ( ab->min - r->origin );
    mm::vec3 ttop = invR * ( ab->max - r->origin );

    // re-order intersections to find smallest and largest on each axis
    mm::vec3 tmin = mm::min( ttop, tbot );
    mm::vec3 tmax = mm::max( ttop, tbot );

    // find the largest tmin and the smallest tmax
    float largest_tmin = mm::max( mm::max( tmin.x, tmin.y ), mm::max( tmin.x, tmin.z ) );
    float smallest_tmax = mm::min( mm::min( tmax.x, tmax.y ), mm::min( tmax.x, tmax.z ) );

    return smallest_tmax > largest_tmin ? ( largest_tmin >= 0 ? mm::vec2( largest_tmin, smallest_tmax ) : mm::vec2( smallest_tmax, largest_tmin ) ) : INVALID;
  }

  static bool is_intersecting_ar( shape* aa, shape* bb )
  {
    return is_intersecting_ra( bb, aa );
  }

  static mm::vec2 intersect_ar( shape* aa, shape* bb )
  {
    return intersect_ra( bb, aa );
  }

  static bool is_intersecting_rp( shape* aa, shape* bb )
  {
    auto a = static_cast<ray*>( aa );
    auto b = static_cast<plane*>( bb );

    float denom = mm::dot( a->direction, b->get_normal() );

    if( !mm::impl::is_eq( denom, 0.0f ) )
    {
      float t = mm::dot( b->get_point() - a->origin, b->get_normal() );
      float tmp = t * denom;

      if( tmp > 0.0f )
      {
        return true;
      }
    }

    return false;
  }

  static mm::vec2 intersect_rp( shape* aa, shape* bb )
  {
    auto a = static_cast<ray*>( aa );
    auto b = static_cast<plane*>( bb );

    float denom = mm::dot( a->direction, b->get_normal() );

    if( !mm::impl::is_eq( denom, 0.0f ) )
    {
      float t = mm::dot( b->get_point() - a->origin, b->get_normal() );
      float tmp = t * denom;

      if( tmp > 0.0f )
      {
        return t / denom;
      }
    }

    return INVALID;
  }

  static bool is_intersecting_pr( shape* aa, shape* bb )
  {
    return is_intersecting_rp( bb, aa );
  }

  static mm::vec2 intersect_pr( shape* aa, shape* bb )
  {
    return intersect_rp( bb, aa );
  }
}

void shape::set_up_intersection()
{
  //order doesnt matter
  _is_on_right_side.set_elements( 6 );
  _is_on_right_side.add<sphere, plane>( inner::is_on_right_side_sp );
  _is_on_right_side.add<aabb, plane>( inner::is_on_right_side_ap );
  _is_on_right_side.add<plane, sphere>( inner::is_on_right_side_ps );
  _is_on_right_side.add<plane, aabb>( inner::is_on_right_side_pa );

  /////////////
  _is_intersecting.set_elements( 6 );
  _is_intersecting.add<aabb, aabb>( inner::is_intersecting_aa );
  _is_intersecting.add<aabb, sphere>( inner::is_intersecting_as );
  _is_intersecting.add<aabb, ray>( inner::is_intersecting_ar );
  _is_intersecting.add<aabb, frustum>( inner::is_intersecting_af );
  _is_intersecting.add<aabb, plane>( inner::is_intersecting_ap );

  _is_intersecting.add<plane, aabb>( inner::is_intersecting_pa );
  _is_intersecting.add<plane, sphere>( inner::is_intersecting_ps );
  _is_intersecting.add<plane, ray>( inner::is_intersecting_pr );
  _is_intersecting.add<plane, plane>( inner::is_intersecting_pp );

  _is_intersecting.add<sphere, aabb>( inner::is_intersecting_sa );
  _is_intersecting.add<sphere, sphere>( inner::is_intersecting_ss );
  _is_intersecting.add<sphere, ray>( inner::is_intersecting_sr );
  _is_intersecting.add<sphere, frustum>( inner::is_intersecting_sf );
  _is_intersecting.add<sphere, plane>( inner::is_intersecting_sp );

  _is_intersecting.add<frustum, aabb>( inner::is_intersecting_fa );
  _is_intersecting.add<frustum, sphere>( inner::is_intersecting_fs );
  //_is_intersecting.add<frustum, frustum>( inner::is_intersecting_ff ); //TODO

  _is_intersecting.add<ray, aabb>( inner::is_intersecting_ra );
  _is_intersecting.add<ray, sphere>( inner::is_intersecting_rs );
  _is_intersecting.add<ray, triangle>( inner::is_intersecting_rt );
  _is_intersecting.add<ray, plane>( inner::is_intersecting_rp );

  _is_intersecting.add<triangle, ray>( inner::is_intersecting_tr );

  //order matters
  _is_inside.set_elements( 6 );
  _is_inside.add<aabb, aabb>( inner::is_inside_aa );
  _is_inside.add<aabb, sphere>( inner::is_inside_as );
  _is_inside.add<sphere, aabb>( inner::is_inside_sa );
  _is_inside.add<sphere, sphere>( inner::is_inside_ss );

  _intersect.set_elements( 6 );
  _intersect.add<aabb, ray>( inner::intersect_ar );
  _intersect.add<ray, aabb>( inner::intersect_ra );
  _intersect.add<plane, ray>( inner::intersect_pr );
  _intersect.add<ray, plane>( inner::intersect_rp );
  _intersect.add<sphere, ray>( inner::intersect_sr );
  _intersect.add<ray, sphere>( inner::intersect_rs );
  //_intersect.add<triangle, ray>( inner::intersect_tr ); //TODO
  //_intersect.add<triangle, ray>( inner::intersect_rt );

  //usage
  //is_on_the_right_side.go(new aabb(), new sphere());

  is_setup = true;
}

#endif
