function cloud_input_dirname(path) {
	return path.replace(/\\/g,'/').replace(/\/[^\/]*$/, '');
}

var cloud_input_init_loc = "";
if (document.getElementById("cloud_input_1.0_init")) {
	cloud_input_init_loc = document.getElementById("cloud_input_1.0_init").src;
}
var cloud_input_static = cloud_input_dirname(cloud_input_init_loc) + "/";

function cloud_input_load() {
	if (!document.getElementById("cloud_input_1.0")) {
		var script = document.createElement("script");
		script.setAttribute("id", "cloud_input_1.0");
		script.setAttribute("src", cloud_input_static + "cloud_input.js");
		script.onload = function() {
			var el = document.getElementById("cloud_input_1.0_init");
			el.parentNode.removeChild(el);
			CloudInput.init.call(CloudInput, cloud_input_static);
			CloudInput.turnOn.call(CloudInput);
		}
		document.body.appendChild(script);
	} else {
		CloudInput.toggleOnOff();
	}
}

if (typeof jQuery == 'undefined') {
	var script = document.createElement("script");
	script.setAttribute("src", cloud_input_static + "jquery.js");
	script.onload = cloud_input_load;
	document.body.appendChild(script);
} else {
	cloud_input_load();
}
