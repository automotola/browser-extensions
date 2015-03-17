/*
 * For Safari
 */


var messageListeners = {},
	bridgeId = 'forge-bridge' + forge.config.uuid

window.__msg = messageListeners
if(safari.self.toString().indexOf('SafariExtensionPopover')==-1)
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
		if (id) {
			var listener = function(reply) {
				sendToTabs(id, reply)
				unlisten(id, listener)
			}
			listen(id, listener)
		}
		if (event == 'broadcast') {
			sendToTabs('broadcast', data)
		} else {
			var activeTab = safari.application.activeBrowserWindow.activeTab;
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
	var listener = function(data) {
		callback(data)
		unlisten(id, listener)
	}
	listen(id, listener)
	sendToTabs('broadcast', {type: type, content: content, id: id})
}

forge.message.toFocussed = function (type, content, callback,error) {
	var activeTab = safari.application.activeBrowserWindow.activeTab
	if(!activeTab.page) return 
	var id = forge.tools.UUID()

	var listener = function(data) {
		callback(data)
		unlisten(id, listener)
	}

	listen(id, listener)
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









// var msgListeners = {};

// internal.dispatchMessage = function (msgEvt) {
// 	// Received all messages, filter by those relevant to this extension
// 	console.log('receive message (dispatchMessage)', JSON.stringify(msgEvt.message))
// 	if (msgEvt.name === ('forge-bridge'+forge.config.uuid)) {
// 		var msg = msgEvt.message;
// 		if (msg.method === 'message') {
// 			// Handle incoming messages
// 			if (msg.params.event === 'broadcast' || msg.params.event === 'toFocussed') {
// 				if (msg.params.data.uuid) {
// 					safariListen(msg.params.data.uuid, function (reply) {
// 						if (msgEvt.isPopover) {
// 							//reply to the popover
// 							msgEvt.callback(reply, msg.params.data.uuid);
// 						} else {
// 							safariBroadcast(msg.params.data.uuid, reply);
// 						}
// 					});
// 				}
// 				if (msg.params.event === 'broadcast') {
// 					safariBroadcast('broadcast', msg.params.data, msgEvt.isPopover);
// 				} else {
// 					var activeTab = safari.application.activeBrowserWindow.activeTab;
// 					activeTab.page && activeTab.page.dispatchMessage('forge-bridge'+forge.config.uuid, {method: 'message', event: 'broadcast', data: msg.params.data});
// 				}
// 			} else if (msgListeners[msg.params.event]) {
// 				msgListeners[msg.params.event].forEach(function (cb) {
// 					cb(msg.params.data);
// 				});
// 			}
// 		} else {
// 			function handleResult (status) {
// 				return function (content) {
// 					// create and send response message
// 					var reply = {
// 						callid: msg.callid,
// 						status: status, 
// 						content: content
// 					};
// 					if (msgEvt.isPopover) {
// 						// reply to popover if that's the source
// 						msgEvt.callback(reply);
// 					} else {
// 						// send to tabs
// 						console.log('send priv message to tabs', JSON.stringify(reply))
// 						safari.application.browserWindows.forEach(function (win) {
// 							win.tabs.forEach(function (tab) {
// 								tab.page && tab.page.dispatchMessage(('forge-bridge'+forge.config.uuid), reply);
// 							});
// 						});
// 					}
// 				};
// 			}
// 			internal.priv.call(
// 				msg.method, msg.params,
// 				handleResult('success'), handleResult('error')
// 			);
// 		}
// 	}
// }

// safari.application.addEventListener('popover', function(event) {
//     event.target.contentWindow.location.reload();
// }, true);

// window.addEventListener("update-window-size", function(e) {
// 	safari.self.height = e.detail.height
// 	safari.self.width = e.detail.width
// }, false)


// if(safari.self.toString().indexOf("SafariExtensionPopover")==-1)
// safari.application.addEventListener("message", internal.dispatchMessage, false);

// var safariListen = function (event, cb) {
// 	if (msgListeners[event]) {
// 		msgListeners[event].push(cb);
// 	} else {
// 		msgListeners[event] = [cb];
// 	}
// };

// var safariBroadcast = function (event, data, isPopover) {
// 	// send to tabs
// 	console.log('send priv message (broadcast)', JSON.stringify(event), JSON.stringify(data))
// 	safari.application.browserWindows.forEach(function(win) {
// 		win.tabs.forEach(function(tab) {
// 			tab.page && tab.page.dispatchMessage('forge-bridge'+forge.config.uuid, {method: "message", event: event, data: data});
// 		});
// 	});
// 	if (!isPopover) {
// 		safari.extension.popovers.forEach(function(popWin) {
// 			popWin.contentWindow.forge && popWin.contentWindow.forge._dispatchMessage({
// 				name: ('forge-bridge'+forge.config.uuid), 
// 				message: {
// 					method: "message", 
// 					event: event, 
// 					data: data}
// 			});
// 		});
// 	}
// };

// forge.message.listen = function (type, callback, error) {
// 	if (typeof type === 'function') {
// 		// no type passed in: shift arguments left one
// 		error = callback;
// 		callback = type;
// 		type = null;
// 	}
	
// 	safariListen("broadcastBackground", function (message) {
// 		if (type === null || type === message.type) {
// 			callback(message.content, function(reply) {
// 				if (message.uuid) {
// 					safariBroadcast(message.uuid, reply);
// 				}
// 			});
// 		}
// 	});
// };
// forge.message.broadcast = function (type, content, callback, error) {
// 	var msgUUID = forge.tools.UUID();
// 	safariBroadcast('broadcast', {
// 		type: type,
// 		content: content,
// 		uuid: msgUUID
// 	});
// 	safariListen(msgUUID, function (data) {
// 		callback(data);
// 	});
// };
// forge.message.toFocussed = function (type, content, callback,error) {
// 	var msgUUID = forge.tools.UUID();
// 	var data = {type: type,	content: content, uuid: msgUUID};
// 	var activeTab = safari.application.activeBrowserWindow.activeTab;

// 	activeTab.page && activeTab.page.dispatchMessage('forge-bridge'+forge.config.uuid, {method: "message", event: 'broadcast', data: data});

// 	safariListen(msgUUID, function (data) {
// 		callback(data);
// 	});
// };

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
			if (params.keepFocus) {
				var currentTab = safari.application.activeBrowserWindow.activeTab;
				safari.application.activeBrowserWindow.openTab().url = params.url;
				currentTab.activate();
			} else {
				safari.application.activeBrowserWindow.openTab().url = params.url;
			}
			success();
		},
		getCurrentTabUrl: function(params, success) {
			var tab = safari.application.activeBrowserWindow.activeTab
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
	  watch: function(p, cb) {
	    
	  }
	}
}

//forge['_dispatchMessage'] = internal.dispatchMessage;
