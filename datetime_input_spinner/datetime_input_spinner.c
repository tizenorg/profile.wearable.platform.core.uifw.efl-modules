#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif

#include <Elementary.h>
#include <unicode/udat.h>
#include <unicode/ustring.h>
#include "elm_widget.h"
#include "elm_widget_datetime.h"

#define DATETIME_FIELD_COUNT    6
#define FIELD_FORMAT_LEN        3
#define BUFF_SIZE               100

typedef struct _Input_Spinner_Module_Data Input_Spinner_Module_Data;

struct _Input_Spinner_Module_Data
{
   Elm_Datetime_Module_Data mod_data;
   Evas_Object *ampm_box;
   Evas_Object *am_button;
   Evas_Object *pm_button;
};

static void
_field_value_set(struct tm *tim, Elm_Datetime_Field_Type field_type, int val)
{
   if (field_type >= (DATETIME_FIELD_COUNT - 1)) return;

   int *timearr[]= { &tim->tm_year, &tim->tm_mon, &tim->tm_mday, &tim->tm_hour, &tim->tm_min };
   *timearr[field_type] = val;
}

static void
_spinner_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type field_type;
   struct tm tim;
   int value ;

   layout_mod = (Input_Spinner_Module_Data *)data;

   elm_datetime_value_get(layout_mod->mod_data.base, &tim);
   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type");
   value =  elm_spinner_value_get(obj);

   if (field_type == ELM_DATETIME_YEAR)
     {
       value -= 1900;
     }
   else if (field_type == ELM_DATETIME_MONTH)
     {
        value -= 1;
     }

   _field_value_set(&tim, field_type, value);

   elm_datetime_value_set(layout_mod->mod_data.base, &tim);
}
static void
_spinner_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   char *str;
   int len, max_len;
   double min, max;
   Eina_List *l, *fields = NULL;

   str = event_info;
   len = strlen(str);
   elm_spinner_min_max_get(obj, &min, &max);
   max_len = log10(abs(max)) + 1;

   if (max_len <= len)
     {
        Input_Spinner_Module_Data *layout_mod = data;
        fields = layout_mod->mod_data.fields_sorted_get(layout_mod->mod_data.base);

        Evas_Object *spinner_next = NULL;
        l = eina_list_data_find_list(fields, obj);
        if (l)
          {
             while (eina_list_next(l))
               {
                  spinner_next = eina_list_data_get(eina_list_next(l));
                  if (spinner_next && !evas_object_visible_get(spinner_next))
                    {
                       l = eina_list_next(l);
                       spinner_next = NULL;
                    }
                  else
                    break;
               }
          }

        if (spinner_next)
          {
             elm_layout_signal_emit(obj, "elm,action,entry,toggle", "elm");
             edje_object_message_signal_process(elm_layout_edje_get(obj));
             elm_layout_signal_emit(spinner_next, "elm,action,entry,toggle", "elm");
          }
        else
          elm_layout_signal_emit(obj, "elm,action,entry,toggle", "elm");
     }
}

static void
_ampm_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   struct tm curr_time;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);

   if ((curr_time.tm_hour >= 12) && (!strcmp(elm_object_text_get(obj), dgettext("elementary", "AM"))))
     curr_time.tm_hour -= 12;
   else if ((curr_time.tm_hour < 12) && (!strcmp(elm_object_text_get(obj), dgettext("elementary", "PM"))))
     curr_time.tm_hour += 12;
   else
     return;

   elm_datetime_value_set(layout_mod->mod_data.base, &curr_time);
}

static void
_access_set(Evas_Object *obj, Elm_Datetime_Field_Type field_type)
{
   const char* type = NULL;

   switch (field_type)
     {
       case ELM_DATETIME_YEAR:
        type = "datetime field, year";
        break;

      case ELM_DATETIME_MONTH:
        type = "datetime field, month";
        break;

     case ELM_DATETIME_DATE:
        type = "datetime field, date";
        break;

      case ELM_DATETIME_HOUR:
        type = "datetime field, hour";
        break;

      case ELM_DATETIME_MINUTE:
        type = "datetime field, minute";
        break;

      case ELM_DATETIME_AMPM:
        type = "datetime field, AM PM";
        break;

      default:
        break;
     }

   _elm_access_text_set
      (_elm_access_info_get(obj), ELM_ACCESS_TYPE, type);
   _elm_access_callback_set
      (_elm_access_info_get(obj), ELM_ACCESS_STATE, NULL, NULL);
}

static void
_spinner_special_value_set(Evas_Object *obj, Elm_Datetime_Field_Type field_type,
                           struct tm tim, const char *fmt)
{
   int val;
   char *p, *locale;
   char buf[BUFF_SIZE] = {0, };
   int32_t pos;
   UDateFormat *dt_formatter = NULL;
   UErrorCode status = U_ZERO_ERROR;
   UDate date;
   UChar pattern[BUFF_SIZE] = {0, };
   UChar str[BUFF_SIZE] = {0, };
   UChar ufield[BUFF_SIZE] = {0, };
   UCalendar *calendar = NULL;

   //Current locale get form env.
   locale = getenv("LC_TIME");
   if (!locale)
     {
        locale = setlocale(LC_TIME, NULL);
        if (!locale) return;
     }

   //Separate up with useless string in locale.
   p = strstr(locale, ".UTF-8");
   if (p) *p = 0;

   switch (field_type)
     {
      case ELM_DATETIME_YEAR:
         if (!strcmp(fmt, "%y"))
           u_uastrncpy(pattern, "yy", strlen("yy"));
         else
           u_uastrncpy(pattern, "yyyy", strlen("yyyy"));

         val = tim.tm_year + 1900;
         break;

      case ELM_DATETIME_MONTH:
         if (!strcmp(fmt, "%m"))
           u_uastrncpy(pattern, "MM", strlen("MM"));
         else if (!strcmp(fmt, "%B"))
           u_uastrncpy(pattern, "MMMM", strlen("MMMM"));
         else
           u_uastrncpy(pattern, "MMM", strlen("MMM"));

         val = tim.tm_mon + 1;
         break;

      case ELM_DATETIME_DATE:
         if (!strcmp(fmt, "%e"))
           u_uastrncpy(pattern, "d", strlen("d"));
         else
           u_uastrncpy(pattern, "dd", strlen("dd"));

         val = tim.tm_mday;
         break;

      case ELM_DATETIME_HOUR:
         if (!strcmp(fmt, "%l"))
           {
              if (!strcmp(locale,"ja_JP"))
                u_uastrncpy(pattern, "K", strlen("K"));
              else
                u_uastrncpy(pattern, "h", strlen("h"));
           }
         else if (!strcmp(fmt, "%I"))
           {
              if (!strcmp(locale,"ja_JP"))
                u_uastrncpy(pattern, "KK", strlen("KK"));
              else
                u_uastrncpy(pattern, "hh", strlen("hh"));
           }
         else if (!strcmp(fmt, "%k"))
           u_uastrncpy(pattern, "H", strlen("H"));
         else
           u_uastrncpy(pattern, "HH", strlen("HH"));

         val = tim.tm_hour;
         break;

      case ELM_DATETIME_MINUTE:
         u_uastrncpy(pattern, "mm", strlen("mm"));

         val = tim.tm_min;
         break;

      default:
         return;
     }

   //Open a new UDateFormat for formatting and parsing dates and times.
   dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, pattern, -1, &status);
   if (!dt_formatter) return;

   snprintf(buf, sizeof(buf), "%d", val);
   u_uastrcpy(ufield, buf);
   pos = 0;

   //Open a UCalendar.
   //A UCalendar may be used to convert a millisecond value to a year, month, and day.
   calendar = ucal_open(NULL, -1, locale, UCAL_GREGORIAN, &status);
   if (!calendar)
     {
        udat_close(dt_formatter);
        return;
     }
   ucal_clear(calendar);

   //Parse a string into an date/time using a UDateFormat.
   udat_parseCalendar(dt_formatter, calendar, ufield, sizeof(ufield), &pos, &status);
   date = ucal_getMillis(calendar, &status);
   ucal_close(calendar);

   udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
   u_austrcpy(buf, str);
   udat_close(dt_formatter);
   elm_spinner_special_value_add(obj, val, buf);
}

static void
_colon_location_set(Input_Spinner_Module_Data *layout_mod)
{
   int loc;

   if (layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, ELM_DATETIME_AMPM, &loc))
     elm_layout_signal_emit(layout_mod->mod_data.base, "elm,state,ampm,visible", "elm");
   else
     elm_layout_signal_emit(layout_mod->mod_data.base, "elm,state,ampm,invisible", "elm");

   if (loc == 5)
     {
        elm_object_signal_emit(layout_mod->mod_data.base, "elm,state,colon,visible,field3", "elm");
        elm_object_signal_emit(layout_mod->mod_data.base, "elm,state,colon,invisible,field4", "elm");
     }
   else
     {
        elm_object_signal_emit(layout_mod->mod_data.base, "elm,state,colon,visible,field4", "elm");
        elm_object_signal_emit(layout_mod->mod_data.base, "elm,state,colon,invisible,field3", "elm");
     }

   edje_object_message_signal_process(elm_layout_edje_get(layout_mod->mod_data.base));
}

static void
_theme_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   const char *style;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   style = elm_object_style_get(layout_mod->mod_data.base);

   if (strstr(style, "time_layout"))
     _colon_location_set(layout_mod);
}

EAPI void
field_value_display(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type field_type;
   int min, max, value;
   struct tm tim;
   const char *fmt;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod || !obj) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &tim);

   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");
   fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, field_type);

   if(field_type == ELM_DATETIME_AMPM)
     {
        if ((tim.tm_hour >= 0) && (tim.tm_hour < 12))
          {
             elm_layout_signal_emit(layout_mod->am_button, "elm,action,button,selected", "elm");
             elm_layout_signal_emit(layout_mod->pm_button, "elm,action,button,unselected", "elm");
          }
        else
          {
             elm_layout_signal_emit(layout_mod->am_button, "elm,action,button,unselected", "elm");
             elm_layout_signal_emit(layout_mod->pm_button, "elm,action,button,selected", "elm");
          }
     }
   else if (field_type == ELM_DATETIME_MONTH)
     {
        layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, field_type, &min, &max);
        elm_spinner_min_max_set(obj, 1 + min, 1 + max);
        _spinner_special_value_set(obj, field_type, tim, fmt);
        elm_spinner_value_set(obj, 1 + tim.tm_mon);

     }
   else if (field_type == ELM_DATETIME_YEAR)
     {
        layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, field_type, &min, &max);
        elm_spinner_min_max_set(obj, 1900 + min, 1900 + max);
        _spinner_special_value_set(obj, field_type, tim, fmt);
        elm_spinner_value_set(obj, 1900 + tim.tm_year);
     }
   else if (field_type == ELM_DATETIME_HOUR)
     {
        elm_object_style_set(obj, "vertical");
        layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, field_type, &min, &max);
        elm_spinner_min_max_set(obj, min, max);
        _spinner_special_value_set(obj, field_type, tim, fmt);
        elm_spinner_value_set(obj, tim.tm_hour);
     }
   else
     {
        if (field_type == ELM_DATETIME_MINUTE)
          {
             value = tim.tm_min;
             elm_object_style_set(obj, "vertical");
          }
        else
          value = tim.tm_mday;

        layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, field_type, &min, &max);
        elm_spinner_min_max_set(obj, min, max);
        _spinner_special_value_set(obj, field_type, tim, fmt);
        elm_spinner_value_set(obj, value);
     }

}

EAPI Evas_Object *
field_create(Elm_Datetime_Module_Data *module_data, Elm_Datetime_Field_Type  field_type)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *field_obj;
   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod) return NULL;

   if (field_type == ELM_DATETIME_AMPM)
     {
        field_obj = elm_box_add(layout_mod->mod_data.base);
        evas_object_size_hint_weight_set(field_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(field_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_padding_set(field_obj, 0, ELM_SCALE_SIZE(26));
        layout_mod->ampm_box = field_obj;


        layout_mod->am_button = elm_button_add(layout_mod->ampm_box);
        elm_object_style_set(layout_mod->am_button, "datetime/ampm");
        elm_object_text_set(layout_mod->am_button, dgettext("elementary", "AM"));
        evas_object_smart_callback_add(layout_mod->am_button, "clicked", _ampm_clicked_cb, layout_mod);
        evas_object_show(layout_mod->am_button);
        elm_box_pack_end(layout_mod->ampm_box, layout_mod->am_button);

        layout_mod->pm_button = elm_button_add(layout_mod->ampm_box);
        elm_object_style_set(layout_mod->pm_button, "datetime/ampm");
        elm_object_text_set(layout_mod->pm_button, dgettext("elementary", "PM"));
        evas_object_smart_callback_add(layout_mod->pm_button, "clicked", _ampm_clicked_cb, layout_mod);
        evas_object_show(layout_mod->pm_button);
        elm_box_pack_end(layout_mod->ampm_box, layout_mod->pm_button);
     }
   else
     {
        field_obj = elm_spinner_add(layout_mod->mod_data.base);
        elm_spinner_editable_set(field_obj, EINA_TRUE);
        elm_object_style_set(field_obj, "vertical_date_picker");
        elm_spinner_step_set(field_obj, 1);
        elm_spinner_wrap_set(field_obj, EINA_TRUE);
        elm_spinner_label_format_set(field_obj, "%02.0f");
        if (field_type == ELM_DATETIME_HOUR || field_type == ELM_DATETIME_MONTH)
          elm_spinner_interval_set(field_obj, 0.2);
        else
          elm_spinner_interval_set(field_obj, 0.1);
        evas_object_smart_callback_add(field_obj, "changed", _spinner_changed_cb, layout_mod);
        evas_object_smart_callback_add(field_obj, "entry,changed", _spinner_entry_changed_cb, layout_mod);
     }
   evas_object_data_set(field_obj, "_field_type", (void *)field_type);

   // ACCESS
   _access_set(field_obj, field_type);

   return field_obj;
}

EAPI void
field_format_changed(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod || !obj) return;

   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   if (field_type == ELM_DATETIME_AMPM)
     _colon_location_set(layout_mod);
}

EAPI Elm_Datetime_Module_Data *
obj_hook(Evas_Object *obj EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   layout_mod = ELM_NEW(Input_Spinner_Module_Data);
   if (!layout_mod) return NULL;

   evas_object_smart_callback_add(obj, "theme,changed", _theme_changed_cb, layout_mod);

   return ((Elm_Datetime_Module_Data*)layout_mod);
}

EAPI void
obj_unhook(Elm_Datetime_Module_Data *module_data)
{
   Input_Spinner_Module_Data *layout_mod;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod) return;

   if (layout_mod)
     {
        free(layout_mod);
        layout_mod = NULL;
     }
}

EAPI void
obj_hide(Elm_Datetime_Module_Data *module_data EINA_UNUSED)
{
   return;
}

// module api funcs needed
EAPI int
elm_modapi_init(void *m EINA_UNUSED)
{
   return 1; // succeed always
}

EAPI int
elm_modapi_shutdown(void *m EINA_UNUSED)
{
   return 1; // succeed always
}
