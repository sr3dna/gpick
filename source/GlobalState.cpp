/*
 * Copyright (c) 2009, Albertas Vyšniauskas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the software author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "GlobalState.h"
#include "Paths.h"
#include "ScreenReader.h"
#include "Converter.h"
#include "uiUtilities.h"

#include "dynv/DynvMemoryIO.h"
#include "dynv/DynvVarString.h"
#include "dynv/DynvVarInt32.h"
#include "dynv/DynvVarColor.h"
#include "dynv/DynvVarPtr.h"
#include "dynv/DynvVarFloat.h"
#include <stdlib.h>
#include <glib/gstdio.h>


#include <fstream>
#include <iostream>
using namespace std;

int global_state_term(GlobalState *gs){

	//write settings to file
	gchar* config_file = build_config_path("settings");
	ofstream f(config_file, ios::out | ios::trunc | ios::binary);
	if (f.is_open()){
		gsize size;
		gchar* data=g_key_file_to_data(gs->settings, &size, 0);

		f.write(data, size);
		g_free(data);
		f.close();
	}
	g_free(config_file);
	
	//destroy color list, random generator and other systems
	converters_term((Converters*)dynv_system_get(gs->params, "ptr", "Converters"));
	
	color_list_destroy(gs->colors);
	random_destroy(gs->random);
	g_key_file_free(gs->settings);
	color_names_destroy(gs->color_names);
	sampler_destroy(gs->sampler);
	screen_reader_destroy(gs->screen_reader);
	dynv_system_release(gs->params);
	lua_close(gs->lua);
	
	return 0;
}

int global_state_init(GlobalState *gs){

	//create and load settings
	gs->settings = g_key_file_new();
	gchar* config_file = build_config_path("settings");
	if (!(g_key_file_load_from_file(gs->settings, config_file, G_KEY_FILE_KEEP_COMMENTS, 0))){
		g_free(config_file);
		config_file = build_config_path(NULL);
		g_mkdir(config_file, S_IRWXU);
	}
	g_free(config_file);
	
	//check if user has user_init.lua file, if not, then create empty file
	gchar *user_init_file = build_config_path("user_init.lua");
	FILE *user_init = fopen(user_init_file, "r");
	if (user_init){
		fclose(user_init);
	}else{
		user_init = fopen(user_init_file, "w");
		fclose(user_init);
	}
	g_free(user_init_file);

	gs->screen_reader = screen_reader_new();

	gs->sampler = sampler_new(gs->screen_reader);
	//create and seed random generator
	gs->random = random_new("SHR3");
	gulong seed_value=time(0)|1;
	random_seed(gs->random, &seed_value);
	
	//create and load color names
	gs->color_names = color_names_new();
	gchar* tmp;
	color_names_load_from_file(gs->color_names, tmp=build_filename("colors.txt"));
	g_free(tmp);
	color_names_load_from_file(gs->color_names, tmp=build_filename("colors0.txt"));
	g_free(tmp);
	
	
	//create dynamic parameter system
	struct dynvHandlerMap* handler_map=dynv_handler_map_create();
	dynv_handler_map_add_handler(handler_map, dynv_var_string_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_int32_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_color_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_ptr_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_float_new());
	gs->params = dynv_system_create(handler_map);
	
	//create color list / callbacks must be defined elsewhere
	gs->colors = color_list_new(handler_map);

	
	dynv_handler_map_release(handler_map);
	
	
	//create and load lua state
	lua_State *L= luaL_newstate();
	luaL_openlibs(L);

	int status;
	lua_ext_colors_openlib(L);
	
	gchar* lua_root_path = build_filename("?.lua");
	gchar* lua_user_path = build_config_path("?.lua");
	
	gchar* lua_path = g_strjoin(";", lua_root_path, lua_user_path, (void*)0);
	
	lua_pushstring(L, "package");
	lua_gettable(L, LUA_GLOBALSINDEX);
	lua_pushstring(L, "path");
	lua_pushstring(L, lua_path);
	lua_settable(L, -3);
	
	g_free(lua_path);
	g_free(lua_root_path);
	g_free(lua_user_path);
	
	tmp=build_filename("init.lua");
	status = luaL_loadfile(L, tmp) || lua_pcall(L, 0, 0, 0);
	if (status) {
		cerr<<"init script load failed: "<<lua_tostring (L, -1)<<endl;
	}
	g_free(tmp);
	
	gs->lua = L;
	dynv_system_set(gs->params, "ptr", "lua_State", L);
	
	//create converter system
	Converters* converters = converters_init(gs->params);
	
	gchar** source_array;
	gsize source_array_size;
	if ((source_array = g_key_file_get_string_list(gs->settings, "Converter", "Names", &source_array_size, 0))){
		gboolean* copy_array;	
		gsize copy_array_size=0;
		gboolean* paste_array;	
		gsize paste_array_size=0;
		copy_array = g_key_file_get_boolean_list(gs->settings, "Converter", "Copy", &copy_array_size, 0);
		paste_array = g_key_file_get_boolean_list(gs->settings, "Converter", "Paste", &paste_array_size, 0);

		gsize source_array_i = 0;
		Converter* converter;
		
		if (copy_array_size>0 || paste_array_size>0){
						
			while (source_array_i<source_array_size){
				converter = converters_get(converters, source_array[source_array_i]);
				if (converter){
					if (source_array_i<copy_array_size) converter->copy = converter->serialize_available & copy_array[source_array_i];
					if (source_array_i<paste_array_size) converter->paste = converter->deserialize_available & paste_array[source_array_i];
				}
				++source_array_i;
			}
		}else{
			while (source_array_i<source_array_size){
				converter = converters_get(converters, source_array[source_array_i]);
				if (converter){
					converter->copy = converter->serialize_available;
					converter->paste = converter->deserialize_available;
				}
				++source_array_i;
			}
		}
		
		if (copy_array) g_free(copy_array);
		if (paste_array) g_free(paste_array);
		
		converters_reorder(converters, (const char**)source_array, source_array_size);
		g_strfreev(source_array);

	}else{
		//Initialize default values

		const char* name_array[]={
			"color_web_hex",
			"color_css_rgb",
			"color_css_hsl",
		};
		
		source_array_size = sizeof(name_array)/sizeof(name_array[0]);
		gsize source_array_i = 0;
		Converter* converter;
		
		while (source_array_i<source_array_size){
			converter = converters_get(converters, name_array[source_array_i]);
			if (converter){
				converter->copy = converter->serialize_available;
				converter->paste = converter->deserialize_available;
			}
			++source_array_i;
		}
		
		converters_reorder(converters, name_array, source_array_size);
	}
	
	converters_rebuild_arrays(converters, CONVERTERS_ARRAY_TYPE_COPY);
	converters_rebuild_arrays(converters, CONVERTERS_ARRAY_TYPE_PASTE);
	
	converters_set_display(converters, converters_get(converters, g_key_file_get_string_with_default(gs->settings, "Converter", "Display", "color_web_hex")));
	
	return 0;
}
