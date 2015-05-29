forge['cookies'] = {
  set: function (options, success, error) {
    internal.priv.call("cookies.set", options, success, error);
  },
  get: function (domain, path, name, success, error) {
    internal.priv.call("cookies.get", {
      domain: domain, path: path, name: name
    }, success, error);
  },
  watch: function (domain, path, name, updateCallback) {
    internal.priv.call("cookies.watch", {
      domain: domain, path: path, name: name
    }, updateCallback);
  }
};
