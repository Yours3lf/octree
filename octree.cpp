#include "framework.h"

#include "octree.h"

using namespace prototyper;

int main( int argc, char** argv )
{
  vector<pair<unsigned, shape*> > objects;

  //contains the id of a box or sphere
  auto o = new octree<unsigned>(aabb(vec3(0), vec3(1)));

  shape::set_up_intersection();
  o->set_up_octree(&o);

  int size = 2500;

  objects.reserve( size * size );

  int counter = 0;
  for(int x = 0; x < size; ++x)
    for(int y = 0; y < size; ++y)
    {
      objects.push_back(make_pair(counter++, new aabb(vec3(x * 10, 0, y * 10), vec3(1))));
      o->insert(objects.back().first, objects.back().second);
    }

    /*
    cout << o->has_children() << endl;
    cout << o->is_empty() << endl;
    cout << o->is_root() << endl;

    for( int c = 0; c < 4; ++c )
    {
    objects.push_back(make_pair(c, new sphere(vec3(1,1,1), 1)));
    o->insert(objects[c].first, (sphere*)objects[c].second);
    }


    for( int c = 0; c < 4; ++c )
    {
    ((sphere*)objects[c].second)->center += vec3(c * 10, 1, 1);
    }

    o->update(objects);

    for(int c = 0; c < 100; ++c)
    o->update(objects);

    //o->remove(3);

    o->update(objects);
    */

    map<string, string> args;

    for( int c = 1; c < argc; ++c )
    {
      args[argv[c]] = c + 1 < argc ? argv[c + 1] : "";
      ++c;
    }

    cout << "Arguments: " << endl;
    for_each( args.begin(), args.end(), []( pair<string, string> p )
    {
      cout << p.first << " " << p.second << endl;
    } );

    uvec2 screen( 0 );
    bool fullscreen = false;
    bool silent = false;
    string title = "Basic CPP to get started";

    /*
    * Process program arguments
    */

    stringstream ss;
    ss.str( args["--screenx"] );
    ss >> screen.x;
    ss.clear();
    ss.str( args["--screeny"] );
    ss >> screen.y;
    ss.clear();

    if( screen.x == 0 )
    {
      screen.x = 1280;
    }

    if( screen.y == 0 )
    {
      screen.y = 720;
    }

    try
    {
      args.at( "--fullscreen" );
      fullscreen = true;
    }
    catch( ... ) {}

    try
    {
      args.at( "--help" );
      cout << title << ", written by Marton Tamas." << endl <<
        "Usage: --silent      //don't display FPS info in the terminal" << endl <<
        "       --screenx num //set screen width (default:1280)" << endl <<
        "       --screeny num //set screen height (default:720)" << endl <<
        "       --fullscreen  //set fullscreen, windowed by default" << endl <<
        "       --help        //display this information" << endl;
      return 0;
    }
    catch( ... ) {}

    try
    {
      args.at( "--silent" );
      silent = true;
    }
    catch( ... ) {}

    /*
    * Initialize the OpenGL context
    */

    framework frm;
    frm.init( screen, title, fullscreen );

    //set opengl settings
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
    glFrontFace( GL_CCW );
    glEnable( GL_CULL_FACE );
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClearDepth( 1.0f );

    frm.get_opengl_error();

    /*
    * Set up mymath
    */

    camera<float> cam;
    frame<float> the_frame;

    float cam_fov = 45.0f;
    float cam_near = 1.0f;
    float cam_far = 1000.0f;

    the_frame.set_perspective( radians( cam_fov ), ( float )screen.x / ( float )screen.y, cam_near, cam_far );

    frame<float> top_frame;
    camera<float> cam_top;
    top_frame.set_perspective( radians( cam_fov ), ( float )screen.x / ( float )screen.y, cam_near, cam_far );
    cam_top.rotate_x( radians( 90.0f ) );
    cam_top.move_forward( -70.0f );

    glViewport( 0, 0, screen.x, screen.y );

    /*
    * Set up the scene
    */

    float move_amount = 5;

    GLuint box = frm.create_box();

    /*
    * Set up the shaders
    */

    GLuint debug_shader = 0;
    frm.load_shader( debug_shader, GL_VERTEX_SHADER, "../shaders/octree/debug.vs" );
    frm.load_shader( debug_shader, GL_FRAGMENT_SHADER, "../shaders/octree/debug.ps" );

    GLuint lighting_shader = 0;
    frm.load_shader( lighting_shader, GL_VERTEX_SHADER, "../shaders/octree/lighting.vs" );
    frm.load_shader( lighting_shader, GL_FRAGMENT_SHADER, "../shaders/octree/lighting.ps" );

    GLint debug_mvp_mat_loc = glGetUniformLocation( debug_shader, "mvp" );
    GLint debug_col_loc = glGetUniformLocation( debug_shader, "col" );

    GLint lighting_mvp_mat_loc = glGetUniformLocation( lighting_shader, "mvp" );
    GLint lighting_normal_mat_loc = glGetUniformLocation( lighting_shader, "normal_mat" );
    GLint lighting_thecolor_loc = glGetUniformLocation( lighting_shader, "thecolor" );

    /*
    * Handle events
    */

    //allows to switch between the topdown and normal camera
    camera<float>* cam_ptr = &cam;

    bool cull = true;
    bool render_octree = false;

    bool warped = false, ignore = true;
    vec2 movement_speed = vec2(0);

    auto event_handler = [&]( const sf::Event & ev )
    {
      switch( ev.type )
      {
      case sf::Event::MouseMoved:
        {
          vec2 mpos( ev.mouseMove.x / float( screen.x ), ev.mouseMove.y / float( screen.y ) );

          if( warped )
          {
            ignore = false;
          }
          else
          {
            frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
            warped = true;
            ignore = true;
          }

          if( !ignore && all( notEqual( mpos, vec2( 0.5 ) ) ) )
          {
            cam.rotate( radians( -180.0f * ( mpos.x - 0.5f ) ), vec3( 0.0f, 1.0f, 0.0f ) );

            if( cam_ptr != &cam_top )
              cam.rotate_x( radians( -180.0f * ( mpos.y - 0.5f ) ) );

            frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
            warped = true;
          }
        }
      case sf::Event::KeyPressed:
        {
          if( ev.key.code == sf::Keyboard::Space )
          {
            cull = !cull;
          }

          if( ev.key.code == sf::Keyboard::C )
          {
            cam_ptr = &cam;
          }

          if( ev.key.code == sf::Keyboard::X )
          {
            cam_ptr = &cam_top;
          }

          if( ev.key.code == sf::Keyboard::O )
          {
            render_octree = !render_octree;
          }
        }
      default:
        break;
      }
    };

    /*
    * Render
    */

    sf::Clock timer;
    timer.restart();

    sf::Clock movement_timer;
    movement_timer.restart();

    frm.display( [&]
    {
      frm.handle_events( event_handler );

      float seconds = movement_timer.getElapsedTime().asMilliseconds() / 1000.0f;

      if( seconds > 0.01667 )
      {
        //move camera
        if( sf::Keyboard::isKeyPressed( sf::Keyboard::A ) )
        {
          movement_speed.x -= move_amount;
        }

        if( sf::Keyboard::isKeyPressed( sf::Keyboard::D ) )
        {
          movement_speed.x += move_amount;
        }

        if( sf::Keyboard::isKeyPressed( sf::Keyboard::W ) )
        {
          movement_speed.y += move_amount;
        }

        if( sf::Keyboard::isKeyPressed( sf::Keyboard::S ) )
        {
          movement_speed.y -= move_amount;
        }

        if( cam_ptr == &cam_top )
          cam_ptr->move_up( movement_speed.y * seconds );
        else
          cam_ptr->move_forward( movement_speed.y * seconds );

        cam_ptr->move_right( movement_speed.x * seconds );
        movement_speed *= 0.5;

        movement_timer.restart();
      }

      glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

      glBindVertexArray( box );

      //render the boxes
      glUseProgram( lighting_shader );
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); //normal rendering

      mat4 projection = cam_ptr == &cam ? the_frame.projection_matrix : top_frame.projection_matrix;
      mat4 view = cam_ptr->get_matrix();

      frustum f;
      f.set_up( cam, the_frame );
      unsigned counter_octree = 0;
      unsigned counter_brute = 0;

      /**
      glUniform3f(lighting_thecolor_loc, 0, 1, 0);
      for(auto& c : objects)
      {
        if( c.second->intersects(&f) )
        {
          ++counter_brute;
          mvm.push_matrix();
          mvm.translate_matrix(static_cast<aabb*>(c.second)->pos);
          glUniformMatrix4fv( lighting_mvp_mat_loc, 1, false, &ppl.get_model_view_projection_matrix( *cam_ptr )[0][0] );
          glUniformMatrix4fv( lighting_normal_mat_loc, 1, false, &ppl.get_normal_matrix()[0][0] );
          mvm.pop_matrix();

          glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );
        }
      }
      /**/

      /**/
      if(cull)
      {
        static vector<unsigned> culled_objs;
        culled_objs.clear();
        o->get_culled_objects( culled_objs, &f );
        counter_octree = culled_objs.size();
        glUniform3f(lighting_thecolor_loc, 0, 1, 0);
        for(auto& c : culled_objs)
        {
          if(objects[c].second->is_intersecting(&f))
          {
            ++counter_brute;

            mat4 model = create_translation( static_cast<aabb*>( objects[c].second )->get_pos() );
              mat4 mv = view * model;
              mat4 normal_mat = mv;
              mat4 mvp = projection * mv;

            glUniformMatrix4fv( lighting_mvp_mat_loc, 1, false, &mvp[0][0] );
            glUniformMatrix4fv( lighting_normal_mat_loc, 1, false, &normal_mat[0][0] );

            glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );
          }
        }
      }
      else
      {
        glUniform3f(lighting_thecolor_loc, 0, 1, 0);
        for(auto& c : objects)
        {
          if(o->is_in_frustum(c.first, &f))
          {
            ++counter_octree;

            if(objects[c.first].second->is_intersecting(&f))
            {
              ++counter_brute;

              glUniform3f(lighting_thecolor_loc, 0, 1, 0);
            }
            else
              //glUniform3f(lighting_thecolor_loc, 1, 0, 0); //paint objects red that didn't pass any of the culling
              glUniform3f(lighting_thecolor_loc, 0, 1, 0); //only color red the object that failed the octree culling
          }
          else
            glUniform3f(lighting_thecolor_loc, 1, 0, 0);

          mat4 trans = create_translation( static_cast<aabb*>( objects[c.first].second )->get_pos() );
          mat4 mv = view * trans;
          mat4 mvp = projection * mv;
          mat4 normal_mat = mv;
          glUniformMatrix4fv( lighting_mvp_mat_loc, 1, false, &mvp[0][0] );
          glUniformMatrix4fv( lighting_normal_mat_loc, 1, false, &normal_mat[0][0] );

          glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );
        }
      }
      /**/

      ss.str("");
      ss << "Counter octree: " << counter_octree << " - Counter brute: " << counter_brute << " - Display culled objects: " << (!cull ? "true" : "false") << " - Render octree: " << (render_octree ? "true" : "false");
      frm.set_title(ss.str());

      //render the octree
      if( render_octree )
      {
        glUseProgram( debug_shader );
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); //WIREFRAME

        static std::vector<aabb> boxes;
        boxes.clear();
        o->get_boxes( boxes );
        for(auto& c : boxes)
        {
          if(c.is_intersecting(&f))
            glUniform3f(debug_col_loc, 1, 1, 0 );
          else
            glUniform3f(debug_col_loc, 1, 0, 0 );

          mat4 trans = create_translation( c.get_pos() ) * create_scale( c.get_extents() );
          mat4 mv = view * trans;
          mat4 mvp = projection * mv;
          glUniformMatrix4fv( debug_mvp_mat_loc, 1, false, &mvp[0][0] );

          glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );
        }
      }

      //render the top view
      glUseProgram( 0 );
      glMatrixMode( GL_MODELVIEW );
      glLoadMatrixf( &view[0].x );

      glMatrixMode( GL_PROJECTION );
      glLoadMatrixf( &projection[0].x );

      /**/
      if( cam_ptr == &cam_top )
      {
        //draw camera
        mat4 m = mat4::identity;

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);

        vec3 cam_base = cam.pos;

        //m = create_translation( cam.pos + vec3( 0, 1, 0 ) );
        //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
        //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

        glBegin(GL_TRIANGLES);

        float nw = the_frame.near_lr.x - the_frame.near_ll.x;
        float nh = the_frame.near_ul.y - the_frame.near_ll.y;

        float fw = the_frame.far_lr.x - the_frame.far_ll.x;
        float fh = the_frame.far_ul.y - the_frame.far_ll.y;

        vec3 nc = cam.pos - cam.view_dir * the_frame.near_ll.z;
        vec3 fc = cam.pos - cam.view_dir * the_frame.far_ll.z;

        vec3 right = -normalize( cross( cam.up_vector, cam.view_dir ) );

        //near top left
        vec3 ntl = nc + cam.up_vector * nh - right * nw;
        vec3 ntr = nc + cam.up_vector * nh + right * nw;
        vec3 nbl = nc - cam.up_vector * nh - right * nw;
        vec3 nbr = nc - cam.up_vector * nh + right * nw;

        vec3 ftl = fc + cam.up_vector * fh - right * fw;
        vec3 ftr = fc + cam.up_vector * fh + right * fw;
        vec3 fbl = fc - cam.up_vector * fh - right * fw;
        vec3 fbr = fc - cam.up_vector * fh + right * fw;

        glVertex3fv((GLfloat*)&cam_base);
        glVertex3fv((GLfloat*)&ftl);
        glVertex3fv((GLfloat*)&fbl);

        glVertex3fv((GLfloat*)&cam_base);
        glVertex3fv((GLfloat*)&ftr);
        glVertex3fv((GLfloat*)&fbr);

        glVertex3fv((GLfloat*)&cam_base);
        glVertex3fv((GLfloat*)&ftl);
        glVertex3fv((GLfloat*)&ftr);

        glVertex3fv((GLfloat*)&cam_base);
        glVertex3fv((GLfloat*)&fbl);
        glVertex3fv((GLfloat*)&fbr);

        //far plane
        //m = create_translation( vec3( ntl.x, 1, ntl.z ) );
        //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
        //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

        //m = create_translation( vec3( ntr.x, 1, ntr.z ) );
        //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
        //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

        //near plane
        //m = create_translation( vec3( ftl.x, 1, ftl.z ) );
        //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
        //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

        //m = create_translation( vec3( ftr.x, 1, ftr.z ) );
        //glUniformMatrix4fv( debug_model_mat_loc, 1, false, &m[0][0] );
        //glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

        glEnd();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_CULL_FACE);
      }
      /**/

      //draw reference grid
      glUseProgram( 0 );
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); //WIREFRAME
      glDisable( GL_TEXTURE_2D );

      glMatrixMode( GL_MODELVIEW );
      glLoadMatrixf( &view[0].x );

      glMatrixMode( GL_PROJECTION );
      glLoadMatrixf( &projection[0].x );

      int size = 20;
      size /= 2;

      for( int x = -size; x < size + 1; x++ )
      {
        glBegin( GL_LINE_LOOP );
        glVertex3f( x, -2, -size );
        glVertex3f( x, -2, size );
        glEnd();
      };

      for( int y = -size; y < size + 1; y++ )
      {
        glBegin( GL_LINE_LOOP );
        glVertex3f( -size, -2, y );
        glVertex3f( size, -2, y );
        glEnd();
      };

      glEnable( GL_TEXTURE_2D );

      frm.get_opengl_error();
    }, silent );

    return 0;
}
