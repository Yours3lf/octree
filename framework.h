#ifndef framework_h
#define framework_h

#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <mymath/mymath.h>

#ifdef USE_CL

#include <SFML/OpenGL.hpp>
#include <CL/cl.h>
#include <CL/cl_gl.h>

#if defined (__APPLE__) || defined(MACOSX)
#define

_SHARING_EXTENSION "cl_APPLE_gl_sharing"
#else
#define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
#endif

#endif

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <streambuf>
#include <list>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

#ifdef _WIN32
#include <Windows.h>
#endif
#undef NEAR
#undef near
#undef far

using namespace mymath;
using namespace std;

#define MAX_SOURCE_SIZE (0x100000)
#define INFOLOG_SIZE 4096
#define GET_INFOLOG_SIZE INFOLOG_SIZE - 1
#define STRINGIFY(s) #s

#include "intersection.h"

namespace prototyper
{
  class framework
  {
    sf::Window the_window;
    sf::Event the_event;

#ifdef USE_CL
    cl_device_id device;
    cl_uint uiNumDevsUsed;
#endif

    bool run;

    static void shader_include( std::string& text, const std::string& path )
    {
      size_t start_pos = 0;
      std::string include_dir = "#include ";

      while( ( start_pos = text.find( include_dir, start_pos ) ) != std::string::npos )
      {
        int pos = start_pos + include_dir.length() + 1;
        int length = text.find( "\"", pos );
        std::string filename = text.substr( pos, length - pos );
        std::string content = "";

        std::ifstream f;
        f.open( ( path + filename ).c_str() );

        if( f.is_open() )
        {
          content = std::string( ( std::istreambuf_iterator<char>( f ) ),
            std::istreambuf_iterator<char>() );
        }
        else
        {
          cerr << "Couldn't include shader file: " << filename << endl;
          return;
        }

        text.replace( start_pos, ( length + 1 ) - start_pos, content );
        start_pos += content.length();
      }
    }


    void compile_shader( const char* text, const GLuint& program, const GLenum& type, const std::string& additional_str ) const
    {
      GLchar infolog[INFOLOG_SIZE];

      GLuint id = glCreateShader( type );
      std::string str = text;
      str = additional_str + str;
      const char* c = str.c_str();
      glShaderSource( id, 1, &c, 0 );
      glCompileShader( id );

      GLint success;
      glGetShaderiv( id, GL_COMPILE_STATUS, &success );

      if( !success )
      {
        glGetShaderInfoLog( id, GET_INFOLOG_SIZE, 0, infolog );
        cerr << infolog << endl;
      }
      else
      {
        glAttachShader( program, id );
        glDeleteShader( id );
      }
    }

    void link_shader( const GLuint& shader_program ) const
    {
      glLinkProgram( shader_program );

      GLint success;
      glGetProgramiv( shader_program, GL_LINK_STATUS, &success );

      if( !success )
      {
        GLchar infolog[INFOLOG_SIZE];
        glGetProgramInfoLog( shader_program, GET_INFOLOG_SIZE, 0, infolog );
        cout << infolog << endl;
      }

      glValidateProgram( shader_program );

      glGetProgramiv( shader_program, GL_VALIDATE_STATUS, &success );

      if( !success )
      {
        GLchar infolog[INFOLOG_SIZE];
        glGetProgramInfoLog( shader_program, GET_INFOLOG_SIZE, 0, infolog );
        cout << infolog << endl;
      }
    }

#ifdef USE_CL
    int is_extension_supported( const char* support_str, const char* ext_string, size_t ext_buffer_size )
    {
      size_t offset = 0;
      const char* space_substr = strstr( ext_string + offset, " "/*, ext_buffer_size - offset*/ );
      size_t space_pos = space_substr ? space_substr - ext_string : 0;
      while( space_pos < ext_buffer_size )
      {
        if( strncmp( support_str, ext_string + offset, space_pos ) == 0 )
        {
          // Device supports requested extension!
          cout << "Info: Found extension support " << support_str << endl;
          return 1;
        }
        // Keep searching -- skip to next token string
        offset = space_pos + 1;
        space_substr = strstr( ext_string + offset, " "/*, ext_buffer_size - offset*/ );
        space_pos = space_substr ? space_substr - ext_string : 0;
      }
      cerr << "Warning: Extension not supported " << support_str << endl;
      return 0;
    }
#endif

#ifdef _WIN32
    char* realpath( const char* path, char** ret )
    {
      char* the_ret = 0;

      if( !ret )
      {
        the_ret = new char[MAX_PATH];
      }
      else
      {
        if( !*ret )
        {
          *ret = new char[MAX_PATH];
        }
        else
        {
          unsigned long s = strlen( *ret );

          if( s < MAX_PATH )
          {
            delete[] * ret;
            *ret = new char[MAX_PATH];

          }

          the_ret = *ret;
        }
      }

      unsigned long size = GetFullPathNameA( path, MAX_PATH, the_ret, 0 );

      if( size > MAX_PATH )
      {
        //too long path
        cerr << "Path too long, truncated." << endl;
        delete[] the_ret;
        return "";
      }

      if( ret )
      {
        *ret = the_ret;
      }

      return the_ret;
    }
#endif

  public:

#ifdef USE_CL
    cl_command_queue cl_cq;
    cl_context cl_GPU_context;
    cl_int cl_err_num;
#endif

    void set_mouse_visibility( bool vis )
    {
      the_window.setMouseCursorVisible( vis );
    }

    uvec2 get_window_size()
    {
      return uvec2( the_window.getSize().x, the_window.getSize().y );
    }

    ivec2 get_window_pos()
    {
      return ivec2( the_window.getPosition().x, the_window.getPosition().y );
    }

    void show_cursor( bool show )
    {
      the_window.setMouseCursorVisible( show );
    }

    void* get_window_handle()
    {
      return the_window.getSystemHandle();
    }

    void resize( int x, int y, unsigned w, unsigned h )
    {
      the_window.setPosition( sf::Vector2<int>( x, y ) );
      the_window.setSize( sf::Vector2<unsigned>( w, h ) );
    }

    void set_title( const string& str )
    {
      the_window.setTitle( str );
    }

    void set_mouse_pos( const ivec2& xy )
    {
      sf::Mouse::setPosition( sf::Vector2i( xy.x, xy.y ), the_window );
    }

    float get_random_num( float min, float max )
    {
      return min + ( max - min ) * (float)rand() / (float)RAND_MAX; //min...max
    }

    void init( const uvec2& screen = uvec2( 1280, 720 ), const string& title = "", const bool& fullscreen = false )
    {
#ifdef REDIRECT
      //Redirect the STD output
      FILE* file_stream = 0;

      file_stream = freopen( "stdout.txt", "w", stdout );

      if( !file_stream )
      {
        cerr << "Error rerouting the standard output (std::cout)!" << endl;
      }

      file_stream = freopen( "stderr.txt", "w", stderr );

      if( !file_stream )
      {
        cerr << "Error rerouting the standard output (std::cerr)!" << endl;
      }

#endif

      run = true;

      srand( time( 0 ) );

      unsigned x = screen.x > 0 ? screen.x : 1280;
      unsigned y = screen.y > 0 ? screen.y : 720;
      auto vm = sf::VideoMode( x, y, 32 );
      auto st = fullscreen ? sf::Style::Fullscreen : sf::Style::Default;
      the_window.create( vm, title, st );

      if( !the_window.isOpen() )
      {
        cerr << "Couldn't initialize SFML." << endl;
        the_window.close();
        return;
      }

      the_window.setPosition( sf::Vector2i( 0, 0 ) );

      GLenum glew_error = glewInit();

      glGetError(); //ignore glew errors

      cout << "Vendor: " << glGetString( GL_VENDOR ) << endl;
      cout << "Renderer: " << glGetString( GL_RENDERER ) << endl;
      cout << "OpenGL version: " << glGetString( GL_VERSION ) << endl;
      cout << "GLSL version: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << endl;

      if( glew_error != GLEW_OK )
      {
        cerr << "Error initializing GLEW: " << glewGetErrorString( glew_error ) << endl;
        the_window.close();
        return;
      }

      if( !GLEW_VERSION_4_3 )
      {
        cerr << "Error: " << STRINGIFY( GLEW_VERSION_4_3 ) << " is required" << endl;
        the_window.close();
        exit( 0 );
      }

      glEnable( GL_DEBUG_OUTPUT );
    }

#ifdef USE_CL
    void initCL()
    {
      cl_platform_id cp_platform;

      cl_uint num_platforms = 0;
      cl_err_num = clGetPlatformIDs( 0, NULL, &num_platforms );
      if( cl_err_num != CL_SUCCESS )
      {
        cout << " Error in clGetPlatformIDs call!" << endl;
        get_opencl_error( cl_err_num );
        return;
      }
      if( num_platforms == 0 )
      {
        cout << "No OpenCL platform found!" << endl;
        return;
      }

      cl_platform_id *cl_platforms = new cl_platform_id[num_platforms];

      // get platform info for each platform
      cl_err_num = clGetPlatformIDs( num_platforms, cl_platforms, NULL );
      char ch_buffer[1024];
      bool found_dev = false;
      for( cl_uint i = 0; i < num_platforms; ++i )
      {
        cl_err_num = clGetPlatformInfo( cl_platforms[i], CL_PLATFORM_PROFILE, 1024, &ch_buffer, NULL );
        if( cl_err_num == CL_SUCCESS )
        {
          if( strstr( ch_buffer, "FULL_PROFILE" ) != NULL )
          {
            cp_platform = cl_platforms[i];
            cl_err_num = clGetPlatformInfo( cl_platforms[i], CL_PLATFORM_NAME, 1024, &ch_buffer, NULL );

            // Get the number of GPU devices available to the platform
            cl_uint dev_count = 0;
            cl_err_num = clGetDeviceIDs( cp_platform, CL_DEVICE_TYPE_GPU, 0, NULL, &dev_count );
            if( cl_err_num == CL_SUCCESS )
            {
              if( dev_count > 0 )
              {
                // Create the device list
                cl_device_id *cl_devices = new cl_device_id[dev_count];
                cl_err_num = clGetDeviceIDs( cp_platform, CL_DEVICE_TYPE_GPU, dev_count, cl_devices, NULL );
                for( int j = 0; j < dev_count; j++ )
                {
                  // Get string containing supported device extensions
                  size_t ext_size = 1024;
                  char ext_string[1024];
                  cl_err_num = clGetDeviceInfo( cl_devices[j], CL_DEVICE_EXTENSIONS, ext_size, ext_string, &ext_size );
                  // Search for GL support in extension string (space delimited)
                  int supported = is_extension_supported( GL_SHARING_EXTENSION, ext_string, ext_size );
                  if( cl_err_num == CL_SUCCESS && supported )
                  {
                    // Device supports context sharing with OpenGL
                    cl_err_num = clGetDeviceInfo( cl_devices[j], CL_DEVICE_NAME, ext_size, ext_string, &ext_size );
                    cout << "Found OpenCL platform: " << ch_buffer << endl;
                    cout << "Found GL Sharing Support: " << ext_string << endl;
                    device = cl_devices[j];
                    found_dev = true;
                    break;
                  }
                }
                delete[] cl_devices;
              }
            }

            if( found_dev == true )
              break;
          }
        }
      }
      delete[] cl_platforms;

      if( found_dev != true )
      {
        cout << "Not found GL Sharing Support!" << endl;
        return;
      }
      // Create CL context properties, add WGL context & handle to DC
      cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(), // WGL Context
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(), // WGL HDC
        CL_CONTEXT_PLATFORM, (cl_context_properties)cp_platform, // OpenCL platform
        0
      };
      // Find CL capable devices in the current GL context
      cl_GPU_context = clCreateContext( properties, 1, &device, NULL, 0, 0 );

      // create a command-queue
      cl_cq = clCreateCommandQueue( cl_GPU_context, device, 0, &cl_err_num );
    }
#endif

#ifdef USE_CL
    void cl_clean_up()
    {
      //release openCL
      if( cl_cq )clReleaseCommandQueue( cl_cq );
      if( cl_GPU_context )clReleaseContext( cl_GPU_context );
    }
#endif

    void set_vsync( bool vsync )
    {
      the_window.setVerticalSyncEnabled( vsync );
    }

    template< class t >
    void handle_events( const t& f )
    {
      while( the_window.pollEvent( the_event ) )
      {
        if( the_event.type == sf::Event::Closed ||
          (
          the_event.type == sf::Event::KeyPressed &&
          the_event.key.code == sf::Keyboard::Escape
          )
          )
        {
          run = false;
        }

        f( the_event );
      }
    }

    template< class t >
    void display( const t& f, const bool& silent = false )
    {
      while( run )
      {
        f();

        the_window.display();
      }
    }

    void force_display()
    {
      the_window.display();
    }

    void create_depth_texture( GLuint *tex, const uvec2 &size )
    {
      glGenTextures( 1, tex );

      glBindTexture( GL_TEXTURE_2D, *tex );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0 );
    }

    void create_shmap_texture( GLuint *tex, const uvec2 &size )
    {
      glGenTextures( 1, tex );

      glBindTexture( GL_TEXTURE_2D, *tex );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
      glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0 );
    }

    void create_color_texture( GLuint *tex, const uvec2 &size )
    {
      glGenTextures( 1, tex );

      glBindTexture( GL_TEXTURE_2D, *tex );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
    }

    void load_shader( GLuint& program, const GLenum& type, const string& filename, bool print = false, const std::string& additional_str = "" ) const
    {
      ifstream f( filename );

      if( !f.is_open() )
      {
        cerr << "Couldn't load shader: " << filename << endl;
        return;
      }

      string str( ( istreambuf_iterator<char>( f ) ),
        istreambuf_iterator<char>() );

      shader_include( str, filename.substr( 0, filename.rfind( "/" ) + 1 ) );

      if( print )
        cout << str << endl;

      if( !program ) program = glCreateProgram();

      compile_shader( str.c_str(), program, type, additional_str );
      link_shader( program );
    }

#ifdef USE_CL
    void load_cl_program( cl_program &program, const string& filename )
    {
      // Load the kernel source code into the array source_str
      ifstream f( filename );
      if( !f.is_open() )
      {
        cerr << "Couldn't load cl program: " << filename << endl;
        return;
      }

      string str( ( istreambuf_iterator<char>( f ) ),
        istreambuf_iterator<char>() );

      size_t size = str.size();
      const char *source = str.c_str();
      if( !program )
        program = clCreateProgramWithSource( cl_GPU_context, 1, (const char **)&source, &size, &cl_err_num );

      // build the program
      cl_err_num = clBuildProgram( program, 0, NULL, "-cl-single-precision-constant -cl-mad-enable -cl-no-signed-zeros -cl-finite-math-only -cl-fast-relaxed-math", NULL, NULL );
      char c_build_log[10240];
      clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG,
        sizeof( c_build_log ), c_build_log, NULL );
      get_opencl_error( cl_err_num );
      cout << c_build_log << endl;

    }
#endif


#ifdef USE_CL
    void cl_aquire_buffer( cl_mem *buff, cl_uint count )
    {
      glFlush();
      cl_err_num = clEnqueueAcquireGLObjects( cl_cq, count, buff, 0, NULL, NULL );
      get_opencl_error( cl_err_num );
    }
#endif

#ifdef USE_CL
    void cl_release_buffer( cl_mem *buff, cl_uint count )
    {
      clEnqueueReleaseGLObjects( cl_cq, count, buff, 0, NULL, NULL );
      cl_err_num = clFinish( cl_cq );
      get_opencl_error( cl_err_num );
    }
#endif

#ifdef USE_CL
    void execute_kernel( cl_kernel kernel, size_t global_work_size[2], size_t local_work_size[2], int dim )
    {
      cl_err_num = clEnqueueNDRangeKernel( cl_cq, kernel, dim, NULL, global_work_size, local_work_size, 0, NULL, NULL );
      get_opencl_error( cl_err_num );
    }
#endif

    GLuint create_quad( const vec3& ll, const vec3& lr, const vec3& ul, const vec3& ur ) const
    {
      vector<float> vertices;
      vector<float> tex_coords;
      vector<unsigned> indices;
      GLuint vao = 0;
      GLuint vertex_vbo = 0, tex_coord_vbo = 0, index_vbo = 0;

      vertices.reserve( 4 * 3 );
      tex_coords.reserve( 4 * 2 );
      indices.reserve( 2 * 3 );

      indices.push_back( 0 );
      indices.push_back( 1 );
      indices.push_back( 2 );

      indices.push_back( 0 );
      indices.push_back( 2 );
      indices.push_back( 3 );

      /*vertices.push_back( vec3( -1, -1, 0 ) );
      vertices.push_back( vec3( 1, -1, 0 ) );
      vertices.push_back( vec3( 1, 1, 0 ) );
      vertices.push_back( vec3( -1, 1, 0 ) );*/
      vertices.push_back( ll.x );
      vertices.push_back( ll.y );
      vertices.push_back( ll.z );

      vertices.push_back( lr.x );
      vertices.push_back( lr.y );
      vertices.push_back( lr.z );

      vertices.push_back( ur.x );
      vertices.push_back( ur.y );
      vertices.push_back( ur.z );

      vertices.push_back( ul.x );
      vertices.push_back( ul.y );
      vertices.push_back( ul.z );

      tex_coords.push_back( 0 );
      tex_coords.push_back( 0 );

      tex_coords.push_back( 1 );
      tex_coords.push_back( 0 );

      tex_coords.push_back( 1 );
      tex_coords.push_back( 1 );

      tex_coords.push_back( 0 );
      tex_coords.push_back( 1 );

      glGenVertexArrays( 1, &vao );
      glBindVertexArray( vao );

      glGenBuffers( 1, &vertex_vbo );
      glBindBuffer( GL_ARRAY_BUFFER, vertex_vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof(float)* vertices.size(), &vertices[0], GL_STATIC_DRAW );
      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, 0, 0, 0 );

      glGenBuffers( 1, &tex_coord_vbo );
      glBindBuffer( GL_ARRAY_BUFFER, tex_coord_vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof(float)* tex_coords.size(), &tex_coords[0], GL_STATIC_DRAW );
      glEnableVertexAttribArray( 1 );
      glVertexAttribPointer( 1, 2, GL_FLOAT, 0, 0, 0 );

      glGenBuffers( 1, &index_vbo );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_vbo );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)* indices.size(), &indices[0], GL_STATIC_DRAW );

      glBindVertexArray( 0 );

      return vao;
    }

    inline GLuint create_box() const
    {
      vector<float> vertices;
      vector<float> normals;
      vector<unsigned> indices;
      GLuint vao = 0;
      GLuint vertex_vbo = 0, normal_vbo = 0, index_vbo = 0;

      vertices.reserve( 4 * 3 * 6 );
      normals.reserve( 4 * 3 * 6 );
      indices.reserve( 2 * 3 * 6 );

      //front
      vertices.push_back( -1 );
      vertices.push_back( -1 );
      vertices.push_back( 1 );

      vertices.push_back( 1 );
      vertices.push_back( -1 );
      vertices.push_back( 1 );

      vertices.push_back( 1 );
      vertices.push_back( 1 );
      vertices.push_back( 1 );

      vertices.push_back( -1 );
      vertices.push_back( 1 );
      vertices.push_back( 1 );

      indices.push_back( 0 );
      indices.push_back( 1 );
      indices.push_back( 2 );

      indices.push_back( 0 );
      indices.push_back( 2 );
      indices.push_back( 3 );

      normals.push_back( 0 );
      normals.push_back( 0 );
      normals.push_back( 1 );

      normals.push_back( 0 );
      normals.push_back( 0 );
      normals.push_back( 1 );

      normals.push_back( 0 );
      normals.push_back( 0 );
      normals.push_back( 1 );

      normals.push_back( 0 );
      normals.push_back( 0 );
      normals.push_back( 1 );

      //back
      vertices.push_back( -1 );
      vertices.push_back( -1 );
      vertices.push_back( -1 );

      vertices.push_back( 1 );
      vertices.push_back( -1 );
      vertices.push_back( -1 );

      vertices.push_back( 1 );
      vertices.push_back( 1 );
      vertices.push_back( -1 );

      vertices.push_back( -1 );
      vertices.push_back( 1 );
      vertices.push_back( -1 );

      indices.push_back( 6 );
      indices.push_back( 5 );
      indices.push_back( 4 );

      indices.push_back( 7 );
      indices.push_back( 6 );
      indices.push_back( 4 );

      normals.push_back( 0 );
      normals.push_back( 0 );
      normals.push_back( -1 );

      normals.push_back( 0 );
      normals.push_back( 0 );
      normals.push_back( -1 );

      normals.push_back( 0 );
      normals.push_back( 0 );
      normals.push_back( -1 );

      normals.push_back( 0 );
      normals.push_back( 0 );
      normals.push_back( -1 );

      //left
      vertices.push_back( -1 );
      vertices.push_back( -1 );
      vertices.push_back( 1 );

      vertices.push_back( -1 );
      vertices.push_back( -1 );
      vertices.push_back( -1 );

      vertices.push_back( -1 );
      vertices.push_back( 1 );
      vertices.push_back( 1 );


      vertices.push_back( -1 );
      vertices.push_back( 1 );
      vertices.push_back( -1 );

      indices.push_back( 10 );
      indices.push_back( 9 );
      indices.push_back( 8 );

      indices.push_back( 11 );
      indices.push_back( 9 );
      indices.push_back( 10 );

      normals.push_back( -1 );
      normals.push_back( 0 );
      normals.push_back( 0 );

      normals.push_back( -1 );
      normals.push_back( 0 );
      normals.push_back( 0 );

      normals.push_back( -1 );
      normals.push_back( 0 );
      normals.push_back( 0 );

      normals.push_back( -1 );
      normals.push_back( 0 );
      normals.push_back( 0 );

      //right
      vertices.push_back( 1 );
      vertices.push_back( -1 );
      vertices.push_back( 1 );

      vertices.push_back( 1 );
      vertices.push_back( -1 );
      vertices.push_back( -1 );

      vertices.push_back( 1 );
      vertices.push_back( 1 );
      vertices.push_back( 1 );

      vertices.push_back( 1 );
      vertices.push_back( 1 );
      vertices.push_back( -1 );

      indices.push_back( 12 );
      indices.push_back( 13 );
      indices.push_back( 14 );

      indices.push_back( 14 );
      indices.push_back( 13 );
      indices.push_back( 15 );

      normals.push_back( 1 );
      normals.push_back( 0 );
      normals.push_back( 0 );

      normals.push_back( 1 );
      normals.push_back( 0 );
      normals.push_back( 0 );

      normals.push_back( 1 );
      normals.push_back( 0 );
      normals.push_back( 0 );

      normals.push_back( 1 );
      normals.push_back( 0 );
      normals.push_back( 0 );

      //up
      vertices.push_back( -1 );
      vertices.push_back( 1 );
      vertices.push_back( 1 );

      vertices.push_back( 1 );
      vertices.push_back( 1 );
      vertices.push_back( 1 );

      vertices.push_back( 1 );
      vertices.push_back( 1 );
      vertices.push_back( -1 );

      vertices.push_back( -1 );
      vertices.push_back( 1 );
      vertices.push_back( -1 );

      indices.push_back( 16 );
      indices.push_back( 17 );
      indices.push_back( 18 );

      indices.push_back( 16 );
      indices.push_back( 18 );
      indices.push_back( 19 );

      normals.push_back( 0 );
      normals.push_back( 1 );
      normals.push_back( 0 );

      normals.push_back( 0 );
      normals.push_back( 1 );
      normals.push_back( 0 );

      normals.push_back( 0 );
      normals.push_back( 1 );
      normals.push_back( 0 );

      normals.push_back( 0 );
      normals.push_back( 1 );
      normals.push_back( 0 );

      //down
      vertices.push_back( -1 );
      vertices.push_back( -1 );
      vertices.push_back( 1 );

      vertices.push_back( 1 );
      vertices.push_back( -1 );
      vertices.push_back( 1 );

      vertices.push_back( 1 );
      vertices.push_back( -1 );
      vertices.push_back( -1 );

      vertices.push_back( -1 );
      vertices.push_back( -1 );
      vertices.push_back( -1 );

      indices.push_back( 22 );
      indices.push_back( 21 );
      indices.push_back( 20 );

      indices.push_back( 23 );
      indices.push_back( 22 );
      indices.push_back( 20 );

      normals.push_back( 0 );
      normals.push_back( -1 );
      normals.push_back( 0 );

      normals.push_back( 0 );
      normals.push_back( -1 );
      normals.push_back( 0 );

      normals.push_back( 0 );
      normals.push_back( -1 );
      normals.push_back( 0 );

      normals.push_back( 0 );
      normals.push_back( -1 );
      normals.push_back( 0 );

      glGenVertexArrays( 1, &vao );
      glBindVertexArray( vao );

      glGenBuffers( 1, &vertex_vbo );
      glBindBuffer( GL_ARRAY_BUFFER, vertex_vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof(float)* vertices.size(), &vertices[0], GL_STATIC_DRAW );
      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, 0, 0, 0 );

      glGenBuffers( 1, &normal_vbo );
      glBindBuffer( GL_ARRAY_BUFFER, normal_vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof(float)* normals.size(), &normals[0], GL_STATIC_DRAW );
      glEnableVertexAttribArray( 2 );
      glVertexAttribPointer( 2, 3, GL_FLOAT, 0, 0, 0 );

      glGenBuffers( 1, &index_vbo );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_vbo );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned)* indices.size(), &indices[0], GL_STATIC_DRAW );

      glBindVertexArray( 0 );

      return vao;
    }

    GLuint load_image( const std::string& str )
    {
      sf::Image im;
      im.loadFromFile( str );
      unsigned w = im.getSize().x;
      unsigned h = im.getSize().y;

      GLuint tex = 0;
      glGenTextures( 1, &tex );
      glBindTexture( GL_TEXTURE_2D, tex );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, im.getPixelsPtr() );

      return tex;
    }

    std::string get_app_path()
    {

      char fullpath[4096];
      std::string app_path;

      /* /proc/self is a symbolic link to the process-ID subdir
       * of /proc, e.g. /proc/4323 when the pid of the process
       * of this program is 4323.
       *
       * Inside /proc/<pid> there is a symbolic link to the
       * executable that is running as this <pid>.  This symbolic
       * link is called "exe".
       *
       * So if we read the path where the symlink /proc/self/exe
       * points to we have the full path of the executable.
       */

#ifdef __unix__
      int length;
      length = readlink( "/proc/self/exe", fullpath, sizeof( fullpath ) );

      /* Catch some errors: */

      if( length < 0 )
      {
        my::log << my::lock << "Couldnt read app path. Error resolving symlink /proc/self/exe." << my::endl << my::unlock;
        loop::get().shutdown();
      }

      if( length >= 4096 )
      {
        my::log << my::lock << "Couldnt read app path. Path too long. Truncated." << my::endl << my::unlock;
        loop::get().shutdown();
      }

      /* I don't know why, but the string this readlink() function
       * returns is appended with a '@'.
       */
      fullpath[length] = '\0';       /* Strip '@' off the end. */

#endif

#ifdef _WIN32

      if( GetModuleFileName( 0, (char*)&fullpath, sizeof(fullpath)-1 ) == 0 )
      {
        cerr << "Couldn't get the app path." << endl;
        return "";
      }

#endif

      app_path = fullpath;

#ifdef _WIN32
      app_path = app_path.substr( 0, app_path.rfind( "\\" ) + 1 );
#endif

#ifdef __unix__
      config::get().app_path = config::get().app_path.substr( 0, config::get().app_path.rfind( "/" ) + 1 );
#endif

      app_path += "../";

      char* res = 0;

      res = realpath( app_path.c_str(), 0 );

      if( res )
      {
#if _WIN32
        app_path = std::string( res );
        delete[] res;
#endif

#if __unix__
        config::get().app_path = std::string( res ) + "/";
        free( res ); //the original linux version of realpath uses malloc
#endif
      }

      std::replace( app_path.begin(), app_path.end(), '\\', '/' );

      return app_path;
    }

    void get_opengl_error( const bool& ignore = false ) const
    {
      bool got_error = false;
      GLenum error = 0;
      error = glGetError();
      string errorstring = "";

      while( error != GL_NO_ERROR )
      {
        if( error == GL_INVALID_ENUM )
        {
          //An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.
          errorstring += "OpenGL error: invalid enum...\n";
          got_error = true;
        }

        if( error == GL_INVALID_VALUE )
        {
          //A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.
          errorstring += "OpenGL error: invalid value...\n";
          got_error = true;
        }

        if( error == GL_INVALID_OPERATION )
        {
          //The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.
          errorstring += "OpenGL error: invalid operation...\n";
          got_error = true;
        }

        if( error == GL_STACK_OVERFLOW )
        {
          //This command would cause a stack overflow. The offending command is ignored and has no other side effect than to set the error flag.
          errorstring += "OpenGL error: stack overflow...\n";
          got_error = true;
        }

        if( error == GL_STACK_UNDERFLOW )
        {
          //This command would cause a stack underflow. The offending command is ignored and has no other side effect than to set the error flag.
          errorstring += "OpenGL error: stack underflow...\n";
          got_error = true;
        }

        if( error == GL_OUT_OF_MEMORY )
        {
          //There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.
          errorstring += "OpenGL error: out of memory...\n";
          got_error = true;
        }

        if( error == GL_TABLE_TOO_LARGE )
        {
          //The specified table exceeds the implementation's maximum supported table size.  The offending command is ignored and has no other side effect than to set the error flag.
          errorstring += "OpenGL error: table too large...\n";
          got_error = true;
        }

        error = glGetError();
      }

      if( got_error && !ignore )
      {
        cerr << errorstring;

        return;
      }
    }

    void check_fbo_status( const GLenum& target = GL_FRAMEBUFFER )
    {
      GLenum error_code = glCheckFramebufferStatus( target );

      switch( error_code )
      {
        case GL_FRAMEBUFFER_COMPLETE:
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT." << endl;
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS." << endl;
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT." << endl;
          break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
          cerr << "GL_FRAMEBUFFER_UNSUPPORTED." << endl;
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER." << endl;
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER." << endl;
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE." << endl;
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_ARB:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS." << endl;
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_ARB:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT." << endl;
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
          cerr << "GL_FRAMEBUFFER_INCOMPLETE_FORMATS." << endl;
          break;
        default:
          cerr << "Unknown Frame Buffer error cause: " << error_code << endl;
          break;
      }
    }

#ifdef USE_CL
    // Helper function to get OpenCL error string from constant
    // *********************************************************************
    void get_opencl_error( cl_int error )
    {
      if( error == 0 )
        return;
      static const char* errorString[] = {
        "CL_SUCCESS",
        "CL_DEVICE_NOT_FOUND",
        "CL_DEVICE_NOT_AVAILABLE",
        "CL_COMPILER_NOT_AVAILABLE",
        "CL_MEM_OBJECT_ALLOCATION_FAILURE",
        "CL_OUT_OF_RESOURCES",
        "CL_OUT_OF_HOST_MEMORY",
        "CL_PROFILING_INFO_NOT_AVAILABLE",
        "CL_MEM_COPY_OVERLAP",
        "CL_IMAGE_FORMAT_MISMATCH",
        "CL_IMAGE_FORMAT_NOT_SUPPORTED",
        "CL_BUILD_PROGRAM_FAILURE",
        "CL_MAP_FAILURE",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "CL_INVALID_VALUE",
        "CL_INVALID_DEVICE_TYPE",
        "CL_INVALID_PLATFORM",
        "CL_INVALID_DEVICE",
        "CL_INVALID_CONTEXT",
        "CL_INVALID_QUEUE_PROPERTIES",
        "CL_INVALID_COMMAND_QUEUE",
        "CL_INVALID_HOST_PTR",
        "CL_INVALID_MEM_OBJECT",
        "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
        "CL_INVALID_IMAGE_SIZE",
        "CL_INVALID_SAMPLER",
        "CL_INVALID_BINARY",
        "CL_INVALID_BUILD_OPTIONS",
        "CL_INVALID_PROGRAM",
        "CL_INVALID_PROGRAM_EXECUTABLE",
        "CL_INVALID_KERNEL_NAME",
        "CL_INVALID_KERNEL_DEFINITION",
        "CL_INVALID_KERNEL",
        "CL_INVALID_ARG_INDEX",
        "CL_INVALID_ARG_VALUE",
        "CL_INVALID_ARG_SIZE",
        "CL_INVALID_KERNEL_ARGS",
        "CL_INVALID_WORK_DIMENSION",
        "CL_INVALID_WORK_GROUP_SIZE",
        "CL_INVALID_WORK_ITEM_SIZE",
        "CL_INVALID_GLOBAL_OFFSET",
        "CL_INVALID_EVENT_WAIT_LIST",
        "CL_INVALID_EVENT",
        "CL_INVALID_OPERATION",
        "CL_INVALID_GL_OBJECT",
        "CL_INVALID_BUFFER_SIZE",
        "CL_INVALID_MIP_LEVEL",
        "CL_INVALID_GLOBAL_WORK_SIZE",
      };

      const int errorCount = sizeof( errorString ) / sizeof( errorString[0] );

      const int index = -error;

      cout << ( ( index >= 0 && index < errorCount ) ? errorString[index] : "Unspecified Error" ) << endl;
    }
#endif
  };

  // Round Up Division function
  inline size_t round_up( int group_size, int global_size )
  {
    int r = global_size % group_size;
    if( r == 0 )
      return global_size;
    else
      return global_size + group_size - r;
  }

  class MM_16_BYTE_ALIGNED mesh;
  class material;
  class MM_16_BYTE_ALIGNED spot_light;
  class MM_16_BYTE_ALIGNED point_light;
  class MM_16_BYTE_ALIGNED directional_light;
  class texture;
  class MM_16_BYTE_ALIGNED animation_node;
  class MM_16_BYTE_ALIGNED channel;
  class animation;
  class MM_16_BYTE_ALIGNED object;
  class MM_16_BYTE_ALIGNED bone_info;
  class MM_16_BYTE_ALIGNED scene;
  class MM_16_BYTE_ALIGNED light;

  class MM_16_BYTE_ALIGNED object
  {
  public:
    vector<int> mesh_idx; //also material index
    mat4 transformation;
  };

  class MM_16_BYTE_ALIGNED bone_info
  {
  public:
    mat4 offset, final_trans;
  };

  class MM_16_BYTE_ALIGNED scene
  {
  public:
    vector<object> objects;
    vector<mesh> meshes;
    vector<material> materials;
    camera<float> cam;
    frame<float> f;
    vector<spot_light> spot_lights;
    vector<point_light> point_lights;
    vector<directional_light> directional_lights;
    vector<texture> textures;
    map< string, int > bone_mapping;
    vector<animation> animations;
    vector< bone_info > bi;
    mat4 global_inv_trans;
    float aspect, near, far, fov;
  };

  class material
  {
  public:
    std::string diffuse_file, specular_file, normal_file;
    GLuint diffuse_tex, specular_tex, normal_tex;
    bool is_transparent, is_animated;
  };

  enum attenuation_type
  {
    FULL = 0, LINEAR, attLAST
  };

  class MM_16_BYTE_ALIGNED light
  {
  public:
    camera<float> cam;
    frame<float> the_frame;
    vec3 diffuse_color, specular_color;
    float attenuation_coeff, radius;
    attenuation_type att_type;
    shape* bv;
  };

  class MM_16_BYTE_ALIGNED spot_light : public light
  {
  public:
    float spot_exponent, spot_cutoff;
    mm::mat4 shadow_mat;
  };

  class MM_16_BYTE_ALIGNED point_light : public light
  {
  public:
    mm::mat4 shadow_mat[6];
  };

  class MM_16_BYTE_ALIGNED directional_light : public light
  {
  public:
  };

  class texture
  {
  public:
    GLuint texid;
    std::string filename;
    unsigned miplevels;
    bool is_transparent;
    int w, h;
    GLenum internal_format;
  };

  class MM_16_BYTE_ALIGNED animation_node
  {
  public:
    string name;
    int bone_idx;
    int animation_id, channel_id;
    animation_node* parent;
    mat4 transformation;
    vector< animation_node > children;
  };

  class animation_channel
  {
  public:
    string name;
    vector< pair<vec3, float> > positions;
    vector< pair<quat, float> > rotations;
    vector< pair<vec3, float> > scalings;
  };

  class animation
  {
  public:
    float duration, ticks_per_second;
    vector< animation_channel > channels;
  };

  class MM_16_BYTE_ALIGNED mesh
  {
  public:
    std::vector< unsigned > indices;
    std::vector< float > vertices;
    std::vector< float > normals;
    std::vector< float > tangents;
    std::vector< float > tex_coords;
    std::vector< ivec4 > bone_ids;
    std::vector< vec4 > bone_weights;

    unsigned rendersize;

    GLuint vao;
    GLuint vbos[8];

    shape* trans_bv;
    shape* local_bv;

    animation_node* root_node;

    mat4 transformation;
    mat4 inv_transformation;

    enum vbo_type
    {
      VERTEX = 0, TEX_COORD, NORMAL, TANGENT, BONE_IDS, BONE_WEIGHTS, INDEX
    };

    static void update_animation( float time, scene& s, mat4* bones )
    {
      auto get_interpolated_scaling = []( float time, animation_channel* c ) -> vec3
      {
        if( c->scalings.size() < 2 )
          return c->scalings[0].first;

        int i = 0;
        for( ; i < c->scalings.size(); ++i )
        {
          if( time <= c->scalings[i + 1].second )
            break;
        }

        int idx = i;
        int next_idx = i + 1;

        assert( next_idx < c->scalings.size() );

        float dt = c->scalings[next_idx].second - c->scalings[idx].second;
        float factor = ( time - c->scalings[idx].second ) / dt;

        assert( factor >= 0 && factor <= 1 );

        vec3 start = c->scalings[idx].first;
        vec3 end = c->scalings[next_idx].first;

        return mix( start, end, factor );
      };

      auto get_interpolated_rotation = []( float time, animation_channel* c ) -> quat
      {
        if( c->rotations.size() < 2 )
          return c->rotations[0].first;

        int i = 0;
        for( ; i < c->rotations.size(); ++i )
        {
          if( time <= c->rotations[i + 1].second )
            break;
        }

        int idx = i;
        int next_idx = i + 1;

        assert( next_idx < c->rotations.size() );

        float dt = c->rotations[next_idx].second - c->rotations[idx].second;
        float factor = ( time - c->rotations[idx].second ) / dt;

        assert( factor >= 0 && factor <= 1 );

        quat start = c->rotations[idx].first;
        quat end = c->rotations[next_idx].first;

        quat q = mix( start, end, factor );

        return normalize( q );
      };

      auto get_interpolated_position = []( float time, animation_channel* c ) -> vec3
      {
        if( c->positions.size() < 2 )
          return c->positions[0].first;

        int i = 0;
        for( ; i < c->positions.size(); ++i )
        {
          if( time <= c->positions[i + 1].second )
            break;
        }

        int idx = i;
        int next_idx = i + 1;

        assert( next_idx < c->positions.size() );

        float dt = c->positions[next_idx].second - c->positions[idx].second;
        float factor = ( time - c->positions[idx].second ) / dt;

        assert( factor >= 0 && factor <= 1 );

        vec3 start = c->positions[idx].first;
        vec3 end = c->positions[next_idx].first;

        return mix( start, end, factor );
      };

      std::function< void( mesh*, animation_node*, const mat4& ) > traverse_node;
      traverse_node = [&]( mesh* m, animation_node* n, const mat4& parent_transform )
      {
        mat4 node_transform = n->transformation;

        if( n->channel_id > -1 && n->animation_id > -1 )
        {
          float ticks_per_sec = s.animations[n->animation_id].ticks_per_second != 0 ? s.animations[n->animation_id].ticks_per_second : 25;
          float time_ticks = ticks_per_sec * time;
          float animation_time = std::fmod( time_ticks, s.animations[n->animation_id].duration );

          vec3 interpolated_scaling = get_interpolated_scaling( animation_time, &s.animations[n->animation_id].channels[n->channel_id] );
          quat interpolated_rotation = get_interpolated_rotation( animation_time, &s.animations[n->animation_id].channels[n->channel_id] );
          vec3 interpolated_position = get_interpolated_position( animation_time, &s.animations[n->animation_id].channels[n->channel_id] );

          node_transform = create_translation( interpolated_position ) * mat4_cast( interpolated_rotation ) * create_scale( interpolated_scaling );
        }

        mat4 global_trans = parent_transform * node_transform;

        if( n->bone_idx > -1 )
        {
          mat4 m = s.global_inv_trans * global_trans * s.bi[n->bone_idx].offset;
          s.bi[n->bone_idx].final_trans = m;
        }

        for( int i = 0; i < n->children.size(); ++i )
          traverse_node( m, &n->children[i], global_trans );
      };

      for( int c = 0; c < s.meshes.size(); ++c )
      {
        traverse_node( &s.meshes[c], s.meshes[c].root_node, mat4::identity );
      }

      for( int i = 0; i < s.bi.size(); ++i )
        bones[i] = s.bi[i].final_trans;
    }

    static void save_meshes( const std::string& path, vector<mesh>& meshes )
    {
      fstream f;
      f.open( path.c_str(), ios::out );

      if( !f.is_open() )
      {
        cerr << "Couldn't save meshes into file: " << path << endl;
      }

      int counter = 0;
      int global_max_index = 0;
      for( auto& c : meshes )
      {
        f << "o object" << counter++ << endl;

        for( int i = 0; i < c.vertices.size(); i += 3 )
        {
          f << "v " << c.vertices[i + 0] << " " << c.vertices[i + 0] << " " << c.vertices[i + 0] << endl;
        }

        for( int i = 0; i < c.tex_coords.size(); i += 2 )
        {
          f << "vt " << c.tex_coords[i + 0] << " " << c.tex_coords[i + 0] << endl;
        }

        for( int i = 0; i < c.normals.size(); i += 3 )
        {
          f << "vn " << c.normals[i + 0] << " " << c.normals[i + 0] << " " << c.normals[i + 0] << endl;
        }

        int max_index = global_max_index;
        for( int i = 0; i < c.indices.size(); i += 3 )
        {
          f << "f " << max_index + c.indices[i + 0] + 1 << "/"
            << max_index + c.indices[i + 0] + 1 << "/"
            << max_index + c.indices[i + 0] + 1 << " ";
          f << max_index + c.indices[i + 1] + 1 << "/"
            << max_index + c.indices[i + 1] + 1 << "/"
            << max_index + c.indices[i + 1] + 1 << " ";
          f << max_index + c.indices[i + 2] + 1 << "/"
            << max_index + c.indices[i + 2] + 1 << "/"
            << max_index + c.indices[i + 2] + 1 << endl;

          global_max_index = max( global_max_index, max_index + c.indices[i + 0] + 1 );
          global_max_index = max( global_max_index, max_index + c.indices[i + 1] + 1 );
          global_max_index = max( global_max_index, max_index + c.indices[i + 2] + 1 );
        }

        f << endl;
      }

      f.close();
    }

    static void load_into_meshes( const std::string& filename, scene& s, const bool& flip = false )
    {
      Assimp::Importer the_importer;

      std::string path = filename.substr( 0, filename.rfind( "/" ) + 1 );

      const aiScene* the_scene = the_importer.ReadFile( filename.c_str(),
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_LimitBoneWeights |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_SplitLargeMeshes |
        aiProcess_FindDegenerates |
        aiProcess_FindInvalidData |
        aiProcess_FindInstances |
        aiProcess_ValidateDataStructure |
        aiProcess_OptimizeMeshes |
        aiProcess_Triangulate |
        ( flip ? aiProcess_FlipUVs : 0 ) );

      if( !the_scene )
      {
        std::cerr << the_importer.GetErrorString() << std::endl;
        return;
      }

      if( the_scene->mNumCameras > 0 )
      {
        s.aspect = the_scene->mCameras[0]->mAspect;
        s.near = the_scene->mCameras[0]->mClipPlaneNear;
        s.far = the_scene->mCameras[0]->mClipPlaneFar;
        s.fov = the_scene->mCameras[0]->mHorizontalFOV;

        s.cam.pos.x = the_scene->mCameras[0]->mPosition.x;
        s.cam.pos.y = the_scene->mCameras[0]->mPosition.y;
        s.cam.pos.z = the_scene->mCameras[0]->mPosition.z;

        s.cam.view_dir.x = the_scene->mCameras[0]->mLookAt.x;
        s.cam.view_dir.y = the_scene->mCameras[0]->mLookAt.y;
        s.cam.view_dir.z = the_scene->mCameras[0]->mLookAt.z;

        s.cam.up_vector.x = the_scene->mCameras[0]->mUp.x;
        s.cam.up_vector.y = the_scene->mCameras[0]->mUp.y;
        s.cam.up_vector.z = the_scene->mCameras[0]->mUp.z;

        s.f.set_perspective( s.fov, s.aspect, s.near, s.far );
      }

      for( int c = 0; c < the_scene->mNumLights; ++c )
      {
        if( the_scene->mLights[c]->mType == aiLightSource_SPOT )
        {
          s.spot_lights.push_back( spot_light() );
          s.spot_lights.back().cam.pos.x = the_scene->mLights[c]->mPosition.x;
          s.spot_lights.back().cam.pos.y = the_scene->mLights[c]->mPosition.y;
          s.spot_lights.back().cam.pos.z = the_scene->mLights[c]->mPosition.z;

          s.spot_lights.back().cam.rotate_x( the_scene->mLights[c]->mDirection.x );
          s.spot_lights.back().cam.rotate_y( the_scene->mLights[c]->mDirection.y );
          s.spot_lights.back().cam.rotate_z( the_scene->mLights[c]->mDirection.z );

          s.spot_lights.back().spot_cutoff = the_scene->mLights[c]->mAngleOuterCone;

          s.spot_lights.back().diffuse_color.x = the_scene->mLights[c]->mColorDiffuse.r;
          s.spot_lights.back().diffuse_color.y = the_scene->mLights[c]->mColorDiffuse.g;
          s.spot_lights.back().diffuse_color.z = the_scene->mLights[c]->mColorDiffuse.b;

          s.spot_lights.back().specular_color.x = the_scene->mLights[c]->mColorSpecular.r;
          s.spot_lights.back().specular_color.y = the_scene->mLights[c]->mColorSpecular.g;
          s.spot_lights.back().specular_color.z = the_scene->mLights[c]->mColorSpecular.b;

          if( the_scene->mLights[c]->mAttenuationLinear > 0 )
            s.spot_lights.back().radius = 2.0f / the_scene->mLights[c]->mAttenuationLinear;

          if( the_scene->mLights[c]->mAttenuationQuadratic > 0 )
            s.spot_lights.back().radius = std::sqrt( 1.0f / the_scene->mLights[c]->mAttenuationQuadratic );

          s.spot_lights.back().attenuation_coeff = 0.25;
          s.spot_lights.back().att_type = LINEAR;
          s.spot_lights.back().spot_exponent = 20;

          s.spot_lights.back().bv = new sphere( s.spot_lights.back().cam.pos, s.spot_lights.back().radius );
          s.f.set_perspective( s.spot_lights.back().spot_cutoff, 1, 1, s.spot_lights.back().radius );
        }

        //TODO point lights
        //TODO directional lights
      }

      s.objects.push_back( object() );

      int orig_size = s.meshes.size();

      s.meshes.resize( s.meshes.size() + the_scene->mNumMeshes );
      s.materials.resize( s.materials.size() + the_scene->mNumMeshes );

      for( unsigned int c = 0; c < the_scene->mNumMeshes; ++c )
      {
        aiMaterial* mtl = the_scene->mMaterials[the_scene->mMeshes[c]->mMaterialIndex];

        auto grab_texture = [&]( aiTextureType t, std::string& filename, GLuint& tex, bool& trans, bool srgb )
        {
          tex = 0;

          aiString texpath;
          if( mtl->GetTexture( t, 0, &texpath ) == AI_SUCCESS )
          {
            std::string tex_filename = path + texpath.C_Str();

            auto it = std::find_if( s.textures.begin(), s.textures.end(), [&]( const texture& a ) -> bool
            {
              return a.filename == tex_filename;
            } );

            GLuint orig_tex = 0;
            unsigned miplevels = 0;
            bool is_transparent = false;

            if( it == s.textures.end() )
            {
              sf::Image im;
              if( im.loadFromFile( tex_filename ) )
              {
                s.textures.push_back( texture() );
                glGenTextures( 1, &s.textures.back().texid );
                orig_tex = s.textures.back().texid;
                s.textures.back().filename = tex_filename;
                s.textures.back().miplevels = std::log2( float( std::max( im.getSize().x, im.getSize().y ) ) ) + 1;
                miplevels = s.textures.back().miplevels;
                glBindTexture( GL_TEXTURE_2D, s.textures.back().texid );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4 );

                s.textures.back().w = im.getSize().x;
                s.textures.back().h = im.getSize().y;

                if( srgb )
                {
                  glTexStorage2D( GL_TEXTURE_2D, s.textures.back().miplevels, GL_SRGB8_ALPHA8, im.getSize().x, im.getSize().y );
                  s.textures.back().internal_format = GL_SRGB8_ALPHA8;
                }
                else
                {
                  glTexStorage2D( GL_TEXTURE_2D, s.textures.back().miplevels, GL_RGBA8, im.getSize().x, im.getSize().y );
                  s.textures.back().internal_format = GL_RGBA8;
                }

                glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, im.getSize().x, im.getSize().y, GL_RGBA, GL_UNSIGNED_BYTE, im.getPixelsPtr() );

                glGenerateMipmap( GL_TEXTURE_2D );

                s.textures.back().is_transparent = false;

                if( tex_filename.rfind( "vase_plant.png" ) != string::npos )
                {
                  int a = 0;
                }

                for( unsigned y = 0; y < im.getSize().y; ++y )
                {
                  for( unsigned x = 0; x < im.getSize().x; ++x )
                  {
                    if( im.getPixel( x, y ).a < 255 )
                    {
                      s.textures.back().is_transparent = true;
                      break;
                    }
                  }

                  if( s.textures.back().is_transparent )
                    break;
                }

                is_transparent = s.textures.back().is_transparent;
              }
              else
              {
                std::cerr << "couldn't load texture: " << filename << endl;
              }
            }
            else
            {
              orig_tex = it->texid;
              miplevels = it->miplevels;
              is_transparent = it->is_transparent;
            }

            trans = is_transparent;

            //create texture view
            {
              assert( orig_tex > 0 );

              glGenTextures( 1, &tex );
              if( srgb )
                glTextureView( tex, GL_TEXTURE_2D, orig_tex, GL_SRGB8_ALPHA8, 0, miplevels, 0, 1 );
              else
                glTextureView( tex, GL_TEXTURE_2D, orig_tex, GL_RGBA8, 0, miplevels, 0, 1 );

              glBindTexture( GL_TEXTURE_2D, tex );

              glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
              glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
              glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
              glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
              glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4 );
            }
          }
        };

        /**
        //uncomment to find out which assimp type the texture belongs to
        grab_texture( aiTextureType_AMBIENT, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_DIFFUSE, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_DISPLACEMENT, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_EMISSIVE, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_HEIGHT, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_LIGHTMAP, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_NONE, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_NORMALS, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_OPACITY, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_REFLECTION, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_SHININESS, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_SPECULAR, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        grab_texture( aiTextureType_UNKNOWN, s.materials[c].diffuse_file, s.materials[c].diffuse_tex );
        /**/

        int cc = orig_size + c;

        s.objects.back().mesh_idx.push_back( cc );
        s.objects.back().transformation = mat4::identity;

        s.materials[cc].is_transparent = false;
        s.materials[cc].is_animated = false;

        bool dummy = false;
        grab_texture( aiTextureType_DIFFUSE, s.materials[cc].diffuse_file, s.materials[cc].diffuse_tex, s.materials[cc].is_transparent, true );
        grab_texture( aiTextureType_NORMALS, s.materials[cc].normal_file, s.materials[cc].normal_tex, dummy, false ); //for collada
        //grab_texture( aiTextureType_HEIGHT, s.materials[cc].normal_file, s.materials[cc].normal_tex, dummy, false ); //for obj...
        grab_texture( aiTextureType_SPECULAR, s.materials[cc].specular_file, s.materials[cc].specular_tex, dummy, true );

        //write out face indices
        s.meshes[cc].indices.resize( the_scene->mMeshes[c]->mNumFaces * 3 );
        for( unsigned int d = 0; d < the_scene->mMeshes[c]->mNumFaces; ++d )
        {
          const aiFace* faces = &the_scene->mMeshes[c]->mFaces[d];
          s.meshes[cc].indices.push_back( faces->mIndices[0] );
          s.meshes[cc].indices.push_back( faces->mIndices[1] );
          s.meshes[cc].indices.push_back( faces->mIndices[2] );
        }

        //write out vertices
        s.meshes[cc].vertices.resize( the_scene->mMeshes[c]->mNumVertices * 3 );
        memcpy( &s.meshes[cc].vertices[0], &the_scene->mMeshes[c]->mVertices[0], the_scene->mMeshes[c]->mNumVertices * sizeof(float)* 3 );

        //write out normals
        if( the_scene->mMeshes[c]->mNormals )
        {
          s.meshes[cc].normals.resize( the_scene->mMeshes[c]->mNumVertices * 3 );
          memcpy( &s.meshes[cc].normals[0], &the_scene->mMeshes[c]->mNormals[0], the_scene->mMeshes[c]->mNumVertices * sizeof(float)* 3 );
        }

        //write out tex coords
        if( the_scene->mMeshes[c]->mTextureCoords[0] )
        {
          s.meshes[cc].tex_coords.resize( the_scene->mMeshes[c]->mNumVertices * 2 );
          for( int i = 0; i < the_scene->mMeshes[c]->mNumVertices; ++i )
          {
            const aiVector3D* tex_coord = &the_scene->mMeshes[c]->mTextureCoords[0][i];
            s.meshes[cc].tex_coords[i * 2 + 0] = tex_coord->x;
            s.meshes[cc].tex_coords[i * 2 + 1] = tex_coord->y;
          }
        }

        //bone ids
        if( the_scene->mMeshes[c]->mBones )
        {
          s.materials[cc].is_animated = true;

          s.meshes[cc].bone_weights.resize( the_scene->mMeshes[c]->mNumVertices );
          for( auto& i : s.meshes[cc].bone_weights )
            i = vec4( 0 );
          s.meshes[cc].bone_ids.resize( the_scene->mMeshes[c]->mNumVertices );

          for( int d = 0; d < the_scene->mMeshes[c]->mNumBones; ++d )
          {
            unsigned bone_index = 0;
            string bone_name = string( the_scene->mMeshes[c]->mBones[d]->mName.data );

            if( s.bone_mapping.find( bone_name ) == s.bone_mapping.end() )
            {
              bone_index = s.bone_mapping.size();
              s.bi.resize( s.bi.size() + 1 );
              memcpy( &s.bi[bone_index].offset[0][0], &the_scene->mMeshes[c]->mBones[d]->mOffsetMatrix, sizeof( mat4 ) );
              s.bi[bone_index].offset = transpose( s.bi[bone_index].offset );
              s.bone_mapping[bone_name] = bone_index;
            }
            else
            {
              bone_index = s.bone_mapping[bone_name];
            }

            for( int e = 0; e < the_scene->mMeshes[c]->mBones[d]->mNumWeights; ++e )
            {
              int vertex_id = the_scene->mMeshes[c]->mBones[d]->mWeights[e].mVertexId;
              float weight = the_scene->mMeshes[c]->mBones[d]->mWeights[e].mWeight;

              auto add_bone_data = [&]()
              {
                for( int f = 0; f < 4; ++f )
                {
                  if( s.meshes[cc].bone_weights[vertex_id][f] == 0.0f )
                  {
                    s.meshes[cc].bone_weights[vertex_id][f] = weight;
                    s.meshes[cc].bone_ids[vertex_id][f] = bone_index;
                    return;
                  }
                }

                assert( 0 ); //more bones than we can handle
              };

              add_bone_data();
            }
          }
        }

        if( s.meshes[cc].normals.size() > 0 && s.meshes[cc].tex_coords.size() > 0 )
        {
          s.meshes[cc].tangents.resize( s.meshes[cc].normals.size() );

          for( int i = 0; i < s.meshes[cc].indices.size(); i += 3 )
          {
            int index_1 = s.meshes[cc].indices[i + 0];
            int index_2 = s.meshes[cc].indices[i + 1];
            int index_3 = s.meshes[cc].indices[i + 2];

            mm::vec3 n;
            n[0] = s.meshes[cc].normals[index_1 * 3 + 0];
            n[1] = s.meshes[cc].normals[index_1 * 3 + 1];
            n[2] = s.meshes[cc].normals[index_1 * 3 + 2];

            mm::vec3 v[3];
            v[0][0] = s.meshes[cc].vertices[index_1 * 3 + 0];
            v[0][1] = s.meshes[cc].vertices[index_1 * 3 + 1];
            v[0][2] = s.meshes[cc].vertices[index_1 * 3 + 2];

            v[1][0] = s.meshes[cc].vertices[index_2 * 3 + 0];
            v[1][1] = s.meshes[cc].vertices[index_2 * 3 + 1];
            v[1][2] = s.meshes[cc].vertices[index_2 * 3 + 2];

            v[2][0] = s.meshes[cc].vertices[index_3 * 3 + 0];
            v[2][1] = s.meshes[cc].vertices[index_3 * 3 + 1];
            v[2][2] = s.meshes[cc].vertices[index_3 * 3 + 2];

            mm::vec2 t[3];
            t[0][0] = s.meshes[cc].tex_coords[index_1 * 2 + 0];
            t[0][1] = s.meshes[cc].tex_coords[index_1 * 2 + 1];

            t[1][0] = s.meshes[cc].tex_coords[index_2 * 2 + 0];
            t[1][1] = s.meshes[cc].tex_coords[index_2 * 2 + 1];

            t[2][0] = s.meshes[cc].tex_coords[index_3 * 2 + 0];
            t[2][1] = s.meshes[cc].tex_coords[index_3 * 2 + 1];

            mm::vec3 tangent = mm::calc_tangent( v, t, n );

            s.meshes[cc].tangents[index_1 * 3 + 0] = tangent.x;
            s.meshes[cc].tangents[index_1 * 3 + 1] = tangent.y;
            s.meshes[cc].tangents[index_1 * 3 + 2] = tangent.z;

            s.meshes[cc].tangents[index_2 * 3 + 0] = tangent.x;
            s.meshes[cc].tangents[index_2 * 3 + 1] = tangent.y;
            s.meshes[cc].tangents[index_2 * 3 + 2] = tangent.z;

            s.meshes[cc].tangents[index_3 * 3 + 0] = tangent.x;
            s.meshes[cc].tangents[index_3 * 3 + 1] = tangent.y;
            s.meshes[cc].tangents[index_3 * 3 + 2] = tangent.z;
          }
        }
      }

      {
        mat4 trans;
        memcpy( &trans[0][0], &the_scene->mRootNode->mTransformation, sizeof( mat4 ) );
        s.global_inv_trans = inverse( transpose( trans ) );

        s.animations.resize( s.animations.size() + the_scene->mNumAnimations );

        for( int d = 0; d < the_scene->mNumAnimations; ++d )
        {
          s.animations[d].duration = the_scene->mAnimations[d]->mDuration;
          s.animations[d].ticks_per_second = the_scene->mAnimations[d]->mTicksPerSecond;

          s.animations[d].channels.resize( the_scene->mAnimations[d]->mNumChannels );

          for( int e = 0; e < s.animations[d].channels.size(); ++e )
          {
            s.animations[d].channels[e].name = the_scene->mAnimations[d]->mChannels[e]->mNodeName.C_Str();

            s.animations[d].channels[e].positions.resize( the_scene->mAnimations[d]->mChannels[e]->mNumPositionKeys );
            s.animations[d].channels[e].rotations.resize( the_scene->mAnimations[d]->mChannels[e]->mNumRotationKeys );
            s.animations[d].channels[e].scalings.resize( the_scene->mAnimations[d]->mChannels[e]->mNumScalingKeys );

            for( int f = 0; f < s.animations[d].channels[e].positions.size(); ++f )
            {
              s.animations[d].channels[e].positions[f] = make_pair( vec3( the_scene->mAnimations[d]->mChannels[e]->mPositionKeys[f].mValue[0],
                the_scene->mAnimations[d]->mChannels[e]->mPositionKeys[f].mValue[1],
                the_scene->mAnimations[d]->mChannels[e]->mPositionKeys[f].mValue[2] ),
                the_scene->mAnimations[d]->mChannels[e]->mPositionKeys[f].mTime );
            }

            for( int f = 0; f < s.animations[d].channels[e].rotations.size(); ++f )
            {
              s.animations[d].channels[e].rotations[f] = make_pair( quat( vec4( the_scene->mAnimations[d]->mChannels[e]->mRotationKeys[f].mValue.x,
                the_scene->mAnimations[d]->mChannels[e]->mRotationKeys[f].mValue.y,
                the_scene->mAnimations[d]->mChannels[e]->mRotationKeys[f].mValue.z,
                the_scene->mAnimations[d]->mChannels[e]->mRotationKeys[f].mValue.w ) ),
                the_scene->mAnimations[d]->mChannels[e]->mRotationKeys[f].mTime );
            }

            for( int f = 0; f < s.animations[d].channels[e].scalings.size(); ++f )
            {
              s.animations[d].channels[e].scalings[f] = make_pair( vec3( the_scene->mAnimations[d]->mChannels[e]->mScalingKeys[f].mValue[0],
                the_scene->mAnimations[d]->mChannels[e]->mScalingKeys[f].mValue[1],
                the_scene->mAnimations[d]->mChannels[e]->mScalingKeys[f].mValue[2] ),
                the_scene->mAnimations[d]->mChannels[e]->mScalingKeys[f].mTime );
            }
          }
        }

        auto find_animation_channel = [&]( animation_node* node )
        {
          for( int d = 0; d < s.animations.size(); ++d )
          {
            for( int e = 0; e < s.animations[d].channels.size(); ++e )
            {
              if( s.animations[d].channels[e].name == node->name )
              {
                node->animation_id = d;
                node->channel_id = e;
                return;
              }
            }
          }

          node->animation_id = -1;
          node->channel_id = -1;
        };

        std::function< void( aiNode*, animation_node* ) > traverse_node;
        traverse_node = [&]( aiNode* node, animation_node* n )
        {
          n->name = node->mName.C_Str();
          find_animation_channel( n );
          n->children.resize( node->mNumChildren );

          memcpy( &n->transformation[0][0], &node->mTransformation[0][0], sizeof( mat4 ) );
          n->transformation = transpose( n->transformation );

          n->bone_idx = -1;

          for( int d = 0; d < s.bone_mapping.size(); ++d )
          {
            auto it = s.bone_mapping.find( n->name );
            if( it != s.bone_mapping.end() )
            {
              n->bone_idx = it->second;
            }
          }

          for( int d = 0; d < n->children.size(); ++d )
          {
            n->children[d].parent = n;

            traverse_node( node->mChildren[d], &n->children[d] );
          }
        };

        for( int c = orig_size; c < s.meshes.size(); ++c )
        {
          s.meshes[c].root_node = new animation_node();

          s.meshes[c].root_node->parent = 0;

          traverse_node( the_scene->mRootNode, s.meshes[c].root_node );
        }
      }

      for( auto& c : s.meshes )
        c.upload();
    }

    void write_mesh( const std::string& path )
    {
      std::fstream f;
      f.open( path, std::ios::out | std::ios::binary );

      if( !f.is_open() )
      {
        std::cerr << "Couldn't open file." << std::endl;
        return;
      }

      unsigned size = indices.size();
      f.write( (const char*)&size, sizeof( unsigned ) ); //write out num of faces
      size = vertices.size() / 3.0f;
      f.write( (const char*)&size, sizeof( unsigned ) ); //write out num of vertices
      size = !normals.empty();
      f.write( (const char*)&size, sizeof( unsigned ) ); //does it have normals?
      size = !tex_coords.empty();
      f.write( (const char*)&size, sizeof( unsigned ) ); //does it have tex coords?

      f.write( (const char*)&indices[0], sizeof(unsigned)* indices.size() );
      /*for( int c = 0; c < indices.size(); ++c )
        {
        f.write((const char*)&indices[c].x, sizeof(uvec3));
        }*/

      f.write( (const char*)&vertices[0], sizeof(float)* vertices.size() );
      /*for( int c = 0; c < vertices.size(); ++c )
        {
        f.write((const char*)&vertices[c].x, sizeof(vec3));
        }*/

      if( !normals.empty() )
      {
        f.write( (const char*)&normals[0], sizeof(float)* normals.size() );
        /*for( int c = 0; c < normals.size(); ++c )
            {
            f.write((const char*)&normals[c].x, sizeof(vec3));
            }*/
      }

      if( !tex_coords.empty() )
      {
        f.write( (const char*)&tex_coords[0], sizeof(float)* tex_coords.size() );
        /*for( int c = 0; c < tex_coords.size(); ++c )
            {
            f.write((const char*)&tex_coords[c].x, sizeof(vec2));
            }*/
      }

      f.close();
    }

    void read_mesh( const std::string& path )
    {
      std::fstream f;
      f.open( path, std::ios::in | std::ios::binary );

      if( !f.is_open() )
      {
        std::cerr << "Couldn't open file." << std::endl;
        return;
      }

      unsigned int num_faces = 0, num_vertices = 0, has_normals = 0, has_tex_coords = 0;
      char* buffer = new char[sizeof( unsigned int )];

      f.read( buffer, sizeof( unsigned int ) );
      num_faces = *(unsigned int*)buffer;
      f.read( buffer, sizeof( unsigned int ) );
      num_vertices = *(unsigned int*)buffer;
      f.read( buffer, sizeof( unsigned int ) );
      has_normals = *(unsigned int*)buffer;
      f.read( buffer, sizeof( unsigned int ) );
      has_tex_coords = *(unsigned int*)buffer;

#ifdef WRITESTATS
      std::cout << "Num faces: " << num_faces << std::endl;
      std::cout << "Num vertices: " << num_vertices << std::endl;
      std::cout << "Has normals: " << ( has_normals ? "yes" : "no" ) << std::endl;
      std::cout << "Has tex coords: " << ( has_tex_coords ? "yes" : "no" ) << std::endl;
#endif

      delete[] buffer;
      //buffer = new char[sizeof( unsigned int ) * 3];

#ifdef WRITECOMPONENTS
      std::cout << "Indices: " << std::endl;
#endif
      indices.resize( num_faces );
      f.read( (char*)&indices[0], sizeof(unsigned)* num_faces );
      /*
        for( int c = 0; c < num_faces; ++c )
        {
        f.read( buffer, sizeof( unsigned int ) * 3 );
        unsigned int x = *( unsigned int* )( buffer + 0 * sizeof( unsigned int ) );
        unsigned int y = *( unsigned int* )( buffer + 1 * sizeof( unsigned int ) );
        unsigned int z = *( unsigned int* )( buffer + 2 * sizeof( unsigned int ) );
        indices.push_back( uvec3( x, y, z ) );
        #ifdef WRITECOMPONENTS
        std::cout << x << " " << y << " " << z << std::endl;
        #endif
        }*/

      //delete [] buffer;
      //buffer = new char[sizeof( float ) * 3];

#ifdef WRITECOMPONENTS
      std::cout << std::endl << "Vertices: " << std::endl;
#endif
      vertices.resize( num_vertices * 3 );
      f.read( (char*)&vertices[0], sizeof(float)* num_vertices * 3 );
      /*for( int c = 0; c < num_vertices; ++c )
      {
      f.read( buffer, sizeof( float ) * 3 );
      float x = *( float* )( buffer + 0 * sizeof( float ) );
      float y = *( float* )( buffer + 1 * sizeof( float ) );
      float z = *( float* )( buffer + 2 * sizeof( float ) );
      vertices.push_back( vec3( x, y, z ) );
      #ifdef WRITECOMPONENTS
      std::cout << x << " " << y << " " << z << std::endl;
      #endif
      }*/

      if( has_normals )
      {
#ifdef WRITECOMPONENTS
        std::cout << std::endl << "Normals: " << std::endl;
#endif
        normals.resize( num_vertices * 3 );
        f.read( (char*)&normals[0], sizeof(float)* num_vertices * 3 );
        /*for( int c = 0; c < num_vertices; ++c )
        {
        f.read( buffer, sizeof( float ) * 3 );
        float x = *( float* )( buffer + 0 * sizeof( float ) );
        float y = *( float* )( buffer + 1 * sizeof( float ) );
        float z = *( float* )( buffer + 2 * sizeof( float ) );
        normals.push_back( vec3( x, y, z ) );
        #ifdef WRITECOMPONENTS
        std::cout << x << " " << y << " " << z << std::endl;
        #endif
        }*/
      }

      //delete [] buffer;
      //buffer = new char[sizeof( float ) * 2];

      if( has_tex_coords )
      {
#ifdef WRITECOMPONENTS
        std::cout << std::endl << "Tex coords: " << std::endl;
#endif
        tex_coords.resize( num_vertices * 2 );
        f.read( (char*)&tex_coords[0], sizeof(float)* num_vertices * 2 );
        /*for( int c = 0; c < num_vertices; ++c )
        {
        f.read( buffer, sizeof( float ) * 2 );
        float x = *( float* )( buffer + 0 * sizeof( float ) );
        float y = *( float* )( buffer + 1 * sizeof( float ) );
        tex_coords.push_back( vec2( x, y ) );
        #ifdef WRITECOMPONENTS
        std::cout << x << " " << y << std::endl;
        #endif
        }*/
      }

      //delete [] buffer;

      f.close();
    }

    void upload()
    {
      glGenVertexArrays( 1, &vao );
      glBindVertexArray( vao );

      glGenBuffers( 1, &vbos[VERTEX] );
      glBindBuffer( GL_ARRAY_BUFFER, vbos[VERTEX] );
      glBufferData( GL_ARRAY_BUFFER, sizeof(float)* vertices.size(), &vertices[0], GL_STATIC_DRAW );
      glEnableVertexAttribArray( VERTEX );
      glVertexAttribPointer( VERTEX, 3, GL_FLOAT, 0, 0, 0 );

      if( normals.size() > 0 )
      {
        glGenBuffers( 1, &vbos[NORMAL] );
        glBindBuffer( GL_ARRAY_BUFFER, vbos[NORMAL] );
        glBufferData( GL_ARRAY_BUFFER, sizeof(float)* normals.size(), &normals[0], GL_STATIC_DRAW );
        glEnableVertexAttribArray( NORMAL );
        glVertexAttribPointer( NORMAL, 3, GL_FLOAT, 0, 0, 0 );
      }

      if( tex_coords.size() > 0 )
      {
        glGenBuffers( 1, &vbos[TEX_COORD] );
        glBindBuffer( GL_ARRAY_BUFFER, vbos[TEX_COORD] );
        glBufferData( GL_ARRAY_BUFFER, sizeof(float)* tex_coords.size(), &tex_coords[0], GL_STATIC_DRAW );
        glEnableVertexAttribArray( TEX_COORD );
        glVertexAttribPointer( TEX_COORD, 2, GL_FLOAT, 0, 0, 0 );
      }

      if( tangents.size() > 0 )
      {
        glGenBuffers( 1, &vbos[TANGENT] );
        glBindBuffer( GL_ARRAY_BUFFER, vbos[TANGENT] );
        glBufferData( GL_ARRAY_BUFFER, sizeof(float)* tangents.size(), &tangents[0], GL_STATIC_DRAW );
        glEnableVertexAttribArray( TANGENT );
        glVertexAttribPointer( TANGENT, 3, GL_FLOAT, 0, 0, 0 );
      }

      if( bone_ids.size() > 0 )
      {
        glGenBuffers( 1, &vbos[BONE_IDS] );
        glBindBuffer( GL_ARRAY_BUFFER, vbos[BONE_IDS] );
        glBufferData( GL_ARRAY_BUFFER, sizeof(ivec4)* bone_ids.size(), &bone_ids[0], GL_STATIC_DRAW );
        glEnableVertexAttribArray( BONE_IDS );
        glVertexAttribIPointer( BONE_IDS, 4, GL_INT, 0, 0 );
      }

      if( bone_weights.size() > 0 )
      {
        glGenBuffers( 1, &vbos[BONE_WEIGHTS] );
        glBindBuffer( GL_ARRAY_BUFFER, vbos[BONE_WEIGHTS] );
        glBufferData( GL_ARRAY_BUFFER, sizeof(vec4)* bone_weights.size(), &bone_weights[0], GL_STATIC_DRAW );
        glEnableVertexAttribArray( BONE_WEIGHTS );
        glVertexAttribPointer( BONE_WEIGHTS, 4, GL_FLOAT, 0, 0, 0 );
      }

      glGenBuffers( 1, &vbos[INDEX] );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbos[INDEX] );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned)* indices.size(), &indices[0], GL_STATIC_DRAW );

      glBindVertexArray( 0 );
      //glBindBuffer( GL_ARRAY_BUFFER, 0 );
      //glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

      rendersize = indices.size();
    }

    void render()
    {
      glBindVertexArray( vao );
      glDrawElements( GL_TRIANGLES, rendersize, GL_UNSIGNED_INT, 0 );
    }
  };
}

#endif
