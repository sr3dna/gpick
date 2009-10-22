
gpick = {};
gpick.converters = {};
gpick.user = {};

require('helpers')
suggest('user_init')

gpick.serialize_web_hex = function (color_object)
	local c = color_object:get_color();
	return '#' .. string.format('%02X%02X%02X', round(c:red()*255), round(c:green()*255), round(c:blue()*255));
end;

gpick.deserialize_web_hex = function (text, color_object)
	local c = color:new();

	local find_start, find_end, red, green, blue = string.find(text, '#([%x][%x])([%x][%x])([%x][%x])[^%x]?');
	
	if find_start~=nil then
	
		red = tonumber(red, 16);
		green = tonumber(green, 16);
		blue = tonumber(blue, 16);
		
		c:red(red/255);
		c:green(green/255);
		c:blue(blue/255);
		
		color_object:set_color(c);
		
		return 1-(math.atan(find_start-1)/math.pi)-(math.atan(string.len(text)-find_end)/math.pi);
	else
		return -1;
	end;
end;


gpick.serialize_web_hex_3_digit = function (color_object)
	local c = color_object:get_color();
	return '#' .. string.format('%01X%01X%01X', round(c:red()*15), round(c:green()*15), round(c:blue()*15));
end;

gpick.deserialize_web_hex_3_digit = function (text, color_object)
	local c = color:new();

	local find_start, find_end, red, green, blue = string.find(text, '#([%x])([%x])([%x])[^%x]?');
	
	if find_start~=nil then
	
		red = tonumber(red, 16);
		green = tonumber(green, 16);
		blue = tonumber(blue, 16);
		
		c:red(red/15);
		c:green(green/15);
		c:blue(blue/15);
		
		color_object:set_color(c);
		
		return 1-(math.atan(find_start-1)/math.pi)-(math.atan(string.len(text)-find_end)/math.pi);
	else
		return -1;
	end;
end;

gpick.serialize_css_hsl = function (color_object)
	local c = color_object:get_color();
	c = c:rgb_to_hsl();
	return 'hsl(' .. string.format('%d, %d%%, %d%%', round(c:hue()*360), round(c:saturation()*100), round(c:lightness()*100)) .. ')';
end;


gpick.serialize_css_rgb = function (color_object)
	local c = color_object:get_color();
	return 'rgb(' .. string.format('%d, %d, %d', round(c:red()*255), round(c:green()*255), round(c:blue()*255)) .. ')';
end;

gpick.deserialize_css_rgb = function (text, color_object)
	local c = color:new();

	local find_start, find_end, red, green, blue = string.find(text, 'rgb%(([%d]*)[%s]*,[%s]*([%d]*)[%s]*,[%s]*([%d]*)%)');
	
	if find_start~=nil then
	
		c:rgb(math.min(1, red/255), math.min(1, green/255), math.min(1, blue/255));

		color_object:set_color(c);
		
		return 1-(math.atan(find_start-1)/math.pi)-(math.atan(string.len(text)-find_end)/math.pi);
	else
		return -1;
	end;
end;



gpick.serialize_css_color_hex = function (color_object)
	return 'color: ' .. gpick.serialize_web_hex(color_object);
end;

gpick.serialize_css_background_color_hex = function (color_object)
	return 'background-color: ' .. gpick.serialize_web_hex(color_object);
end;

gpick.serialize_css_border_color_hex = function (color_object)
	return 'border-color: ' .. gpick.serialize_web_hex(color_object);
end;

gpick.serialize_css_border_top_color_hex = function (color_object)
	return 'border-top-color: ' .. gpick.serialize_web_hex(color_object);
end;

gpick.serialize_css_border_right_color_hex = function (color_object)
	return 'border-right-color: ' .. gpick.serialize_web_hex(color_object);
end;

gpick.serialize_css_border_bottom_color_hex = function (color_object)
	return 'border-bottom-color: ' .. gpick.serialize_web_hex(color_object);
end;

gpick.serialize_css_border_left_hex = function (color_object)
	return 'border-left-color: ' .. gpick.serialize_web_hex(color_object);
end;

gpick.converters['color_web_hex'] = {			human_readable = 'Web: hex code',
												serialize = gpick.serialize_web_hex,
												deserialize = gpick.deserialize_web_hex };

gpick.converters['color_web_hex_3_digit'] =	{	human_readable = 'Web: hex code (3 digits)',
												serialize = gpick.serialize_web_hex_3_digit,
												deserialize = gpick.deserialize_web_hex_3_digit };

gpick.converters['color_css_hsl'] = {			human_readable = 'CSS: hue saturation lightness',
												serialize = gpick.serialize_css_hsl,
												deserialize = nil };

gpick.converters['color_css_rgb'] = {			human_readable = 'CSS: red green blue',
												serialize = gpick.serialize_css_rgb,
												deserialize = gpick.deserialize_css_rgb};


gpick.converters['css_color_hex'] = {			human_readable = 'CSS(color)',
												serialize = gpick.serialize_css_color_hex,
												deserialize = nil };
gpick.converters['css_background_color_hex'] = {		human_readable = 'CSS(background-color)',
												serialize = gpick.serialize_css_background_color_hex,
												deserialize = nil };
gpick.converters['css_border_color_hex'] = {	human_readable = 'CSS(border-color)',
												serialize = gpick.serialize_css_border_color_hex,
												deserialize = nil };
gpick.converters['css_border_top_color_hex'] = {		human_readable = 'CSS(border-top-color)', 
												serialize = gpick.serialize_css_border_top_color_hex,
												deserialize = nil };
gpick.converters['css_border_right_color_hex'] = {		human_readable = 'CSS(border-right-color)', 
												serialize = gpick.serialize_css_border_right_color_hex,
												deserialize = nil };
gpick.converters['css_border_bottom_color_hex'] = {		human_readable = 'CSS(border-bottom-color)', 
												serialize = gpick.serialize_css_border_bottom_color_hex,
												deserialize = nil };
gpick.converters['css_border_left_hex'] = {		human_readable = 'CSS(border-left-color)', 
												serialize = gpick.serialize_css_border_left_hex,
												deserialize = nil };

gpick.color_serialize = function(converter, color_object)
	return gpick.converters[converter].serialize(color_object);
end;

gpick.color_deserialize = function(converter, text, color_object)
	return gpick.converters[converter].deserialize(text, color_object);
end;


gpick.color_converters_get = function()
	local converters = {};
	for k,v in pairs(gpick.converters) do table.insert(converters, k) end
	return converters;
end;