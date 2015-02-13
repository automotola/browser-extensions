forge['cookies'] = {
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
