/*
 *  Copyright (C) 2020  Julian Maurice
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "module.h"
#include "commands.h"
#include "printtext.h"
#include "levels.h"
#include "signals.h"
#include "rocketchat-protocol.h"
#include "rocketchat-queries.h"
#include "rocketchat-result-callbacks.h"
#include "rocketchat-message.h"
#include "jansson.h"

#define command_bind_rocketchat(cmd, category, func) \
	command_bind_proto(cmd, ROCKETCHAT_PROTOCOL, category, func)

static void result_cb_browseChannels(ROCKETCHAT_SERVER_REC *server, json_t *json, json_t *userdata)
{
	json_t *error, *result, *channels, *channel;
	const char *id, *name, *fname;
	size_t index;

	error = json_object_get(json, "error");
	if (error) {
		return;
	}

	result = json_object_get(json, "result");
	channels = json_object_get(result, "results");
	json_array_foreach(channels, index, channel) {
		id = json_string_value(json_object_get(channel, "_id"));
		name = json_string_value(json_object_get(channel, "name"));
		fname = json_string_value(json_object_get(channel, "fname"));

		printtext(server, NULL, MSGLEVEL_CLIENTCRAP, "%s (ID: %s)", fname ? fname : name, id);
	}
}

static void result_cb_loadHistory(ROCKETCHAT_SERVER_REC *server, json_t *json, json_t *userdata)
{
	json_t *error, *result, *messages, *message;
	const char *target;
	size_t index;

	error = json_object_get(json, "error");
	if (error) {
		return;
	}

	target = json_string_value(json_object_get(userdata, "target"));

	result = json_object_get(json, "result");
	messages = json_object_get(result, "messages");

	for (index = json_array_size(messages); index > 0; index--) {
		const char *username;
		GDateTime *datetime;
		char *msg, *datetime_formatted;
		json_int_t ts;

		message = json_array_get(messages, index - 1);

		username = json_string_value(json_object_get(json_object_get(message, "u"), "username"));
		msg = rocketchat_format_message(server, message);

		ts = json_integer_value(json_object_get(json_object_get(message, "ts"), "$date"));
		datetime = g_date_time_new_from_unix_local(ts / 1000);
		datetime_formatted = g_date_time_format(datetime, "%c");

		printtext(server, target, MSGLEVEL_CLIENTCRAP, "<%s> %s (%s)", username, msg, datetime_formatted);

		g_free(msg);
		g_free(datetime_formatted);
		g_date_time_unref(datetime);
	}
}


static void cmd_rocketchat_channels(const char *data, ROCKETCHAT_SERVER_REC *server, WI_ITEM_REC *item)
{
	ROCKETCHAT_RESULT_CALLBACK_REC *callback;
	json_t *params, *param;

	param = json_object();
	json_object_set_new(param, "page", json_integer(0));
	json_object_set_new(param, "offset", json_integer(0));
	json_object_set_new(param, "limit", json_integer(100));

	params = json_array();
	json_array_append_new(params, param);

	callback = rocketchat_result_callback_new(result_cb_browseChannels, NULL);
	rocketchat_call(server, "browseChannels", params, callback);
}

static void cmd_rocketchat_users(const char *data, ROCKETCHAT_SERVER_REC *server, WI_ITEM_REC *item)
{
	ROCKETCHAT_RESULT_CALLBACK_REC *callback;
	json_t *params, *param;
	void *free_arg;
	char *text;

	if (!cmd_get_params(data, &free_arg, 1 | PARAM_FLAG_GETREST, &text)) {
		return;
	}

	param = json_object();
	json_object_set_new(param, "text", json_string(text));
	json_object_set_new(param, "workspace", json_string("all"));
	json_object_set_new(param, "type", json_string("users"));
	json_object_set_new(param, "page", json_integer(0));
	json_object_set_new(param, "offset", json_integer(0));
	json_object_set_new(param, "limit", json_integer(100));

	params = json_array();
	json_array_append_new(params, param);

	callback = rocketchat_result_callback_new(result_cb_browseChannels, NULL);
	rocketchat_call(server, "browseChannels", params, callback);
}

static void cmd_rocketchat_history(const char *data, ROCKETCHAT_SERVER_REC *server, WI_ITEM_REC *item)
{
	json_t *params, *userdata;
	const char *target;
	const char *rid;
	ROCKETCHAT_RESULT_CALLBACK_REC *callback;

	target = window_item_get_target(item);
	if (item->type == module_get_uniq_id_str("WINDOW ITEM TYPE", "QUERY")) {
		rid = rocketchat_query_get_rid((ROCKETCHAT_QUERY_REC *)item);
	} else {
		rid = target;
	}

	params = json_array();
	json_array_append(params, json_string(rid));
	json_array_append(params, json_null());
	json_array_append(params, json_integer(10));
	json_array_append(params, json_null());

	userdata = json_object();
	json_object_set_new(userdata, "target", json_string(target));

	callback = rocketchat_result_callback_new(result_cb_loadHistory, userdata);
	rocketchat_call(server, "loadHistory", params, callback);
}

void fe_rocketchat_commands_init(void)
{
	command_bind_rocketchat("rocketchat channels", NULL, (SIGNAL_FUNC)cmd_rocketchat_channels);
	command_bind_rocketchat("rocketchat users", NULL, (SIGNAL_FUNC)cmd_rocketchat_users);
	command_bind_rocketchat("rocketchat history", NULL, (SIGNAL_FUNC)cmd_rocketchat_history);
}

void fe_rocketchat_commands_deinit(void)
{
	command_unbind("rocketchat channels", (SIGNAL_FUNC)cmd_rocketchat_channels);
	command_unbind("rocketchat users", (SIGNAL_FUNC)cmd_rocketchat_users);
	command_unbind("rocketchat history", (SIGNAL_FUNC)cmd_rocketchat_history);
}
