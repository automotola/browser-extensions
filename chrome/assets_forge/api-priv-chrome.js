/*
 * For Chrome.
 */

var callbacks = {},
tabPorts = {}

chrome.runtime.onConnect.addListener(function(port) {
	if (port.name === 'bridge') {
		port.onMessage.addListener(function(msg) {
			function handleResult(status) {
				return function(content) {
					// create and send response message
					var reply = {callid: msg.callid, status: status, content: content};
					port.postMessage(reply);
				}
			}

			//to keep it simple I removed callback
			//later if we need this we can rewrite this part and support callback
			if(msg.method == 'message.toFocussed') {
				apiImpl.message.toFocussed(msg.params)
				return
			}
			internal.priv.call(
					msg.method, msg.params,
					handleResult('success'), handleResult('error')
			);
		});
	} else if(port.name === 'message:to-non-priv') {
		// Special request from non-priv to pass this message on to other non-priv pages
		port.onMessage.addListener(function(message) {
			forge.message.broadcast(message.type, message.content)
		});
	} else if(port.name === 'message:to-priv') {
		if(port.sender.tab) {
			var id = port.sender.tab.id
			tabPorts[id] = tabPorts[id] || []
			tabPorts[id].push(port) //can be several ports - iframes


			port.onDisconnect.addListener(function() {
				//console.log('closed port from tab', port.sender.tab.url)
				var ports = tabPorts[id]
				ports.splice(ports.indexOf(port), 1)
			})
		}

		port.onMessage.addListener(function(message) {
			//console.log('got message from tab', message)
			if(callbacks[message.callid]) {
				callbacks[message.callid](message.content)
				delete callbacks[message.callid]
			}

			var listeners = messageListeners[message.type] || []
			for (var i = 0; i < listeners.length; i++) {
				listeners[i] && listeners[i](message.content, function(reply) {
					// send back reply
					//console.log('send reply', {content: reply, callid: message.callid})
					port.postMessage({content: reply, callid: message.callid}) // reply to tab
				})
			}
		})
	}
});




/**
 * Add a listener for broadcast messages sent by non-privileged pages where your app is active.
 *
 * NOTE: unlike other methods, we assume that the message.listen method has been overriden
 * separately at the non-privilged level.
 *
 * @param {string} type (optional) an arbitrary string: if included, the callback will only be fired
 *  for messages broadcast with the same type; if omitted, the callback will be fired for all messages
 * @param {function} callback a function which will be called
 *  with the contents of relevant broadcast messages as
 *  its first argument
 * @param {function} error Not used.
 */

var messageListeners = {}
forge.message.listen = function(type, callback, error)
{
	if (typeof(type) === 'function') {
		// no type passed in: shift arguments left one
		error = callback;
		callback = type;
		type = null;
	}

	var listeners = messageListeners[type] = messageListeners[type] || []
	if(listeners.indexOf(callback)==-1) listeners.push(callback)
};
/**
 * Broadcast a message to all non-privileged pages where your extension is active.
 *
 * NOTE: this method requires the "tabs" permissions in your app configuration.
 *
 * @param {string} type an arbitrary string which limits the listeners which will receive this message
 * @param {*} content the message body which will be passed into active listeners
 * @param {function} callback reply function
 * @param {function} error Not used.
 */

var broadcastPorts = {}
forge.message.broadcast = function(type, content, callback, error) {
	var callid = forge.tools.UUID(), isCallbackSet = false

	chrome.windows.getAll({populate: true}, function (windows) {
		windows.forEach(function (win) {
			win.tabs.forEach(function (tab) {
				var ports = tabPorts[tab.id]
				//skip hosted pages
				if (tab.url.indexOf('chrome-extension:') != -1 || !ports || ports.length==0) return
				var msg = {type: type, callid: callid, content: content}
				if(!isCallbackSet && callback) {
					callbacks[callid] = callback
					isCallbackSet = true
				}
				//console.log('send message to tab', tab.id, msg)
				for (var i = 0; i < ports.length; i++) {
					ports[i].postMessage(msg)
				}
			})
		})
	})


	// var popupPort = chrome.runtime.connect()
	// // Also to popup
	// if (callback) {
	// 	popupPort.onMessage.addListener(function listener(message) {
	// 		// this is a reply from a recipient non-privileged page
	// 		callback(message);
	// 		popupPort.onMessage.removeListener(listener)
	// 	});
	// }
	// popupPort.postMessage({type: type, content: content});
}

/**
 * @return {boolean}
 */
forge.is.desktop = function() {
	return true;
};

/**
 * @return {boolean}
 */
forge.is.chrome = function() {
	return true;
};

// Private API implementation
var apiImpl = {
	message: {
		toFocussed: function(params, callback, error) {
			chrome.windows.getCurrent(function (win) {
				chrome.tabs.getSelected(win.id, function (tab) {
					var ports = tabPorts[tab.id]
					if(!ports) return
					var callid = forge.tools.UUID()
					if(callback) callbacks[callid] = callback
					var msg = {type: params.type, callid: callid, content: params.content}
					for (var i = 0; i < ports.length; i++) {
						ports[i].postMessage(msg)
					}
				});
			});
		}
	},
	notification: {
		create: function(params, success, error) {
      chrome.notifications.create('', {
        type: "basic",
        title: params.title,
        message: params.text,
        iconUrl: params.iconURL,
        buttons: params.buttons
      }, function(id) {
        success && chrome.notifications.onButtonClicked.addListener(success);
      });
		}
	},
	prefs: {
		get: function (params, success, error) {
			success(window.localStorage.getItem(params.key));
		},
		set: function (params, success, error) {
			success(window.localStorage.setItem(params.key, params.value));
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
		clear: function (params, success, error) 	{
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
    ajax: function (params, success, error) {
      var http = new XMLHttpRequest()
      var headers = params.headers
      var data = params.data

      http.open(params.type, params.url, true)

      if (data) {
        if (!headers || !headers['Content-Type']) {
          http.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded')
        }

        if (typeof data != 'string') {
          try {
            data = JSON.stringify(params.data)
          }
          catch (e) {
            data = null
            console.log(e)
          }
        }
      }

      if (headers) {
        Object.keys(headers).forEach(function(key) {
          http.setRequestHeader(key, headers[key])
        })
      }

      http.onreadystatechange = function() {
        if (http.readyState == 4) {
          if (http.status == 200 || http.status == 201)
            success(http.responseText, http.statusText, http)
          else
            error({message: http.statusText})
        }
      }

      http.send(data)
    }
	},
	tabs: {
    allTabs: function(params, success) {
      var tabs = [];

      chrome.windows.getAll({populate: true}, function (windows) {
        windows.forEach(function (win) {
          win.tabs.forEach(function (tab) {
              tabs.push({
                url: tab.url,
                id: tab.id,
                title: tab.title
              });
            });
          });

        success(tabs);
      });
    },
    reload: function(params, success) {
      var code = { code: "window.location.reload(true)" }
      try {
        chrome.tabs.executeScript(params.id, code)
      } catch(e) {
      }
    },
    getCurrentTabUrl: function(params, success) {
      chrome.tabs.getSelected(function(tab) {
        success(tab.url);
      });
    },
    open: function (params, success, error) {
      chrome.tabs.create({url: params.url, selected: !params.keepFocus}, success);
    },
		updateCurrent: function (params, success, error) {
			chrome.tabs.update({url: params.url}, success);
		},
		closeCurrent: function (params, success, error) {
			if (params.hash) {
				var window, tab, url;

				chrome.windows.getAll({populate: true}, function (windows) {
					while (windows.length) {
						window = windows.pop();

						chrome.tabs.getAllInWindow(window.id, function (tabs) {
							while (tabs.length) {
								tab = tabs.pop();
								url = tab.url;

								if (url.indexOf(params.hash) > 0) {
									chrome.tabs.remove(tab.id);
									success();
									return;
								}
							}
						})
					}
				})
			} else {
				error({
					message: 'hash was not passed to as part of params to closeCurrent',
					type: 'UNEXPECTED_FAILURE'
				});
			}
		}
	},
	button: {
		setIcon: function (url, success, error) {
			// NOTE: When setIcon() is called, it specifies the relative path to an image inside the extension
			chrome.browserAction.setIcon({ "path": url } );
			success();
		},
		setURL: function (url, success, error) {
			if(url.length > 0) {
				if (url.indexOf('/') === 0) {
					url = 'src' + url;
				} else {
					url = 'src/' + url;
				}
			}

			chrome.browserAction.setPopup({popup: url});
			success();
		},
		onClicked: {
			addListener: function (params, callback, error) {
				chrome.browserAction.setPopup({popup: ''});
				chrome.browserAction.onClicked.addListener(callback);
			}
		},
		setBadge: function (number, success, error) {
			chrome.browserAction.setBadgeText({text: (number) ? number.toString() : ''});
			success();
		},
		setBadgeBackgroundColor: function (colorArray, success, error) {
			if (colorArray instanceof Array) {
				colorArray = {color:colorArray};
			}

			chrome.browserAction.setBadgeBackgroundColor(colorArray);
			success();
		},
		setTitle: function(title, success, error) {
			success(chrome.browserAction.setTitle({title: title}));
		}
	},
	logging: {
		log: function(params, success, error) {
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
		getURL: function(params, success, error) {
 			name = params.name.toString();
			if (name.indexOf("http://") === 0 || name.indexOf("https://") === 0) {
 				success(name);
			} else {
				success(chrome.runtime.getURL('src'+(name.substring(0,1) == '/' ? '' : '/')+name));
			}
		}
	},
  cookies: {
    /*
     * spec: https://developer.chrome.com/extensions/cookies#method-set
     */
    set: function(options, cb) {
      cb = cb || function () {}
      chrome.cookies.set(options, cb)
    },
    /*
     * spec: https://developer.chrome.com/extensions/cookies#method-getAll
     */
    get: function(p, cb) {
      chrome.cookies.getAll({domain: p.domain}, function(cookies) {
        for (var i = 0; i < cookies.length; i ++) {
          var cookie = cookies[i];

          if (p.name == cookie.name && p.path == cookie.path) {
            cb(cookie.value);
            return;
          }
        }
        cb()
      })
    },
    /*
     * spec: https://developer.chrome.com/extensions/cookies#event-onChanged
     */
    watch: function(p, cb) {
      chrome.cookies.onChanged.addListener(function(e) {
        var c = e.cookie
        if (!c || !c.name || p.path != c.path || p.name != c.name) return
        if (c.domain.indexOf(p.domain) == -1) return
        if (e.cause == "explicit") cb(c.value)
        if (e.cause == "expired_overwrite") cb()
      })
    }
  }
}
