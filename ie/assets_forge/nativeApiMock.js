var listeners = {};

window.messaging = {
  bg_listen: function(addonId, type, cb, error) {
    listeners[type] = cb;
  },
  bg_broadcast: function(addonId, type, content, cb, error) { },
  bg_toFocussed: function(addonId, type, content, cb, error) { }
};

window.extensions = {
  log: function() { },
  notification: function() { },

  prefs_get: function(addonId, key, success, error) { },
  prefs_set: function(addonId, key, value,  success, error) { },
  prefs_keys: function(addonId, success, error) { },
  prefs_all: function(addonId, success, error) { },
  prefs_clear: function(addonId, what, success, error) { },

  xhr: function(method, url, data, contentType, headers, success, error) {
    $.ajax(url, {
      method: method,
      data:data,
      contentType:contentType,
      headers:headers,
      success:success,
      error:error,
      dataType: 'text'
    });
  },

  cookies_get: function(url, name, success, error) { },
  cookies_remove: function(url, name) { }
};
