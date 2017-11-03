#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include <assert.h>

#include "elm_priv.h"
#include "efl_ui_list_segarray.h"

#undef DBG
#define DBG(...) do { \
    fprintf(stderr, __FILE__ ":" "%d %s ", __LINE__, __PRETTY_FUNCTION__); \
    fprintf(stderr,  __VA_ARGS__);                                     \
    fprintf(stderr, "\n"); fflush(stderr);                              \
  } while(0)

#define MY_CLASS EFL_UI_LIST_PRECISE_LAYOUTER_CLASS


typedef struct _Efl_Ui_List_Precise_Layouter_Data
{
   Eina_Bool initialized;
   Eina_Hash* size_information;
   Eina_Size2D min;
   Efl_Model* model;
   Efl_Ui_List_Model *modeler;
   unsigned int count_total;
   Efl_Future *count_future;
} Efl_Ui_List_Precise_Layouter_Data;

typedef struct _Efl_Ui_List_Precise_Layouter_Size
{
  Eina_Size2D min;
  double weight_x, weight_y;
} Efl_Ui_List_Precise_Layouter_Size;


typedef struct _Efl_Ui_List_Precise_Layouter_Callback_Data
{
  Efl_Ui_List_Precise_Layouter_Data* pd;
  Efl_Ui_List_Precise_Layouter_Size* size;
} Efl_Ui_List_Precise_Layouter_Callback_Data;

#include "efl_ui_list_precise_layouter.eo.h"

static void _initilize(Eo *, Efl_Ui_List_Precise_Layouter_Data*, Efl_Ui_List_Model*);
static void _finalize(Eo *, Efl_Ui_List_Precise_Layouter_Data*);

static void
_item_min_calc(Efl_Ui_List_Precise_Layouter_Data *pd, Evas_Object *layout, Efl_Ui_List_Precise_Layouter_Size *size, Eina_Size2D min)
{
   int pad[4];
   efl_gfx_size_hint_margin_get(layout, &pad[0], &pad[1], &pad[2], &pad[3]);
   efl_gfx_size_hint_weight_get(layout, &size->weight_x, &size->weight_y);

   min.w += pad[0] + pad[1];
   min.h += pad[2] + pad[3];

//   if(_horiz(pd->orient))
//     {
//        pdp->realized.w -= item->minw;
//        pd->realized.w += w;
//        if(pd->realized.h <= h)
//          pd->realized.h = h;
//        else if (pd->realized.h < item->minh)
//          {
//             pd->realized.h = h;
//             EINA_INARRAY_FOREACH(&pd->items.array, it)
//               {
//                  litem = *it;
//                  if (!litem) continue;
//                  if (pd->realized.h < litem->minh)
//                    pd->realized.h = litem->minh;
//
//                  if (litem != item && litem->minh == item->minh)
//                    break;
//               }
//          }
//     }
//   else
//     {

      pd->min.h += min.h - size->min.h;

      if(pd->min.w <= min.w)
        pd->min.w = min.w;
      else if (pd->min.w == size->min.w)
        {
           Efl_Ui_List_Precise_Layouter_Size *size_item;
           Eina_Iterator *size_iterator;

           pd->min.w = min.w;
           size_iterator = eina_hash_iterator_data_new(pd->size_information);
           EINA_ITERATOR_FOREACH(size_iterator, size_item)
             {
                if (pd->min.w < size_item->min.w)
                  pd->min.w = size_item->min.w;

                if (size->min.w == size_item->min.w)
                  break;
             }
           eina_iterator_free(size_iterator);
        }
//     }

   size->min.w = min.w;
   size->min.h = min.h;
}

static void
_count_then(void * data, Efl_Event const* event)
{
   Efl_Ui_List_Precise_Layouter_Data *pd = data;
   EINA_SAFETY_ON_NULL_RETURN(pd);
   pd->count_future = NULL;

   pd->count_total = *(int*)((Efl_Future_Event_Success*)event->info)->value;
}

static void
_count_error(void * data, Efl_Event const* event EINA_UNUSED)
{
   Efl_Ui_List_Precise_Layouter_Data *pd = data;
   EINA_SAFETY_ON_NULL_RETURN(pd);
   pd->count_future = NULL;
}

static void
_on_item_size_hint_change(void *data, Evas *e EINA_UNUSED,
                    Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Efl_Ui_List_Precise_Layouter_Callback_Data *cb_data = data;
   Eina_Size2D min = efl_gfx_size_hint_combined_min_get(obj);
   _item_min_calc(cb_data->pd, obj, cb_data->size, min);
}

static void
_initilize(Eo *obj EINA_UNUSED, Efl_Ui_List_Precise_Layouter_Data *pd, Efl_Ui_List_Model *modeler)
{
   if(pd->initialized)
     return;

   pd->modeler = modeler;
   pd->initialized = EINA_TRUE;
   efl_ui_list_model_load_range_set(modeler, 0, 0); // load all
   pd->min.w = 0;
   pd->min.h = 0;
}

static void
_finalize(Eo *obj EINA_UNUSED, Efl_Ui_List_Precise_Layouter_Data *pd)
{
   Efl_Ui_List_Precise_Layouter_Callback_Data *cb_data;
   Efl_Ui_List_LayoutItem *layout_item;
   Eina_Iterator *item_iterator;
   void *data;

   item_iterator = eina_hash_iterator_key_new(pd->size_information);
   while(eina_iterator_next(item_iterator, &data))
     {
        layout_item = *(Efl_Ui_List_LayoutItem **)data;
        cb_data = evas_object_event_callback_del(layout_item->layout, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _on_item_size_hint_change);
        efl_ui_list_model_unrealize(pd->modeler, layout_item);
        free(cb_data);
     }
   eina_iterator_free(item_iterator);

   eina_hash_free_buckets(pd->size_information);
   pd->modeler = NULL;
   pd->min.w = 0;
   pd->min.h = 0;

   pd->initialized = EINA_FALSE;
}

EOLIAN static Efl_Object *
_efl_ui_list_precise_layouter_efl_object_constructor(Eo *obj EINA_UNUSED, Efl_Ui_List_Precise_Layouter_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->size_information = eina_hash_pointer_new(&free);
   pd->initialized = EINA_FALSE;
   pd->count_future = NULL;

   return obj;
}

EOLIAN static void
_efl_ui_list_precise_layouter_efl_ui_list_relayout_model_set(Eo *obj EINA_UNUSED, Efl_Ui_List_Precise_Layouter_Data *pd, Efl_Model *model)
{
   if (pd->model == model)
     return;

   pd->count_total = 0;

   if (pd->count_future)
     {
        efl_future_cancel(pd->count_future);
        pd->count_future = NULL;
     }

   if (pd->model)
     {
        _finalize(obj, pd);
        efl_unref(pd->model);
        pd->model = NULL;
     }

   if (model)
     {
        pd->model = model;
        efl_ref(pd->model);
        pd->count_future = efl_model_children_count_get(pd->model);
        efl_future_then(pd->count_future, &_count_then, &_count_error, NULL, pd);
     }
}

EOLIAN static void
_efl_ui_list_precise_layouter_efl_ui_list_relayout_layout_do
  (Eo *obj EINA_UNUSED, Efl_Ui_List_Precise_Layouter_Data *pd
   , Efl_Ui_List_Model *modeler, int first, int count, Eina_Accessor *nodes)
{
   Efl_Ui_List_LayoutItem* layout_item;
   Efl_Ui_List_SegArray_Node* items_node;
   Efl_Ui_List_Precise_Layouter_Size* size;
   int i = 0, j = 0;
   Eina_Bool horiz = EINA_FALSE/*_horiz(pd->orient)*/, zeroweight = EINA_FALSE;
   Evas_Coord ow, oh, want, minw, minh, scr_x, scr_y;
   int boxx, boxy, boxw, boxh, length, /*pad, */extra = 0, rounding = 0;
   int boxl = 0, boxr = 0, boxt = 0, boxb = 0;
   double cur_pos = 0, scale, box_align[2],  weight[2] = { 0, 0 };
   Eina_Bool box_fill[2] = { EINA_FALSE, EINA_FALSE };

   DBG("layout_do first %d count %d", first, count);
   /* EINA_SAFETY_ON_NULL_RETURN(items); */

    _initilize(obj, pd, modeler);

   // cache size of new items
   EINA_ACCESSOR_FOREACH(nodes, i, items_node)
     {
        DBG("node first %d", items_node->first);
        for(j = 0; j != items_node->length; ++j)
          {
            layout_item = items_node->pointers[j];

        size = eina_hash_find(pd->size_information, &layout_item);
        if(!size)
        {
           if(!layout_item->layout)
             {
              DBG("no layout, realizing");
               efl_ui_list_model_realize(modeler, layout_item);
             }
           else
             {
              DBG("already realized");
           /* if(!layout_item->layout) */
           /*   { */
           /*     // error */
           /*     continue; */
           /*   } */
            }

           size = calloc(1, sizeof(Efl_Ui_List_Precise_Layouter_Size));
           eina_hash_add(pd->size_information, &layout_item, size);
           Eina_Size2D min = efl_gfx_size_hint_combined_min_get(layout_item->layout);
           if(min.w && min.h)
             {
//                DBG("size was calculated");
                _item_min_calc(pd, layout_item->layout, size, min);
             }
           Efl_Ui_List_Precise_Layouter_Callback_Data *cb_data = calloc(1, sizeof(Efl_Ui_List_Precise_Layouter_Callback_Data));
           cb_data->pd = pd;
           cb_data->size = size;
           evas_object_event_callback_add(layout_item->layout, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _on_item_size_hint_change, cb_data);
        }
          }
     }

   evas_object_geometry_get(modeler, &boxx, &boxy, &boxw, &boxh);
   efl_gfx_size_hint_margin_get(modeler, &boxl, &boxr, &boxt, &boxb);

   scale = evas_object_scale_get(modeler);
/*    // Box align: used if "item has max size and fill" or "no item has a weight" */
/*    // Note: cells always expand on the orthogonal direction */
   box_align[0] = 0;/*pd->align.h;*/
   box_align[1] = 0;/*pd->align.v;*/
   if (box_align[0] < 0)
     {
        box_fill[0] = EINA_TRUE;
        box_align[0] = 0.5;
     }
   if (box_align[1] < 0)
     {
        box_fill[1] = EINA_TRUE;
        box_align[1] = 0.5;
     }

/*    count = eina_inarray_count(&pd->items.array); */

   // box outer margin
   boxw -= boxl + boxr;
   boxh -= boxt + boxb;
   boxx += boxl;
   boxy += boxt;

   int average_item_size = 10; //eina_inarray_count(&pd->items.array) ? (/*horz*/ EINA_FALSE ? pd->realized.w : pd->realized.h) / eina_inarray_count(&pd->items.array) : AVERAGE_SIZE_INIT;
   if(!average_item_size)
     average_item_size = 10;//AVERAGE_SIZE_INIT;

   // total space & available space
   if (horiz)
     {
        int pad;
        length = boxw;
        want = 100;//pd->realized.w;
        //pad = pd->pad.scalable ? (pd->pad.h * scale) : pd->pad.h;
        pad = 0;

        // padding can not be squeezed (note: could make it an option)
        length -= pad * (count - 1);
        // available space. if <0 we overflow
        extra = length - want;

        minw = 100;//pd->realized.w + boxl + boxr + pad * (count - 1);
        minh = 100;//pd->realized.h + boxt + boxb;
        /* if (pd->item_count > count) */
        /*   minw = pd->item_count * average_item_size; */
     }
   else
     {
        int pad;
        length = boxh;
        want = pd->min.h;
        pad = 0;//pd->pad.scalable ? (pd->pad.v * scale) : pd->pad.v;

        //FIXME?? use eina_hash_population(pd->size_information) or count
        int population = eina_hash_population(pd->size_information);
        // padding can not be squeezed (note: could make it an option)
        length -= pad * (population - 1); //(count - 1);
        // available space. if <0 we overflow
        extra = length - want;

        minw = pd->min.w + boxl + boxr;
        minh = pd->min.h + pad * (/*count*/ population - 1) + boxt + boxb;
        if (pd->count_total > population) //count)
          minh *= (pd->count_total / population); //count);
     }

//   if (pd->min.h != minh || pd->min.w != minw)
//     {
//        pd->min.w = minw;
//        pd->min.h = minh;
//     }

   efl_ui_list_model_min_size_set(modeler, pd->min);
   if (extra < 0) extra = 0;

   weight[0] = 1;//pd->weight.x;
   weight[1] = 1;//pd->weight.y;
   if (EINA_DBL_EQ(weight[!horiz], 0))
     {
        if (box_fill[!horiz])
          {
             // box is filled, set all weights to be equal
             zeroweight = EINA_TRUE;
          }
        else
          {
             // move bounding box according to box align
             cur_pos = extra * box_align[!horiz];
          }
        weight[!horiz] = count;
     }

   elm_interface_scrollable_content_viewport_geometry_get
              (modeler, NULL, NULL, &ow, &oh);

   elm_interface_scrollable_content_pos_get(modeler, &scr_x, &scr_y);
/*    cur_pos += average_item_size * pd->realized.start; */
/*    // scan all items, get their properties, calculate total weight & min size */
   // cache size of new items
   /* EINA_ACCESSOR_FOREACH(items, i, layout_item) */
   EINA_ACCESSOR_FOREACH(nodes, i, items_node)
     {
        for(j = 0; j != items_node->length;++j)
        {
          layout_item = items_node->pointers[j];
        double cx, cy, cw, ch, x, y, w, h;
        double align[2];
        int item_pad[4];
        Eina_Size2D max;
        int pad = 0;

        size = eina_hash_find(pd->size_information, &layout_item);

        if(size)
          {
//        DBG("size information for item %d width %d height %d", i, size->min.w, size->min.h);

        assert(layout_item->layout != NULL);
        efl_gfx_size_hint_align_get(layout_item->layout, &align[0], &align[1]);
        max = efl_gfx_size_hint_max_get(layout_item->layout);
        efl_gfx_size_hint_margin_get(layout_item->layout, &item_pad[0], &item_pad[1], &item_pad[2], &item_pad[3]);

        if (align[0] < 0) align[0] = -1;
        if (align[1] < 0) align[1] = -1;
        if (align[0] > 1) align[0] = 1;
        if (align[1] > 1) align[1] = 1;

        if (max.w <= 0) max.w = INT_MAX;
        if (max.h <= 0) max.h = INT_MAX;
        if (max.w < size->min.w) max.w = size->min.w;
        if (max.h < size->min.h) max.h = size->min.h;

        /* // extra rounding up (compensate cumulative error) */
        if ((i == (count - 1)) && (cur_pos - floor(cur_pos) >= 0.5))
           rounding = 1;

        if (horiz)
          {
             cx = boxx + cur_pos;
             cy = boxy;
             cw = size->min.w + rounding + (zeroweight ? 1.0 : size->weight_x) * extra / weight[0];
             ch = boxh;
             cur_pos += cw + pad;
          }
        else
          {
             cx = boxx;
             cy = boxy + cur_pos;
             cw = boxw;
             ch = size->min.h + rounding + (zeroweight ? 1.0 : size->weight_y) * extra / weight[1];
             cur_pos += ch + pad;
          }

        // horizontally
        if (max.w < INT_MAX)
          {
             w = MIN(MAX(size->min.w - item_pad[0] - item_pad[1], max.w), cw);
             if (align[0] < 0)
               {
                  // bad case: fill+max are not good together
                  x = cx + ((cw - w) * box_align[0]) + item_pad[0];
               }
             else
               x = cx + ((cw - w) * align[0]) + item_pad[0];
          }
        else if (align[0] < 0)
          {
             // fill x
             w = cw - item_pad[0] - item_pad[1];
             x = cx + item_pad[0];
          }
        else
          {
             w = size->min.w - item_pad[0] - item_pad[1];
             x = cx + ((cw - w) * align[0]) + item_pad[0];
          }

        // vertically
        if (max.h < INT_MAX)
          {
             h = MIN(MAX(size->min.h - item_pad[2] - item_pad[3], max.h), ch);
             if (align[1] < 0)
               {
                  // bad case: fill+max are not good together
                  y = cy + ((ch - h) * box_align[1]) + item_pad[2];
               }
             else
               y = cy + ((ch - h) * align[1]) + item_pad[2];
          }
        else if (align[1] < 0)
          {
             // fill y
             h = ch - item_pad[2] - item_pad[3];
             y = cy + item_pad[2];
          }
        else
          {
             h = size->min.h - item_pad[2] - item_pad[3];
             y = cy + ((ch - h) * align[1]) + item_pad[2];
          }

        if (horiz)
          {
             if (h < pd->min.h) h = pd->min.h;
             if (h > oh) h = oh;
          }
        else
          {
             if (w < pd->min.w) w = pd->min.w;
             if (w > ow) w = ow;
          }

//        DBG("------- size_h:%d -- h:%.2f --- ", size->min.h, h);
        evas_object_geometry_set(layout_item->layout, (x + 0 - scr_x), (y + 0 - scr_y), w, h);

        /* layout_item->x = x; */
        /* layout_item->y = y; */

/* //        fprintf(stderr, "x: %.2f y: %.2f w: %.2f h: %.2f old x: %.2f old y: %.2f old w: %.2f old h: %.2f\n" */
/* //                , (x + 0 - pd->pan.x), (y + 0 - pd->pan.y), (float)w, (float)h */
/* //               , (float)litem->x, (float)litem->y, (float)litem->w, (float)litem->h); fflush(stderr); */
/* //        printf("obj=%d currpos=%.2f moved to X=%.2f, Y=%.2f average_item_size %d\n", litem->index, cur_pos, x, y */
/* //               , eina_inarray_count(&pd->items.array) ? (/\*horz*\/ EINA_FALSE ? pd->realized.w : pd->realized.h) / eina_inarray_count(&pd->items.array) : AVERAGE_SIZE_INIT); */
          }
        }
     }
}

#include "efl_ui_list_precise_layouter.eo.c"
