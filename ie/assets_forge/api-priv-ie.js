;
/**
 * api-priv-ie.js
 */


/**
 * debug logger
 */
function loggerpriv(message) {
  window.console.log(message);
}

function clone(obj) {
  if (null == obj || "object" != typeof obj) return obj;
  var copy = obj.constructor();
  for (var attr in obj) {
      if (obj.hasOwnProperty(attr)) copy[attr] = obj[attr];
  }
  return copy;
}


/**
 * console.log|error|debug
 */
function logBackground(level, message, color) {
    var element = document.getElementById("forgeconsole");
    if (element) {
        var node = document.createElement("span");
        node.innerText = message + "\n";
        if (color) {
            node.style.color = color
        }
        element.appendChild(node);
        element.scrollIntoView(false);
    } else {
        window.extensions.log("fallback-priv" + level, message);
    }
};
window.console = {
    log   : function(message){ logBackground("log",   parseArgs(arguments));  },
    debug : function(message){ logBackground("debug", parseArgs(arguments)); },
    info  : function(message){ logBackground("info",  parseArgs(arguments), 'blue'); },
    warn  : function(message){ logBackground("warn",  parseArgs(arguments), 'orange'); },
    error : function(message){ logBackground("error", parseArgs(arguments), 'red'); }
};

function parseArgs(arr) {
  var res = []
  for (var i = 0; i < arr.length; i++) {
    res.push(parseObject(arr[i]))
  }
  return res.join(', ')
}

function parseObject(obj, level) {
  level = level || 0
  var indent = new Array(level * 2).join(' ')
  level++
  var message
  if (Array.isArray(obj)) {
    message = obj
  }
  else if (typeof obj == 'object') {
    message = '{\n'
    for (var index in obj) {
      if (obj.hasOwnProperty(index)) {
        var value = obj[index]

        if (!!(value && value.constructor && value.call && value.apply)) {
          value = "[function]"
        }

        if (typeof obj == 'object') {
          value = parseObject(value, level)
        }

        message += indent + '  ' + index + ':  ' + value + '\n'
      }
    }
    message += indent + ' }'
  }
  else {
    message = obj
  }

  return message
}


/**
 * logMessage
 */
logMessage = function(message, level) {
    if ('logging' in forge.config) {
        var eyeCatcher = forge.config.logging.marker || 'FORGE';
    } else {
        var eyeCatcher = 'FORGE';
    }
    message = '[' + eyeCatcher + ' BG' + '] '
            + (message.indexOf('\n') === -1 ? '' : '\n') + message;

    // Also log to the console if it exists.
    if (typeof console !== "undefined") {
        switch (level) {
        case 10:
            if (console.debug !== undefined && !(console.debug.toString && console.debug.toString().match('alert'))) {
                console.debug(message);
            }
            break;
        case 30:
            if (console.warn !== undefined && !(console.warn.toString && console.warn.toString().match('alert'))) {
                console.warn(message);
            }
            break;
        case 40:
        case 50:
            if (console.error !== undefined && !(console.error.toString && console.error.toString().match('alert'))) {
                console.error(message);
            }
            break;
        default:
        case 20:
            if (console.info !== undefined && !(console.info.toString && console.info.toString().match('alert'))) {
                console.info(message);
            }
            break;
        }
    }
};


/**
 * Identity
 */
forge.is.desktop = function() {
    return true;
};

forge.is.ie = function() {
    return true;
};



/**
 * Add a listener for broadcast messages sent by non-privileged pages where your app is active.
 *
 * NOTE: unlike other methods, we assume that the message.listen method has been overriden
 * separately at the non-privilged level.
 *
 * @param {String} type (optional) an arbitrary string: if included,
 *        the callback will only be fired for messages broadcast with
 *        the same type; if omitted, the callback will be fired for
 *        all messages
 * @param {Function} success a function which will be called
 *        with the contents of relevant broadcast messages as its
 *        first argument
 * @param {Function} error Not used.
 */
forge.message.listen = function(type, callback, error)
{
    if (typeof(type) === 'function') {
        // no type passed in: shift arguments left one
        error = callback;
        callback = type;
        type = null;
    }
    type = (type === null ? "*" : type);
    if (typeof callback !== 'function') callback = function(){};
    if (typeof error !== 'function') error = function(e) {
        loggerpriv("forge.message.listen error -> " + e);
    }
    /*loggerpriv("forge.message.listen" +
               " -> " + forge.config.uuid +
               " -> " + type +
               " -> " + typeof callback +
               " -> " + typeof error);*/

    window.messaging.bg_listen(forge.config.uuid, type, function(content, reply) {
        callback(safe_jparse(content), function(content) {
            reply(safe_jstringify(content));
        });
    }, error);
};


/**
 * Broadcast a message to all non-privileged pages where your extension is active.
 *
 * NOTE: this method requires the "tabs" permissions in your app configuration.
 *
 * @param type     an arbitrary string which limits the listeners which will receive this message
 * @param content  the message body which will be passed into active listeners
 * @param success reply function
 */
forge.message.broadcast = function(type, content, callback, error)
{
    if (typeof(type) === 'function') {
        // no type passed in: shift arguments left one
        error = callback;
        callback = content;
        content = type;
        type = null;
    }
    type = (type === null ? "*" : type);
    if (typeof callback !== 'function') callback = function(){};
    if (typeof error !== 'function') error = function(e) {
        loggerpriv("forge.message.broadcast error -> " + e);
    };
    /*loggerpriv("forge.message.broadcast" +
               " -> " + forge.config.uuid +
               " -> " + type +
               " -> " + safe_jstringify(content) +
               " -> " + typeof callback +
               " -> " + typeof error);*/

    window.messaging.bg_broadcast(forge.config.uuid, type,
                                  safe_jstringify(content),
                                  function(content) {
                                      callback(safe_jparse(content));
                                  }, error);
};


/**
 * Post a message to focussed non-privileged page where your extension is active
 */
forge.message.toFocussed = function(type, content, callback, error) {
    if (typeof(type) === 'function') {
        // no type passed in: shift arguments left one
        error = callback;
        callback = content;
        content = type;
        type = null;
    }
    type = (type === null ? "*" : type);
    if (typeof callback !== 'function') callback = function(){};
    if (typeof error !== 'function') error = function(e) {
        loggerpriv("forge.message.toFocussed error -> " + e);
    };
    /*loggerpriv("forge.message.toFocussed" +
               " -> " + forge.config.uuid +
               " -> " + type +
               " -> " + content +
               " -> " + typeof callback +
               " -> " + typeof error);*/

    window.messaging.bg_toFocussed(forge.config.uuid, type,
                                   safe_jstringify(content),
                                   function(content) {
                                       callback(safe_jparse(content));
                                   }, error);
};


/**
 * Calls from content-script
 */
forge.message.listen("bridge", function(msg, reply) {
    function handleResult(status) {
        return function(content) { // create and send response message
            var response = { callid: msg.callid, status: status, content: content };
            reply(response);
        }
    }
    internal.priv.call(msg.method, msg.params,
                       handleResult('success'),
                       handleResult('error'));
});



/**
 * Private API implementation
 */
var apiImpl = {
    notification: {
        create: function(params, success, error) {
            loggerpriv("notification.create()");
            if (window.extensions.notification("", // icon
                                               params.title,
                                               params.text)) {
                success();
            } else {
                error({
                    message: "No permission for notifications, make sure " +
                             "notifications is included in permissions " +
                             "array of the configuration file"
                });
            }
        }
    },
    prefs: {
        /**
         * Retrieve preference from localStorage.
         *
         * @param {Object} params Should have a "key" attribute set.
         * @param {Function} success Receives the preference value as its only parameter.
         * @param {Function} error Receives an object with message and otherwise undefined schema.
         */
        get: function(params, success, error) {
            console.log('Get prefs', params);

            window.extensions.prefs_get(forge.config.uuid, params.key,
                                        function(value) {
                                            try {
                                                value = JSON.parse(value);
                                            } catch (e) {
                                                console.log("prefs.get" +
                                                           " -> " + value + " is not JSON parseable");
                                                return success(null);
                                            }
                                            success(value);
                                        },
                                        error ? error : function(){});
        },

        /**
         * Set a preference in localStorage
         *
         * @param {Object} params should have "key" and "value" attributes.
         * @param {Function} success Receives the X as it's only parameter.
         * @param {Function} error Receives an object with message and otherwise undefined schema.
         */
        set: function(params, success, error) {
            var json = "object could not be stringified";
            try {
                json = safe_jstringify(params.value);
            } catch (e) { error(json); }
            window.extensions.prefs_set(forge.config.uuid,
                                        params.key, json,
                                        function(value) {
                                            try {
                                                value = JSON.parse(value);
                                            } catch (e) {
                                                loggerpriv("prefs.set" +
                                                           " -> " + value + " is not JSON parseable");
                                                return success(null);
                                            }
                                            success(value);
                                        },
                                        error ? error : function(){});
        },

        /**
         * Find the keys of all preferences that have been set.
         *
         * @param {Objects} params Not used.
         * @param {Function} success Receives the keys as it's only parameter.
         * @param {Function} error Receives an object with message and otherwise undefined schema.
         */
        keys: function(params, success, error) {
            window.extensions.prefs_keys(forge.config.uuid,
                                         function(result) {
                                             try {
                                                 loggerpriv("prefs.keys" +
                                                            " -> " + result);
                                                 result = JSON.parse(result);
                                             } catch (e) {
                                                 loggerpriv("prefs.keys" +
                                                            " -> " + result + " is not JSON parseable");
                                                 return success(null);
                                             }
                                             success(result);
                                         },
                                         error ? error : function(){});
        },

        /**
         * Find the keys and values of all preferences that have been set in localStorage.
         *
         * @param {Objects} params Not used.
         * @param {Function} success Receives an object with the keys and values as it's only parameter.
         * @param {Function} error Receives an object with message and otherwise undefined schema.
         */
        all: function(params, success, error) {
            window.extensions.prefs_all(forge.config.uuid,
                                        function(result) {
                                            try {
                                                loggerpriv("prefs.all" +
                                                           " -> " + result);
                                                result = JSON.parse(result);
                                            } catch (e) {
                                                loggerpriv("prefs.all" +
                                                           " -> " + result + " is not JSON parseable");
                                                return success(null);
                                            }
                                            success(keys);
                                        },
                                        error ? error : function(){});
        },

        /**
         * Expunge a single persisted setting from localStorage,
         * reverting it back to its default value (if available).
         *
         * @param {String} key Preference to forget.
         * @param {Function} success Receives no arguments..
         * @param {Function} error Receives an object with message and otherwise undefined schema.
         */
        clear: function(params, success, error) {
            window.extensions.prefs_clear(forge.config.uuid, params.key,
                                          success ? success : function(){},
                                          error ? error : function(){});
        },

        /**
         * Expunge all persisted settings from localStorage,
         * reverting back to defaults (if available).
         *
         * @param {Objects} params Not used.
         * @param {Function} success Receives no arguments.
         * @param {Function} error Receives an object with message and otherwise undefined schema.
         */
        clearAll: function(params, success, error) {
            window.extensions.prefs_clear(forge.config.uuid, "*",
                                          success ? success : function(){},
                                          error ? error : function(){});
        }
    },

    request: {
        /**
         * Implementation of api.ajax
         *
         * success and error callbacks are taken from positional arguments,
         * not from the options.success and options.error values.
         */
        ajax: function(params_, success, error) {
            /*loggerpriv("request.ajax " +
                       " -> " + JSON.stringify(params_) +
                       " -> " + typeof success +
                       " -> " + typeof error);*/

            // create dummy callbacks if required
            params_.success = typeof params_.success === 'function' ? params_.success : function(){};
            params_.error   = typeof params_.error   === 'function' ? params_.error   : function(){};

            // Copy params to prevent overwriting of original success/error
            var params = clone(params_);
            params.success = success;
            params.error = function(xhr, status, err) {
                json = safe_jstringify(params);
                error({ message: 'api.ajax with ' + json + ' failed. ' + status + ': ' + err,
                        status: status, err: err });
            }

            // check arguments
            params.type = params.type ? params.type : "GET";
            //params.data = params.data ? params.data : ""; doesn't work for jquery
            params.timeout = params.timeout ? params.timeout : 60000;
            // params.headers = params.headers ? params.headers : {}; doesn't work for jquery
            params.accepts = params.accepts ? params.accepts : ["*/*"];
            params.accepts = typeof params.accepts === "string" ? [params.accepts] : params.accepts;

            // encode data
            if (params.type === "GET") {
                params.url = internal.generateURI(params.url, params.data);
                params.data = "";
            } else if (params.data) {
                params.data = internal.generateQueryString(params.data);
                params.contentType = params.contentType ? params.contentType : "application/x-www-form-urlencoded";
            }

            try {
                // TODO headers
                params.contentType = params.contentType ? params.contentType : "application/x-www-form-urlencoded";
                window.extensions.xhr(params.type,
                                      params.url,
                                      params.data,
                                      params.contentType,
                                      JSON.stringify(params.headers),
                                      function(data) {
                                          if (params.dataType === "json") {
                                              try {
                                                  var json = JSON.parse(data);
                                                  data = json;
                                              }  catch (e) {
                                                  loggerpriv("request.ajax" +
                                                             " json error -> " + data);
                                              }
                                          }
                                          success(data);
                                      },
                                      function(data) {
                                          if (typeof error !== "function") return;
                                          error(data);
                                     });
            } catch (e) {
                var json = "exception could not be stringified";
                try {
                    json = safe_jstringify(e);
                } catch (e) {}
                loggerpriv("request.ajax exception -> " + json);
                error({ message: json });
            }
        }
    },

    tabs: {

        /**
         * Open a new tab.
         *
         * @param {Objects} params Should contain a "url" attribute, and optionally "keepFocus"
         * @param {Function} success Receives no arguments.
         * @param {Function} error Receives an object with message and otherwise undefined schema.
         */
        open: function(params, success, error) {
            loggerpriv("tabs.open" +
                       " -> " + params +
                       " -> " + typeof success +
                       " -> " + typeof error);
            window.accessible.open(params.url,
                                   params.keepFocus,
                                   typeof success === "function" ? success : function(){},
                                   typeof error   === "function" ? error   : function(){});
        },

        /**
         * Closes a tab which contains a specified hash in the URL
         * @param params contains a hash, which is appended to the url of the tab to be closed
         * @param success - not used
         * @param error
         */
        closeCurrent: function(params, success, error) {
            loggerpriv("tabs.closeCurrent" +
                       " -> " + params +
                       " -> " + typeof success +
                       " -> " + typeof error);
            error({ message: "cannot call closeCurrent() from background" });
        }
    },

    button: {
        setIcon: function(url, success, error) {
            loggerpriv("button.setIcon" +
                       " -> " + forge.config.uuid +
                       " -> " + url +
                       " -> " + typeof success +
                       " -> " + typeof error);
            window.controls.button_setIcon(
                forge.config.uuid,
                url,
                typeof success === "function" ? success : function(){},
                typeof error   === "function" ? error   : function(){});
        },

        setURL: function(url, success, error) {
            loggerpriv("button.setURL" +
                       " -> " + forge.config.uuid +
                       " -> " + url +
                       " -> " + typeof success +
                       " -> " + typeof error);
            window.controls.button_setURL(
                forge.config.uuid,
                url,
                typeof success === "function" ? success : function(){},
                typeof error   === "function" ? error   : function(){});
        },

        onClicked: {
            /**
             * Sets a listener for button click
             */
            addListener: function(params, callback, error) {
                loggerpriv("button.onClicked.addListener" +
                           " -> " + forge.config.uuid +
                           " -> " + params +
                           " -> " + typeof callback +
                           " -> " + typeof error);
                window.controls.button_onClicked(forge.config.uuid, callback);
            }
        },

        setBadge: function(text, success, error) {
            loggerpriv("button.setBadge" +
                       " -> " + forge.config.uuid +
                       " -> " + text +
                       " -> " + typeof success +
                       " -> " + typeof error);
            window.controls.button_setBadge(
                forge.config.uuid,
                text,
                typeof success === "function" ? success : function(){},
                typeof error   === "function" ? error   : function(){});
        },

        setBadgeBackgroundColor: function(colorArray, success, error) {
            loggerpriv("button.setBadgeBackgroundColor" +
                       " -> " + forge.config.uuid +
                       " -> " + colorArray +
                       " -> " + typeof success +
                       " -> " + typeof error);
            if (!(colorArray instanceof Array) ||
                colorArray.length != 4) {
                return error({ message: "Invalid colorArray parameter" });
            }
            window.controls.button_setBadgeBackgroundColor(
                forge.config.uuid,
                colorArray[0], colorArray[1], colorArray[2], colorArray[3],
                typeof success === "function" ? success : function(){},
                typeof error   === "function" ? error   : function(){});
        },

        setTitle: function(title, success, error) {
            loggerpriv("button.setTitle" +
                       " -> " + forge.config.uuid +
                       " -> " + title +
                       " -> " + typeof success +
                       " -> " + typeof error);
            window.controls.button_setTitle(
                forge.config.uuid,
                title,
                typeof success === "function" ? success : function(){},
                typeof error   === "function" ? error   : function(){});
        }

    },
    logging: {
        log: function(params, success, error) {
            if (typeof console !== "undefined" && typeof console.log !== "undefined") {
                console.log(params.message);
                success();
            } else {
                error();
            }
        }
    },
    tools: {
        getURL: function(params, success, error) {
            name = params.name.toString();
            if (name.indexOf("http://") === 0 || name.indexOf("https://") === 0) {
                success(name);
            } else {
                var url = window.extensions.getURL(name);
                success(url);
            }
        }
    },
    cookies: {
      get: function (params, success, error) {
          window.extensions.cookies_get('https://' + params.domain + params.path, params.name,
              function (content) {
                  if (typeof success === "function") {
                      var res = function (str) {
                          var obj = []
                          var pairs = str.split(/; */);

                          pairs.forEach(function (pair) {
                              var eq_idx = pair.indexOf('=')
                              if (eq_idx < 0) {
                                  return;
                              }

                              var key = pair.substr(0, eq_idx).trim()
                              var val = pair.substr(++eq_idx, pair.length).trim();

                              // quoted values
                              if ('"' == val[0]) {
                                  val = val.slice(1, -1);
                              }
                              var valdec = "";
                              try {
                                  valdec = decodeURIComponent(val);

                              } catch (e) {
                                  valdec = val;
                              }
                              obj.push({ name: key, value: valdec });
                          });

                          return obj;
                      }(content);

                      if (res.length > 0)
                        success(res[0].value);
                      else
                        success();
                  }
              },

              typeof error === "function" ? error : function () { }
          );
      },
      set: function (params, success) {
        success = success || function () {}
        setTimeout(success, 10)
      },
      watch: function(params, success) {
        var old;

        function check() {
          apiImpl.cookies.get(params, function(cookie) {
            if (cookie != old) {
              old = cookie;
              success(cookie);
            }

            setTimeout(check, 5000);
          });
        }

        check();
      },
      remove: function (params) {
          return window.extensions.cookies_remove(params.url, params.name);
      }
    }
}



