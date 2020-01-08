#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Wl2.h>
#include <wayland-egl.h>

#ifdef GL_GLES
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#endif

#include "ecore_wl2_suite.h"

#define WIDTH 480
#define HEIGHT 360

typedef struct _Test_Data {
     Ecore_Wl2_Display *display;
     Ecore_Wl2_Window *win;
     Ecore_Wl2_Frame_Cb_Handle *frame_callback_handler;
     Ecore_Event_Handler *handler;

     struct wl_surface *surface;
     struct wl_egl_window *egl_window;

     int width;
     int height;
     int frame_callback_count;

#ifdef GL_GLES
     EGLDisplay egl_display;
     EGLConfig egl_conf;
     EGLSurface egl_surface;
     EGLContext egl_context;
#endif
} Test_Data;

static Ecore_Wl2_Display *
_display_connect(void)
{
   return ecore_wl2_display_connect(NULL);
}

static Ecore_Wl2_Window *
_window_create(Ecore_Wl2_Display *disp)
{
   return ecore_wl2_window_new(disp, NULL, 100, 100, WIDTH, HEIGHT);
}

static struct wl_surface *
_surface_get(Ecore_Wl2_Window *win)
{
   return ecore_wl2_window_surface_get(win);
}

#ifdef GL_GLES
static void
_init_egl(Test_Data *td)
{
   eglBindAPI(EGL_OPENGL_API);
   EGLint num_config;

   EGLint attributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE
   };

   td->egl_display = eglGetDisplay((EGLNativeDisplayType)ecore_wl2_display_get(td->display));
   eglInitialize(td->egl_display, NULL, NULL);
   eglChooseConfig(td->egl_display, attributes, &td->egl_conf, 1, &num_config);
   td->egl_context = eglCreateContext(td->egl_display, td->egl_conf, EGL_NO_CONTEXT, NULL);

   td->egl_window = wl_egl_window_create(td->surface, td->width, td->height);
   td->egl_surface = eglCreateWindowSurface(td->egl_display,
                                            td->egl_conf, td->egl_window, NULL);

   eglMakeCurrent(td->egl_display, td->egl_surface, td->egl_surface, td->egl_context);
}

static void
_term_egl(Test_Data *td)
{
   eglDestroySurface(td->egl_display, td->egl_surface);
   wl_egl_window_destroy(td->egl_window);
   eglDestroyContext(td->egl_display, td->egl_context);
   eglTerminate(td->egl_display);
}
#endif

EFL_START_TEST(wl2_window_new)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_surface_test)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   struct wl_surface *surf;
   int id = -1;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   surf = ecore_wl2_window_surface_get(win);
   ck_assert(surf != NULL);

   id = ecore_wl2_window_surface_id_get(win);
   ck_assert_int_gt(id, 0);

   ck_assert_int_eq(wl_proxy_get_id((struct wl_proxy *)surf), id);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_rotation)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   int rot = -1;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   rot = ecore_wl2_window_rotation_get(win);
   ck_assert_int_ge(rot, 0);

   ecore_wl2_window_rotation_set(win, 90);

   rot = ecore_wl2_window_rotation_get(win);
   fail_if(rot != 90);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_output_find)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Ecore_Wl2_Output *out;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   out = ecore_wl2_window_output_find(win);
   ck_assert(out != NULL);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_aux_hints_supported_get)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_List *l;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   l = ecore_wl2_window_aux_hints_supported_get(win);
   ck_assert(l != NULL);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_display_get)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ck_assert(ecore_wl2_window_display_get(win) != NULL);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_alpha)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_Bool alpha = EINA_FALSE;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_alpha_set(win, EINA_TRUE);

   alpha = ecore_wl2_window_alpha_get(win);
   fail_if(alpha != EINA_TRUE);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_floating_mode)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_Bool f = EINA_FALSE;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_floating_mode_set(win, EINA_TRUE);

   f = ecore_wl2_window_floating_mode_get(win);
   fail_if(f != EINA_TRUE);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_focus_skip)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_Bool skip = EINA_FALSE;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_focus_skip_set(win, EINA_TRUE);

   skip = ecore_wl2_window_focus_skip_get(win);
   fail_if(skip != EINA_TRUE);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_fullscreen)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_Bool full = EINA_FALSE;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_fullscreen_set(win, EINA_TRUE);

   full = ecore_wl2_window_fullscreen_get(win);
   fail_if(full != EINA_TRUE);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_maximize)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_Bool m = EINA_FALSE;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_maximized_set(win, EINA_TRUE);

   m = ecore_wl2_window_maximized_get(win);
   fail_if(m != EINA_TRUE);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_preferred_rot)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   int rot = 0;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_preferred_rotation_set(win, 90);

   rot = ecore_wl2_window_preferred_rotation_get(win);
   fail_if(rot != 90);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_rotation_app)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_Bool r = EINA_FALSE;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_rotation_app_set(win, EINA_TRUE);

   r = ecore_wl2_window_rotation_app_get(win);
   fail_if(r != EINA_TRUE);
}
EFL_END_TEST

EFL_START_TEST(wl2_wm_window_rotation_app)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_Bool r = EINA_FALSE;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_wm_rotation_supported_set(win, EINA_TRUE);

   r = ecore_wl2_window_wm_rotation_supported_get(win);
   fail_if(r != EINA_TRUE);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_geometry)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   int x, y, w, h;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_geometry_set(win, 10, 10, 100, 100);

   ecore_wl2_window_geometry_get(win, &x, &y, &w, &h);

   fail_if(x != 10);
   fail_if(y != 10);
   fail_if(w != 100);
   fail_if(h != 100);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_type)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Ecore_Wl2_Window_Type type = ECORE_WL2_WINDOW_TYPE_NONE;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_type_set(win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   type = ecore_wl2_window_type_get(win);
   fail_if(type != ECORE_WL2_WINDOW_TYPE_TOPLEVEL);
}
EFL_END_TEST

#ifdef GL_GLES
static void
_test_activated_frame_cb(Ecore_Wl2_Window *win EINA_UNUSED, uint32_t timestamp EINA_UNUSED, void *data)
{
   Test_Data *td = data;

   td->frame_callback_count++;
   if (td->frame_callback_count % 4 == 0)
     glClearColor(0.0, 1.0, 0.0, 0.0);
   else if (td->frame_callback_count % 4 == 1)
     glClearColor(0.0, 0.0, 1.0, 0.0);
   else if (td->frame_callback_count % 4 == 2)
     glClearColor(0.0, 0.0, 0.0, 1.0);
   else
     glClearColor(1.0, 0.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);
   glFlush();

   eglSwapBuffers(td->egl_display, td->egl_surface);

   ecore_wl2_window_commit(td->win, EINA_TRUE);
}

static Eina_Bool
_test_activated_configure_complete(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Test_Data *td = data;

   td->frame_callback_handler = ecore_wl2_window_frame_callback_add(td->win, _test_activated_frame_cb, td);
   ecore_wl2_window_commit(td->win, EINA_TRUE);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_test_activated_window_activate(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   //TC Pass
   ecore_main_loop_quit();

   return ECORE_CALLBACK_PASS_ON;
}

EFL_START_TEST(wl2_window_activated)
{
   Test_Data *td;

   ecore_wl2_init();

   td = calloc(1, sizeof(Test_Data));
   td->width = WIDTH;
   td->height = HEIGHT;
   td->frame_callback_count = 0;

   td->display = _display_connect();
   ck_assert(td->display != NULL);

   td->win = _window_create(td->display);
   ck_assert(td->win != NULL);

   ecore_wl2_window_type_set(td->win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   td->surface = _surface_get(td->win);
   ck_assert(td->surface != NULL);

   ecore_wl2_window_show(td->win);

   _init_egl(td);
   td->handler = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE,
                                         _test_activated_configure_complete, td);
   ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_ACTIVATE,
                           _test_activated_window_activate, NULL);

   ecore_main_loop_begin();

   _term_egl(td);
   ecore_wl2_shutdown();
   free(td);
}
EFL_END_TEST
#else
EFL_START_TEST(wl2_window_activated)
{
   fail_if("No GL enabled GL should be enabled for API test");
}
EFL_END_TEST
#endif

EFL_START_TEST(wl2_window_aspect)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   int w, h;
   unsigned int aspect;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_aspect_set(win, 1, 1, 3);
   ecore_wl2_window_aspect_get(win, &w, &h, &aspect);

   fail_if(w != 1);
   fail_if(h != 1);
   fail_if(aspect != 3);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_title)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   const char *title;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_title_set(win, "TEST");
   title = ecore_wl2_window_title_get(win);

   fail_if(strcmp(title, "TEST"));
}
EFL_END_TEST

EFL_START_TEST(wl2_window_class)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   const char *class;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_class_set(win, "TEST");
   class = ecore_wl2_window_class_get(win);

   fail_if(strcmp(class, "TEST"));
}
EFL_END_TEST

EFL_START_TEST(wl2_window_available_rotation)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   Eina_Bool ret;
   int rots[2] = { 90, 180 };
   int *ret_rots;
   unsigned int ret_count;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_available_rotations_set(win, rots, 2);

   ret = ecore_wl2_window_available_rotations_get(win, &ret_rots, &ret_count);

   fail_if(ret != EINA_TRUE);
   fail_if(ret_rots[0] != 90);
   fail_if(ret_rots[1] != 180);
   fail_if(ret_count != 2);

   free(ret_rots);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_role)
{
   Ecore_Wl2_Display *disp;
   Ecore_Wl2_Window *win;
   const char *role;

   disp = _display_connect();
   ck_assert(disp != NULL);

   win = _window_create(disp);
   ck_assert(win != NULL);

   ecore_wl2_window_role_set(win, "TEST");
   role = ecore_wl2_window_role_get(win);

   fail_if(strcmp(role, "TEST"));
}
EFL_END_TEST

static void
_test_commit_frame_cb(Ecore_Wl2_Window *win EINA_UNUSED, uint32_t timestamp EINA_UNUSED, void *data)
{
   Test_Data *td = data;
   td->frame_callback_count++;
   ecore_main_loop_quit();
}

static Eina_Bool
_test_commit_configure_complete(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Test_Data *td = data;

   td->frame_callback_handler = ecore_wl2_window_frame_callback_add(td->win, _test_commit_frame_cb, td);
   ecore_wl2_window_commit(td->win, EINA_TRUE);

   return ECORE_CALLBACK_PASS_ON;
}

EFL_START_TEST(wl2_window_commit)
{
   Test_Data *td;

   ecore_wl2_init();

   td = calloc(1, sizeof(Test_Data));
   td->width = WIDTH;
   td->height = HEIGHT;
   td->frame_callback_count = 0;

   td->display = _display_connect();
   ck_assert(td->display != NULL);

   td->win = _window_create(td->display);
   ck_assert(td->win != NULL);

   ecore_wl2_window_type_set(td->win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   td->surface = _surface_get(td->win);
   ck_assert(td->surface != NULL);

   ecore_wl2_window_show(td->win);

   td->handler = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE,
                                         _test_commit_configure_complete, td);

   ecore_main_loop_begin();

   //Check if the frame callback was called properly by ecore_wl2_window_commit().
   fail_if(td->frame_callback_count == 0);

   ecore_wl2_shutdown();
   free(td);
}
EFL_END_TEST

static void
_test_frame_callback_frame_cb(Ecore_Wl2_Window *win EINA_UNUSED, uint32_t timestamp EINA_UNUSED, void *data)
{
   Test_Data *td = data;
   td->frame_callback_count++;
   if (td->frame_callback_count == 1)
     {
        ecore_wl2_window_frame_callback_del(td->frame_callback_handler);
        td->frame_callback_handler = NULL;
        ecore_main_loop_quit();
     }
}

static Eina_Bool
_test_frame_callback_configure_complete(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Test_Data *td = data;

   td->frame_callback_handler = ecore_wl2_window_frame_callback_add(td->win, _test_frame_callback_frame_cb, td);
   ecore_wl2_window_commit(td->win, EINA_TRUE);

   return ECORE_CALLBACK_PASS_ON;
}

EFL_START_TEST(wl2_window_frame_callback)
{
   Test_Data *td;

   ecore_wl2_init();

   td = calloc(1, sizeof(Test_Data));
   td->width = WIDTH;
   td->height = HEIGHT;
   td->frame_callback_count = 0;

   td->display = _display_connect();
   ck_assert(td->display != NULL);

   td->win = _window_create(td->display);
   ck_assert(td->win != NULL);

   ecore_wl2_window_type_set(td->win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   td->surface = _surface_get(td->win);
   ck_assert(td->surface != NULL);

   ecore_wl2_window_show(td->win);

   td->handler = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE,
                                         _test_frame_callback_configure_complete, td);

   ecore_main_loop_begin();

   //Check if the frame callback called after then it sets NULL or not.
   fail_if(td->frame_callback_count != 1);
   fail_if(td->frame_callback_handler != NULL);

   ecore_wl2_shutdown();
   free(td);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_free)
{
   Ecore_Wl2_Window *t_win;
   Test_Data *td;

   ecore_wl2_init();

   td = calloc(1, sizeof(Test_Data));

   td->display = _display_connect();
   ck_assert(td->display != NULL);

   td->win = _window_create(td->display);
   ck_assert(td->win != NULL);

   ecore_wl2_window_type_set(td->win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   td->surface = _surface_get(td->win);
   ck_assert(td->surface != NULL);

   ecore_wl2_window_show(td->win);

   t_win = ecore_wl2_display_window_find_by_surface(td->display, td->surface);

   fail_if(td->win != t_win);

   ecore_wl2_window_free(td->win);
   t_win = ecore_wl2_display_window_find_by_surface(td->display, td->surface);

   //Check the returned window with freed window.
   fail_if(td->win == t_win);

   ecore_wl2_shutdown();
   free(td);
}
EFL_END_TEST

static Eina_Bool
_test_hide_window_hide(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   //TC pass
   ecore_main_loop_quit();

   return ECORE_CALLBACK_PASS_ON;
}

EFL_START_TEST(wl2_window_hide)
{
   Test_Data *td;

   ecore_wl2_init();

   td = calloc(1, sizeof(Test_Data));
   td->width = WIDTH;
   td->height = HEIGHT;
   td->frame_callback_count = 0;

   td->display = _display_connect();
   ck_assert(td->display != NULL);

   td->win = _window_create(td->display);
   ck_assert(td->win != NULL);

   ecore_wl2_window_type_set(td->win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   td->surface = _surface_get(td->win);
   ck_assert(td->surface != NULL);

   ecore_wl2_window_show(td->win);

   ecore_wl2_window_hide(td->win);
   td->handler = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_HIDE,
                                         _test_hide_window_hide, NULL);

   ecore_main_loop_begin();

   ecore_wl2_shutdown();
   free(td);
}
EFL_END_TEST

EFL_START_TEST(wl2_window_shell_surface_exists)
{
   Test_Data *td;

   ecore_wl2_init();

   td = calloc(1, sizeof(Test_Data));
   td->width = WIDTH;
   td->height = HEIGHT;
   td->frame_callback_count = 0;

   td->display = _display_connect();
   ck_assert(td->display != NULL);

   td->win = _window_create(td->display);
   ck_assert(td->win != NULL);

   ecore_wl2_window_type_set(td->win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   td->surface = _surface_get(td->win);
   ck_assert(td->surface != NULL);

   ecore_wl2_window_show(td->win);

   //window_show function will create shell surface. then checks it.
   fail_if(ecore_wl2_window_shell_surface_exists(td->win) == EINA_FALSE);

   ecore_wl2_shutdown();
   free(td);
}
EFL_END_TEST

static Eina_Bool
_test_show_window_show(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   //TC pass
   ecore_main_loop_quit();

   return ECORE_CALLBACK_PASS_ON;
}

EFL_START_TEST(wl2_window_show)
{
   Test_Data *td;

   ecore_wl2_init();

   td = calloc(1, sizeof(Test_Data));
   td->width = WIDTH;
   td->height = HEIGHT;
   td->frame_callback_count = 0;

   td->display = _display_connect();
   ck_assert(td->display != NULL);

   td->win = _window_create(td->display);
   ck_assert(td->win != NULL);

   ecore_wl2_window_type_set(td->win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   td->surface = _surface_get(td->win);
   ck_assert(td->surface != NULL);

   ecore_wl2_window_show(td->win);

   td->handler = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_SHOW,
                                         _test_show_window_show, NULL);

   ecore_main_loop_begin();

   ecore_wl2_shutdown();
   free(td);
}
EFL_END_TEST

static Eina_Bool _window_configure_event_called = EINA_FALSE;

static Eina_Bool
_test_update_window_configure(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   _window_configure_event_called = EINA_TRUE;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_test_update_window_configure_complete(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   //Checks if the configure_complete calling before configure calling
   //when ecore_wl2_window_update_begin() called.
   fail_if(_window_configure_event_called == EINA_TRUE);

   ecore_main_loop_quit();

   return ECORE_CALLBACK_PASS_ON;
}

EFL_START_TEST(wl2_window_update_begin)
{
   Test_Data *td;

   ecore_wl2_init();

   td = calloc(1, sizeof(Test_Data));
   td->width = WIDTH;
   td->height = HEIGHT;
   td->frame_callback_count = 0;

   td->display = _display_connect();
   ck_assert(td->display != NULL);

   td->win = _window_create(td->display);
   ck_assert(td->win != NULL);

   ecore_wl2_window_type_set(td->win, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);

   td->surface = _surface_get(td->win);
   ck_assert(td->surface != NULL);

   ecore_wl2_window_show(td->win);

   ecore_wl2_window_update_begin(td->win);
   ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_CONFIGURE,
                           _test_update_window_configure, NULL);
   ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE,
                           _test_update_window_configure_complete, NULL);

   ecore_main_loop_begin();

   ecore_wl2_shutdown();
   free(td);
}
EFL_END_TEST

void
ecore_wl2_test_window(TCase *tc)
{
   if (getenv("WAYLAND_DISPLAY"))
     {
        /* window tests can only run if there is an existing compositor */
        tcase_add_test(tc, wl2_window_new);
        tcase_add_test(tc, wl2_window_surface_test);
        tcase_add_test(tc, wl2_window_rotation);
        tcase_add_test(tc, wl2_window_output_find);
        if (getenv("E_START"))
          tcase_add_test(tc, wl2_window_aux_hints_supported_get);
        tcase_add_test(tc, wl2_window_display_get);
        tcase_add_test(tc, wl2_window_alpha);
        tcase_add_test(tc, wl2_window_floating_mode);
        tcase_add_test(tc, wl2_window_focus_skip);
        tcase_add_test(tc, wl2_window_fullscreen);
        tcase_add_test(tc, wl2_window_maximize);
        tcase_add_test(tc, wl2_window_preferred_rot);
        tcase_add_test(tc, wl2_window_rotation_app);
        tcase_add_test(tc, wl2_wm_window_rotation_app);
        tcase_add_test(tc, wl2_window_geometry);
        tcase_add_test(tc, wl2_window_type);
        tcase_add_test(tc, wl2_window_activated);
        tcase_add_test(tc, wl2_window_available_rotation);
        tcase_add_test(tc, wl2_window_aspect);
        tcase_add_test(tc, wl2_window_class);
        tcase_add_test(tc, wl2_window_title);
        tcase_add_test(tc, wl2_window_role);
        tcase_add_test(tc, wl2_window_commit);
        tcase_add_test(tc, wl2_window_frame_callback);
        tcase_add_test(tc, wl2_window_free);
        tcase_add_test(tc, wl2_window_hide);
        tcase_add_test(tc, wl2_window_shell_surface_exists);
        tcase_add_test(tc, wl2_window_show);
        tcase_add_test(tc, wl2_window_update_begin);
     }
}
