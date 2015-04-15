/*
 * For Safari
 */


var messageListeners = {},
	bridgeId = 'forge-bridge' + forge.config.uuid
var isPopup = safari.self.toString().indexOf('SafariExtensionPopover')==-1

window.__msg = messageListeners
if(isPopup)
safari.application.addEventListener('message', onMessage, false)

function onMessage(msgEvt) {
	if(msgEvt.name != bridgeId) return
	var msg = msgEvt.message
	//public method call or broadcast
	if(msg.method == 'message') {
		return processMessage(msg)
	}

	function handleResult (status) {
		return function (content) {
			// create and send response message
			var reply = {callid: msg.callid, status: status, content: content }
			// send to tabs
			safari.application.browserWindows.forEach(function(win) {
				win.tabs.forEach(function (tab) {
					tab.page && tab.page.dispatchMessage(bridgeId, reply)
				})
			})
		}
	}

	internal.priv.call(
		msg.method, msg.params,
		handleResult('success'), handleResult('error')
	)
}

function processMessage(msg) {
	var event = msg.params.event,
		data = msg.params.data,
		id = data && data.id

	if (event == 'broadcast' || event == 'toFocussed') {
		//we dont need reply for broadcast from tabs
		// if (id) {
		// 	var listener = function(reply) {
		// 		sendToTabs(id, reply)
		// 		unlisten(id, listener)
		// 	}
		// 	listen(id, listener)
		// }
		if (event == 'broadcast') {
			sendToTabs('broadcast', data)
		} else {

			var activeTab = safari.application.activeBrowserWindow && safari.application.activeBrowserWindow.activeTab;
			activeTab.page && activeTab.page.dispatchMessage(bridgeId, {method: 'message', event: 'broadcast', data: msg.params.data})
		}
		return
	}

	var listeners = messageListeners[event]
	if(!listeners) return
	listeners.forEach(function(cb) {
		cb(data)
	})

}

forge.message.listen = function(type, callback, error) {
	if (typeof(type) === 'function') {
		// no type passed in: shift arguments left one
		error = callback
		callback = type
		type = null
	}

	listen('broadcast-background', function(message) {
		if (type !== null && type !== message.type) return
		callback(message.content, function(reply) {
			if (!message.id) return
			sendToTabs(message.id, reply)
		})
	})
}

forge.message.broadcast = function (type, content, callback, error) {
	var id = forge.tools.UUID()
	if(callback) {
		var listener = function(data) {
			callback(data)
			unlisten(id, listener)
		}
		listen(id, listener)
	}
	sendToTabs('broadcast', {type: type, content: content, id: id})
}

forge.message.toFocussed = function (type, content, callback,error) {
	var activeTab = safari.application.activeBrowserWindow && safari.application.activeBrowserWindow.activeTab
	if(!activeTab.page) return
	var id = forge.tools.UUID()

	if(callback) {
		var listener = function(data) {
			callback(data)
			unlisten(id, listener)
		}

		listen(id, listener)
	}
	var data = {type: type,	content: content, id: id}
	activeTab.page.dispatchMessage(bridgeId, {method: 'message', event: 'broadcast', data: data})

}

function listen(type, cb) {
	var listeners = messageListeners[type] = messageListeners[type] || []
	if(listeners.indexOf(cb) == -1) listeners.push(cb)
}

function unlisten(type, cb) {
	var listeners = messageListeners[type] = messageListeners[type] || [],
		i = listeners.indexOf(cb)
	if(i != -1) listeners.splice(i, 1)
	if(listeners.length == 0) delete messageListeners[type]
}

function sendToTabs(event, data) {
	safari.application.browserWindows.forEach(function(win) {
		win.tabs.forEach(function(tab) {
			tab.page && tab.page.dispatchMessage(bridgeId, {method: 'message', event: event, data: data})
		})
	})
}

safari.application.addEventListener('popover', function(event) {
	var popup = safari.extension.popovers[0]
	if(popup) {
		popup.width = 0
		popup.height = 0
	}
  event.target.contentWindow.location.reload();
}, true);

window.addEventListener("update-window-size", function(e) {
	safari.self.height = e.detail.height
	safari.self.width = e.detail.width
}, false)

window.addEventListener("close-popup", function(e) {
	safari.self.hide()
}, false)

forge.is.desktop = function() {
	return true;
};

forge.is.safari = function() {
	return true;
};

// Private API implementation
var apiImpl = {
	notification: {
		create: function (params, success, error) {
			if (window.webkitNotifications) {
				if (window.webkitNotifications.checkPermission() == 0) {
					// 0 is PERMISSION_ALLOWED
					var notification = window.webkitNotifications.createNotification(
						'', // icon
						params.title,
						params.text
					);
					notification.show();
					success();
				} else {
					error({message: "Permission to send notification not granted by user.", type: "UNAVAILABLE"});
				}
			} else {
				error({message: 'Notifications are not supported by the browser', type: "UNAVAILABLE"});
			}
		}
	},
	prefs: {
		get: function (params, success, error) {
			success(decodeURIComponent(window.localStorage.getItem(params.key)));
		},
		set: function (params, success, error) {
			success(window.localStorage.setItem(params.key, encodeURIComponent(params.value)));
		},
		keys: function (params, success, error) {
			var keys = [];
			for (var i=0; i<window.localStorage.length; i++) {
				keys.push(window.localStorage.key(i));
			}
			success(keys);
		},
		all: function (params, success, error) {
			var all = {};
			for (var i=0; i<window.localStorage.length; i++) {
				var key = window.localStorage.key(i)
				all[key] = window.localStorage.getItem(key);
			}
			success(all);
		},
		clear: function (params, success, error) {
			success(window.localStorage.removeItem(params.key));
		},
		clearAll: function (params, success, error) {
			success(window.localStorage.clear());
		}
	},

	request: {
		/**
		 * Implementation of api.ajax
		 *
		 * success and error callbacks are taken from positional arguments,
		 * not from the options.success and options.error values.
		 */
		ajax: function (params_, success, error) {
			// Copy params to prevent overwriting of original success/error
			var params = $.extend({}, params_);
			params.success = success;
			params.error = function(xhr, status, err) {
				error({
					message: 'api.ajax with ' + JSON.stringify(params) + 'failed. ' + status + ': ' + err,
					type: "EXPECTED_FAILURE",
					statusCode: xhr.status
				});
			}
			$.ajax(params);
		}
	},
	tabs: {
		closeCurrent: function (params, success, error) {
			if (params.hash) {
				var windows = safari.application.browserWindows;
				for (var i = 0, wLength = windows.length; i < wLength; i++) {
					var tabs = windows[i].tabs;
					for (var j = 0, tLength = tabs.length; j < tLength; j++) {
						if (tabs[j].url && tabs[j].url.indexOf(params.hash) > -1) {
							tabs[j].close();
						}
					}
				}
			}
		},
		open: function (params, success, error) {
      var currentTab = safari.application.activeBrowserWindow.activeTab;
      var tab = safari.application.activeBrowserWindow.openTab();

      tab.addEventListener("navigate", success, false);

      tab.url = params.url;

      if (params.keepFocus) currentTab.activate();
		},
		getCurrentTabUrl: function(params, success) {
			var tab = safari.application.activeBrowserWindow && safari.application.activeBrowserWindow.activeTab
			if(tab) success(tab.url)
		}

	},

	button: {
		setIcon: function (url, success, error) {
			var toolbarButton = safari.extension.toolbarItems[0];

			if (toolbarButton) {
				toolbarButton.image = url;
				success();
			} else {
				error({message: "No toolbar button found", type: "UNAVAILABLE"});
			}
		},
		onClicked: {
			addListener: function (params, callback, error) {
				var toolbarButton = safari.extension.toolbarItems[0];
				var popover = toolbarButton.popover;

				if (popover) {
					//popover must be cleared before setting new one
					var popoverId = popover.identifier;
					if (popover.visible) {
						//needs to be hidden before being removed
						popover.hide();
					}
					safari.extension.removePopover(popoverId);
				}
				safari.application.addEventListener("command", function (event) {
					if (event.command == 'forge-button') {
						callback();
					}
				}, false);
			}
		},
		setURL: function (url, success, error) {
			var toolbarButton,
				popover,
				popoverId;

			toolbarButton = safari.extension.toolbarItems[0];
			popover = toolbarButton.popover;

			if (popover) {
				//popover must be cleared before setting new one
				popoverId = popover.identifier;
				if (popover.visible) {
					//needs to be hidden before being removed
					popover.hide();
				}
				popover = null;
				safari.extension.removePopover(popoverId);
			}

			if (url) {
				//update the url to point to the source directory
				url = safari.extension.baseURI + (url[0] === '/'? 'src' + url: 'src/' + url);

				toolbarButton.popover = safari.extension.createPopover(forge.tools.UUID(), url);
				toolbarButton.command = null;
			}

			success();
		},
		setBadge: function (number, success, error) {
			var toolbarButton = safari.extension.toolbarItems[0];
			if(isNaN(parseInt(number))) {
				var img = toolbarButton.image.split('.'),
					ext = img.pop(),
					prefix = number ? '-' + number : ''
				if(toolbarButton.prevPrefix == prefix) return
				img = img.join('.')
				if(toolbarButton.prevPrefix) img = img.split(toolbarButton.prevPrefix)[0]
				if(prefix) img = img.split(prefix)[0]

				toolbarButton.image =  img + prefix + '.' + ext
				toolbarButton.prevPrefix = prefix
				return success()
			}

			try {
				toolbarButton.badge = parseInt(number, 10);
				success();
			} catch(e) {
				toolbarButton.badge = 0;
				error({message: "Error setting badge number "+e, type: "UNEXPECTED_FAILURE"});
			}
		},
		setBadgeBackgroundColor: function (colorArray, success, error) {
			error({message: 'Safari does not support badge colors', type: "UNAVAILABLE"});
		},
		setTitle: function (title, success, error){
			var toolbarButton = safari.extension.toolbarItems[0];
			toolbarButton.toolTip = title;
			success();
		}
	},

	logging: {
		log: function (params, success, error) {
			if (typeof console !== "undefined") {
				switch (params.level) {
					case 10:
						if (console.debug !== undefined && !(console.debug.toString && console.debug.toString().match('alert'))) {
							console.debug(params.message);
						}
						break;
					case 30:
						if (console.warn !== undefined && !(console.warn.toString && console.warn.toString().match('alert'))) {
							console.warn(params.message);
						}
						break;
					case 40:
					case 50:
						if (console.error !== undefined && !(console.error.toString && console.error.toString().match('alert'))) {
							console.error(params.message);
						}
						break;
					default:
					case 20:
						if (console.info !== undefined && !(console.info.toString && console.info.toString().match('alert'))) {
							console.info(params.message);
						}
						break;
				}
			}
			success();
		}
	},
	tools: {
		getURL: function (params, success, error) {
			var name = params.name.toString();
			if (name.indexOf("http://") === 0 || name.indexOf("https://") === 0) {
				success(name);
			} else {
				success(safari.extension.baseURI+('src'+(name.substring(0,1) == '/' ? '' : '/')+name));
			}
		}
	},
	cookies: {
	  get: function(p, cb) {
	  	setTimeout(cb, 10)
	  },
	  set: function(p, cb) {
      cb = cb || function () {}
	  	setTimeout(cb, 10)
    },
	  watch: function(p, cb) {
	  }
	}
}

//forge['_dispatchMessage'] = internal.dispatchMessage;
