#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_FOCUS_COMPOSITION_ADAPTER_PROTECTED
#define EFL_UI_FOCUS_OBJECT_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

#include "efl_ui_focus_composition_adapter.eo.h"

#define MY_CLASS EFL_UI_FOCUS_MANAGER_ROOT_FOCUS_CLASS

typedef struct {
   Efl_Ui_Focus_Object *replacement_object;

   Evas_Object *rect;
   Eina_Bool rect_registered;

   Eina_List *iterator_list;
   Eina_Future *focus_transfer;
} Efl_Ui_Focus_Manager_Root_Focus_Data;

static Eina_Value
_unregister_rect(Eo *obj, void *data EINA_UNUSED, const Eina_Value v EINA_UNUSED)
{
   Efl_Ui_Focus_Manager_Root_Focus_Data *pd = efl_data_scope_get(obj, MY_CLASS);

   efl_ui_focus_manager_calc_unregister(obj, pd->rect);
   pd->rect_registered = EINA_FALSE;
   efl_ui_focus_composition_adapter_focus_manager_parent_set(pd->rect, NULL);
   efl_ui_focus_composition_adapter_focus_manager_object_set(pd->rect, NULL);
   pd->focus_transfer = NULL;
   return EINA_VALUE_EMPTY;
}

static Efl_Ui_Focus_Object*
_trap(Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *obj)
{
   if (pd->rect == obj) return pd->replacement_object;
   return obj;
}

static void
_state_eval(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   Efl_Ui_Focus_Object *sub;

   sub = efl_ui_focus_manager_request_subchild(obj, efl_ui_focus_manager_root_get(obj));

   if (sub == pd->rect)
     {
        sub = efl_ui_focus_manager_request_move(obj, EFL_UI_FOCUS_DIRECTION_NEXT, pd->rect, EINA_FALSE);
        if (sub == pd->rect)
          sub = NULL;
     }

   EINA_SAFETY_ON_TRUE_RETURN(sub == pd->rect);

   if (sub && pd->rect_registered)
     {
        pd->focus_transfer = efl_loop_job(efl_main_loop_get());
        efl_future_then(obj, pd->focus_transfer, _unregister_rect);
     }
   else if (!sub && !pd->rect_registered)
     {
        Efl_Ui_Focus_Object *root;

        if (pd->focus_transfer)
          eina_future_cancel(pd->focus_transfer);
        pd->focus_transfer = NULL;

        root = efl_ui_focus_manager_root_get(obj);
        efl_ui_focus_manager_calc_register(obj, pd->rect, root, NULL);
        efl_ui_focus_composition_adapter_focus_manager_parent_set(pd->rect, root);
        efl_ui_focus_composition_adapter_focus_manager_object_set(pd->rect, obj);
        pd->rect_registered = EINA_TRUE;
     }
}

EOLIAN static Eina_Bool
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_calc_register(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *child, Efl_Ui_Focus_Object *parent, Efl_Ui_Focus_Manager *redirect)
{
   if (efl_ui_focus_manager_calc_register(efl_super(obj, MY_CLASS), child, parent, redirect))
     {
        if (child != pd->rect && pd->rect_registered)
          _state_eval(obj, pd);

        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_calc_register_logical(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *child, Efl_Ui_Focus_Object *parent, Efl_Ui_Focus_Manager *redirect)
{
   if (!parent) return EINA_FALSE;
   if (efl_ui_focus_manager_calc_register_logical(efl_super(obj, MY_CLASS), child, parent, redirect))
     {
        if (redirect && pd->rect_registered)
          _state_eval(obj, pd);

        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EOLIAN static void
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_calc_unregister(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *child)
{
   efl_ui_focus_manager_calc_unregister(efl_super(obj, MY_CLASS), child);

   if (child != pd->rect)
     _state_eval(obj, pd);
}

EOLIAN static void
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_manager_focus_set(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *focus)
{
   EINA_SAFETY_ON_NULL_RETURN(focus);
   efl_ui_focus_manager_focus_set(efl_super(obj, MY_CLASS), _trap(pd, focus));
}

EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_manager_focus_get(const Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   return _trap(pd, efl_ui_focus_manager_focus_get(efl_super(obj, MY_CLASS)));
}

EOLIAN static Efl_Ui_Focus_Relations *
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_fetch(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *child)
{
   return efl_ui_focus_manager_fetch(efl_super(obj, MY_CLASS), _trap(pd, child));
}

EOLIAN static Efl_Ui_Focus_Manager_Logical_End_Detail
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_logical_end(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   Efl_Ui_Focus_Manager_Logical_End_Detail res;

   res = efl_ui_focus_manager_logical_end(efl_super(obj, MY_CLASS));

   res.element = _trap(pd, res.element);
   return res;
}

EOLIAN static Eina_Iterator *
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_border_elements_get(const Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   if (pd->rect_registered)
     return eina_list_iterator_new(pd->iterator_list);

   return efl_ui_focus_manager_border_elements_get(efl_super(obj, MY_CLASS));
}

EOLIAN static Eina_Iterator *
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_viewport_elements_get(const Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Eina_Rect viewport EINA_UNUSED)
{
   if (pd->rect_registered)
     return eina_list_iterator_new(pd->iterator_list);

   return efl_ui_focus_manager_border_elements_get(efl_super(obj, MY_CLASS));
}

EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_request_move(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Direction direction, Efl_Ui_Focus_Object *child, Eina_Bool logical)
{
   return _trap(pd, efl_ui_focus_manager_request_move(efl_super(obj, MY_CLASS), direction, child, logical));
}

EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_move(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Direction direction)
{
   return _trap(pd, efl_ui_focus_manager_move(efl_super(obj, MY_CLASS), direction));
}

EOLIAN static Efl_Canvas_Object*
_efl_ui_focus_manager_root_focus_canvas_object_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   return pd->replacement_object;
}

EOLIAN static void
_efl_ui_focus_manager_root_focus_canvas_object_set(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Canvas_Object *canvas_object)
{
   //if canvas object is NULL trigger it as root
   if (!canvas_object)
     canvas_object = efl_ui_focus_manager_root_get(obj);

   if (canvas_object == pd->replacement_object) return;

   if (pd->replacement_object)
     {
        pd->iterator_list = eina_list_remove(pd->iterator_list, pd->rect);
        pd->replacement_object = NULL;
     }

   pd->replacement_object = canvas_object;
   if (pd->replacement_object)
     {
        efl_ui_focus_composition_adapter_canvas_object_set(pd->rect, pd->replacement_object);
        pd->iterator_list = eina_list_append(pd->iterator_list, pd->rect);
     }
}

static void
_focus_changed(void *data, const Efl_Event *ev)
{
   Eo *root;

   root = efl_ui_focus_manager_root_get(data);

   efl_ui_focus_object_focus_set(root, efl_ui_focus_object_focus_get(ev->object));
}

EFL_CALLBACKS_ARRAY_DEFINE(composition_cb,
   { EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, _focus_changed },
)

EOLIAN static Efl_Object*
_efl_ui_focus_manager_root_focus_efl_object_constructor(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   pd->rect = efl_add_ref(EFL_UI_FOCUS_COMPOSITION_ADAPTER_CLASS, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->rect, NULL);
   efl_event_callback_array_add(pd->rect, composition_cb(), obj);

   return efl_constructor(efl_super(obj, MY_CLASS));
}

EOLIAN static void
_efl_ui_focus_manager_root_focus_efl_object_destructor(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   efl_unref(pd->rect);
   pd->rect = NULL;

   efl_destructor(efl_super(obj, MY_CLASS));
}

EOLIAN static Efl_Object*
_efl_ui_focus_manager_root_focus_efl_object_finalize(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   //set it to NULL so the root manager is passed to the manager
   if (!pd->replacement_object)
     efl_ui_focus_manager_root_focus_canvas_object_set(obj, NULL);

   _state_eval(obj, pd);

   return efl_finalize(efl_super(obj, MY_CLASS));
}

#include "efl_ui_focus_manager_root_focus.eo.c"
